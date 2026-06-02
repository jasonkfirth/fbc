#include "fbcunit.bi"

#if not defined( __FB_DOS__ ) and not defined( __FB_JS__ )

''
'' FreeBASIC thread runtime tests
'' ------------------------------
''
'' File: stress.bas
''
'' Purpose:
''
''     Exercise repeated thread creation and mutex-protected shared state.
''
'' Responsibilities:
''
''     - create and join many short-lived threads
''     - verify mutex-protected aggregation under contention
''     - verify deterministic shared-state updates under contention
''
'' This file intentionally does NOT contain:
''
''     - scheduler fairness measurements
''     - platform-specific thread priorities
''     - detached-thread lifetime tests already covered by self.bas
''

SUITE( fbc_tests.threads.stress )

	#if defined( __FB_WII__ )
		''
		'' Wii LWP threads are real OS threads, but the console memory budget is
		'' much smaller than desktop targets.  Keep this as a real contention
		'' test without asking Dolphin or hardware to host a desktop-sized
		'' thread fan-out.
		''
		const THREAD_COUNT = 4
		const ROUNDS = 2
		const ITERATIONS = 100
	#else
		const THREAD_COUNT = 32
		const ROUNDS = 8
		const ITERATIONS = 400
	#endif

	dim shared as any ptr g_mutex
	dim shared as longint g_total
	dim shared as integer g_failures

	sub worker( byval arg as any ptr )
		dim as integer id = cast( integer, arg )
		dim as integer i
		dim as longint local_total = 0
		dim as string local_text

		for i = 1 to ITERATIONS
			local_total += id + i
			local_text = "thread-" + str( id ) + "-" + str( i )

			if( len( local_text ) <= 0 ) then
				MutexLock( g_mutex )
				g_failures += 1
				MutexUnlock( g_mutex )
			end if
		next

		MutexLock( g_mutex )
		g_total += local_total
		MutexUnlock( g_mutex )
	end sub

	function expected_total() as longint
		dim as integer id
		dim as integer i
		dim as longint total = 0

		for id = 0 to THREAD_COUNT - 1
			for i = 1 to ITERATIONS
				total += id + i
			next
		next

		function = total
	end function

	TEST( create_join_mutex_contention )
		dim as any ptr handles(0 to THREAD_COUNT - 1)
		dim as integer round_index
		dim as integer i

		for round_index = 0 to ROUNDS - 1
			g_mutex = MutexCreate()
			CU_ASSERT( g_mutex <> 0 )
			if( g_mutex = 0 ) then
				exit for
			end if

			g_total = 0
			g_failures = 0

			for i = 0 to THREAD_COUNT - 1
				handles(i) = ThreadCreate( @worker, cast( any ptr, i ) )
				CU_ASSERT( handles(i) <> 0 )
				if( handles(i) = 0 ) then
					g_failures += 1
				end if
			next

			for i = 0 to THREAD_COUNT - 1
				if( handles(i) <> 0 ) then
					ThreadWait( handles(i) )
				end if
			next

			CU_ASSERT_EQUAL( g_failures, 0 )
			CU_ASSERT_EQUAL( g_total, expected_total() )

			MutexDestroy( g_mutex )
			g_mutex = 0
		next
	END_TEST

END_SUITE

#endif

'' end of stress.bas
