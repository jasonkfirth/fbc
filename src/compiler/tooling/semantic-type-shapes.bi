'' Project: FreeBASIC compiler - written type structure
'' File: tooling/semantic-type-shapes.bi
'' Purpose: Observe native declaration type structure before aliases are erased.
'' Responsibilities: Serial parser entry, nesting, array and convention facts.
'' This file intentionally does NOT contain: complexity scores or lint policy.
#ifndef __FB_SEMANTIC_TYPE_SHAPES_BI__
#define __FB_SEMANTIC_TYPE_SHAPES_BI__
#include once "lexer/lex.bi"
declare sub fbSemanticModelTypeShapeDeclaration(byval is_array as integer = FALSE)
declare function fbSemanticModelTypeShapeBegin(byref source as LEX_LOCATION) as integer
declare sub fbSemanticModelTypeShapeKind(byval handle as integer, byref kind as const string)
declare sub fbSemanticModelTypeShapeConvention(byval handle as integer, byval is_explicit as integer, byval foreign_language as integer)
declare sub fbSemanticModelTypeShapePointers(byval handle as integer, byval written_count as integer)
declare sub fbSemanticModelTypeShapeEnd(byval handle as integer, byval accepted as integer, byref source as LEX_LOCATION)
declare sub fbSemanticModelTypeShapeArray(byval dimensions as integer)
declare sub fbSemanticModelResetTypeShapes()
declare sub fbSemanticModelFinishTypeShapes()
#endif
'' end of tooling/semantic-type-shapes.bi
