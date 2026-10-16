'' Project: FreeBASIC compiler - accepted primitive type occurrences
'' File: tooling/semantic-primitive-types.bi
'' Purpose: Define the observer for parsed primitive type operands.
'' Responsibilities: Non-owning parser inputs and serial module lifecycle.
'' This file intentionally does NOT contain type grammar or lint policy.

#ifndef __FB_SEMANTIC_PRIMITIVE_TYPES_BI__
#define __FB_SEMANTIC_PRIMITIVE_TYPES_BI__
#include once "lexer/lex.bi"

declare sub fbSemanticModelPrimitiveType _
	( byval dtype as integer, byval token as integer, byref source as LEX_LOCATION )
declare sub fbSemanticModelResetPrimitiveTypes( )
declare sub fbSemanticModelFinishPrimitiveTypes( )

#endif
'' end of tooling/semantic-primitive-types.bi
