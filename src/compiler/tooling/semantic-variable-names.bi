'' Project: FreeBASIC compiler - original variable declaration names
'' File: tooling/semantic-variable-names.bi
'' Purpose: Define the serial observer for accepted variable spellings.
'' Responsibilities: Non-owning declaration inputs and per-module lifecycle.
'' This file intentionally does NOT contain: naming policy or symbol allocation.

#ifndef __FB_SEMANTIC_VARIABLE_NAMES_BI__
#define __FB_SEMANTIC_VARIABLE_NAMES_BI__

#include once "symbols/symb.bi"
#include once "lexer/lex.bi"

declare sub fbSemanticModelVariableDeclarationSpelling _
	( byval sym as FBSYMBOL ptr, byval spelling as const zstring ptr, _
	  byref kind as const string, byref source as LEX_LOCATION )
declare sub fbSemanticModelResetVariableNames( )
declare sub fbSemanticModelFinishVariableNames( )

#endif

'' end of tooling/semantic-variable-names.bi
