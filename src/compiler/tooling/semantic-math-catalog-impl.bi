'' Project: FreeBASIC compiler - mathematical catalogue inputs
'' File: tooling/semantic-math-catalog-impl.bi
'' Purpose: Snapshot additional active mathematical definitions at literal input.
'' Responsibilities: Exact identities and native token values in a fixed catalogue.
'' Ownership: Uses the existing read-only scalar probe; retains no lexer pointers.
'' This file intentionally does NOT expand macros or construct expression ASTs.
#ifndef __FB_SEMANTIC_MATH_CATALOG_IMPL_BI__
#define __FB_SEMANTIC_MATH_CATALOG_IMPL_BI__
#include once "tooling/semantic-math-catalog-names.bi"

private sub hMathCatalogInputs(byval expression as longint, definitions() as longint, dtypes() as integer, values() as ulongint)
	'' Version 1 contains 21 triples, in the pinned catalogue's ordinal order:
	'' active MD identity, schema datatype and 16 hexadecimal binary64 bits.
	'' The legacy three-entry snapshot shares these same captured definitions.
	dim as string payload = "1"
	for ordinal as integer = 0 to 20
		dim as string spelling = hMathCatalogName(ordinal)
		dim as longint definition = 0
		dim as integer dtype = 0, previous = -1
		dim as ulongint bits = 0
		select case spelling
		case "M_PI": previous = 0
		case "M_E": previous = 1
		case "M_TAU": previous = 2
		end select
		if( previous >= 0 ) then
			definition = definitions(previous): dtype = dtypes(previous): bits = values(previous)
		else
			dim as FBSYMBOL ptr sym = symbLookupByNameAndClass(@symbGetGlobalNamespc(), strptr(spelling), FB_SYMBCLASS_DEFINE)
			if( sym <> NULL ) then
				definition = hDefinition(sym)
				hMathConstantValue(sym, dtype, bits)
			end if
		end if
		payload += TABCHAR + fbSemanticModelNumber(definition) + TABCHAR + fbSemanticModelTypeNumber(culng(dtype)) + TABCHAR + hex(bits, 16)
	next
	fbSemanticModelAppendDetail("K" + TABCHAR + "expression" + TABCHAR + fbSemanticModelNumber(expression) + _
		TABCHAR + "visible-math-catalog-inputs" + TABCHAR + fbSemanticModelEscape(payload))
end sub

#endif
'' end of tooling/semantic-math-catalog-impl.bi
