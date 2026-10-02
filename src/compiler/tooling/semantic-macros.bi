'' Project: FreeBASIC compiler - semantic macro provenance
'' File: tooling/semantic-macros.bi
'' Purpose: Observe macro definitions, attempts, arguments, and replacement text.
'' Responsibilities: Define producer hooks and token origin ownership.
'' This file intentionally does NOT contain: macro parsing or callback execution.

#ifndef __FB_SEMANTIC_MACROS_BI__
#define __FB_SEMANTIC_MACROS_BI__

#include once "lexer/lex.bi"

declare sub fbSemanticModelResetMacros( )
declare function fbSemanticModelMacroBegin( byval sym as FBSYMBOL ptr, byref source as LEX_LOCATION ) as longint
declare sub fbSemanticModelMacroEnter( byval identity as longint, byref previous as longint )
declare sub fbSemanticModelMacroLeave( byval previous as longint, byref phase as const string )
declare function fbSemanticModelMacroLoader( ) as longint
declare function fbSemanticModelMacroCurrentPhase( ) as string
declare function fbSemanticModelMacroTokenOrigin( ) as longint
declare sub fbSemanticModelMacroPhase( byref phase as const string )
declare sub fbSemanticModelMacroArgument _
	( byval identity as longint, byval ordinal as integer, byval argument as LEXPP_ARG ptr, byval wide as integer )
declare sub fbSemanticModelMacroSegment _
	( byval identity as longint, byval ordinal as integer, byval token_ordinal as integer, _
	  byval parameter as integer, byref kind as const string, byval offset as integer, byval units as integer )
declare sub fbSemanticModelMacroCallback _
	( byval identity as longint, byref value as const string, byval error_code as integer )
declare sub fbSemanticModelMacroCallbackW _
	( byval identity as longint, byval value as const wstring ptr, byval error_code as integer )
declare sub fbSemanticModelMacroResult _
	( byval identity as longint, byref outcome as const string, byval value as const any ptr, _
	  byval units as integer, byval wide as integer, byval arguments as integer, byref ending as LEX_LOCATION )
declare sub fbSemanticModelMacroPushOrigin( byval identity as longint, byval resume_length as integer, byval units as integer )
declare sub fbSemanticModelMacroLifecycle _
	( byval sym as FBSYMBOL ptr, byref action as const string, byref spelling as const string, byref source as LEX_LOCATION )
declare sub fbSemanticModelMacroOrigin _
	( byref domain as const string, byval subject as longint, byval expansion as longint, byref role as const string )
declare sub fbSemanticModelMacroConsumed( byval expansion as longint, byval counter as longint )
declare sub fbSemanticModelMacroExpressionOrigins( byval expression as longint, byval first_counter as longint, byval last_counter as longint )

#endif

'' end of tooling/semantic-macros.bi
