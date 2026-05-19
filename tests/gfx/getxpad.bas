' TEST_MODE : COMPILE_AND_RUN_OK

const XPAD_STATUS_MISSING = 0
const XPAD_STATUS_CONNECTED = 1
const XPAD_STATUS_DISCONNECTED = 2

dim buttons as integer
dim lx as single
dim ly as single
dim rx as single
dim ry as single
dim lt as single
dim rt as single
dim dpad as integer
dim status as long

status = getxpad(0)
ASSERT( status >= XPAD_STATUS_MISSING )
ASSERT( status <= XPAD_STATUS_DISCONNECTED )

status = getxpad(0, buttons, lx)
ASSERT( status >= XPAD_STATUS_MISSING )
ASSERT( status <= XPAD_STATUS_DISCONNECTED )

if( status <> XPAD_STATUS_CONNECTED ) then
	ASSERT( buttons = 0 )
	ASSERT( lx = 0.0 )
end if

status = getxpad(0, buttons, lx, ly, rx, ry, lt, rt, dpad)
ASSERT( status >= XPAD_STATUS_MISSING )
ASSERT( status <= XPAD_STATUS_DISCONNECTED )

if( status = XPAD_STATUS_CONNECTED ) then
	ASSERT( lx >= -1.0 andalso lx <= 1.0 )
	ASSERT( ly >= -1.0 andalso ly <= 1.0 )
	ASSERT( rx >= -1.0 andalso rx <= 1.0 )
	ASSERT( ry >= -1.0 andalso ry <= 1.0 )
	ASSERT( lt >= 0.0 andalso lt <= 1.0 )
	ASSERT( rt >= 0.0 andalso rt <= 1.0 )
else
	ASSERT( buttons = 0 )
	ASSERT( lx = 0.0 )
	ASSERT( ly = 0.0 )
	ASSERT( rx = 0.0 )
	ASSERT( ry = 0.0 )
	ASSERT( lt = 0.0 )
	ASSERT( rt = 0.0 )
	ASSERT( dpad = 0 )
end if
