/'
    FreeBASIC DOS provider tests: freebasic-tcp.bas
    Verify TCP byte I/O against an independent echo peer while graphics and
    a worker thread are optionally active. Thread state is protected by the
    test mutex. This does not implement a network server or game protocol.
'/

#lang "fb"
#include once "fbgfx.bi"

dim shared testMutex as any ptr
dim shared stopWorker as integer

private sub phase( byref message as const string )
	dim handle as integer = freefile
	if open( "PHASE.TXT" for append as #handle ) <> 0 then end 1
	print #handle, message
	close #handle
end sub

private sub fail( byref message as const string )
	phase( "FAIL " & message )
	screen 0
	end 1
end sub

private sub worker( byval parameter as any ptr )
	do
		MutexLock( testMutex )
		dim finished as integer = stopWorker
		MutexUnlock( testMutex )
		if finished then exit do
		sleep 1, 1
	loop
end sub

'' Mode bits: 1 enables VESA graphics, 2 starts the thread before OPEN TCP.
dim modeBits as integer
if len( command( 1 ) ) then modeBits = valint( command( 1 ) )
if modeBits < 0 or modeBits > 3 then fail( "mode must be 0 through 3" )
dim originalNoexcept as string = environ( "WATT32-NOEXC" )
phase( "start mode=" & str( modeBits ) )

if modeBits and 1 then
	if ScreenRes( 640, 480, 15, 2 ) <> 0 then fail( "ScreenRes" )
	ScreenSet 1, 0
	line (0, 0)-(639, 479), rgb(32, 96, 160), bf
	ScreenCopy 1, 0
end if

dim threadHandle as any ptr
if modeBits and 2 then
	testMutex = MutexCreate()
	if testMutex = 0 then fail( "MutexCreate" )
	threadHandle = ThreadCreate( @worker )
	if threadHandle = 0 then fail( "ThreadCreate" )
end if

dim hostName as string = "10.0.2.2"
if len( command( 2 ) ) then hostName = command( 2 )
dim payloadBytes as integer = 512
if len( command( 3 ) ) then payloadBytes = valint( command( 3 ) )
if payloadBytes < 1 or payloadBytes > 32768 then fail( "payload must be 1 through 32768 bytes" )
dim blockBytes as integer = 1
if len( command( 4 ) ) then blockBytes = valint( command( 4 ) )
if blockBytes < 1 or blockBytes > 4096 then fail( "block must be 1 through 4096 bytes" )
dim transfer(0 to 4095) as ubyte
dim netFile as integer = freefile
dim openResult as integer = open tcp( "host=" & hostName & ",port=7779,timeout=1000" as #netFile )
if environ( "WATT32-NOEXC" ) <> originalNoexcept then fail( "environment changed" )
if openResult <> 0 then
	sleep 30, 1
	fail( "OPEN TCP status=" & str( openResult ) )
end if
phase( "connected" )

'' Force scheduling after socket initialization, even on a very fast host.
'' Watt's default SIGILL handler used to terminate the process at this point.
sleep 30, 1

'' Fixed byte pattern, independent of INTEGER size and native byte order.
dim sent as integer
while sent < payloadBytes
	dim amount as integer = iif( payloadBytes - sent < blockBytes, payloadBytes - sent, blockBytes )
	for i as integer = 0 to amount - 1
		transfer(i) = (17 * (sent + i) + 3) and 255
	next
	if put( #netFile, , transfer(0), amount ) <> 0 then fail( "PUT" )
	sent += amount
wend
phase( "sent " & str( payloadBytes ) & " bytes" )

dim received as integer
while received < payloadBytes
	dim started as double = timer
	while eof( netFile )
		if eoc( netFile ) then fail( "closed before complete reply" )
		dim elapsed as double = timer - started
		'' x87 can retain more precision in TIMER's return than in a stored DOUBLE.
		'' A tiny negative rounding difference is not a midnight rollover.
		if elapsed < -43200 then elapsed += 86400
		if elapsed > 5 then fail( "receive timeout after " & str( received ) & " bytes, elapsed=" & str( timer - started ) )
		sleep 1, 1
	wend
	'' EOF/EOC may fill a DOS receive buffer, but must not consume its bytes.
	if eof( netFile ) or eoc( netFile ) then fail( "readiness lost buffered data" )
	if loc( netFile ) < 1 then fail( "LOC lost buffered data" )
	dim amount as integer = iif( payloadBytes - received < blockBytes, payloadBytes - received, blockBytes )
	dim count as uinteger
	if get( #netFile, , transfer(0), amount, count ) <> 0 then fail( "GET" )
	if count = 0 or count > cuint( amount ) then fail( "invalid GET byte count" )
	for i as integer = 0 to cint( count ) - 1
		if transfer(i) <> ((17 * (received + i) + 3) and 255) then fail( "wrong reply byte" )
	next
	received += count
wend
phase( "received " & str( received ) & " bytes" )

close #netFile
if threadHandle then
	MutexLock( testMutex )
	stopWorker = 1
	MutexUnlock( testMutex )
	ThreadWait( threadHandle )
	MutexDestroy( testMutex )
end if
if modeBits and 1 then screen 0
phase( "PASS TCP echo and thread cleanup" )
end 0

' end of freebasic-tcp.bas
