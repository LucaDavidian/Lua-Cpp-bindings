#ifndef LUA_OBJECT_H
#define LUA_OBJECT_H

#include "LuaStack.h"

class LuaObject
{
	friend class LuaVM;

public:
	LuaObject(lua_State *L) : L(L), mLuaRef(LUA_NOREF), mLuaType(LuaType::NONE) {}
	LuaObject(lua_State *L, int luaRef) : L(L), mLuaRef(luaRef) 
	{
		mLuaType = static_cast<LuaType>(lua_rawgeti(L, LUA_REGISTRYINDEX, mLuaRef));
		lua_pop(L, 1);
	}

	LuaObject(const LuaObject &other) : L(other.L), mLuaType(other.mLuaType)
	{
		lua_rawgeti(L, LUA_REGISTRYINDEX, other.mLuaRef);
		mLuaRef= luaL_ref(L, LUA_REGISTRYINDEX);
	}

	LuaObject(LuaObject &&other) : L(other.L), mLuaRef(other.mLuaRef), mLuaType(other.mLuaType)
	{
		other.mLuaRef = LUA_NOREF;
	}

	~LuaObject()
	{
		luaL_unref(L, LUA_REGISTRYINDEX, mLuaRef);  // remove entry from registry and free the ref for reuse
	}

	LuaObject &operator=(LuaObject const &other);
	LuaObject &operator=(LuaObject &&other);
	
	//bool IsNil() const { lua_rawgeti(L, LUA_REGISTRYINDEX, mLuaRef); bool is = lua_type(L, -1) == LUA_TNIL; lua_pop(L, 1); return is; }
	bool IsNil() const { return mLuaRef == LUA_REFNIL; }
	bool IsBoolean() const { lua_rawgeti(L, LUA_REGISTRYINDEX, mLuaRef); bool is = lua_type(L, -1) == LUA_TBOOLEAN; lua_pop(L, 1);  return is; }
	bool IsString() const { lua_rawgeti(L, LUA_REGISTRYINDEX, mLuaRef); bool is = lua_type(L, -1) == LUA_TSTRING; lua_pop(L, 1); return is; }
	bool IsNumber() const { lua_rawgeti(L, LUA_REGISTRYINDEX, mLuaRef); bool is = lua_type(L, -1) == LUA_TNUMBER; lua_pop(L, 1); return is; }
	bool IsTable() const { lua_rawgeti(L, LUA_REGISTRYINDEX, mLuaRef); bool is = lua_type(L, -1) == LUA_TTABLE; lua_pop(L, 1); return is; }
	bool IsFunction() const { lua_rawgeti(L, LUA_REGISTRYINDEX, mLuaRef); bool is = lua_type(L, -1) == LUA_TFUNCTION; lua_pop(L, 1); return is; }
	bool IsUserdata() const { lua_rawgeti(L, LUA_REGISTRYINDEX, mLuaRef); bool is = lua_type(L, -1) == LUA_TUSERDATA; lua_pop(L, 1); return is; }
	bool IsLightUserdata() const { lua_rawgeti(L, LUA_REGISTRYINDEX, mLuaRef); bool is = lua_type(L, -1) == LUA_TLIGHTUSERDATA; lua_pop(L, 1); return is; }
	bool IsThread() const { lua_rawgeti(L, LUA_REGISTRYINDEX, mLuaRef); bool is = lua_type(L, -1) == LUA_TTHREAD; lua_pop(L, 1); return is; }

	template <typename T>
	T Cast()
	{
		//lua_geti(L, LUA_REGISTRYINDEX, mLuaRef);             // push value from registry on the stack
		lua_rawgeti(L, LUA_REGISTRYINDEX, mLuaRef);            // same - slightly faster
		auto value = LuaStack<T>::FromLua(L, lua_gettop(L));   // get value from stack
		lua_pop(L, 1);                                         // pop value from stack (balance)
		
		return value;
	}

	LuaObject Get(const std::string &key)
	{
		int refType = lua_rawgeti(L, LUA_REGISTRYINDEX, mLuaRef);  // push referenced value onto the stack, return the type

		lua_pushstring(L, key.c_str());  // push key
		lua_gettable(L, -2);             // pop key and table and push value

		if (lua_type(L, -1) == LUA_TNIL)
		{
			lua_pop(L, 1);
			return LuaObject(L);
		}

		return LuaObject(L, luaL_ref(L, LUA_REGISTRYINDEX));
	}

	LuaObject Get(int index)
	{
		int refType = lua_rawgeti(L, LUA_REGISTRYINDEX, mLuaRef);  // push referenced value onto the stack, return the type

		int valueType = lua_geti(L, -1, index);   // pop index and table and push value

		if (lua_type(L, -1) == LUA_TNIL)
		{
			lua_pop(L, 1);
			return LuaObject(L);
		}	

		return LuaObject(L, luaL_ref(L, LUA_REGISTRYINDEX));
	}

	bool IsValid() const
	{
		return mLuaRef != LUA_NOREF;
	}

	template <typename T>
	operator T() const
	{
		LuaType refType = static_cast<LuaType>(lua_rawgeti(L, LUA_REGISTRYINDEX, mLuaRef));  // push referenced value onto the stack, return the type

		T t = LuaStack<std::remove_cv_t<std::remove_reference_t<T>>>::FromLua(L, -1);
		lua_pop(L, 1);

		return t;
	}

	enum class LuaType
	{
		NONE = LUA_TNONE,
		NIL = LUA_TNIL,
		BOOLEAN = LUA_TBOOLEAN,
		LIGHTUSERDATA = LUA_TLIGHTUSERDATA,
		NUMBER = LUA_TNUMBER,
		STRING = LUA_TSTRING,
		TABLE = LUA_TTABLE,
		FUNCTION = LUA_TFUNCTION,
		USERDATA = LUA_TUSERDATA,
		THREAD = LUA_TTHREAD,
	};

private:
	lua_State *L;
	int mLuaRef;
	LuaType mLuaType;
};

#endif  // LUA_OBJECT_H