'' Project: FreeBASIC compiler - semantic AST loading observations
'' File: tooling/semantic-loads.bi
'' Purpose: Observe actual loader order and actions introduced after snapshots.
'' Responsibilities: Define origin, load interval, call and check-transfer hooks.
'' This file intentionally does NOT contain: AST loading or IR emission decisions.

#ifndef __FB_SEMANTIC_LOADS_BI__
#define __FB_SEMANTIC_LOADS_BI__

#include once "ast/ast.bi"

declare function fbSemanticModelCurrentLoad( ) as longint
declare sub fbSemanticModelResetLoads( )
declare sub fbSemanticModelLoadOrigin( byval node as ASTNODE ptr, byval identity as longint )
declare function fbSemanticModelLoadBegin( byval node as ASTNODE ptr ) as longint
declare sub fbSemanticModelLoadEnd( byval identity as longint, byval has_result as integer )
declare sub fbSemanticModelLoadCall( byval callee as FBSYMBOL ptr, byval indirect as integer, byref role as const string )
declare sub fbSemanticModelLoadCheck( byref kind as const string, byval continuation as FBSYMBOL ptr )
declare sub fbSemanticModelLoadAssembly( byval head as ASTASMTOK ptr )
declare sub fbSemanticModelFinishLoads( byval commit as integer )

#endif

'' end of tooling/semantic-loads.bi
