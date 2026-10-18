'' FreeBASIC runtime tests: android-poll.bas
'' Purpose: Exercise native notifications and an idle worker on a real device.
'' Responsibilities: Checked JNI calls, Unicode, denied policy and finite polling.
'' This fixture does not install a service or change the device's power policy.
'' Threading: Each context has one owner. Only the counter crosses threads,
'' under its mutex; joining ends all worker access before main-thread cleanup.
#lang "fb"
#ifndef __FB_ANDROID__
	#error This test requires the Android runtime and APK template
#endif
#include once "fbnotify.bi"
#include once "fbidle.bi"
#include once "fbrandom.bi"
#inclib "log"

const POLL_CHECKS as long = 20

extern "C"
	declare function android_log_write cdecl alias "__android_log_write" _
		( byval priority as long, byval tag as const zstring ptr, byval text as const zstring ptr ) as long
end extern

'' A single log call keeps each test record intact across concurrent threads.
sub record_result( byref text as const string )
	android_log_write(4, "FBCServicesTest", strptr(text))
end sub

sub require( byval condition as long, byref message as const string )
	if condition = 0 then
		record_result("ANDROID_FAIL=" & message)
		end 1 ' fblint: disable-line FBL-CF-005 -- A failed assertion terminates this isolated test APK.
	end if
end sub

type POLL_WORK
	mutex as any ptr
	checks as long
	status as long
	context as any ptr
end type

sub small_stack_worker( byval user as any ptr )
	dim result as long ptr = cptr(long ptr, user)
	dim context as any ptr = 0
	*result = FBC.notifyCreate("Small stack refusal", context)
	if context <> 0 then FBC.notifyDestroy(context)
end sub

function poll_once( byval user as any ptr ) as long
	dim work as POLL_WORK ptr = cptr(POLL_WORK ptr, user)
	mutexlock work->mutex
	work->checks = clng(work->checks + 1)
	dim checks as long = work->checks
	mutexunlock work->mutex
	dim body as string = "Check " & ltrim(str(checks)) & " " & chr(&hf0, &h9f, &h8c, &h8d)
	work->status = FBC.notifySend(work->context, "Polling " & chr(&he2, &h98, &h83), body)
	record_result("ANDROID_POLL=" & ltrim(str(checks)) & " STATUS=" & ltrim(str(work->status)))
	return clng(work->status = FB.SERVICE_OK andalso checks < POLL_CHECKS)
end function

sub poll_worker( byval user as any ptr )
	dim work as POLL_WORK ptr = cptr(POLL_WORK ptr, user)
	work->status = FBC.notifyCreate("FreeBASIC polling test", work->context)
	if work->status <> FB.SERVICE_OK then exit sub
	dim state as FB.IDLE_STATE
	dim status as long = FBC.idleInit(state, 1500)
	while status = FB.SERVICE_OK andalso state.running
		status = FBC.idleStep(state, @poll_once, user, 100)
	wend
	if status <> FB.SERVICE_OK then work->status = status
	dim close_status as long = FBC.notifyClose(work->context)
	dim destroy_status as long = FBC.notifyDestroy(work->context)
	if close_status <> FB.SERVICE_OK then work->status = close_status
	if destroy_status <> FB.SERVICE_OK then work->status = destroy_status
end sub

dim context as any ptr = 0
require(FBC.notifyCreate("FreeBASIC device test", context) = FB.SERVICE_OK, "create")
dim permission as long = FBC.notifyPermission(context, 1)
for attempt as long = 1 to 100
	if permission <> FB.SERVICE_DENIED then exit for
	sleep 100, 1
	permission = FBC.notifyPermission(context)
next
'' An external test controller can block notifications for this test package.
'' Refusal must be explicit, with no exception, fallback post or leaked handle.
if permission = FB.SERVICE_DENIED then
	require(FBC.notifySend(context, "Denied", "Must not appear") = FB.SERVICE_DENIED, "denied send")
	require(FBC.notifyDestroy(context) = FB.SERVICE_OK, "denied cleanup")
	record_result("ANDROID_NOTIFICATION_DENIED=PASS")
	end 0
end if
require(permission = FB.SERVICE_OK, "permission")
require(FBC.notifySend(context, "Foreground", "Device notification test") = FB.SERVICE_OK, "foreground post")
require(FBC.notifySend(context, "bad" & chr(0), "body") = FB.SERVICE_INVALID, "embedded NUL")
require(FBC.notifySend(context, chr(&hed, &ha0, &h80), "body") = FB.SERVICE_INVALID, "malformed UTF-8")
const RANDOM_BYTES as uinteger = 32
dim entropy(0 to RANDOM_BYTES - 1) as ubyte
for index as long = 0 to ubound(entropy)
	entropy(index) = &haa
next
dim random_status as long = FBC.secureRandom(@entropy(0), RANDOM_BYTES)
require(random_status >= FB.SECURE_RANDOM_OK andalso random_status <= FB.SECURE_RANDOM_IO_ERROR, "system random status")
'' Older Android kernels can lack getrandom. Refusal is valid, but none of the
'' prefilled bytes may survive a provider failure. Only status is logged.
if random_status <> FB.SECURE_RANDOM_OK then
	for index as long = 0 to ubound(entropy)
		require(entropy(index) = 0, "failed random wipe")
	next
end if
record_result("ANDROID_RANDOM_STATUS=" & ltrim(str(random_status)))
for index as long = 0 to ubound(entropy)
	entropy(index) = 0
next
dim work as POLL_WORK
work.mutex = mutexcreate()
require(work.mutex <> 0, "mutex")
dim small_stack_status as long
dim small_worker as any ptr = threadcreate(@small_stack_worker, @small_stack_status, 65536)
require(small_worker <> 0, "small worker")
threadwait small_worker
require(small_stack_status = FB.SERVICE_UNAVAILABLE, "small stack refusal")
record_result("ANDROID_SMALL_STACK_REFUSAL=PASS")
'' The Java VM needs more stack than BASIC's default polling-only thread.
dim worker as any ptr = threadcreate(@poll_worker, @work, 1024 * 1024)
require(worker <> 0, "worker")
record_result("ANDROID_READY=1")
'' The controller presses Home, observes further posts, then resumes the same
'' activity. No graphics or UI object is accessed by the worker's callback.
threadwait worker
mutexdestroy work.mutex
require(work.status = FB.SERVICE_OK andalso work.checks = POLL_CHECKS, "worker completion")
require(FBC.notifyClose(context) = FB.SERVICE_OK, "foreground close")
require(FBC.notifyDestroy(context) = FB.SERVICE_OK, "destroy")
record_result("ANDROID_POLL_NATIVE=PASS")
'' end of android-poll.bas
