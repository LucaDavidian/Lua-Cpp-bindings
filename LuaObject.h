#ifndef LUA_OBJECT_H
#define LUA_OBJECT_H

#include "LuaStack.h"
#include <cassert>

class LuaObject
{
	friend class LuaVM;

public:

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

public:
	LuaObject(lua_State *L = nullptr) : L(L), mLuaRef(LUA_NOREF), mLuaType(LuaType::NONE) {}

	LuaObject(lua_State *L, int luaRef) : L(L), mLuaRef(luaRef) 
	{
		mLuaType = static_cast<LuaType>(lua_rawgeti(L, LUA_REGISTRYINDEX, mLuaRef));
		lua_pop(L, 1);
	}

	LuaObject(const LuaObject &other) : L(other.L)
	{
		mLuaType = static_cast<LuaType>(lua_rawgeti(L, LUA_REGISTRYINDEX, other.mLuaRef));  // get the referenced object from registry and push on the stack
		mLuaRef = luaL_ref(L, LUA_REGISTRYINDEX);                                            // pop the object, store in registry and return the reference 
	}

	LuaObject(LuaObject &&other) : L(other.L), mLuaRef(other.mLuaRef), mLuaType(other.mLuaType)
	{
		other.mLuaType = LuaType::NONE;
		other.mLuaRef = LUA_NOREF;
	}

	~LuaObject()
	{
		if (mLuaRef != LUA_NOREF)
			luaL_unref(L, LUA_REGISTRYINDEX, mLuaRef);  // remove entry from registry and free the ref id for reuse

		if (mLuaCurrentKey != LUA_NOREF)
			luaL_unref(L, LUA_REGISTRYINDEX, mLuaRef);
	}

	LuaObject &operator=(LuaObject const &other)
	{
		LuaObject temp(other);
		Swap(temp);

		return *this;
	}

	LuaObject &operator=(LuaObject &&other)
	{
		LuaObject temp(std::move(other));
		Swap(temp);

		return *this;
	}

	void Swap(LuaObject &other)
	{
		int tempLuaRef = mLuaRef;
		mLuaRef = other.mLuaRef;
		other.mLuaRef = tempLuaRef;

		lua_State *tempL = L;
		L = other.L;
		other.L = tempL;

		LuaType tempLuaType = mLuaType;
		mLuaType = other.mLuaType;
		other.mLuaType = tempLuaType;
	}
	
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
		//lua_geti(L, LUA_REGISTRYINDEX, mLuaRef);                                             // push value from registry on the stack
		LuaType refType = static_cast<LuaType>(lua_rawgeti(L, LUA_REGISTRYINDEX, mLuaRef));    // same - slightly faster (no metatable lookup)
		auto value = LuaStack<T>::FromLua(L, lua_gettop(L));                                   // get value from stack
		lua_pop(L, 1);                                                                         // pop value from stack
		
		return value;
	}

	LuaObject Get(const std::string &key)
	{
		LuaType refType = static_cast<LuaType>(lua_rawgeti(L, LUA_REGISTRYINDEX, mLuaRef));  // push referenced value onto the stack, return the type

		lua_pushstring(L, key.c_str());  // push key
		lua_gettable(L, -2);             // pop key and push value

		if (lua_type(L, -1) == LUA_TNIL)
		{
			lua_pop(L, 1);
			return LuaObject(L);
		}

		LuaObject luaObject(L, luaL_ref(L, LUA_REGISTRYINDEX));
		
		lua_pop(L, 1);  // pop table

		return luaObject;
	}

	LuaObject Get(int index)
	{
		LuaType refType = static_cast<LuaType>(lua_rawgeti(L, LUA_REGISTRYINDEX, mLuaRef));  // push referenced value onto the stack, return the type
		LuaType valueType = static_cast<LuaType>(lua_geti(L, -1, index));                    // push value

		if (lua_type(L, -1) == LUA_TNIL)
		{
			lua_pop(L, 1);
			return LuaObject(L);
		}	

		LuaObject luaObject(L, luaL_ref(L, LUA_REGISTRYINDEX));
		
		lua_pop(L, 1);  // pop table

		return luaObject;
	}

	std::pair<LuaObject, LuaObject> GetNext()
	{
		LuaObject value;
		LuaObject key;

		LuaType refType = static_cast<LuaType>(lua_rawgeti(L, LUA_REGISTRYINDEX, mLuaRef));  // push referenced object on the stack

		if (mLuaCurrentKey == LUA_NOREF)
			lua_pushnil(L);
		else
			lua_rawgeti(L, LUA_REGISTRYINDEX, mLuaCurrentKey);  // push last key onto stack

		int hasNext = lua_next(L, -2);
		assert(hasNext);

		lua_pushvalue(L, -2);                                  // copy key on stack
		key = LuaObject(L, luaL_ref(L, LUA_REGISTRYINDEX));    // pop key, store in registry and return ref
		value = LuaObject(L, luaL_ref(L, LUA_REGISTRYINDEX));  // pop value, store in registry and return ref
		
		luaL_unref(L, LUA_REGISTRYINDEX, mLuaCurrentKey);      // unref old key
		mLuaCurrentKey = luaL_ref(L, LUA_REGISTRYINDEX);       // pop last key and store in registry for iteration

		lua_pop(L, 1);  // pop table

		return std::make_pair(key, value);
	}

	void Reset()
	{
		mLuaCurrentKey = LUA_NOREF;
	}

	bool HasNext() const
	{
		if (!IsValid())
			return false;

		LuaType refType = static_cast<LuaType>(lua_rawgeti(L, LUA_REGISTRYINDEX, mLuaRef));

		if (mLuaCurrentKey == LUA_NOREF)
			lua_pushnil(L);
		else
			LuaType keyRefType = static_cast<LuaType>(lua_rawgeti(L, LUA_REGISTRYINDEX, mLuaCurrentKey));

		bool hasNext = (bool)lua_next(L, -2);  // pop the key and push key-value pair

		if (hasNext)
			lua_pop(L, 3);  // pop table and key-value pair
		else
		{
			lua_pop(L, 1);  // pop table
			mLuaCurrentKey = LUA_NOREF;
		}

		return hasNext;		
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

private:
	lua_State *L;
	LuaType mLuaType;
	int mLuaRef;
	
	int mutable mLuaCurrentKey = LUA_NOREF;
};

#endif  // LUA_OBJECT_H