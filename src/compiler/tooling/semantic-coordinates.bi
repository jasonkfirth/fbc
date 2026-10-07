'' Project: FreeBASIC compiler - semantic source coordinates
'' File: tooling/semantic-coordinates.bi
'' Purpose: Resolve observed physical positions in the already-open revision.
'' Responsibilities: Define coordinate lookup and source range serialization.
'' This file intentionally does NOT contain: parsing or logical line remapping.

#ifndef __FB_SEMANTIC_COORDINATES_BI__
#define __FB_SEMANTIC_COORDINATES_BI__

#include once "lexer/lex.bi"

declare sub fbSemanticModelResetCoordinates( )

declare function fbSemanticModelCoordinateFact _
	( byref first as LEX_LOCATION, byref last as LEX_LOCATION ) as string
declare sub fbSemanticModelExportCoordinates _
	( byref domain as const string, byval subject as longint, byref role as const string, _
	  byref first as LEX_LOCATION, byref last as LEX_LOCATION )

#endif

'' end of tooling/semantic-coordinates.bi
