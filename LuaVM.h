#ifndef LUA_VM_H
#define LUA_VM_H

#include <string>
#include <lua.hpp>
#include "../Reflection/separate headers/reflect.hpp"

// Stack class template explicit specializations provide interface with Lua stack
template <typename T>
struct LuaStack
{
	static void ToLua(lua_State *L, T &t)
	{
		static Reflect::Any object;

		object = Reflect::AnyRef(t);
		lua_pushlightuserdata(L, &object);  // #TODO_pass_by_value_or_reference
	}

	static T &FromLua(lua_State *L, int index)
	{
		auto object = static_cast<Reflect::Any*>(lua_touserdata(L, index));
		return *object->TryCast<T>();
	}

	static bool Is(lua_State *L, int index)
	{
		return lua_isuserdata(L, index);
	}
};

template <>
struct LuaStack<int>
{
	static void ToLua(lua_State *L, int i)
	{
		lua_pushinteger(L, i);
	}

	static int FromLua(lua_State *L, int index)
	{
		//return luaL_checkinteger(L, index);
		lua_Integer result = lua_tointeger(L, index);
		return result;
	}

	static bool Is(lua_State *L, int index)
	{
		return lua_isinteger(L, index);
	}
};

template <>
struct LuaStack<unsigned int>
{
	static void ToLua(lua_State *L, int i)
	{
		lua_pushinteger(L, i);
	}

	static int FromLua(lua_State *L, int index)
	{
		//return luaL_checkinteger(L, index);
		lua_Integer result = lua_tointeger(L, index);
		return result;
	}

	static bool Is(lua_State *L, int index)
	{
		return lua_isinteger(L, index);
	}
};

template <>
struct LuaStack<float>
{
	static void ToLua(lua_State *L, float f)
	{
		lua_pushnumber(L, f);
	}

	static float FromLua(lua_State *L, int index)
	{
		lua_Number result = lua_tonumber(L, index);
		return result;
	}

	static bool Is(lua_State *L, int index)
	{
		return lua_isnumber(L, index);
	}
};

template <>
struct LuaStack<std::string>
{
	static void ToLua(lua_State *L, std::string &string)
	{
		lua_pushstring(L, string.c_str());
	}

	static std::string FromLua(lua_State *L, int index)
	{
		return lua_tostring(L, index);
	}

	static bool Is(lua_State *L, int index)
	{
		return lua_isstring(L, index);
	}
};

#include "LuaObject.h"

class LuaVM
{
public:
	LuaVM();
	LuaVM(LuaVM const &other) = delete;
	LuaVM(LuaVM &&other) = delete;

	~LuaVM() { Close(); }

	LuaVM &operator=(LuaVM const &other) = delete;
	LuaVM &operator=(LuaVM &&other) = delete;

	template <typename T>
	void BindType();

	template <typename Ret, typename... Args>
	void BindNativeFunction(Ret(*funPtr)(Args...), const std::string &name);

	template <typename... Args>
	void CallScriptFunction(const std::string &functionName, Args&&... args);

	LuaObject GetGlobal(const std::string &name);

	bool LoadFile(std::string const &scriptFile);
	bool LoadString(std::string const &script);
	bool Execute();

	void Close();
private:
	lua_State *L;

	// Lua C functions
	static int NewObject(lua_State*);

	static int Finalize(lua_State*);    // __gc metamethod
	static int Index(lua_State*);       // __index metamethod
	static int NewIndex(lua_State*);    // __newindex metamethod

	static int InvokeMethod(lua_State*);
	static int InvokeFunction(lua_State*);

	std::vector<Reflect::Function*> mFreeFunctions;
};

template <typename T>
void LuaVM::BindType()
{
	auto *typeDescriptor = Reflect::Resolve<T>();          // get the type descriptor of the type to bind

	/* 
	* Create a global table named as the mapped type:      
	*/

	lua_newtable(L);                                       // create a new table and push it on the stack
	lua_pushvalue(L, -1);                                  // push a copy of the value on the stack (needed later to add the new method)
	lua_setglobal(L, typeDescriptor->GetName().c_str());   // pop the table from the stack and create a global variable with the name of the type

	/* 
	* Add the new method to the table:
	* a "new" method is added to the global table which lets scripts create objects of that type as Type.new()
	* every new method has the type descriptor of the type of object to be created as an upvalue
	*/

	//lua_pushcfunction(L, NewObject);
	//lua_setfield(L, -2, "new");
	lua_pushstring(L, "new");
	lua_pushlightuserdata(L, (void*)typeDescriptor);  // push an upvalue for the C closure (type descriptor)
	lua_pushcclosure(L, NewObject, 1);                // pop the upvalue (type descriptor) and push the C closure/function on the stack
	lua_settable(L, -3);                              // pop the key ("new") and the value (closure) from the stack and put them into the new type table at index
}

template <typename Ret, typename... Args>
void LuaVM::BindNativeFunction(Ret(*funPtr)(Args...), const std::string &functionName)
{
	Reflect::Function *memberFunction = new Reflect::FreeFunction(funPtr, functionName);  // create a function meta object
	mFreeFunctions.push_back(memberFunction);           // store the pointer to the dynamic memory to be deallocated when closing the VM

	lua_pushlightuserdata(L, (void*)memberFunction);    // push function meta object to the stack
	lua_pushcclosure(L, &InvokeFunction, 1);            // push the invoker on the stack with the function meta object as upvalue
	lua_setglobal(L, functionName.c_str());             // set a global with the name of the function 
}

#include <iostream>

template <typename... Args>
void LuaVM::CallScriptFunction(std::string const &functionName, Args&&... args)  
{
	constexpr size_t numArgs = sizeof...(Args);

	int stackSize = lua_gettop(L);

	lua_getglobal(L, functionName.c_str());  // push function on stack

	if (lua_isfunction(L, -1))
	{
		// args --> to lua
		(LuaStack<std::remove_cv_t<std::remove_reference_t<Args>>>::ToLua(L, args), ...);

		if (lua_pcall(L, numArgs, LUA_MULTRET, 0) != LUA_OK)  // pops function and arguments and call function
			std::cout << "error calling function: " + functionName << std::endl;
	}
	else
		std::cout << "can't find function: " + functionName << std::endl;;

	int numReturnValues = lua_gettop(L) - stackSize;
}

#endif  // LUA_VM_H
