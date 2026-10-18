'' FreeBASIC runtime: fbnotify.bi
'' Purpose: Expose checked native desktop and Android notifications.
'' Responsibilities: Stable statuses, UTF-8 limits and context lifetime.
'' Platform presentation, workers and network polling are not implemented here.
#ifndef __FB_NOTIFY_BI__
#define __FB_NOTIFY_BI__
#if __FB_LANG__ <> "fb"
	#error fbnotify.bi requires the fb dialect
#endif
#include once "fbservices.bi"
'' The Unix adapter uses native mutexes even in a single-threaded BASIC app.
'' Android provides pthread interfaces in libc and has no separate libpthread.
#if not defined(__FB_ANDROID__) and (defined(__FB_LINUX__) or _
    defined(__FB_FREEBSD__) or defined(__FB_NETBSD__) or _
    defined(__FB_OPENBSD__) or defined(__FB_DRAGONFLY__))
	#inclib "pthread"
#endif
namespace FB
	const NOTIFY_APP_MAX as ulong = 127
	const NOTIFY_TITLE_MAX as ulong = 63
	const NOTIFY_BODY_MAX as ulong = 255
end namespace
namespace FBC
	extern "rtlib"
		'' All strings contain UTF-8, not the Windows ANSI code page. Embedded
		'' NUL and oversize strings are rejected. Initialize context to zero.
		'' Create, use, poll and destroy a context on the same thread.
		declare function notifyCreate alias "fb_NotifyCreate" _
			( byref app_name as const string, byref context as any ptr ) as long
		declare function notifySend alias "fb_NotifySend" _
			( byval context as any ptr, byref title as const string, _
			  byref body as const string ) as long
		'' Pump only this context's native window, without consuming app input.
		declare function notifyPoll alias "fb_NotifyPoll" _
			( byval context as any ptr ) as long
		declare function notifyClose alias "fb_NotifyClose" _
			( byval context as any ptr ) as long
		'' Clears context after success; destroying an already-zero context is OK.
		declare function notifyDestroy alias "fb_NotifyDestroy" _
			( byref context as any ptr ) as long
		'' Android: query permission, or request it with 1 or TRUE. A request
		'' returns DENIED until the user grants it. Other backends return OK.
		declare function notifyPermission alias "fb_NotifyPermission" _
			( byval context as any ptr, byval request as long = 0 ) as long
	end extern
end namespace
#endif
'' end of fbnotify.bi
