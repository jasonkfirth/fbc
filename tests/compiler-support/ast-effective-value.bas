'' Project: FreeBASIC compiler AST regression tests
'' File: ast-effective-value.bas
'' Purpose: Exercise effective-value queries on empty and linked AST nodes.
'' Responsibilities: Check NULL, both LINK directions and side-effect-only LINKs.
'' This file intentionally does NOT construct a complete compiler environment.
#include once "core/fb.bi"
#include once "core/fbint.bi"
#include once "ast/ast.bi"

'' The runner inserts the exact current production functions here.
' @PRODUCTION_FUNCTIONS@

dim as ASTNODE lhs, rhs, link, outer
lhs.class = AST_NODECLASS_CONST
rhs.class = AST_NODECLASS_VAR
link.class = AST_NODECLASS_LINK
link.l = @lhs
link.r = @rhs
outer.class = AST_NODECLASS_LINK
outer.link.ret = AST_LINK_RETURN_LEFT
outer.l = @link

if astGetEffectiveNode(NULL) <> NULL then end 1
if astGetEffectiveClass(NULL) <> AST_NODECLASS_NOP then end 2
if astGetEffectiveNode(@lhs) <> @lhs then end 3
link.link.ret = AST_LINK_RETURN_LEFT
if astGetEffectiveNode(@outer) <> @lhs then end 4
if astGetEffectiveClass(@outer) <> AST_NODECLASS_CONST then end 5
link.link.ret = AST_LINK_RETURN_RIGHT
if astGetEffectiveNode(@outer) <> @rhs then end 6
if astGetEffectiveClass(@outer) <> AST_NODECLASS_VAR then end 7
link.link.ret = AST_LINK_RETURN_NONE
if astGetEffectiveNode(@outer) <> NULL then end 8
if astGetEffectiveClass(@outer) <> AST_NODECLASS_NOP then end 9
link.link.ret = AST_LINK_RETURN_LEFT
link.l = NULL
if astGetEffectiveNode(@outer) <> NULL then end 10
if astGetEffectiveClass(@outer) <> AST_NODECLASS_NOP then end 11
link.link.ret = AST_LINK_RETURN_RIGHT
link.r = NULL
if astGetEffectiveNode(@outer) <> NULL then end 12
if astGetEffectiveClass(@outer) <> AST_NODECLASS_NOP then end 13

'' end of ast-effective-value.bas
