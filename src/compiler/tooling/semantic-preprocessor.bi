'' Project: FreeBASIC compiler - semantic preprocessing
'' File: tooling/semantic-preprocessor.bi
'' Purpose: Observe preprocessing decisions without changing their evaluation.
'' Responsibilities: Define conditional, defined-name, and inactive-range hooks.
'' This file intentionally does NOT contain: directive parsing or macro callbacks.

#ifndef __FB_SEMANTIC_PREPROCESSOR_BI__
#define __FB_SEMANTIC_PREPROCESSOR_BI__

#include once "lexer/lex.bi"

declare sub fbSemanticModelResetPreprocessor( )
declare function fbSemanticModelPPBranch _
	( byval token as integer, byref source as LEX_LOCATION, byval first_branch as integer ) as longint
declare sub fbSemanticModelPPDecision _
	( byval branchid as longint, byref evaluation as const string, _
	  byval result as integer, byval selected as integer, byref ending as LEX_LOCATION )
declare sub fbSemanticModelPPEnd( byref source as LEX_LOCATION )
declare sub fbSemanticModelPPDefined _
	( byval sym as FBSYMBOL ptr, byref spelling as const string, byref source as LEX_LOCATION )
declare sub fbSemanticModelPPSkipped _
	( byval branchid as longint, byref start_site as LEX_LOCATION, byref end_site as LEX_LOCATION )
declare function fbSemanticModelPPCurrentBranch( ) as longint

#endif

'' end of tooling/semantic-preprocessor.bi
