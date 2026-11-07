'' Project: FreeBASIC compiler semantic observations
'' File: tooling/semantic-string-search.bi
'' Purpose: Preserve original operands and ownership for native string searches.
'' Responsibilities: Record typed INSTR/INSTRREV inputs and per-module counts.
'' This file intentionally does NOT infer patterns or change runtime evaluation.

'' Parsing and sidecar emission are serial. Reset this count for each module so
'' a compiler request containing several roots cannot mix their search facts.
dim shared as longint semantic_string_search_count

sub fbSemanticModelResetStringSearches()
	semantic_string_search_count = 0
end sub

sub fbSemanticModelStringSearch(byval result as ASTNODE ptr, byref kind as const string, _
	byval haystack as longint, byval pattern as longint, byval start_at as longint, _
	byval is_any as integer, byref first as LEX_LOCATION, byval nonphysical_start as longint)
	if( fbSemanticModelFullEnabled() = FALSE ) then exit sub
	if( fbSemanticModelMacroProvenanceEnabled() = FALSE ) then exit sub
	if( lex.ctx->semantic_probe ) then exit sub
	if( result = NULL ) then exit sub

	dim as longint statement = fbSemanticModelCurrentStatement()
	dim as longint sourceid = fbSemanticModelCurrentSource()
	if( (haystack <= 0) or (pattern <= 0) or (start_at < 0) or _
	    (statement <= 0) or (sourceid <= 0) or (semantic_string_search_count >= 1000000) ) then
		fbSemanticModelFailAt("original string search operands are unavailable")
		exit sub
	end if
	if( (kind <> "instr") and (kind <> "instrrev") ) then
		fbSemanticModelFailAt("unknown original string search kind")
		exit sub
	end if

	dim as LEX_LOCATION last = lexGetLastLocation()
	dim as longint expressionid = fbSemanticModelOriginalExpression(result, first, nonphysical_start, _
		last, lexGetNonphysicalTokenCount())
	dim as longint owner = fbSemanticModelSymbolId(@symbGetGlobalNamespc())
	if( (expressionid <= haystack) or (expressionid <= pattern) or _
	    (expressionid <= start_at) or (owner <= 0) ) then
		fbSemanticModelFailAt("completed string search identity is unavailable")
		exit sub
	end if

	semantic_string_search_count += 1
	dim as string role = "original-string-search:" + fbSemanticModelNumber(semantic_string_search_count)
	dim as string payload = kind + TABCHAR + fbSemanticModelNumber(statement) + TABCHAR + _
		fbSemanticModelNumber(sourceid) + TABCHAR + fbSemanticModelNumber(haystack) + TABCHAR + _
		fbSemanticModelNumber(pattern) + TABCHAR + fbSemanticModelNumber(start_at) + TABCHAR + _
		fbSemanticModelNumber(abs(is_any <> FALSE))
	fbSemanticModelAppendDetail("K" + TABCHAR + "expression" + TABCHAR + fbSemanticModelNumber(expressionid) + _
		TABCHAR + role + TABCHAR + fbSemanticModelEscape(payload))
	fbSemanticModelAppendDetail("H" + TABCHAR + "symbol" + TABCHAR + fbSemanticModelNumber(owner) + _
		TABCHAR + "expression" + TABCHAR + fbSemanticModelNumber(expressionid) + TABCHAR + role + TABCHAR + "0")
end sub

sub fbSemanticModelFinishStringSearches()
	if( fbSemanticModelFullEnabled() = FALSE ) then exit sub
	if( fbSemanticModelMacroProvenanceEnabled() = FALSE ) then exit sub
	dim as string owner = fbSemanticModelNumber(fbSemanticModelSymbolId(@symbGetGlobalNamespc()))
	fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + owner + TABCHAR + _
		"original-string-search-count:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity()) + TABCHAR + _
		fbSemanticModelNumber(semantic_string_search_count))
end sub

'' end of tooling/semantic-string-search.bi
