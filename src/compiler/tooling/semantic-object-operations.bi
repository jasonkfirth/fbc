'' Project: FreeBASIC compiler - native object operations
'' File: tooling/semantic-object-operations.bi
'' Purpose: Preserve selected object construction, copying and destruction work.
'' Responsibilities: Nested observations, native choices and retained root links.
'' This file intentionally does NOT contain: constructor selection or AST lowering.

#ifndef __FB_SEMANTIC_OBJECT_OPERATIONS_BI__
#define __FB_SEMANTIC_OBJECT_OPERATIONS_BI__
#include once "core/fbint.bi"

declare sub fbSemanticModelResetObjectOperations( )
declare sub fbSemanticModelFinishObjectOperations( byval commit as integer )
declare function fbSemanticModelObjectOperationBegin( byref kind as const string, _
	byval owner as FBSYMBOL ptr, byval target as FBSYMBOL ptr = NULL, _
	byval selected as FBSYMBOL ptr = NULL, byval options as longint = 0, byval decision as integer = 0 ) as longint
declare sub fbSemanticModelObjectOperationEnd( byval identity as longint, _
	byval root as ASTNODE ptr, byref outcome as const string )
declare sub fbSemanticModelObjectOperationChoice( byref kind as const string, byref value as const string )
declare sub fbSemanticModelObjectOperationNumberChoice( byref kind as const string, byval value as longint )
declare sub fbSemanticModelObjectOperationSymbolChoice( byref kind as const string, byval sym as FBSYMBOL ptr )
declare sub fbSemanticModelObjectOperationSelection( byval identity as longint, byval root as ASTNODE ptr )
declare sub fbSemanticModelObjectOperationNode( byval node as ASTNODE ptr, byval node_id as longint )

#endif

'' end of tooling/semantic-object-operations.bi
