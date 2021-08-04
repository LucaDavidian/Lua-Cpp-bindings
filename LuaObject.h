#ifndef LUA_OBJECT_H
#define LUA_OBJECT_H

class LuaObject
{
	friend class LuaVM;

public:
	LuaObject(lua_State *L) : L(L), mLuaRef(LUA_NOREF) {}
	LuaObject(lua_State *L, int luaRef) : L(L), mLuaRef(luaRef) {}

	LuaObject(const LuaObject &other) : L(other.L)
	{
		lua_rawgeti(L, LUA_REGISTRYINDEX, other.mLuaRef);
		mLuaRef= luaL_ref(L, LUA_REGISTRYINDEX);
	}

	LuaObject(LuaObject &&other) : L(other.L), mLuaRef(other.mLuaRef)
	{
		other.mLuaRef = LUA_NOREF;
	}

	~LuaObject()
	{
		luaL_unref(L, LUA_REGISTRYINDEX, mLuaRef);  // remove entry from registry and free the ref for reuse
	}

	LuaObject &operator=(LuaObject const &other);
	LuaObject &operator=(LuaObject &&other);
	
	//bool IsNil() const { lua_rawgeti(L, LUA_REGISTRYINDEX, mLuaRef); bool is = lua_type(L, -1) == LUA_TNIL; lua_pop(L, 1);return is; }
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

private:
	lua_State *L;
	int mLuaRef;
};

#endif  // LUA_OBJECT_H