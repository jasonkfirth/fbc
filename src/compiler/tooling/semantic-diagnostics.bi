'' Project: FreeBASIC compiler - structured diagnostics
'' File: tooling/semantic-diagnostics.bi
'' Purpose: Publish compiler diagnostics independently of accepted AST models.
'' Responsibilities: Invocation lifetime, source protection and parser context.
'' This file intentionally does NOT expose recovered symbols as semantic facts.

#ifndef __FB_SEMANTIC_DIAGNOSTICS_BI__
#define __FB_SEMANTIC_DIAGNOSTICS_BI__

declare function fbSemanticDiagnosticsBegin( byref filename as const string ) as integer
declare function fbSemanticDiagnosticsEnd( byval succeeded as integer ) as integer
declare sub fbSemanticDiagnosticsProtectFile( byref filename as const string )
declare sub fbSemanticDiagnosticsSource( byref filename as const string )
declare sub fbSemanticDiagnosticsModule( byref filename as const string )
declare sub fbSemanticDiagnosticsContext( byref context_text as const string )

#endif
'' end of tooling/semantic-diagnostics.bi
