'' FreeBASIC runtime tests: notify-native.bas
'' Purpose: Exercise native notification ownership and checked delivery.
'' Responsibilities: Invalid-input refusal, updates, close and wrong-thread use.
'' A Unix test server records delivery; this fixture does not claim presentation.
#lang "fb"
#include once "fbnotify.bi"

sub require( byval condition as long, byref message as const string )
	if condition = 0 then
		print "NOTIFY_FAIL="; message
		end 1 ' fblint: disable-line FBL-CF-005 -- A failed assertion terminates this isolated test process.
	end if
end sub

type OWNER_TEST
	context as any ptr
	status as long
end type

sub wrong_thread( byval user as any ptr )
	dim check as OWNER_TEST ptr = cptr(OWNER_TEST ptr, user)
	check->status = FBC.notifyPoll(check->context)
end sub

dim context as any ptr = 0
require(FBC.notifyCreate("FreeBASIC service test", context) = FB.SERVICE_OK, "create")
require(context <> 0, "context output")
require(FBC.notifyPermission(context) = FB.SERVICE_OK, "permission")
dim check as OWNER_TEST
check.context = context
dim worker as any ptr = threadcreate(@wrong_thread, @check)
require(worker <> 0, "test worker")
threadwait worker
require(check.status = FB.SERVICE_WRONG_THREAD, "owner-thread refusal")
require(FBC.notifySend(context, "bad" & chr(0) & "title", "body") = FB.SERVICE_INVALID, "embedded NUL")
require(FBC.notifySend(context, string(64, "t"), "body") = FB.SERVICE_INVALID, "title limit")
require(FBC.notifySend(context, "title", string(256, "b")) = FB.SERVICE_INVALID, "body limit")
require(FBC.notifySend(context, chr(&hed, &ha0, &h80), "body") = FB.SERVICE_INVALID, "invalid UTF-8")
require(FBC.notifySend(context, "New " & chr(&he2, &h98, &h83), "<b>Post & body</b>") = FB.SERVICE_OK, "send")
require(FBC.notifySend(context, "Update", "Post " & chr(&hf0, &h9f, &h8c, &h8d)) = FB.SERVICE_OK, "update")
require(FBC.notifyPoll(context) = FB.SERVICE_OK, "poll")
require(FBC.notifyClose(context) = FB.SERVICE_OK, "close")
require(FBC.notifyDestroy(context) = FB.SERVICE_OK, "destroy")
require(context = 0, "cleared context")
require(FBC.notifyDestroy(context) = FB.SERVICE_OK, "idempotent destroy")
print "NOTIFY_NATIVE=PASS"
'' end of notify-native.bas
