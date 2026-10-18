''
'' FreeBASIC runtime tests: live.bas
'' Exercise the public secure-random ABI, output bounds and failure clearing.
'' Live runs also check separate threaded buffers. The controlled-provider
'' build is separate and does not establish entropy or live platform support.
''
#lang "fb"
#include once "fbrandom.bi"

dim shared as long check_count

sub require( byval condition as integer, byref description as const string )
	check_count += 1
	if condition = 0 then
		print "SECURE_RANDOM_FAIL="; description
		end 1
	end if
end sub

sub fill_span( byval buffer_data as ubyte ptr, byval byte_count as uinteger )
	for index as uinteger = 0 to byte_count - 1
		buffer_data[index] = &ha5
	next
end sub

sub check_guards( byval buffer_data as ubyte ptr, byval byte_count as uinteger )
	for index as uinteger = 0 to 15
		require( buffer_data[index] = &ha5, "prefix canary" )
		require( buffer_data[16 + byte_count + index] = &ha5, "suffix canary" )
	next
end sub

sub check_zero( byval buffer_data as ubyte ptr, byval byte_count as uinteger )
	for index as uinteger = 0 to byte_count - 1
		if buffer_data[index] <> 0 then
			require( 0, "complete failure wipe" )
		end if
	next
	require( -1, "complete failure wipe" )
end sub

#ifdef TEST_SECURE_RANDOM_FAILURE
extern "C"
	declare sub TestSecureRandomMode( byval mode as long )
end extern
#endif

#if __FB_MT__
type RandomWorker
	status as long
	buffer_data(0 to 63) as ubyte
end type

sub read_worker( byval argument as any ptr )
	dim as RandomWorker ptr job = argument
	for iteration as long = 1 to 32
		job->status = FBC.secureRandom( @job->buffer_data(0), 64 )
		if job->status <> FB.SECURE_RANDOM_OK then exit sub
	next
end sub
#endif

dim as ubyte small(0 to 95)
fill_span( @small(0), 96 )
require( FBC.secureRandom( 0, 0 ) = FB.SECURE_RANDOM_OK, "NULL empty span" )
require( FBC.secureRandom( @small(16), 0 ) = FB.SECURE_RANDOM_OK, "empty span" )
require( FBC.secureRandom( 0, 1 ) = FB.SECURE_RANDOM_INVALID_ARGUMENT, "NULL nonempty span" )
require( FBC.secureRandom( @small(16), FB.SECURE_RANDOM_MAX_BYTES + 1 ) = _
	FB.SECURE_RANDOM_INVALID_ARGUMENT, "request above bound" )
require( FBC.secureRandom( @small(16), not cuint(0) ) = _
	FB.SECURE_RANDOM_INVALID_ARGUMENT, "converted negative count" )
for index as long = 0 to 95
	require( small(index) = &ha5, "invalid and empty calls leave memory unchanged" )
next

dim as uinteger sizes(0 to 8) = { 1, 15, 31, 32, 255, 256, 257, 4096, FB.SECURE_RANDOM_MAX_BYTES }
dim as integer unavailable = command(1) = "--unavailable"
for size_index as long = 0 to ubound(sizes)
	dim as uinteger byte_count = sizes(size_index)
	dim as ubyte ptr storage = allocate( byte_count + 32 )
	require( storage <> 0, "test allocation" )
#ifdef TEST_SECURE_RANDOM_FAILURE
	for mode as long = 1 to 3
		fill_span( storage, byte_count + 32 )
		TestSecureRandomMode( mode )
		dim as long status = FBC.secureRandom( storage + 16, byte_count )
		dim as long expected_status = iif(mode = 1, FB.SECURE_RANDOM_UNAVAILABLE, FB.SECURE_RANDOM_IO_ERROR)
		require( status = expected_status, "controlled provider status" )
		check_guards( storage, byte_count )
		check_zero( storage + 16, byte_count )
	next
	TestSecureRandomMode( 0 )
	fill_span( storage, byte_count + 32 )
	require( FBC.secureRandom( storage + 16, byte_count ) = FB.SECURE_RANDOM_OK, "controlled success" )
	check_guards( storage, byte_count )
#else
	fill_span( storage, byte_count + 32 )
	dim as long status = FBC.secureRandom( storage + 16, byte_count )
	if unavailable then
		require( status = FB.SECURE_RANDOM_UNAVAILABLE, "explicit unavailable platform" )
		check_zero( storage + 16, byte_count )
	else
		require( status = FB.SECURE_RANDOM_OK, "live system provider" )
	end if
	check_guards( storage, byte_count )
#endif
	deallocate( storage )
next

#ifndef TEST_SECURE_RANDOM_FAILURE
if unavailable = 0 then
	dim as ubyte first(0 to 63), second(0 to 63)
	require( FBC.secureRandom( @first(0), 64 ) = FB.SECURE_RANDOM_OK, "first sample" )
	require( FBC.secureRandom( @second(0), 64 ) = FB.SECURE_RANDOM_OK, "second sample" )
	dim as integer differs = 0
	for index as long = 0 to 63
		if first(index) <> second(index) then differs = -1
	next
	'' A repetition sanity check is not an entropy estimate or cryptographic proof.
	require( differs, "distinct live samples" )
#if __FB_MT__
	dim as RandomWorker jobs(0 to 3)
	dim as any ptr handles(0 to 3)
	dim as long started
	for index as long = 0 to 3
		handles(index) = threadcreate( procptr(read_worker), @jobs(index) )
		if handles(index) = 0 then exit for
		started += 1
	next
	for index as long = 0 to started - 1
		threadwait( handles(index) )
	next
	require( started = 4, "worker creation and joins" )
	for index as long = 0 to started - 1
		require( jobs(index).status = FB.SECURE_RANDOM_OK, "concurrent live provider" )
	next
#endif
end if
#endif

#ifdef TEST_SECURE_RANDOM_FAILURE
print "SECURE_RANDOM_CONTROLLED_FAILURES=PASS checks="; check_count
#else
if unavailable then
	print "SECURE_RANDOM_UNAVAILABLE=PASS checks="; check_count
else
	print "SECURE_RANDOM_NATIVE=PASS checks="; check_count
end if
#endif
end 0
'' end of live.bas
