'' Project: FreeBASIC compiler - temporary lifetime observations
'' File: tooling/semantic-temporaries.bi
'' Purpose: Observe native destructor-list transitions before nodes are recycled.
'' Responsibilities: Define registration, scope, flush and conditional hooks.
'' This file intentionally does NOT contain: temporary or destructor selection.

#ifndef __FB_SEMANTIC_TEMPORARIES_BI__
#define __FB_SEMANTIC_TEMPORARIES_BI__

#include once "ast/ast.bi"

declare sub fbSemanticModelResetTemporaries( )
declare sub fbSemanticModelTemporaryAdd( byval item as AST_DTORLIST_ITEM ptr )
declare sub fbSemanticModelTemporaryReference( byval item as AST_DTORLIST_ITEM ptr, byval added as integer )
declare sub fbSemanticModelTemporaryRescope( byval item as AST_DTORLIST_ITEM ptr, byval newcookie as integer )
declare sub fbSemanticModelTemporaryRemove( byval item as AST_DTORLIST_ITEM ptr, byref reason as const string )
declare sub fbSemanticModelTemporaryScopeBegin( byval cookie as integer, byval allocated as integer, byval depth as integer )
declare sub fbSemanticModelTemporaryScopeEnd( byval cookie as integer, byval depth as integer )
declare function fbSemanticModelTemporaryFlushBegin( byval cookie as integer ) as longint
declare sub fbSemanticModelTemporaryFlushAction( byval flush_id as longint, byval item as AST_DTORLIST_ITEM ptr, byval action as ASTNODE ptr )
declare sub fbSemanticModelTemporaryFlushEnd( byval flush_id as longint, byval cookie as integer, byval actions as integer, byval removed as integer )
declare sub fbSemanticModelTemporaryConditional( byval result as FBSYMBOL ptr, byval true_flush as longint, byval false_flush as longint )
declare sub fbSemanticModelTemporaryNode( byval node as ASTNODE ptr, byval identity as longint )
declare sub fbSemanticModelFinishTemporaries( byval commit as integer )

#endif

'' end of tooling/semantic-temporaries.bi
