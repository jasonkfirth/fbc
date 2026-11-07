'' Project: FreeBASIC compiler - native index observations
'' File: tooling/semantic-index-conversion.bi
'' Purpose: Retain original and selected native subscript inputs.
'' Responsibilities: Typed index pairs and independent per-module counts.
'' This file intentionally does NOT impose index-type policy or build casts.
#ifndef __FB_SEMANTIC_INDEX_CONVERSION_BI__
#define __FB_SEMANTIC_INDEX_CONVERSION_BI__
declare sub fbSemanticModelIndexConversion(byval result as ASTNODE ptr, byref kind as const string, _
	byval original_id as longint, byval selected_id as longint, byval ordinal as integer, byval rank as integer)
declare sub fbSemanticModelIndexConversionCounts( )
#endif
'' end of tooling/semantic-index-conversion.bi
