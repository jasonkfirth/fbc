'' Project: FreeBASIC compiler - semantic source contexts
'' File: tooling/semantic-source.bi
'' Purpose: Observe opened revisions, include occurrences, and logical remaps.
'' Responsibilities: Expose serial source lifecycle hooks and source identities.
'' This file intentionally does NOT contain: source parsing or path resolution.

#ifndef __FB_SEMANTIC_SOURCE_BI__
#define __FB_SEMANTIC_SOURCE_BI__

#include once "lexer/lex.bi"

declare sub fbSemanticModelResetSources( )
declare sub fbSemanticModelOpenSource _
	( byref filename as const string, byval depth as integer, _
	  byref kind as const string, byref requested as const string, _
	  byval directive as LEX_LOCATION ptr = NULL )
declare sub fbSemanticModelCloseSource( byval depth as integer )
declare function fbSemanticModelCurrentSource( ) as longint
declare function fbSemanticModelSourceHandle( byval identity as longint, byref format as integer ) as integer
declare sub fbSemanticModelIncludeOutcome _
	( byref requested as const string, byref resolved as const string, _
	  byref outcome as const string, byval directive as LEX_LOCATION ptr = NULL )
declare sub fbSemanticModelSourceRemap _
	( byval logical_line as longint, byref logical_file as const string, _
	  byref directive as LEX_LOCATION )

#endif

'' end of tooling/semantic-source.bi
