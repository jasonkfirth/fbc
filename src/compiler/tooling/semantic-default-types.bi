'' Project: FreeBASIC compiler - default type policy observations
'' File: tooling/semantic-default-types.bi
'' Purpose: Preserve actual DEF table initialization and committed updates.
'' Responsibilities: Expose bounded policy lifecycle and native range receipts.
'' This file intentionally does NOT contain: DEF parsing or default type selection.

#ifndef __FB_SEMANTIC_DEFAULT_TYPES_BI__
#define __FB_SEMANTIC_DEFAULT_TYPES_BI__
#include once "core/fbint.bi"

declare sub fbSemanticModelResetDefaultTypes( )
declare sub fbSemanticModelBeginDefaultTypes( )
declare sub fbSemanticModelFinishDefaultTypes( byval commit as integer )
declare sub fbSemanticModelDefaultTypesChanged( )
declare sub fbSemanticModelDefaultTypeRange _
	( byval raw_first as integer, byval raw_last as integer, byval first as integer, byval last as integer, _
	  byval dtype as integer, byval changed as integer, byval entry_context as longint )

#endif
'' end of tooling/semantic-default-types.bi
