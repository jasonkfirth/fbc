'' Project: FreeBASIC compiler - structured diagnostics
'' File: tooling/semantic-diagnostics.bi
'' Purpose: Publish compiler diagnostics independently of accepted AST models.
'' Responsibilities: Invocation lifetime, source protection and parser context.
'' This file intentionally does NOT contain: recovered symbols or AST facts.

#ifndef __FB_SEMANTIC_DIAGNOSTICS_BI__
#define __FB_SEMANTIC_DIAGNOSTICS_BI__

#include once "core/fbint.bi"
#include once "lexer/lex.bi"

declare sub fbSemanticDiagnosticsPolicy( byval initialized as integer, byval options as integer, _
	byval show_source as integer, byval error_count as integer, byval promoted as integer, _
	byval warning_level as integer = -1 )
declare sub fbSemanticDiagnosticsParameter( byval proc as FBSYMBOL ptr, byval token as integer, _
	byval parameter as integer, byval spelling as const zstring ptr )

declare function fbSemanticDiagnosticsBegin( byref filename as const string ) as integer
declare function fbSemanticDiagnosticsEnd( byval succeeded as integer ) as integer
declare sub fbSemanticDiagnosticsProtectFile( byref filename as const string )
declare sub fbSemanticDiagnosticsSource( byref filename as const string )
declare sub fbSemanticDiagnosticsModule( byref filename as const string )
declare sub fbSemanticDiagnosticsFinishModule( byref outcome as const string, byval error_count as integer )
declare sub fbSemanticDiagnosticsContext( byref context_text as const string )

'' A parser-selected point survives a continued header. It never denotes an
'' editable range, and clearing the context also clears this temporary point.
declare sub fbSemanticDiagnosticsAt( byref context_text as const string, byref source as LEX_LOCATION )

'' Set a point without replacing a diagnostic context selected by an enclosing parser.
declare sub fbSemanticDiagnosticsPoint( byref source as LEX_LOCATION )

'' Invalid headers can leave recovery symbols in the native table. They cannot
'' prove a later declaration conflict, even though ordinary errors still emit.
declare sub fbSemanticDiagnosticsProcedure( byval proc as FBSYMBOL ptr, byval header_valid as integer )
declare function fbSemanticDiagnosticsProcedureValid( byval proc as FBSYMBOL ptr ) as integer

'' Original spellings belong to live native variables. The duplicate checker
'' selects the conflicting binding; the parser supplies the rejected site.
declare sub fbSemanticDiagnosticsVariableCreated( byval sym as FBSYMBOL ptr, byval spelling as const zstring ptr )
declare sub fbSemanticDiagnosticsVariableReleased( byval sym as FBSYMBOL ptr )
declare sub fbSemanticDiagnosticsVariableRejected( byval previous as FBSYMBOL ptr, byval spelling as const zstring ptr )
declare sub fbSemanticDiagnosticsVariableAttempt( )
declare function fbSemanticDiagnosticsVariableCollision( ) as integer

#endif
'' end of tooling/semantic-diagnostics.bi
