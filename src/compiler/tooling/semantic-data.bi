'' Project: FreeBASIC compiler - semantic DATA streams
'' File: tooling/semantic-data.bi
'' Purpose: Observe source DATA items and selected READ/RESTORE lowering.
'' Responsibilities: Define borrowed-input hooks and per-module count lifecycle.
'' This file intentionally does NOT contain: DATA parsing or runtime cursor policy.

#ifndef __FB_SEMANTIC_DATA_BI__
#define __FB_SEMANTIC_DATA_BI__
#include once "ast/ast.bi"
#include once "lexer/lex.bi"

declare sub fbSemanticModelResetData( )
declare sub fbSemanticModelFinishData( )
declare function fbSemanticModelDataBegin( ) as longint
declare sub fbSemanticModelDataItem _
	( byval group_id as longint, byval ordinal as integer, byval item as ASTNODE ptr, _
	  byval raw_text as integer, byref first as LEX_LOCATION, byref last as LEX_LOCATION, _
	  byval first_nonphysical as longint, byval last_nonphysical as longint )
declare sub fbSemanticModelDataDescriptor _
	( byval group_id as longint, byval array as FBSYMBOL ptr, byval previous as FBSYMBOL ptr, _
	  byval label as FBSYMBOL ptr, byval cells as integer )
declare sub fbSemanticModelDataPayload _
	( byval group_id as longint, byval ordinal as integer, byval tag as integer, _
	  byref numeric_text as const string )
declare function fbSemanticModelDataRead _
	( byval expr as ASTNODE ptr, byval ordinal as integer, byref first as LEX_LOCATION, _
	  byref last as LEX_LOCATION, byval first_nonphysical as longint, _
	  byval last_nonphysical as longint ) as longint
declare sub fbSemanticModelDataReadRuntime( byval identity as longint, byval proc as FBSYMBOL ptr )
declare sub fbSemanticModelDataRestore _
	( byval statement as longint, byval label as FBSYMBOL ptr, byval array as FBSYMBOL ptr )
#endif

'' end of tooling/semantic-data.bi
