'' An example program mixing FB graphics and multi-threading
''
'' The active flag is protected by activeLock. Text cursor output is protected
'' by locateLock, while drawing from worker threads is enclosed by ScreenLock
'' and ScreenUnlock. The main thread waits for every worker before destroying
'' the locks or allowing the state object to leave scope.

const SCREEN_W = 800
const SCREEN_H = 600
const SCREEN_DEPTH = 32
const RGB_COMPONENT_MAX = 255
const BOX_SIZE = 50
const BOX_DELAY_MS = 50
const MOUSE_DELAY_MS = 25
const MOUSE_CIRCLE_RADIUS = 20
const ESCAPE_KEY = 27
const MOUSE_STATUS_PADDING = 20
const KEY_STATUS_PADDING = 40
const HEX_BYTE_WIDTH = 2
const INFO_STATUS_ROW = 1
const MOUSE_STATUS_ROW = 2
const KEY_STATUS_ROW = 3

type THREAD_STATE
	active as integer
	activeLock as any ptr
	locateLock as any ptr
end type

sub stopWorkers( byref state as THREAD_STATE )
	mutexlock( state.activeLock )
	state.active = FALSE
	mutexunlock( state.activeLock )
end sub

sub randomBoxes( byval userdata as any ptr )
	dim state as THREAD_STATE ptr = cast(THREAD_STATE ptr, userdata)
	if state = 0 then exit sub

	var my_active = TRUE

	do
		mutexlock( state->activeLock )
		my_active = state->active
		mutexunlock( state->activeLock )

		if( my_active = FALSE ) then
			exit do
		end if

		var x = cint(rnd( ) * (SCREEN_W - BOX_SIZE))
		var y = cint(rnd( ) * (SCREEN_H - BOX_SIZE))

		var r = cint(rnd( ) * RGB_COMPONENT_MAX)
		var g = cint(rnd( ) * RGB_COMPONENT_MAX)
		var b = cint(rnd( ) * RGB_COMPONENT_MAX)

		screenlock( )
		line (x, y) - (x + BOX_SIZE, y + BOX_SIZE), rgb(r, g, b), bf
		screenunlock( )

		sleep BOX_DELAY_MS, 1
	loop
end sub

sub mouseMonitor( byval userdata as any ptr )
	dim state as THREAD_STATE ptr = cast(THREAD_STATE ptr, userdata)
	if state = 0 then exit sub

	var my_active = TRUE

	do
		mutexlock( state->activeLock )
		my_active = state->active
		mutexunlock( state->activeLock )

		if( my_active = FALSE ) then
			exit do
		end if

		dim as integer x, y, buttons
		getmouse x, y, , buttons

		mutexlock( state->locateLock )
		screenlock( )
		locate MOUSE_STATUS_ROW
		print "mouse: ";x;",";y;space( MOUSE_STATUS_PADDING )
		screenunlock( )
		mutexunlock( state->locateLock )

		'' Any buttons pressed, and mouse inside the window?
		if( (buttons <> 0) andalso (x >= 0) andalso (y >= 0) ) then
			screenlock( )
			circle (x, y), MOUSE_CIRCLE_RADIUS, _
			       rgb(RGB_COMPONENT_MAX, RGB_COMPONENT_MAX, RGB_COMPONENT_MAX), , , , f
			screenunlock( )
		end if

		sleep MOUSE_DELAY_MS, 1
	loop
end sub

sub hPrintInfo( byref state as THREAD_STATE )
	windowtitle "FB graphics + multi threading"
	mutexlock( state.locateLock )
	screenlock( )
	locate INFO_STATUS_ROW
	print "press ESC to exit, SPACE to clear screen, mouse buttons to draw some white circles."
	screenunlock( )
	mutexunlock( state.locateLock )
end sub

screenres SCREEN_W, SCREEN_H, SCREEN_DEPTH
if screenptr = 0 then
	print "Unable to create the graphics screen."
	end 1
end if

randomize( timer( ) )

dim state as THREAD_STATE
state.active = TRUE
state.activeLock = mutexcreate( )
if state.activeLock = 0 then
	print "Unable to create the worker-state mutex."
	end 1
end if

state.locateLock = mutexcreate( )
if state.locateLock = 0 then
	mutexdestroy state.activeLock
	print "Unable to create the text-output mutex."
	end 1
end if

var thread1 = threadcreate( @randomBoxes, @state )
var thread2 = threadcreate( @randomBoxes, @state )
var thread3 = threadcreate( @mouseMonitor, @state )

if (thread1 = 0) or (thread2 = 0) or (thread3 = 0) then
	stopWorkers state

	if thread1 <> 0 then threadwait thread1
	if thread2 <> 0 then threadwait thread2
	if thread3 <> 0 then threadwait thread3

	mutexdestroy state.activeLock
	mutexdestroy state.locateLock
	print "Unable to create all graphics worker threads."
	end 1
end if

hPrintInfo state
do
	var k = inkey( )
	if( len( k ) > 0 ) then
		mutexlock( state.locateLock )
		screenlock( )
		locate KEY_STATUS_ROW
		print "last key: " + hex( k[0], HEX_BYTE_WIDTH );
		if( len( k ) > 1 ) then
			print " ";hex( k[1], HEX_BYTE_WIDTH );
		end if
		print space( KEY_STATUS_PADDING )
		screenunlock( )
		mutexunlock( state.locateLock )

		select case( k )
		case chr( ESCAPE_KEY )
			exit do
		case " "
			mutexlock( state.locateLock )
			screenlock( )
			cls
			screenunlock( )
			mutexunlock( state.locateLock )
			hPrintInfo state
		end select
	end if

	sleep 50, 1
loop

stopWorkers state

threadwait( thread1 )
threadwait( thread2 )
threadwait( thread3 )

mutexdestroy( state.activeLock )
mutexdestroy( state.locateLock )
