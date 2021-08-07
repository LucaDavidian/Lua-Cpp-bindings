#ifndef LUA_STACK_H
#define LUA_STACK_H

/*
* Stack primary class template and explicit specializations provide interface with Lua stack
* 
* Lua has 9 types:
* 
* -1: (LUA_TNONE) 
* 0 : LUA_TNIL
* 1 : LUA_TBOOLEAN
* 2 : LUA_TLIGHTUSERDATA
* 3 : LUA_TNUMBER
* 4 : LUA_TSTRING
* 5 : LUA_TTABLE
* 6 : LUA_TFUNCTION
* 7 : LUA_TUSERDATA
* 8 : LUA_TTHREAD
*  
* the Stack structure handles conversions between Lua types to native types (primitive and user defined) in both directions
*/

#include "include/lua.hpp"

template <typename T>
struct LuaStack
{
	/*
	* All user defined types are passed to/from Lua as userdata containing Any variables
	* arguments to be passed by value are passed as-is, arguments to be passed by reference are wrapper in AnyRef;
	* arguments passed by value are full userdata, but the __gc metamethod invokes the AnyRef destructor, which does nothing
	*/

	static void ToLua(lua_State *L, Reflect::Any object)   
	{
		void *rawMem = lua_newuserdata(L, sizeof(Reflect::Any));   
		new (rawMem) Reflect::Any(std::move(object));

		luaL_getmetatable(L, "metatable");
		lua_setmetatable(L, -2);
	}

	// other possible semantics for argument passing:
	// move value to Lua (release ownership)
	// share value with Lua (shared_ptr)

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
struct LuaStack<bool>
{
	static void ToLua(lua_State *L, bool b)
	{
		lua_pushboolean(L, b);
	}

	static bool FromLua(lua_State *L, int index)
	{
		int result = lua_toboolean(L, index);
		return (bool)result;
	}

	static bool Is(lua_State *L, int index)
	{
		return lua_isboolean(L, index);
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
	static void ToLua(lua_State *L, unsigned int i)
	{
		lua_pushinteger(L, i);
	}

	static unsigned int FromLua(lua_State *L, int index)
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
struct LuaStack<long int>
{
	static void ToLua(lua_State *L, long int i)
	{
		lua_pushinteger(L, i);
	}

	static long int FromLua(lua_State *L, int index)
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
struct LuaStack<unsigned long int>
{
	static void ToLua(lua_State *L, unsigned long int i)
	{
		lua_pushinteger(L, i);
	}

	static unsigned long int FromLua(lua_State *L, int index)
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
struct LuaStack<long long int>
{
	static void ToLua(lua_State *L, long long int i)
	{
		lua_pushinteger(L, i);
	}

	static long long int FromLua(lua_State *L, int index)
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
struct LuaStack<unsigned long long int>
{
	static void ToLua(lua_State *L, unsigned long long int i)
	{
		lua_pushinteger(L, i);
	}

	static unsigned long long int FromLua(lua_State *L, int index)
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
struct LuaStack<double>
{
	static void ToLua(lua_State *L, double f)
	{
		lua_pushnumber(L, f);
	}

	static double FromLua(lua_State *L, int index)
	{
		lua_Number result = lua_tonumber(L, index);
		return result;
	}

	static bool Is(lua_State *L, int index)
	{
		return lua_isnumber(L, index);
	}
};

template <unsigned N>
struct LuaStack<const char[N]>
{
	static void ToLua(lua_State *L, const char *cString)
	{
		lua_pushstring(L, cString);
	}

	const char *FromLua(lua_State *L, int index)
	{
		return lua_tostring(L, index);
	}

	static bool Is(lua_State *L, int index)
	{
		return lua_isstring(L, index);
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

// helper functions to push values onto Lua Stack
inline void PushArgsToLua(lua_State *L) 
{
	(void*)L;
}

template <typename Arg, typename... Rest>
void PushArgsToLua(lua_State *L, Arg &&arg, Rest&&... rest)
{
	LuaStack<std::remove_cv_t<std::remove_reference_t<Arg>>>::ToLua(L, std::forward<Arg>(arg));

	PushArgsToLua(L, std::forward<Rest>(rest)...);
}

#endif  // LUA_STACK_H