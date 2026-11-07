'' Project: FreeBASIC compiler - visible mathematical constant inputs
'' File: tooling/semantic-math-constant-impl.bi
'' Purpose: Snapshot selected definitions without expanding or executing macros.
'' Responsibilities: Native token types/values, definition identity and closure.
'' Ownership: Lexer probes own their copied buffers and always restore the parent.
'' This file intentionally does NOT contain: source rewriting or callback calls.

#ifndef __FB_SEMANTIC_MATH_CONSTANT_IMPL_BI__
#define __FB_SEMANTIC_MATH_CONSTANT_IMPL_BI__

private function hMathConstantValue( byval sym as FBSYMBOL ptr, byref dtype as integer, byref bits as ulongint ) as integer

	dtype = 0: bits = 0
	'' The probe accepts one native floating token, not an expression. In
	'' particular it cannot execute function-like macros or preprocessor code.
	if( sym->def.isargless or (sym->def.params <> 0) or (sym->def.dprocz <> NULL) ) then return FALSE
	dim as string buffer_text
	dim as integer units
	if( sym->typ = FB_DATATYPE_CHAR ) then
		if( sym->def.text = NULL ) then return FALSE
		units = len(*sym->def.text)
		if( (units = 0) or (units > 1024) ) then return FALSE
		buffer_text = *sym->def.text
	elseif( sym->typ = FB_DATATYPE_WCHAR ) then
		if( sym->def.textw = NULL ) then return FALSE
		units = len(*sym->def.textw)
		if( (units = 0) or (units > 1024) ) then return FALSE
		'' Numeric syntax is ASCII. Reject non-ASCII units rather than let a
		'' narrowing conversion change a wide replacement into another token.
		for position as integer = 1 to units
			dim as uinteger codeunit = asc(*sym->def.textw, position)
			if( codeunit > 127 ) then return FALSE
			buffer_text += chr(codeunit)
		next
	else
		return FALSE
	end if
	buffer_text += chr(10)
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
	dim as FBTOKEN token, ending
	dim as integer flags = LEXCHECK_NODEFINE or LEXCHECK_NOSYMBOL or LEXCHECK_NOLINECONT
	lexNextToken(@token, flags)
	dim as integer accepted = FALSE
	if( (token.class = FB_TKCLASS_NUMLITERAL) and (typeGetClass(token.dtype) = FB_DATACLASS_FPOINT) ) then
		lexNextToken(@ending, flags)
		if( ending.id = FB_TK_EOL ) then
			dim as double value = val(token.text)
			if( token.dtype = FB_DATATYPE_SINGLE ) then
				dim as single selected = value
				value = selected
			end if
			bits = *cptr(ulongint ptr, @value): dtype = token.dtype
			accepted = TRUE
		end if
	end if
	lexProbeEnd(probe)
	return accepted

end function

#include once "tooling/semantic-math-catalog-impl.bi"

sub fbSemanticModelMathConstantInputs( byval expression as longint )

	if( (fbSemanticModelFullEnabled() = FALSE) or lex.ctx->semantic_probe or (expression <= 0) ) then exit sub
	if( semantic_math_constant_count >= 1000000 ) then
		fbSemanticModelFailAt("mathematical constant observation limit exceeded"): exit sub
	end if
	semantic_math_constant_count += 1
	'' Defines live in the global preprocessor hash even inside namespaces.
	'' Query now: a later #define, #undef or redefinition is a different binding.
	'' Version 1, then M_PI/M_E/M_TAU triples:
	''   active MD identity, schema datatype, expanded binary64 bits (16 hex).
	'' A zero identity proves no active definition. Datatype zero with a
	'' nonzero identity means a definition exists but is not a scalar token.
	dim as longint definitions(0 to 2)
	dim as integer dtypes(0 to 2)
	dim as ulongint values(0 to 2)
	dim as string payload = "1"
	for ordinal as integer = 0 to 2
		dim as string spelling
		select case ordinal
		case 0: spelling = "M_PI"
		case 1: spelling = "M_E"
		case 2: spelling = "M_TAU"
		end select
		dim as FBSYMBOL ptr sym = symbLookupByNameAndClass(@symbGetGlobalNamespc(), strptr(spelling), FB_SYMBCLASS_DEFINE)
		dim as longint definition = 0
		dim as integer dtype = 0
		dim as ulongint bits = 0
		if( sym <> NULL ) then
			definition = hDefinition(sym)
			hMathConstantValue(sym, dtype, bits)
		end if
		definitions(ordinal) = definition: dtypes(ordinal) = dtype: values(ordinal) = bits
		payload += TABCHAR + fbSemanticModelNumber(definition) + TABCHAR + fbSemanticModelTypeNumber(culng(dtype)) + TABCHAR + hex(bits, 16)
	next
	fbSemanticModelAppendDetail("K" + TABCHAR + "expression" + TABCHAR + fbSemanticModelNumber(expression) + _
		TABCHAR + "visible-math-constant-inputs" + TABCHAR + fbSemanticModelEscape(payload))
	hMathCatalogInputs(expression, definitions(), dtypes(), values())

end sub

sub fbSemanticModelMathConstantCounts( )
	if( fbSemanticModelFullEnabled() = FALSE ) then exit sub
	fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(@symbGetGlobalNamespc())) + TABCHAR + _
		"visible-math-constant-count:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity()) + TABCHAR + _
		fbSemanticModelNumber(semantic_math_constant_count))
end sub

#endif

'' end of tooling/semantic-math-constant-impl.bi
