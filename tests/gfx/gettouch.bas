' TEST_MODE : COMPILE_AND_RUN_OK

#include once "fbgfx.bi"

dim x as integer = 123
dim y as integer = 456
dim id as integer = 789
dim result as long

ASSERT( gettouchcount() = 0 )
ASSERT( gettouchhit(0, 0, 10, 10) = 0 )
ASSERT( gettouchhit(5, 5, 3) = 0 )

result = gettouch(0, x, y, id)
ASSERT( result <> 0 )
ASSERT( x = -1 )
ASSERT( y = -1 )
ASSERT( id = -1 )

ASSERT( screenres(64, 64, 32, 1, fb.GFX_NULL) = 0 )

x = 123
y = 456
id = 789

ASSERT( gettouchcount() = 0 )
ASSERT( gettouchhit(0, 0, 10, 10) = 0 )
ASSERT( gettouchhit(5, 5, 3) = 0 )

result = gettouch(0, x, y, id)
ASSERT( result <> 0 )
ASSERT( x = -1 )
ASSERT( y = -1 )
ASSERT( id = -1 )

screen 0

