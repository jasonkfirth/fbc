'' Project: FreeBASIC compiler - original member declaration names
'' File: tooling/semantic-member-names.bi
'' Purpose: Define a non-owning observer for accepted UDT member spellings.
'' Responsibilities: Field/constant inputs and per-module serial lifecycle.
'' This file intentionally does NOT contain: naming policy or symbol allocation.

#ifndef __FB_SEMANTIC_MEMBER_NAMES_BI__
#define __FB_SEMANTIC_MEMBER_NAMES_BI__

#include once "symbols/symb.bi"
#include once "lexer/lex.bi"

declare sub fbSemanticModelMemberDeclarationSpelling _
	( byval sym as FBSYMBOL ptr, byval spelling as const zstring ptr, _
	  byref source as LEX_LOCATION )

declare sub fbSemanticModelResetMemberNames( )
declare sub fbSemanticModelFinishMemberNames( )

#endif

'' end of tooling/semantic-member-names.bi
