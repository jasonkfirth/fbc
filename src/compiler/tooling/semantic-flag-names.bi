'' Project: FreeBASIC compiler - stable semantic flag vocabulary
'' File: tooling/semantic-flag-names.bi
'' Purpose: Expose class-aware flag names without fixed public bit positions.
'' Responsibilities: Bounded vocabulary queries and pure mask decoding.
'' This file intentionally does NOT contain: native symbol access or serialization.

#ifndef __FB_SEMANTIC_FLAG_NAMES_BI__
#define __FB_SEMANTIC_FLAG_NAMES_BI__
declare function fbSemanticFlagDefinitionCount( ) as integer
declare function fbSemanticFlagDefinition( byval ordinal as integer, byref group_name as string, _
    byref flag_name as string, byref bits as ulong, byref classes as string ) as integer
declare function fbSemanticNamedFlags( byval group_id as integer, byval symbol_class as integer, _
    byval bits as ulong, byref unmapped as ulong ) as string
#endif

'' end of tooling/semantic-flag-names.bi

