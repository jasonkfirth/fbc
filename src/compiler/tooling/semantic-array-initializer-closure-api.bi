'' Project: FreeBASIC compiler semantic observations
'' File: tooling/semantic-array-initializer-closure-api.bi
'' Purpose: Declare lifecycle hooks for original array-element receipt coverage.
'' Responsibilities: Per-module reset and full-mode count export.
'' This file intentionally does NOT contain: AST layouts or process-global consumers.

#ifndef FB_ARRAY_INITIALIZER_CLOSURE_API_BI
#define FB_ARRAY_INITIALIZER_CLOSURE_API_BI
declare sub fbSemanticModelResetArrayInitializerInputs( )
declare sub fbSemanticModelExportArrayInitializerInputCount( )
#endif
'' end of tooling/semantic-array-initializer-closure-api.bi
