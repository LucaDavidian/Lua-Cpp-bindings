#include "LuaVM.h"
#include "allocator.hpp"
#include <iostream>

LuaVM::LuaVM() : L(luaL_newstate()) // L(lua_newstate(LuaDefAlloc, nullptr))
{
	luaL_openlibs(L);

	/*
	* create a metatable for mapped native types, add it to the registry and push it on the stack
	* native types can be owned by Lua (full userdata) or owned by C++ (lightuserdata): 
	* in both cases they share the same metatable with __index, __newindex and __gc metamethods
	* (__gc is called by Lua only for managed objects - aka full userdata)
	*/ 

	luaL_newmetatable(L, "metatable");   // add a "metatable" key with a table to be used as metatable for userdata in the Lua Registry and push it onto the stack

	// add __gc metamethod
	lua_pushstring(L, "__gc");           // key 
	lua_pushcclosure(L, Finalize, 0);    // value
	//lua_pushcfunction(L, Finalize);    // same - default 0 upvalues
	lua_settable(L, -3);                 // pop key and value and set table at index 
	//lua_pushcclosure(L, Finalize, 0);  // value
	//lua_setfield(L, -2, "__gc");       // pop value and set table at index with string key

	// add __index metamethod
	lua_pushstring(L, "__index");
	lua_pushcclosure(L, Index, 0);
	lua_settable(L, -3);

	// add __newindex metamethod
	lua_pushstring(L, "__newindex");
	lua_pushcclosure(L, NewIndex, 0);
	lua_settable(L, -3);

	/*
	* create and set a metatable for lightuserdata type
	* NOTE: not necessary anymore, since all objects passed to/from Lua are full userdata
	*/

	//lua_pushlightuserdata(L, 0);
	//lua_newtable(L);
	//
	//// add __index metamethod
	//lua_pushstring(L, "__index");
	//lua_pushcfunction(L, Index);
	//lua_settable(L, -3);

	//// add __newindex metamethod
	//lua_pushstring(L, "__newindex");
	//lua_pushcfunction(L, NewIndex);
	//lua_settable(L, -3);

	//lua_setmetatable(L, -2);
}

bool LuaVM::LoadFile(std::string const &scriptFile)
{
	/*
	* load a script from file (calls luaL_loadfilex(L, scriptFile, NULL))
	* and pushes the compiled chunk on the stack
	*/

	if (luaL_loadfile(L, scriptFile.c_str()) != LUA_OK)
	{
		std::cout << lua_tostring(L, -1) << std::endl;
		return false;
	}

	return true;
}

bool LuaVM::LoadString(std::string const &script)
{
	/*
	* load a script from string and pushes the compiled chunk on the stack
	*/

	if (luaL_loadstring(L, script.c_str()) != LUA_OK)
	{
		std::cout << lua_tostring(L, -1) << std::endl;
		return false;
	}

	return true;
}

bool LuaVM::Execute()
{
	/*
	* execute a loaded script in protected mode
	*/

	if (lua_pcall(L, 0, LUA_MULTRET, 0) != LUA_OK)
	{
		std::cout << lua_tostring(L, -1) << std::endl;
		return false;
	}

	return true;
}

void LuaVM::Close()
{
	if (L)
	{
		lua_close(L);
		L = nullptr;

		for (auto function : mFreeFunctions)
			delete function;
	}
}

LuaObject LuaVM::GetGlobal(const std::string &name)
{
	lua_getglobal(L, name.c_str());             // push a global variable on the stack
	int ref = luaL_ref(L, LUA_REGISTRYINDEX);   // pop the variable from the stack and add it to the registry with key = ref: reg[ref] = value
	
	LuaObject luaObject(L, ref);
	return luaObject;
}

int LuaVM::NewObject(lua_State *L)
{
	/* 
	* Create a new userdata and construct a new object in it:
	* When creating a new object of a user-defined type a userdata is created (which is a buffer of memory
	* allocated and managed by Lua), with the size in bytes of the object to store. Once Lua returns a chunk 
	* of allocated memory, an object is placement-new'ed inside that memory:
	*/

	Reflect::TypeDescriptor *typeDesc = (Reflect::TypeDescriptor*)lua_touserdata(L, lua_upvalueindex(1));  // get upvalue n.1 from C closure (type descriptor of new object)
	
	void *rawMem = lua_newuserdatauv(L, sizeof(Reflect::Any), 1);          // create a full userdata (memory buffer managed by Lua) with 1 uservalue and put it on the stack
	//void *rawMem = lua_newuserdata(L, sizeof(Reflect::any));             // same - default 1 uservalue
	
	std::size_t numLuaArgs = lua_gettop(L) - 1;  // number of arguments passed by Lua is number of elements on the stack - 1, top element is userdata

	bool constructed = false;
	for (auto *constructor : typeDesc->GetConstructors())
		if (constructor->GetNumParams() == numLuaArgs)
		{
			bool argsChecked = true;
			std::vector<Reflect::Any> argsFromStack;

			for (std::size_t argIndex = 1; argIndex <= numLuaArgs; ++argIndex)
			{
				auto *paramType = constructor->GetParamType(argIndex - 1);
				auto *fromLua = paramType->GetMemberFunction("FromLua");
				auto *is = paramType->GetMemberFunction("Is");

				if (*is->Invoke(Reflect::AnyRef(), L, argIndex).TryCast<bool>())
					argsFromStack.push_back(fromLua->Invoke(Reflect::AnyRef(), L, argIndex));
				else
				{
					argsChecked = false;
					break;
				}
			}

			if (argsChecked)
			{
				new(rawMem) Reflect::Any(constructor->NewInstance(argsFromStack));  // construct an Any object in userdata memory (a type-erased void* + a type descriptor)
				constructed = true;
				break;
			}
		}

	if (!constructed)
		luaL_error(L, "cannot construct type %s from provided args", typeDesc->GetName().c_str());

	/* 
	* Associate a metatable with the userdata:
	* A generic metatable with all the metamethods is created at script initialization and stored into the registry.
	* All full userdata (and lightuserdata) share the same generic metatable
	*/

	//luaL_setmetatable(L, "metatable");  // set metatable from Lua registry as metatable for element at top of stack
	luaL_getmetatable(L, "metatable");    // get metatable from Lua registry and put it on the stack
	lua_setmetatable(L, -2);              // pop current top of stack and set it as metatable of element at index

	/* 
	* Associate uservalue(s) with the userdata:
	* each userdata can have uservalues associated with it.
	* Uservalues can be retrieved with lua_getiuservalue(L, userdata_index, uservalue_index)
	* or macro lua_getuservalue(L, userdata_index) which retrieves default uservalue 1.
	* An uservalue is associated with the userdata of the new object, a table, which will be
	* used to add properties to the mapped type on the Lua side (besides the properties of the native type).
	*/

	lua_newtable(L);                 // create a new table and push on the stack
	lua_setiuservalue(L, -2, 1);     // pop top of stack and set as uservalue n.1 of element at index
	//lua_setuservalue(L, -2);       // same - default uservalue n.1

	return 1;    // return a new object
}

int LuaVM::Finalize(lua_State *L)
{
	Reflect::Any *any = static_cast<Reflect::Any*>(lua_touserdata(L, -1));
	
	any->~BasicAny();

	return 0;
}

int LuaVM::Index(lua_State *L)
{
	Reflect::Any *object = static_cast<Reflect::Any*>(lua_touserdata(L, -2));   // arg 1: userdata
	const char *key = lua_tostring(L, -1);                                      // arg 2: key

	const Reflect::TypeDescriptor *typeDesc = object->GetType();

	if (Reflect::DataMember *dataMember = typeDesc->GetDataMember(key))                         // key is a data member
	{
		Reflect::Any value = dataMember->Get(*object);

		// convert any object "to Lua" type and push onto Lua stack
		dataMember->GetType()->GetMemberFunction("ToLua")->Invoke(Reflect::AnyRef(), L, dataMember->Get(*object));

	}
	else if (Reflect::Function const *memberFunction = typeDesc->GetMemberFunction(key))  // key is a member function/method
	{
		lua_pushlightuserdata(L, (void*)memberFunction);
		lua_pushcclosure(L, InvokeMethod, 1);
	}
	else if (lua_getiuservalue(L, -2, 1) != LUA_TNONE)   // key is a script-side property, put user value table t on top of stack
	{
		lua_pushvalue(L, -2);         // put key on top of stack
		lua_gettable(L, -2);          // get t[key] and put it on top of stack

		if (lua_isnil(L, -1))
			luaL_error(L, "type %s doesn't have field %s", typeDesc->GetName().c_str(), key);
	}
	else    // key is not present
		luaL_error(L, "type %s doesn't have field %s", typeDesc->GetName().c_str(), key);

	return 1;
}

int LuaVM::NewIndex(lua_State *L)
{
	Reflect::Any *object = static_cast<Reflect::Any*>(lua_touserdata(L, -3));   // arg 1: userdata
	const char *key = lua_tostring(L, -2);                                      // arg 2: key
	                                                                            // arg 3: value
	const Reflect::TypeDescriptor *typeDesc = object->GetType();

	if (Reflect::DataMember *dataMember = typeDesc->GetDataMember(key))  // set native type property
	{
		Reflect::Any none;
		Reflect::Any result(dataMember->GetType()->GetMemberFunction("FromLua")->Invoke(none, L, -1/* index */));
		dataMember->Set(*object, result);
	}
	else if (lua_getiuservalue(L, -3, 1) != LUA_TNONE)  // set script-side property - put user value table t on top of stack
	{
		lua_pushvalue(L, -3);         // put key on top of stack
		lua_pushvalue(L, -3);         // put value on top of stack
		lua_settable(L, -3);          // set t[k] = v
	}
	else   // key is not present
		luaL_error(L, "type %s doesn't have field %s", typeDesc->GetName().c_str(), key);

	return 0;
}

int LuaVM::InvokeMethod(lua_State *L)
{
	Reflect::Function const *memberFunction = static_cast<Reflect::Function*>(lua_touserdata(L, lua_upvalueindex(1)));  // closure upvalue 1: member function meta opbject
	assert(memberFunction);  // upvalue present
	
	Reflect::Any &object = *static_cast<Reflect::Any*>(lua_touserdata(L, 1));      // first arg is self                             // arg 1: self object

	int numLuaArgs = lua_gettop(L) - 1;
	size_t numFunParams = memberFunction->GetNumParams();

	if (numLuaArgs != numFunParams)
		luaL_error(L, "calling function %s with wrong number of arguments: expected %d, got %d", memberFunction->GetName().c_str(), numFunParams, numLuaArgs);

	std::vector<Reflect::Any> argsFromStack;

	// convert arguments "from Lua" types and pass them to member function
	for (int i = 1; i <= numLuaArgs; i++)  // Lua starts from 1
	{
		const Reflect::TypeDescriptor *paramType = memberFunction->GetParamType(i - 1);
		auto *fromLua = paramType->GetMemberFunction("FromLua");
		auto *is = paramType->GetMemberFunction("Is");

		if (*is->Invoke(Reflect::AnyRef(), L, i + 1).TryCast<bool>())                  // check type of arg on stack
			argsFromStack.emplace_back(fromLua->Invoke(Reflect::AnyRef(), L, i + 1));  // start from 2, arg 1 is self object
		else
			luaL_error(L, "calling function \"%s\" with wrong type for argument %d: expected \"%s\", got \"%s\"", memberFunction->GetName().c_str(), i - 1, paramType->GetName().c_str(), lua_typename(L, lua_type(L, i + 1)));
	}

	Reflect::Any result = memberFunction->Invoke(object, argsFromStack);

	// convert return type "to Lua" type and put value onto stack
	if (result)
		result.GetType()->GetMemberFunction("ToLua")->Invoke(Reflect::AnyRef(), L, result);
	else
		lua_pushnil(L);

	return 1;
}

int LuaVM::InvokeFunction(lua_State *L)
{
	Reflect::Function *function = (Reflect::Function*)lua_touserdata(L, lua_upvalueindex(1));  // closure upvalue 1: member function meta object
	assert(function);  // upvalue present

	int numLuaArgs = lua_gettop(L);
	size_t numFunParams = function->GetNumParams();

	if (numLuaArgs != numFunParams)
		luaL_error(L, "calling function %s with wrong number of arguments: expected %d, got %d", function->GetName().c_str(), numFunParams, numLuaArgs);

	std::vector<Reflect::Any> argsFromStack;

	// convert arguments "from Lua" types and pass them to member function
	for (int i = 1; i <= numLuaArgs; i++)  // Lua starts from 1
	{
		const Reflect::TypeDescriptor *paramType = function->GetParamType(i - 1);
		auto *fromLua = paramType->GetMemberFunction("FromLua");
		auto *is = paramType->GetMemberFunction("Is");

		if (*is->Invoke(Reflect::AnyRef(), L, i).TryCast<bool>())                   // check type of arg on stack
			argsFromStack.emplace_back(fromLua->Invoke(Reflect::AnyRef(), L, i)); 
		else
			luaL_error(L, "calling function \"%s\" with wrong type for argument %d: expected \"%s\", got \"%s\"", function->GetName().c_str(), i - 1, paramType->GetName().c_str(), lua_typename(L, lua_type(L, i + 1)));
	}

	Reflect::Any result = function->Invoke(Reflect::AnyRef(), argsFromStack);

	// convert return type "to Lua" type and put value onto stack
	if (result)
		result.GetType()->GetMemberFunction("ToLua")->Invoke(Reflect::AnyRef(), L, result);
	else
		lua_pushnil(L);

	return 1;
}

