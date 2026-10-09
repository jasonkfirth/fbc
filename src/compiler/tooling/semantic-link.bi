'' Project: FreeBASIC compiler - native link observations
'' File: tooling/semantic-link.bi
'' Purpose: Define invocation-owned linker and source-binding observations.
'' Responsibilities: Independent output, native callbacks and bounded lifetimes.
'' This file intentionally does NOT contain: source heuristics or linker policy.

#ifndef __FB_SEMANTIC_LINK_BI__
#define __FB_SEMANTIC_LINK_BI__

#include once "core/fbint.bi"
#include once "lexer/lex.bi"

declare sub fbSemanticLinkOutput( byref filename as const string )
declare function fbSemanticLinkFilename( ) as string
declare function fbSemanticLinkBegin( ) as integer
declare function fbSemanticLinkEnd( byval succeeded as integer ) as integer
declare function fbSemanticLinkEnabled( ) as integer
declare sub fbSemanticLinkFail( )
declare sub fbSemanticLinkWrite( byref record_text as const string )
declare sub fbSemanticLinkProtect( byref filename as const string )

'' Prepare and complete surround only the actual linker tool invocation.
'' The callback mode is private: it requires a matching journal identity and
'' invocation header. Ordinary compiler arguments never authorize file writes.
declare function fbSemanticLinkPrepare( byref toolpath as const string, byref arguments as string ) as integer
declare sub fbSemanticLinkCompleted( byval native_exit as integer )
declare function fbSemanticLinkCallback( byval argument_count as integer ) as integer

declare sub fbSemanticLinkBindingsReset( )
declare function fbSemanticLinkBindingsEnd( ) as integer
declare sub fbSemanticLinkModule( byref filename as const string )
declare sub fbSemanticLinkSourceOpen( byref filename as const string, byval depth as integer )
declare sub fbSemanticLinkSourceClose( byval depth as integer )
declare sub fbSemanticLinkSourceRemapped( byval depth as integer )
declare sub fbSemanticLinkProcedure( byval proc as FBSYMBOL ptr, byref source_point as LEX_LOCATION, _
    byval header_valid as integer, byval token as integer, byval prototype as integer )
declare sub fbSemanticLinkProcedureReleased( byval proc as FBSYMBOL ptr )
declare function fbSemanticLinkProcedureObjectNeeded( byval proc as FBSYMBOL ptr ) as integer
declare sub fbSemanticLinkProcedureObject( byval proc as FBSYMBOL ptr, byref symbol_text as const string )

#endif
'' end of tooling/semantic-link.bi
