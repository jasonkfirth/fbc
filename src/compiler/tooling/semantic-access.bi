'' Project: FreeBASIC compiler - semantic source accesses
'' File: tooling/semantic-access.bi
'' Purpose: Retain use roles for compiler-bound variable/member occurrences.
'' Responsibilities: Define checked access capture, update, and node association.
'' This file intentionally does NOT contain: alias or external-call effect analysis.

#ifndef __FB_SEMANTIC_ACCESS_BI__
#define __FB_SEMANTIC_ACCESS_BI__

#include once "ast/ast.bi"

declare sub fbSemanticModelResetAccess( )
declare sub fbSemanticModelCaptureAccess(byval binding as longint)
declare sub fbSemanticModelExportAccess( )
declare sub fbSemanticModelExportBindingLink(byval node as ASTNODE ptr, byval identity as longint)
declare function fbSemanticModelDirectSourceWrite(byval identity as longint) as integer

#endif

'' end of tooling/semantic-access.bi
