'' FreeBASIC runtime examples: notify.bas
'' Purpose: Show a passive notification with checked setup and teardown.
'' Responsibilities: UTF-8 text, Android permission and an owner-thread pump.
'' This example does not install a background service or contact a server.
#lang "fb"
#include once "fbnotify.bi"

dim context as any ptr = 0
dim status as long = FBC.notifyCreate("FreeBASIC example", context)
if status <> FB.SERVICE_OK then
	print "Notification setup unavailable, status "; status
	end 1
end if

status = FBC.notifyPermission(context, 1)
'' Android permission prompts are asynchronous. Keep the activity alive while
'' the user answers, and check again without opening another prompt.
for attempt as long = 1 to 100
	if status <> FB.SERVICE_DENIED then exit for
	sleep 100, 1
	status = FBC.notifyPermission(context)
next
if status = FB.SERVICE_OK then
	status = FBC.notifySend(context, "New posts", "There are three new posts to read.")
	print "Notification send status "; status
	'' Windows owns a hidden shell window until notifyDestroy. Polling it
	'' does not remove keyboard input or events from an application's window.
	for tick as long = 1 to 50
		FBC.notifyPoll(context)
		sleep 100, 1
	next
else
	print "Notifications are disabled, status "; status
end if
dim cleanup_status as long = FBC.notifyDestroy(context)
if status <> FB.SERVICE_OK orelse cleanup_status <> FB.SERVICE_OK then end 1
'' end of notify.bas
