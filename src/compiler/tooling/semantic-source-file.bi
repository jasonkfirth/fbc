'' Project: FreeBASIC compiler - semantic source revisions
'' File: tooling/semantic-source-file.bi
'' Purpose: Observe a revision through the compiler's already-open source stream.
'' Responsibilities: Define revision ownership and verification results.
'' This file intentionally does NOT contain: parsing, decoding, or host layouts.

#ifndef __FB_SEMANTIC_SOURCE_FILE_BI__
#define __FB_SEMANTIC_SOURCE_FILE_BI__

'' The revision never owns the CRT FILE*. Close consumes the revision before
'' the compiler closes that stream. Status is 1 for a verified regular file,
'' 2 for an unverified nonregular stream, and 0 for a changed/unreadable file.
declare function fbSemanticSourceOpen _
	( byval stream as any ptr, byval digest as zstring ptr, _
	  byval bytes as ulongint ptr, byval status as long ptr ) as any ptr
declare function fbSemanticSourceClose( byval revision as any ptr ) as long

#endif

'' end of tooling/semantic-source-file.bi
