'' Project: FreeBASIC compiler - accepted source declarations
'' File: tooling/semantic-declarations.bi
'' Purpose: Preserve acceptance order across binding and declaration routes.
'' Responsibilities: Immediate copied receipts and bounded module closure.
'' This file intentionally does NOT contain: declaration parsing or symbol creation.

#ifndef __FB_SEMANTIC_DECLARATIONS_BI__
#define __FB_SEMANTIC_DECLARATIONS_BI__
#include once "core/fbint.bi"

declare sub fbSemanticModelResetDeclarations( )
declare sub fbSemanticModelFinishDeclarations( byval commit as integer )
declare sub fbSemanticModelAcceptedDeclaration( byval sym as FBSYMBOL ptr, _
	byref kind as const string, byval receipt as longint )

#endif

'' end of tooling/semantic-declarations.bi
