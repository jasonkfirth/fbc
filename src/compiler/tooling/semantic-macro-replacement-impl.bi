'' Project: FreeBASIC compiler - macro replacement observations
'' File: tooling/semantic-macro-replacement-impl.bi
'' Purpose: Preserve effective native token classes and formal identities.
'' Responsibilities: Bounded definition snapshots and copied lexer ownership.
'' This file intentionally does NOT contain: lint policy, expansion or callbacks.

#ifndef __FB_SEMANTIC_MACRO_REPLACEMENT_IMPL_BI__
#define __FB_SEMANTIC_MACRO_REPLACEMENT_IMPL_BI__

private function hReplacementWide( byval source as wstring ptr, byref value as string ) as integer

	if( source = NULL ) then return TRUE
	if( len(*source) > 16384 ) then return FALSE
	for position as integer = 1 to len(*source)
		dim as uinteger codeunit = asc(*source, position)
		'' Do not change token boundaries through a lossy encoding conversion.
		if( codeunit > 127 ) then return FALSE
		value += chr(codeunit)
	next
	return TRUE

end function

private function hReplacementKind( byref token as FBTOKEN ) as string

	select case token.id
	case CHAR_LPRNT: return "open"
	case CHAR_RPRNT: return "close"
	case CHAR_LBRACKET: return "index-open"
	case CHAR_RBRACKET: return "index-close"
	case CHAR_COMMA: return "comma"
	case FB_TK_STMTSEP, CHAR_SEMICOLON: return "separator"
	case CHAR_DOT, FB_TK_FIELDDEREF: return "member"
	case CHAR_PLUS, CHAR_MINUS: return "sign"
	case CHAR_TIMES, CHAR_SLASH, CHAR_RSLASH, CHAR_CART, CHAR_AMP, _
		FB_TK_MOD, FB_TK_SHL, FB_TK_SHR, FB_TK_AND, FB_TK_OR, FB_TK_XOR, FB_TK_EQV, FB_TK_IMP, _
		FB_TK_ANDALSO, FB_TK_ORELSE: return "arithmetic"
	case FB_TK_EQ, FB_TK_NE, FB_TK_LT, FB_TK_GT, FB_TK_LE, FB_TK_GE, FB_TK_IS: return "comparison"
	case FB_TK_NOT: return "unary"
	case FB_TK_CAST, FB_TK_CPTR, FB_TK_TYPEOF: return "call"
	end select
	select case token.class
	case FB_TKCLASS_NUMLITERAL: return "number"
	case FB_TKCLASS_STRLITERAL: return "string"
	case FB_TKCLASS_IDENTIFIER: return "identifier"
	case FB_TKCLASS_QUIRKWD: return "call"
	case FB_TKCLASS_KEYWORD: return "keyword"
	end select
	return "other"

end function

private sub hMacroReplacementInputs( byval sym as FBSYMBOL ptr, byval definition as longint, byval symbolid as longint )

	if( fbSemanticModelFullEnabled() = FALSE ) then exit sub
	dim as string status = "unsupported", body, raw_text, records
	dim as integer count, expected_parameters, seen_parameters
	dim as integer token_macro = (sym->typ and FB_DATATYPE_INVALID) <> 0
	dim as integer eligible = (sym->def.params >= 0) and (sym->def.params <= 256)
	eligible and= (sym->def.dprocz = NULL)
	if( token_macro ) then eligible and= (sym->def.mprocw = NULL)
	eligible and= (sym->def.flags and FB_DEFINE_FLAGS_VARIADIC) = 0
	if( eligible ) then
		if( token_macro ) then
			dim as FB_DEFTOK ptr piece = sym->def.tokhead
			while( piece <> NULL )
				if( piece->semantic_paste_before ) then eligible = FALSE: exit while
				select case piece->type
				case FB_DEFTOK_TYPE_PARAM
					if( (piece->paramnum < 0) or (piece->paramnum >= sym->def.params) ) then eligible = FALSE: exit while
					expected_parameters += 1
				case FB_DEFTOK_TYPE_TEX
					if( piece->text <> NULL ) then
						if( len(*piece->text) > 16384 - len(raw_text) ) then eligible = FALSE: exit while
						raw_text += *piece->text
					end if
				case FB_DEFTOK_TYPE_TEXW
					if( hReplacementWide(piece->textw, raw_text) = FALSE ) then eligible = FALSE: exit while
				case else
					eligible = FALSE: exit while
				end select
				if( (len(raw_text) > 16384) or (expected_parameters > 1024) ) then eligible = FALSE: exit while
				piece = piece->next
			wend
		elseif( sym->typ = FB_DATATYPE_CHAR ) then
			if( sym->def.text <> NULL ) then
				if( len(*sym->def.text) > 16384 ) then
					eligible = FALSE
				else
					raw_text = *sym->def.text
				end if
			end if
		elseif( sym->typ = FB_DATATYPE_WCHAR ) then
			eligible = hReplacementWide(sym->def.textw, raw_text)
		else
			eligible = FALSE
		end if
	end if
	if( len(raw_text) > 16384 ) then eligible = FALSE
	if( eligible ) then
		'' Formal references are compiler-owned FB_DEFTOK entries. A prefix absent
		'' from every text piece prevents an ordinary identifier impersonating one.
		dim as string prefix = "__FBC_REPLACEMENT_FORMAL_"
		for attempt as integer = 0 to 32
			if( instr(ucase(raw_text), prefix) = 0 ) then exit for
			prefix += "_"
			if( attempt = 32 ) then eligible = FALSE
		next
		if( token_macro ) then
			dim as FB_DEFTOK ptr piece = sym->def.tokhead
			while( eligible and (piece <> NULL) )
				select case piece->type
				case FB_DEFTOK_TYPE_PARAM
					body += prefix + ltrim(str(piece->paramnum)) + "__"
				case FB_DEFTOK_TYPE_TEX
					if( piece->text <> NULL ) then body += *piece->text
				case FB_DEFTOK_TYPE_TEXW
					eligible = hReplacementWide(piece->textw, body)
				end select
				if( len(body) > 16384 ) then eligible = FALSE
				piece = piece->next
			wend
		else
			body = raw_text
		end if
		'' Multiple statements and explicit continuation fragments have no single
		'' replacement-expression contract. Keep the unsupported witness instead.
		body = trim(body, any chr(13, 10, 32, 9))
		if( instr(body, chr(10)) or instr(body, chr(13)) ) then eligible = FALSE
		if( eligible ) then
			dim as string buffer_text = body + chr(10)
			dim as LEX_PROBE_CTX probe
			lexProbeBegin(probe)
			with *lex.ctx
				.currchar = INVALID: .lahdchar1 = INVALID: .lahdchar2 = INVALID
				.k = 0: .reclevel += 1: .macrodepth = 0
				.semantic_macro_depth = 0: .semantic_eval_origin = 0
				.deflen = len(buffer_text)
				if( env.inf.format = FBFILE_FORMAT_ASCII ) then
					DZstrAssign(.deftext, buffer_text): .defptr = .deftext.data
				else
					DWstrAssignA(.deftextw, buffer_text): .defptrw = .deftextw.data
				end if
			end with
			'' Native lookup retains operator/keyword classification, but NODEFINE
			'' forbids recursive expansion and every preprocessor callback.
			dim as integer flags = LEXCHECK_NODEFINE or LEXCHECK_NOLINECONT or LEXCHECK_NOMULTILINECOMMENT
			for ordinal as integer = 0 to 1024
				dim as FBTOKEN token
				lexNextToken(@token, flags)
				'' A comment fragment must not make a probe read through the saved
				'' parent's file looking for a closing delimiter. Ordinary trailing
				'' comments terminate this one-line replacement without tokenizing it.
				if( (token.id = CHAR_SLASH) andalso (lexCurrentChar() = CHAR_APOST) ) then exit for
				if( (token.id = FB_TK_EOL) or (token.id = FB_TK_COMMENT) or (token.id = FB_TK_REM) ) then
					if( seen_parameters = expected_parameters ) then status = "complete"
					exit for
				end if
				if( (ordinal = 1024) or (token.id = FB_TK_EOF) ) then exit for
				dim as integer formal, dtype
				if( token.class = FB_TKCLASS_IDENTIFIER ) then
					for parameter as integer = 0 to sym->def.params - 1
						if( ucase(token.text) = prefix + ltrim(str(parameter)) + "__" ) then
							formal = parameter + 1: seen_parameters += 1: exit for
						end if
					next
				end if
				if( token.class = FB_TKCLASS_NUMLITERAL ) then dtype = token.dtype
				records += TABCHAR + hReplacementKind(token) + TABCHAR + fbSemanticModelNumber(formal) + _
					TABCHAR + fbSemanticModelTypeNumber(culng(dtype))
				count += 1
			next
			lexProbeEnd(probe)
		end if
	end if
	if( status <> "complete" ) then count = 0: records = ""
	dim as string payload = "1" + TABCHAR + status + TABCHAR + fbSemanticModelNumber(count) + _
		TABCHAR + fbSemanticModelNumber(sym->def.params) + records
	fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + fbSemanticModelNumber(symbolid) + _
		TABCHAR + "macro-replacement-tokens:" + fbSemanticModelNumber(definition) + TABCHAR + fbSemanticModelEscape(payload))

end sub

sub fbSemanticModelMacroReplacementCounts( )
	if( fbSemanticModelFullEnabled() = FALSE ) then exit sub
	fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(@symbGetGlobalNamespc())) + TABCHAR + _
		"macro-replacement-count:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity()) + TABCHAR + _
		fbSemanticModelNumber(semantic_macro_count))
end sub

#endif
'' end of tooling/semantic-macro-replacement-impl.bi
