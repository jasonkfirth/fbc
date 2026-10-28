'' Project: FreeBASIC compiler - consumed lexer tokens
'' File: tooling/semantic-tokens.bas
'' Purpose: Preserve native payloads of tokens actually consumed by the parser.
'' Responsibilities: Copy active token unions, source coordinates and lookup IDs.
'' This file intentionally does NOT contain: lexer reads, probes or fabricated bindings.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-tokens.bi"
#include once "tooling/semantic-constructs.bi"
#include once "parser/parser.bi"
#include once "preprocessor/pp.bi"
#include once "crt/mem.bi"

'' Compilation is serial. The collector owns copied strings and scalar IDs;
'' it never retains a pointer into the rotating lexer/lookahead buffers.
private const CONSUMED_TOKEN_LIMIT = 1000000
private dim shared as longint consumed_count

sub fbSemanticModelResetTokens( )
	consumed_count = 0
end sub

private sub hProperty( byref key as const string, byref value as const string )
	fbSemanticModelAppendProvenance("K" + TABCHAR + "module" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelModuleIdentity( )) + TABCHAR + key + TABCHAR + fbSemanticModelEscape(value))
end sub

sub fbSemanticModelConsumedToken( byval token as FBTOKEN ptr )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( lex.ctx = NULL ) then exit sub
	if( lex.ctx->semantic_probe or lex.ctx->reclevel or pp.skipping or fbGetIsPP() ) then exit sub
	if( token = NULL ) then fbSemanticModelFailAt("consumed token is unavailable"): exit sub
	if( token->class < FB_TKCLASS_IDENTIFIER or token->class > FB_TKCLASS_UNKNOWN or _
		token->len < 0 or token->len > FB_MAXLITLEN or consumed_count >= CONSUMED_TOKEN_LIMIT ) then
		fbSemanticModelFailAt("consumed token payload exceeds its native bounds")
		exit sub
	end if
	dim as string text, text_kind = "bytes", union_kind = "none", numeric_suffix, original
	dim as integer union_value, suffix_known, suffix_char, numeric_known, string_known
	dim as integer original_length, original_digits, original_dot, original_exponent, original_zeros, payload, macro_units
	dim as integer truncated, type_known
	dim as longint wire_type = FB_DATATYPE_INVALID
	if( token->class = FB_TKCLASS_STRLITERAL and token->dtype = FB_DATATYPE_WCHAR ) then
		text_kind = "wide-units"
		for index as integer = 0 to token->len - 1
			text += hex(culng(token->textw[index]), 8)
		next
	else
		text = space(token->len)
		if( token->len ) then memcpy(strptr(text), @token->text[0], token->len)
	end if
	select case token->class
	case FB_TKCLASS_IDENTIFIER, FB_TKCLASS_KEYWORD, FB_TKCLASS_QUIRKWD
		union_kind = "period-position"
		union_value = token->prdpos
		suffix_known = TRUE
		suffix_char = token->suffixchar
		type_known = TRUE
	case FB_TKCLASS_NUMLITERAL
		union_kind = "numeric-suffix-present"
		union_value = abs(token->hassuffix <> FALSE)
		'' hReadNumber owns hassuffix/numeric_suffix, not suffixchar.
		'' The identifier suffix slot can still contain a prior token.
		type_known = TRUE
		numeric_known = TRUE
		numeric_suffix = token->numeric_suffix
		original = token->numeric_original
		original_length = token->numeric_original_length
		original_digits = token->numeric_original_digits
		original_dot = abs(token->numeric_original_dot <> FALSE)
		original_exponent = abs(token->numeric_original_exponent <> FALSE)
		original_zeros = token->numeric_original_leading_zeros
		payload = token->numeric_payload
		macro_units = token->numeric_macro_units
	case FB_TKCLASS_STRLITERAL
		union_kind = "string-escapes-present"
		union_value = abs(token->hasesc <> FALSE)
		string_known = TRUE
		truncated = abs(token->literal_truncated <> FALSE)
		type_known = TRUE
	end select
	if( type_known ) then wire_type = fbSemanticModelTypeCode(culng(token->dtype))
	consumed_count += 1
	dim as string value = fbSemanticModelNumber(token->id) + TABCHAR + fbSemanticModelNumber(token->class) + TABCHAR + _
		fbSemanticModelNumber(culng(token->dtype)) + TABCHAR + fbSemanticModelNumber(abs(type_known)) + TABCHAR + _
		fbSemanticModelNumber(wire_type) + TABCHAR + fbSemanticModelNumber(token->len) + TABCHAR + text_kind + TABCHAR + _
		fbSemanticModelEscape(text) + TABCHAR + fbSemanticModelNumber(abs(token->after_space <> FALSE)) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelCurrentStatement()) + TABCHAR + fbSemanticModelNumber(fbSemanticModelCurrentContext()) + TABCHAR + _
		fbSemanticModelNumber(token->source.source_context) + TABCHAR + fbSemanticModelEscape(token->source.source_file) + TABCHAR + _
		fbSemanticModelNumber(token->source.start_line) + TABCHAR + fbSemanticModelNumber(token->source.start_column) + TABCHAR + _
		fbSemanticModelNumber(token->source.end_line) + TABCHAR + fbSemanticModelNumber(token->source.end_column) + TABCHAR + _
		fbSemanticModelNumber(abs(token->source.is_physical <> FALSE)) + TABCHAR + fbSemanticModelNumber(token->source.macro_identity) + TABCHAR + _
		fbSemanticModelNumber(token->source.macro_start_unit) + TABCHAR + fbSemanticModelNumber(abs(token->source.macro_unit_valid <> FALSE)) + TABCHAR + _
		fbSemanticModelNumber(token->source.raw_start_line) + TABCHAR + fbSemanticModelNumber(token->source.raw_start_column) + TABCHAR + _
		fbSemanticModelNumber(token->source.raw_end_line) + TABCHAR + fbSemanticModelNumber(token->source.raw_end_column) + TABCHAR + _
		fbSemanticModelNumber(abs(token->source.raw_valid <> FALSE)) + TABCHAR + union_kind + TABCHAR + fbSemanticModelNumber(union_value) + TABCHAR + _
		fbSemanticModelNumber(abs(suffix_known)) + TABCHAR + fbSemanticModelNumber(suffix_char) + TABCHAR + _
		fbSemanticModelNumber(abs(numeric_known)) + TABCHAR + fbSemanticModelEscape(numeric_suffix) + TABCHAR + fbSemanticModelEscape(original) + TABCHAR + _
		fbSemanticModelNumber(original_length) + TABCHAR + fbSemanticModelNumber(original_digits) + TABCHAR + fbSemanticModelNumber(original_dot) + TABCHAR + _
		fbSemanticModelNumber(original_exponent) + TABCHAR + fbSemanticModelNumber(original_zeros) + TABCHAR + fbSemanticModelNumber(payload) + TABCHAR + _
		fbSemanticModelNumber(macro_units) + TABCHAR + fbSemanticModelNumber(abs(string_known)) + TABCHAR + fbSemanticModelNumber(truncated) + TABCHAR + _
		fbSemanticModelNumber(token->semantic_lookup)
	hProperty("consumed-token:" + fbSemanticModelNumber(consumed_count), value)
end sub

sub fbSemanticModelFinishTokens( byval commit as integer )
	if( commit and fbSemanticModelFullEnabled() ) then
		hProperty("consumed-token-policy", fbSemanticModelNumber(FB_MAXLITLEN) + TABCHAR + fbSemanticModelNumber(FB_DATATYPE_WCHAR))
		hProperty("consumed-token-count", fbSemanticModelNumber(consumed_count))
	end if
	fbSemanticModelResetTokens()
end sub

'' end of tooling/semantic-tokens.bas
