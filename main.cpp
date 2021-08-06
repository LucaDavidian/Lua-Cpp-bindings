//extern "C"
//{
//	#include "lua.h"
//	#include "lauxlib.h"	
//	#include "lualib.h"
//}

//#include "lua.hpp"
//#include "allocator.hpp"
#include <cassert>
#include <iostream>
#include <string>

#define PRINT_STACK_SIZE()  std::cout << "stack size: " << lua_gettop(L) << std::endl

class Sprite
{
public:
	//Sprite() : Sprite(0.0f, 0.0f) {}
	//Sprite(float x, float y) : x(x), y(y) {}
	Sprite(float x = 0.0f, float y = 0.0f) : x(x), y(y) 
	{
		//std::cout << "creating Sprite" << std::endl;
	}

	~Sprite()
	{
		//std::cout << "destroying Sprite: " << x << ", " << y << std::endl;
	}

	void Move(float x, float y)
	{
		Sprite::x = x;
		Sprite::y = y;
	}

	void Draw()
	{
		std::cout << "x: " << x << " y: " << y << std::endl;
	}

	float GetX() const { return x; }
	float GetY() const { return y; }

	void SetX(float x) { this->x = x; }
	void SetY(float y) { this->y = y; }

	std::string GetName() const { return name; }
	void SetName(const std::string &name) { this->name = name; }
private:
	float x;
	float y;
	std::string name = "a sprite";
};

class SpriteManager
{
public:
	void ManageSprite(Sprite *sprite)
	{
		SpriteManager::sprite = sprite;
		std::cout << "managing sprite: " << sprite->GetX() << ", " << sprite->GetY() << std::endl;
	}

	void UnmanageSprite(Sprite *sprite)
	{
		if (this->sprite == sprite)
		{
			std::cout << "unmanaging sprite: " << sprite->GetX() << ", " << sprite->GetY() << std::endl;
			SpriteManager::sprite = nullptr;
		}
	}
private:
	Sprite *sprite;
} spriteManager;

//int main(int argc, char *argv[])
//{
//	//// create a new Lua VM/state 
//	
//	//lua_State *L = luaL_newstate();
//	
//	//// default allocation 
//	void *ud = nullptr;
//	lua_State *L = lua_newstate(LuaDefAlloc, ud);
//
//	//// arena allocator (#TODO)
//
//	// stack memory pool
//	//const unsigned int POOL_SIZE = 1024 * 100;
//	//unsigned char allocMem[POOL_SIZE];
//	//LuaArenaAllocator luaArenaAlloc(allocMem, sizeof allocMem);
//
//	//// dynamic memory pool
//	//LuaArenaAllocator luaArenaAlloc2(1024 * 100);
//
//	//lua_State *L = lua_newstate(LuaArenaAllocator::LuaAlloc, &luaArenaAlloc);
//
//	//// open Lua auxiliary lib
//	luaL_openlibs(L);
//
//	//// create a metatable, add it to the registry and push onto stack
//	luaL_newmetatable(L, "SpriteMetatable");
//
//	//// define metamethods
//	auto Index = +[](lua_State *L) -> int {
//		Sprite *sprite = static_cast<Sprite*>(lua_touserdata(L, -2));
//		const char *key = lua_tostring(L, -1);
//
//		if (std::string(key) == "x")
//			lua_pushnumber(L, sprite->GetX());
//		else if (std::string(key) == "y")
//			lua_pushnumber(L, sprite->GetY());
//		else
//		{
//			//// get uservalue 1 of user data
//			lua_getiuservalue(L, -2, 1);  
//			lua_pushstring(L, key);
//			lua_gettable(L, -2);
//
//			// if user value has no value with key search inside metatable
//			if (lua_isnil(L, -1))
//			{
//				luaL_getmetatable(L, "SpriteMetatable");
//				//lua_pushstring(L, key);
//				//lua_gettable(L, -2);
//				lua_getfield(L, -1, key);
//			}
//		}
//
//		return 1;
//	};
//
//	auto NewIndex = +[](lua_State *L) -> int {
//		Sprite *sprite = static_cast<Sprite*>(lua_touserdata(L, -3));  // user data 
//		const char *key = lua_tostring(L, -2);                         // key
//		                                                               // value at -1
//		if (std::string(key) == "x")
//			sprite->SetX(lua_tonumber(L, -1));
//		else if (std::string(key) == "y")
//			sprite->SetY(lua_tonumber(L, -1));
//		else
//		{
//			//// raise error
//			//lua_error(L);
//			//luaL_error(L, ("cannot assign to field " + std::string(key)).c_str());
//
//			//// user value (add fields to mapped type)
//			lua_getiuservalue(L, -3, 1);
//			//lua_pushstring(L, key);
//			//lua_pushvalue(L, -3);
//			//lua_settable(L, -3);
//			lua_pushvalue(L, -2);
//			lua_setfield(L, -2, key);
//		}
//
//		return 0;
//	};
//
//	auto DestroySprite = +[](lua_State *L) -> int {
//		Sprite *sprite = static_cast<Sprite*>(lua_touserdata(L, -1));
//
//		// use upvalue
//		SpriteManager *spriteManager = static_cast<SpriteManager*>(lua_touserdata(L, lua_upvalueindex(1)));
//		spriteManager->UnmanageSprite(sprite);
//
//		// destroy sprite
//		sprite->~Sprite();
//
//		return 0;
//	};
//
//	//// add metamethods to metatable
//
//	//lua_pushstring(L, "__index");
//	lua_pushcfunction(L, Index);
//	//lua_settable(L, -3);
//	lua_setfield(L, -2, "__index");
//
//	//lua_pushstring(L, "__newindex");
//	lua_pushcfunction(L, NewIndex);
//	//lua_settable(L, -3);
//	lua_setfield(L, -2, "__newindex");
//
//	//lua_pushstring(L, "__gc");
//	//lua_pushcfunction(L, DestroySprite);
//	//lua_settable(L, -3);
//	lua_pushlightuserdata(L, &spriteManager);
//	lua_pushcclosure(L, DestroySprite, 1);
//	lua_setfield(L, -2, "__gc");
//
//	//// bind Sprite member functions
//	auto NewSprite = [](lua_State *L) -> int {
//		//void *rawMem = lua_newuserdata(L, sizeof(Sprite));     // default: 1 user value
//		void *rawMem = lua_newuserdatauv(L, sizeof(Sprite), 1);  // 1 user value
//		new(rawMem) Sprite;
//
//		// get metatable from registry and push onto stack
//		luaL_getmetatable(L, "SpriteMetatable");
//
//		// set top of stack as metatable for element at index (userdata)
//		lua_setmetatable(L, -2);
//
//		// set user value (a table) for user data
//		lua_newtable(L);
//		//lua_setuservalue(L, -2);    // default set user value 1
//		lua_setiuservalue(L, -2, 1);  // Lua 5.4 multiple uservalues
//
//		// use upvalue 
//		SpriteManager *spriteManager = static_cast<SpriteManager*>(lua_touserdata(L, lua_upvalueindex(1)));
//		spriteManager->ManageSprite(static_cast<Sprite*>(rawMem));
//
//		return 1;
//	};
//
//	auto MoveSprite = [](lua_State *L) -> int {
//		Sprite *sprite = static_cast<Sprite*>(lua_touserdata(L, -3));
//		float x = lua_tonumber(L, -2);  // #TODO_conversion_check
//		float y = lua_tonumber(L, -1);
//
//		sprite->SetX(x);
//		sprite->SetY(y);
//
//		return 0;
//	};
//
//	auto DrawSprite = [](lua_State *L) -> int {
//		Sprite *sprite = static_cast<Sprite*>(lua_touserdata(L, -1));
//
//		sprite->Draw();
//
//		return 0;
//	};
//
//	//// add Sprite constructor to global table
//	//lua_pushcfunction(L, NewSprite);   // macro for lua_pushcclosure(L, (f), 0)
//	lua_pushlightuserdata(L, &spriteManager);
//	lua_pushcclosure(L, NewSprite, 1);
//	lua_setglobal(L, "newSprite");
//
//	//// add Sprite's member functions to Sprite's metatable
//	lua_pushstring(L, "move");
//	lua_pushcfunction(L, MoveSprite);
//	lua_settable(L, -3);
//
//	lua_pushstring(L, "draw");
//	lua_pushcfunction(L, DrawSprite);
//	lua_settable(L, -3);
//
//	//// aligned memory allocation
//	alignas(float) struct Thing
//	{
//		float x = 1.0f;
//		float y = 2.0f;
//	} thing;
//
//	Thing *t = (Thing*)lua_newuserdatauv(L, sizeof(Thing), 0);
//	assert((uintptr_t)t % alignof(float) == 0U);
//
//	//// run script
//	const char *LUA_SCRIPT = R"(
//		sprite = newSprite()
//		sprite:draw()             -- sprite.draw(sprite)
//		sprite:move(2.3, 5.5)     -- sprite.move(sprite, 2.3, 5.5)
//		sprite:draw()  
//		sprite.x = 10.3           -- access fields
//		sprite.y = 11.2
//		sprite.z = 10             -- dynamically add new field to object
//		print('x: ' .. sprite.x .. ' y: ' .. sprite.y .. ' z: ' .. sprite.z)    -- access fields  
//	)";
//
//	//// run script from C string
//	if (luaL_dostring(L, LUA_SCRIPT) != LUA_OK)
//		std::cout << lua_tostring(L, -1) << std::endl;
//
//	//// load and compile script from string
//	//if (luaL_loadstring(L, LUA_SCRIPT) != LUA_OK)
//	//	std::cout << "error loading script from string: " << std::string(lua_tostring(L, -1)) << std::endl;
//	//else
//	//	if (lua_pcall(L, 0, 0, 0) != LUA_OK)   // run script
//	//		std::cout << "error calling script: " << std::string(lua_tostring(L, -1)) << std::endl;
//	
//	//// load and run compiled script
//	//if (luaL_dofile(L, "scripts/script.lua") != LUA_OK)
//	//	std::cout << "error running compiled script: " << std::string(lua_tostring(L, -1)) << std::endl;
//
//
//	//// close Lua VM/state
//	lua_close(L);
//
//	return 0;
//}

#include "LuaVM.h"

std::string Print(const std::string &msg)
{
	std::cout << msg << std::endl;
	return "from C++: " + msg;
}

float GetSpriteX(Sprite &sprite)
{
	sprite.SetX(sprite.GetX() + 10.0f);
	return sprite.GetX();
}

int main(int argc, char *argv[])
{
	// register built-in types
	Reflect::Reflect<void>("void");
	Reflect::Reflect<bool>("bool");
	Reflect::Reflect<int>("int");
	Reflect::Reflect<std::size_t>("size_t")
		.AddConversion<int>();

	// attach stack conversion functions to reflected types
	Reflect::Reflect<float>("float")
		.AddMemberFunction(&LuaStack<float>::FromLua, "FromLua")
		.AddMemberFunction(LuaStack<float>::ToLua, "ToLua")
		.AddMemberFunction(LuaStack<float>::Is, "Is");

	Reflect::Reflect<std::string>("string")
		.AddMemberFunction(&LuaStack<std::string>::FromLua, "FromLua")
		.AddMemberFunction(&LuaStack<std::string>::ToLua, "ToLua")
		.AddMemberFunction(LuaStack<std::string>::Is, "Is");

	// conversions from Lua types to application types
	Reflect::Reflect<lua_Number>("lua_Number")
		.AddConversion<float>();

	// register types
	Reflect::Reflect<Sprite>("Sprite")
		.AddConstructor<>()
		.AddConstructor<float>()
		.AddConstructor<float, float>()
		.AddDataMember<&Sprite::SetX, &Sprite::GetX>("x")
		.AddDataMember<&Sprite::SetY, &Sprite::GetY>("y")
		.AddDataMember<&Sprite::SetName, &Sprite::GetName>("name")
		.AddMemberFunction(&Sprite::Move, "move")    
		.AddMemberFunction(&Sprite::Draw, "draw")
		.AddMemberFunction(&Sprite::GetName, "get_name")
		.AddMemberFunction(&Sprite::SetName, "set_name")
		.AddMemberFunction(&LuaStack<Sprite>::FromLua, "FromLua")
		.AddMemberFunction(&LuaStack<Sprite>::ToLua, "ToLua")
		.AddMemberFunction(LuaStack<Sprite>::Is, "Is");

	static const char *LUA_SCRIPT = R"(

	-- a Lua script

	--s = cprint('running a Lua script...')                -- call C++ free function from Lua
	--print(s)
	--
	---- user-defined types:
	--local sprite1 = Sprite.new()                         -- create object (global table Sprite with new)
	--local sprite2 = Sprite.new(3.5, 5.5)                 -- constructor with arguments
	--
	--print('drawing sprite from Lua '); sprite1:draw()              -- call object's methods (__index)
	--print('moving sprite to 53, 22:'); sprite1:move(53, 22)        -- same as sprite.move(sprite, 1, 2)    
	--print('drawing sprite after move:'); sprite1:draw()
	--
	--sprite2.x = 9                                         -- access object properties - write (__newindex)
	--local x = sprite2.x                                   -- access object properties - read  (__index)	
	--print('x = ' .. x)
	--
	--sprite1.y = 21                                       
	--print('sprite: ' .. 'x: ' .. sprite1.x .. ' y: ' .. sprite1.y)
	--sprite1.draw(sprite1)
	--
	--print(sprite1:get_name())                                -- methods with return value
	--
	--sprite1.z = 42                                                           -- add properties to native object (__newindex and type user value table)
	--print('x: ' .. sprite1.x .. ' y: ' .. sprite1.y .. ' z: ' .. sprite1.z)    -- access non-native property (__index and type user value table)
    --            
	--print('sprite x: ' .. getSpriteX(sprite1))   -- call C++ free function from Lua

	function foo(a, b)                           -- call Lua function from C++ code
		print('result = ' .. a + b)
		return a + b
	end

	doSomething = function(sprite)                       -- C++ can call Lua function passing user defined types (lightuserdata has the same metatable as full userdata)
		print('in doSomething: ' .. getSpriteX(sprite))
		print(sprite.x)
		sprite.y = 3.3
		--sprite.z = 10  -- error
	end

	function doSomethingElse()
		print('not really')
	end

	)";

	// create a Lua state/VM
	LuaVM script;

	// bind types to Lua (all reflection data is exposed to Lua)
	script.BindNativeType<Sprite>();

	// register free function
	script.BindNativeFunction(&Print, "cprint");
	script.BindNativeFunction(&GetSpriteX, "getSpriteX");

	if (!script.LoadString(LUA_SCRIPT))
	{
		std::cout << "error loading Lua script\n";
		return -1;
	}

	if (!script.Execute())
	{
		std::cout << "error executing Lua script\n";
		return -2;
	}

	script.CallScriptFunction("foo", 1, 34);

	Sprite sprite(8.0f, 0.3f);
	std::cout << "before script: ";	sprite.Draw();
	script.CallScriptFunction("doSomething", sprite);  // #TODO_pass_by_value_or_reference
	std::cout << "after script: "; sprite.Draw();

	script.CallScriptFunction("doSomethingElse");
	
	{
		LuaObject lo = script.GetGlobal("doSomething");
		std::cout << "is function: " << std::boolalpha << lo.IsFunction() << std::endl;

		LuaObject lo2 = script.GetGlobal("x");
		if (lo2.IsNumber())
			std::cout << "x = " << lo2.Cast<int>() << std::endl;
	}  // Lua objects destroyed here

	if (!script.LoadFile("scripts/script.lua"))
	{
		std::cout << "error loading Lua script\n";
		return -1;
	}

	if (!script.Execute())
	{
		std::cout << "error executing Lua script\n";
		return -2;
	}

	{
		LuaObject levelTable = script.GetGlobal("level");

		std::cout << "level is a table? " << std::boolalpha << levelTable.IsTable() << std::endl;

		LuaObject name = levelTable.Get("player").Get("name");
		std::string nameString = levelTable.Get("player").Get("name");

		if (name.IsValid())
		{
			std::cout << "name is: " << name.Cast<std::string>() << std::endl;
			std::cout << "name is: " << nameString << std::endl;
		}

		LuaObject transformComponentTable = levelTable.Get("player").Get("components").Get("transform");

		struct TransformComponent
		{
			float x, y, z;
		};

		TransformComponent tc = { transformComponentTable.Get(1), transformComponentTable.Get(2), transformComponentTable.Get(3) };
		std::cout << "Transform Component: x = " << tc.x << ", y = " << tc.y << ", z = " << tc.z << std::endl;

		LuaObject componentTable = levelTable.Get("player").Get("components");

		struct SpriteComponent
		{
			std::string name;
			int numFrames;
		} sc = { componentTable.Get("sprite").Get("name"), componentTable.Get("sprite").Get("num_frames") };

		std::cout << "sprite file: " << sc.name << ", sprite frames: " << sc.numFrames << std::endl;
	}

	Sprite sprite2;
	sprite2.SetName("John Doe");
	sprite.SetName("Hank Marshall");

	script.CallScriptFunction("a_Lua_function", "a string from C++", 10, 23, sprite, sprite2);
	std::cout << "sprite names: " << sprite.GetName() << ", " << sprite2.GetName() << std::endl;

	script.CallScriptFunction("a_Lua_function", "a string from C++", 10, 23, Reflect::AnyRef(sprite), Reflect::AnyRef(sprite2));
	std::cout << "sprite names: " << sprite.GetName() << ", " << sprite2.GetName() << std::endl;

	// close Lua state/VM
	script.Close(); 

	return 0;
}