'' FreeBASIC runtime: fbservices.bi
'' Purpose: Share explicit status values across optional runtime services.
'' Responsibilities: Stable numeric results for callers and platform adapters.
'' This file contains no notification backend or scheduler implementation.
#ifndef __FB_SERVICES_BI__
#define __FB_SERVICES_BI__
#if __FB_LANG__ <> "fb"
	#error fbservices.bi requires the fb dialect
#endif
namespace FB
	const SERVICE_OK as long = 0
	const SERVICE_UNAVAILABLE as long = 1
	const SERVICE_INVALID as long = 2
	const SERVICE_FAILED as long = 3
	const SERVICE_DENIED as long = 4
	const SERVICE_WRONG_THREAD as long = 5
end namespace
#endif
'' end of fbservices.bi
