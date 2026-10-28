'' Project: FreeBASIC compiler - native resolution explanations
'' File: tooling/semantic-resolution.bi
'' Purpose: Observe actual overload scoring and access checks.
'' Responsibilities: Scoped candidate attempts, argument outcomes and native choices.
'' This file intentionally does NOT contain: resolution, conversion or access policy.

#ifndef __FB_SEMANTIC_RESOLUTION_BI__
#define __FB_SEMANTIC_RESOLUTION_BI__

#include once "core/fbint.bi"
#include once "ast/ast.bi"
#include once "lexer/lex.bi"

declare function fbSemanticModelResolutionBegin( byref kind as const string, _
	byval head as FBSYMBOL ptr, byval args as integer, byval options as integer, _
	byval dtype as integer = FB_DATATYPE_INVALID, byval subtype as FBSYMBOL ptr = NULL ) as longint
declare function fbSemanticModelResolutionCandidate( byval scope_id as longint, _
	byval proc as FBSYMBOL ptr, byref pass as const string ) as longint
declare function fbSemanticModelResolutionArgument( byval candidate as longint, _
	byval ordinal as integer, byval param as FBSYMBOL ptr, byval arg as ASTNODE ptr, _
	byval mode as integer, byval score as integer, byref reason as const string ) as integer
declare sub fbSemanticModelResolutionScore( byval candidate as longint, byval score as integer, _
	byval matches as integer, byval exact_known as integer, byval exact as integer, _
	byval params as integer, byref reason as const string, byval error_code as integer )
declare function fbSemanticModelResolutionCastScore( byval candidate as longint, byval score as integer, _
	byval params as integer, byref reason as const string ) as integer
declare sub fbSemanticModelResolutionRank( byval scope_id as longint, byval candidate as longint, _
	byref action as const string, byval score as integer, byval ties as integer, byval chosen as longint )
declare function fbSemanticModelResolutionEnd( byval scope_id as longint, byval chosen as longint, _
	byval confirmation as longint, byval score as integer, byval ties as integer, _
	byval error_code as integer, byval result as FBSYMBOL ptr ) as FBSYMBOL ptr
declare function fbSemanticModelResolutionAccess( byval sym as FBSYMBOL ptr, _
	byval allowed as integer, byref route as const string ) as integer
declare function fbSemanticModelLastResolution( ) as longint
declare sub fbSemanticModelResolutionUse( byval scope_id as longint, byval source as LEX_LOCATION ptr )
declare sub fbSemanticModelResolutionBoundUse( byval scope_id as longint, byval binding as longint )
declare sub fbSemanticModelResetResolution( )
declare sub fbSemanticModelFinishResolution( byval commit as integer )

#endif
'' end of tooling/semantic-resolution.bi
