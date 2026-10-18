'' FreeBASIC graphics tests: drop-receiver.bas
'' Purpose: Observe native drops through the public graphics API.
'' Responsibilities: Unicode copies, event peeking, disabled and expired payloads.
'' Native senders live in separate fixtures; no private queue is modified here.
#lang "fb"
#include once "fbdrop.bi"
#include once "crt/stdio.bi"
#ifdef __FB_WIN32__
	extern "C"
		declare function fb_test_drop( byval window as any ptr ) as long
	end extern
#endif

sub require( byval condition as long, byref message as const string )
	if condition = 0 then
		fprintf(stderr, !"DROP_FAIL=%s\n", strptr(message))
		screen 0
		end 1 ' fblint: disable-line FBL-CF-005 -- A failed assertion terminates this isolated test process after SCREEN 0.
	end if
end sub

#if defined(__FB_WIN32__) and not defined(__FB_GFXLIB3__)
	screencontrol FB.SET_DRIVER_NAME, "GDI"
#endif
require(screenres(240, 128, 32) = 0, "screenres")
#assert sizeof(FB.EVENT) = 20
require(FBC.screenDropEnable(true) = FB.DROP_OK, "drop capability and BASIC TRUE")
dim window_handle as integer
screencontrol FB.GET_WINDOW_HANDLE, window_handle
printf(!"DROP_WINDOW=%llu\n", culngint(window_handle))
fflush(0)
#ifdef __FB_WIN32__
	require(fb_test_drop(cptr(any ptr, window_handle)) <> 0, "native source")
#endif

dim expected_kind as long = FB.DROP_FILES
if command(1) = "text" then expected_kind = FB.DROP_TEXT
dim refusal as long = iif(command(1) = "refuse", 1, 0)
dim found as long = 0
dim saved_id as long = 0
dim event_data as FB.EVENT
dim maximum_ticks as long = iif(command(2) = "timeout", 700, 200)
for tick as long = 1 to maximum_ticks
	if screenevent(0) then
		'' Peeking must retain both the event and its copied payload.
		require(screenevent(0) <> 0, "repeat event peek")
	end if
	while screenevent(@event_data)
		if event_data.type = FB.EVENT_DROP then
			require(refusal = 0, "unexpected accepted drop")
			require(event_data.drop_kind = expected_kind, "payload kind")
			dim expected_count as long = iif(expected_kind = FB.DROP_FILES, 2, 1)
			require(event_data.drop_count = expected_count, "payload count")
			for index as long = 0 to expected_count - 1
				dim copied as string
				require(FBC.screenDropGet(event_data.drop_id, index, copied) = FB.DROP_OK, "payload copy")
				dim expected as string
				if expected_kind = FB.DROP_TEXT then
					expected = "Post " & chr(&he2, &h98, &h83) & " " & chr(&hf0, &h9f, &h8c, &h8d)
				else
					#ifdef __FB_WIN32__
						if index = 0 then
							expected = "C:\drop\a b.txt"
						else
							expected = "C:\drop\snow-" & chr(&he2, &h98, &h83) & "-" & chr(&hf0, &h9f, &h8c, &h8d) & ".txt"
						end if
					#else
						if index = 0 then
							expected = "/tmp/a b.txt"
						else
							expected = "/tmp/snow-" & chr(&he2, &h98, &h83) & "-" & chr(&hf0, &h9f, &h8c, &h8d) & ".txt"
						end if
					#endif
				end if
				require(copied = expected, "exact UTF-8 payload")
				printf(!"DROP_ITEM=%s\n", strptr(copied))
			next
			saved_id = event_data.drop_id
			found += 1
		end if
	wend
	if found <> 0 then exit for
	sleep 10, 1
next
require(found = 1 orelse refusal <> 0, "drop delivered before deadline")
require(FBC.screenDropEnable(0) = FB.DROP_OK, "disable")
if found <> 0 then
	dim copied as string = "unchanged"
	require(FBC.screenDropGet(saved_id, 0, copied) = FB.DROP_EXPIRED, "expired on disable")
	require(copied = "unchanged", "failed copy preserves output")
end if
#ifdef __FB_WIN32__
	require(fb_test_drop(cptr(any ptr, window_handle)) <> 0, "disabled native source")
	for tick as long = 1 to 10
		while screenevent(@event_data)
			require(event_data.type <> FB.EVENT_DROP, "disabled reception")
		wend
		sleep 10, 1
	next
#endif
screen 0
printf(!"DROP_NATIVE=PASS\n")
fflush(0)
'' end of drop-receiver.bas
