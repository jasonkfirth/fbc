/* FreeBASIC Lua 5.3 native ABI qualification
 * File: lua-abi.c
 * Purpose: Report actual C scalar and auxiliary structure layouts.
 * Responsibilities: Compare headers with the loaded Lua runtime version.
 * This file contains no Lua implementation or substitute callback execution.
 */
#include <lua.h>
#include <lauxlib.h>
#include <stddef.h>
#include <stdio.h>
int main(void)
{
    const lua_Number *version = lua_version(NULL);
    if (!version || *version != 503) return 1;
    printf("ABI %zu %zu %zu %zu %zu %zu %zu %zu %zu %d\n", sizeof(lua_Number),
           sizeof(lua_Integer), sizeof(lua_Unsigned), sizeof(lua_Alloc), sizeof(lua_Debug),
           sizeof(luaL_Buffer), offsetof(luaL_Buffer, initb), sizeof(luaL_Stream), sizeof(size_t), LUA_VERSION_NUM);
    return 0;
}
/* end of lua-abi.c */
