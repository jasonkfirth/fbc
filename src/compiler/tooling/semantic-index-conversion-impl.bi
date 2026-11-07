'' Project: FreeBASIC compiler - native index observations
'' File: tooling/semantic-index-conversion-impl.bi
'' Purpose: Export immutable typed pairs before native index offset lowering.
'' Responsibilities: Group identities, dimension edges and complete counts.
'' This file intentionally does NOT infer indices from lowered byte offsets.

'' One frontend owns this state. Reset it with the expression exporter; no
'' worker or runtime thread reads these synchronous metadata observations.
'' Slots are array groups, array dimensions, string groups and pointer groups.
dim shared as longint native_index_counts(0 to 3)
const SEMANTIC_NATIVE_INDEX_LIMIT = 1000000

private sub hResetIndexConversion( )
	for ordinal as integer = 0 to 3
		native_index_counts(ordinal) = 0
	next
end sub

sub fbSemanticModelIndexConversionCounts( )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	dim as string payload = "1"
	for ordinal as integer = 0 to 3
		payload += TABCHAR + fbSemanticModelNumber(native_index_counts(ordinal))
	next
	fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(@symbGetGlobalNamespc( ))) + TABCHAR + _
		"native-index-counts:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )) + TABCHAR + fbSemanticModelEscape(payload))
end sub

sub fbSemanticModelIndexConversion(byval result as ASTNODE ptr, byref kind as const string, _
	byval original_id as longint, byval selected_id as longint, byval ordinal as integer, byval rank as integer)
	if( (fbSemanticModelFullEnabled( ) = FALSE) or lex.ctx->semantic_probe ) then exit sub
	if( result = NULL ) then fbSemanticModelFailAt("native index lacks its result"): exit sub
	dim as longint identity = result->semantic_expression
	if( (original_id <= 0) or (selected_id < original_id) or (selected_id >= identity) or _
		(rank < 1) or (rank > FB_MAXARRAYDIMS) or (ordinal < 0) or (ordinal >= rank) ) then
		fbSemanticModelFailAt("native index typed pair is incomplete"): exit sub
	end if
	dim as integer count_slot
	select case kind
	case "array": count_slot = 0
	case "string": count_slot = 2
	case "pointer": count_slot = 3
	case else: fbSemanticModelFailAt("native index kind is invalid"): exit sub
	end select
	if( (kind <> "array") and ((rank <> 1) or (ordinal <> 0)) ) then
		fbSemanticModelFailAt("scalar index rank is invalid"): exit sub
	end if
	if( ordinal = 0 ) then native_index_counts(count_slot) += 1
	if( kind = "array" ) then native_index_counts(1) += 1
	if( (native_index_counts(count_slot) > SEMANTIC_NATIVE_INDEX_LIMIT) or _
		(native_index_counts(1) > SEMANTIC_NATIVE_INDEX_LIMIT) ) then
		fbSemanticModelFailAt("native index observation limit exceeded"): exit sub
	end if
	dim as string prefix = "K" + TABCHAR + "expression" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR
	if( ordinal = 0 ) then
		fbSemanticModelAppendDetail(prefix + "native-index-kind" + TABCHAR + kind)
		fbSemanticModelAppendDetail(prefix + "native-index-rank" + TABCHAR + fbSemanticModelNumber(rank))
	end if
	dim as string suffix = fbSemanticModelNumber(ordinal)
	fbSemanticModelAppendDetail(prefix + "native-index-original:" + suffix + TABCHAR + fbSemanticModelNumber(original_id))
	fbSemanticModelAppendDetail(prefix + "native-index-selected:" + suffix + TABCHAR + fbSemanticModelNumber(selected_id))
	fbSemanticModelAppendDetail("H" + TABCHAR + "expression" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR + _
		"expression" + TABCHAR + fbSemanticModelNumber(original_id) + TABCHAR + "native-index-conversion:" + suffix + TABCHAR + _
		fbSemanticModelNumber(selected_id))
end sub

'' end of tooling/semantic-index-conversion-impl.bi
