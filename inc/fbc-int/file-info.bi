'' Project: FreeBASIC runtime introspection
'' File: fbc-int/file-info.bi
'' Purpose: Query host file identities without importing native structure layouts.
'' Responsibilities: Describe metadata snapshots and owned CRT stream positions.
'' This file intentionally does NOT contain: hashing, path policy, or file writes.

#ifndef __FBC_INT_FILE_INFO_BI__
#define __FBC_INT_FILE_INFO_BI__

'' Internal runtime ABI. All fields have fixed widths; no native stat structure
'' crosses this interface. Timestamps are compared, rather than interpreted as
'' a portable epoch. A host without inode identities leaves IDENTITY clear.
enum FB_FILE_INFO_FLAGS
	FB_FILE_INFO_EXISTS = 1
	FB_FILE_INFO_REGULAR = 2
	FB_FILE_INFO_IDENTITY = 4
end enum

type FB_FILE_INFO
	identity(0 to 2) as ulongint
	bytes as ulongint
	modified as longint
	modified_fraction as longint
	changed as longint
	changed_fraction as longint
	flags as ulongint
end type

extern "c"
declare function fb_FileQueryInfo _
	( byval filename as const zstring ptr, byval follow_links as long, byval info as FB_FILE_INFO ptr ) as long
declare function fb_FileQueryStreamInfo( byval stream as any ptr, byval info as FB_FILE_INFO ptr ) as long
'' Save owns a position token, not the stream. Restore consumes that token,
'' including on failure. Both functions require an ordinary CRT FILE*. The
'' caller must serialize these observations with reads, seeks, and closes.
declare function fb_CrtFileSavePos( byval stream as any ptr ) as any ptr
declare function fb_CrtFileRestorePos( byval stream as any ptr, byval position as any ptr ) as long
end extern

#endif

'' end of fbc-int/file-info.bi
