'' Project: FreeBASIC compiler semantic observations
'' File: tooling/semantic-array-initializer-closure-export.bi
'' Purpose: Own counted original array-element coverage for one synchronous module.
'' Responsibilities: Reset state and emit one independent full-mode closure header.
'' This file intentionally does NOT contain: initializer role inference, AST ownership or code generation.

#ifndef FB_ARRAY_INITIALIZER_CLOSURE_EXPORT_BI
#define FB_ARRAY_INITIALIZER_CLOSURE_EXPORT_BI

'' The parser/exporter is synchronous within a compiler process. Each accepted
'' element has a native expression identity; the existing expression/detail
'' limits bound this counter long before a signed64 total could overflow.
dim shared as longint semantic_array_initializer_input_count

sub fbSemanticModelResetArrayInitializerInputs( )
    semantic_array_initializer_input_count = 0
end sub

sub fbSemanticModelExportArrayInitializerInputCount( )
    if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
    if( fbSemanticExtraFactsEnabled(FB_SEMEXTRA_INITIALIZERS) = FALSE ) then exit sub
    fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + _
        fbSemanticModelNumber(fbSemanticModelSymbolId(@symbGetGlobalNamespc( ))) + TABCHAR + _
        "original-array-initializer-count" + TABCHAR + fbSemanticModelNumber(semantic_array_initializer_input_count))
end sub

#endif
'' end of tooling/semantic-array-initializer-closure-export.bi
