#ifndef LUA_VM_H
#define LUA_VM_H

#include <string>
#include <iostream>
#include "include/lua.hpp"
#include "../reflection/single include/reflect.hpp"
#include "LuaStack.h"
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
	void BindNativeType();

	template <typename Ret, typename... Args>
	void BindNativeFunction(Ret(*funPtr)(Args...), const std::string &name);

	template <typename... Args>
	void CallScriptFunction(const std::string &functionName, Args&&... args);

	class LuaObject GetGlobal(const std::string &name);

	bool LoadFile(std::string const &scriptFile);
	bool LoadString(std::string const &script);
	bool Execute();

	void Close();
private:
	lua_State *L;

	/* Lua C functions */

	// invoked when creating registered user defined types in Lua
	static int NewObject(lua_State*);

	// in user defined type metatable (used by full userdata and lightuserdata)
	static int Finalize(lua_State*);    // __gc metamethod
	static int Index(lua_State*);       // __index metamethod
	static int NewIndex(lua_State*);    // __newindex metamethod

	// helper functions invoked when calling native free functions or user defined type's methods
	static int InvokeMethod(lua_State*);
	static int InvokeFunction(lua_State*);

	std::vector<Reflect::Function*> mFreeFunctions;
};

template <typename T>
void LuaVM::BindNativeType()
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
	lua_pop(L, 1);  
}

template <typename Ret, typename... Args>
void LuaVM::BindNativeFunction(Ret(*funPtr)(Args...), const std::string &functionName)
{
	Reflect::Function *memberFunction = new Reflect::FreeFunction<Ret, Args...>(funPtr, functionName);  // create a function meta object
	mFreeFunctions.push_back(memberFunction);           // store the pointer to the dynamic memory to be deallocated when closing the VM

	lua_pushlightuserdata(L, (void*)memberFunction);    // push function meta object to the stack
	lua_pushcclosure(L, &InvokeFunction, 1);            // push the invoker on the stack with the function meta object as upvalue
	lua_setglobal(L, functionName.c_str());             // set a global with the name of the function 
}

// #TODO_std_tuple_ret
template <typename... Args>
void LuaVM::CallScriptFunction(std::string const &functionName, Args&&... args)  
{
	constexpr size_t numArgs = sizeof...(Args);

	int stackSize = lua_gettop(L);

	lua_getglobal(L, functionName.c_str());  // push function on stack

	if (lua_isfunction(L, -1))
	{
		// push args to Lua stack
	
		//(LuaStack<std::remove_reference_t<Args>>::ToLua(L, args), ...);  // C++17 fold expression

		PushArgsToLua(L, std::forward<Args>(args)...);                     // recursive function

		if (lua_pcall(L, numArgs, LUA_MULTRET, 0) != LUA_OK)  // pops function and arguments and call function
			std::cout << "error calling function: " + functionName << std::endl;
	}
	else
		std::cout << "can't find function: " + functionName << std::endl;;

	int numReturnValues = lua_gettop(L) - stackSize;
}

#endif  // LUA_VM_H
