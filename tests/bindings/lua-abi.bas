'' FreeBASIC Lua 5.3 native ABI qualification
'' File: lua-abi.bas
'' Purpose: Compare C scalar and auxiliary structure layouts.
'' Responsibilities: Match the binding to the actual loaded runtime version.
'' This file contains no Lua implementation or substitute callback execution.

#lang "fb"
#include once "Lua/lua.bi"
#include once "Lua/lauxlib.bi"

Dim As Const lua_Number Ptr version = lua_version(0)
If version = 0 OrElse *version <> 503 Then End 1
Print "ABI "; SizeOf(lua_Number); " "; SizeOf(lua_Integer); " "; SizeOf(lua_Unsigned); " "; SizeOf(lua_Alloc); " "; _
	SizeOf(lua_Debug); " "; SizeOf(luaL_Buffer); " "; OffsetOf(luaL_Buffer, initb(0)); " "; SizeOf(luaL_Stream); " "; _
	SizeOf(UInteger); " "; LUA_VERSION_NUM
'' end of lua-abi.bas
