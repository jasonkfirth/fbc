'' Project: FreeBASIC compiler - semantic block observations
'' File: tooling/semantic-blocks.bi
'' Purpose: Observe native procedure/scope boundaries and scope-exit cleanup.
'' Responsibilities: Define copied block identities and lifetime handoff hooks.
'' This file intentionally does NOT contain: branch validation or code generation.

#ifndef __FB_SEMANTIC_BLOCKS_BI__
#define __FB_SEMANTIC_BLOCKS_BI__

#include once "ast/ast.bi"

declare sub fbSemanticModelResetBlocks( )
declare sub fbSemanticModelProcedureBlock( byval node as ASTNODE ptr )
declare sub fbSemanticModelScopeBlock( byval node as ASTNODE ptr )
declare function fbSemanticModelScopeExit( byval node as ASTNODE ptr ) as longint
declare sub fbSemanticModelScopeExitEnd( byval node as ASTNODE ptr )
declare sub fbSemanticModelExitCleanup( byval branch as ASTNODE ptr, byval variable as FBSYMBOL ptr, byval owner as FBSYMBOL ptr, byval action as ASTNODE ptr = NULL )
declare sub fbSemanticModelCleanupNode( byval node as ASTNODE ptr, byval identity as longint )
declare sub fbSemanticModelFinishBlocks( byval commit as integer )

#endif

'' end of tooling/semantic-blocks.bi
