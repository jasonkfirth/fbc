'' FreeBASIC Lua callback example
'' File: minmax.bas
'' Purpose: Expose a numeric min/max function to the Lua 5.3 engine.
'' Responsibilities: Checked argument conversion, state ownership and script errors.
'' This example contains no Lua engine implementation or worker threads.
'' Build Win64 GAS64 callbacks with -eunwind: Lua errors use native longjmp.
''
'' Lua (script language) example
''
'' translated from a PB source, originally written by Marco Pontello (marcopon[at]myrealbox.com)
''

#include once "Lua/lua.bi"
#include once "Lua/lauxlib.bi"
#include once "Lua/lualib.bi"

function minmax cdecl(byval L as lua_State ptr) as long
	dim as long argcount = lua_gettop(L), isnum
	dim as lua_Number num, maxnum, minnum

	' Lua argument errors unwind through the engine. This callback keeps
	' only scalar locals, so no owned FreeBASIC strings or objects are skipped.
	if argcount < 1 then return luaL_error(L, "MinMax needs at least one number")
	minnum = lua_tonumberx(L, 1, @isnum)
	if isnum = 0 then return luaL_argerror(L, 1, "number expected")
	maxnum = minnum

	' Start from the first argument; zero is a valid value, not a sentinel.
	for i as long = 2 to argcount
		num = lua_tonumberx(L, i, @isnum)
		if isnum = 0 then return luaL_argerror(L, i, "number expected")
		if num > maxnum then maxnum = num
		if num < minnum then minnum = num
	next
	if lua_checkstack(L, 2) = 0 then return luaL_error(L, "MinMax result stack is full")

	'' Push the results on the virtual stack
	lua_pushnumber( L, minnum )
	lua_pushnumber( L, maxnum )

	'' Return number of returned parameters
	function = 2
end function

private function my_lua_Alloc cdecl _
	( _
		byval ud as any ptr, _
		byval p as any ptr, _
		byval osize as uinteger, _
		byval nsize as uinteger _
	) as any ptr

	' Lua owns every block and returns it to this same allocator.
	if( nsize = 0 ) then
		deallocate( p )
		function = NULL
	else
		function = reallocate( p, nsize )
	end if

end function

''
'' main
''
	dim L as lua_State ptr

	'' Create a lua state
	L = lua_newstate( @my_lua_Alloc, NULL )
	if L = NULL then
		print "Could not create the Lua state"
		end 1
	end if

#if 0
	'' Load only the base lua library (needed for 'print')
	lua_pushcfunction( L, @luaopen_base )
	lua_pushstring( L, "" )
	lua_call( L, 1, 0 )
#else
	'' Load all Lua libraries
	'' (including the base one, which is needed for 'print')
	luaL_openlibs( L )
#endif

	'' Register the function to be called from lua as MinMax
	lua_register( L, "MinMax", @minmax )

	'' Execute the lua script from a file
	dim as long status = luaL_dofile( L, exepath() + "/minmax.lua" )
	if status <> LUA_OK then
		dim as const zstring ptr message = lua_tostring(L, -1)
		if message <> NULL then print "Lua script failed: "; *message
	end if

	'' Release the lua state
	lua_close( L )

	print "Finished! Waiting for keypress to exit..."
	sleep
	if status <> LUA_OK then end 1
	end 0

'' end of minmax.bas
