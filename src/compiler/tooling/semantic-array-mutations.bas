'' Project: FreeBASIC compiler - native array mutations
'' File: tooling/semantic-array-mutations.bas
'' Purpose: Retain parsed ERASE and REDIM targets before runtime lowering consumes them.
'' Responsibilities: Selected array identity, original place and counted closure.
'' This file intentionally does NOT decide whether a collection is useful.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-array-mutations.bi"
#include once "tooling/semantic-source.bi"
#include once "tooling/semantic-constructs.bi"
#include once "tooling/semantic-coordinates.bi"
#include once "tooling/semantic-macros.bi"

'' The parser and exporter are serial. Reset this module-local count before
'' parsing each root, including roots emitted by the same compiler process.
'' Match the bounded source-identity domain used by the wire reader.
const SEMANTIC_ARRAY_MUTATION_LIMIT as longint = 1000000
dim shared as longint semantic_array_mutation_count

sub fbSemanticModelResetArrayMutations()
	semantic_array_mutation_count = 0
end sub

sub fbSemanticModelArrayMutations(byval expr as ASTNODE ptr, byval sym as FBSYMBOL ptr, _
	byref kind as const string, byref first as LEX_LOCATION, byref last as LEX_LOCATION, _
	byval expanded as integer)
	if( fbSemanticModelFullEnabled() = FALSE ) then exit sub
	if( fbSemanticModelMacroProvenanceEnabled() = FALSE ) then exit sub
	if( lex.ctx->semantic_probe ) then exit sub
	if( (expr = NULL) or (sym = NULL) ) then
		fbSemanticModelFailAt("array mutation input is unavailable")
		exit sub
	end if
	dim as longint statement = fbSemanticModelCurrentStatement()
	dim as longint sourceid = fbSemanticModelCurrentSource()
	dim as longint symbolid = fbSemanticModelSymbolId(sym)
	if( (statement = 0) or (sourceid = 0) or (symbolid = 0) or _
	    (symbIsArray(sym) = FALSE) or (semantic_array_mutation_count >= SEMANTIC_ARRAY_MUTATION_LIMIT) ) then
		fbSemanticModelFailAt("array mutation observation exceeds its contract")
		exit sub
	end if
	'' A direct array variable needs no receiver evaluation. Field and pointer
	'' places remain observations, but do not claim that receiver storage is
	'' the collection being cleared. Parentheses do not alter this native tree.
	dim as ASTNODE ptr place = expr
	if( astGetClass(place) = AST_NODECLASS_NIDXARRAY ) then place = place->l
	dim as integer direct_place = FALSE
	if( place <> NULL ) then
		if( (astGetClass(place) = AST_NODECLASS_VAR) and (astGetSymbol(place) = sym) ) then
			direct_place = TRUE
		end if
	end if
	'' This family closes direct variable targets only. Receiver expressions
	'' remain ordinary source uses and cannot prove a write-only local array.
	if( direct_place = FALSE ) then exit sub
	if( (kind <> "erase") and (kind <> "redim") and (kind <> "redim-preserve") ) then
		fbSemanticModelFailAt("unknown source array mutation")
		exit sub
	end if
	semantic_array_mutation_count += 1
	dim as string role = "native-array-mutation:" + fbSemanticModelNumber(semantic_array_mutation_count)
	dim as string payload = kind + TABCHAR + fbSemanticModelNumber(statement) + TABCHAR + fbSemanticModelNumber(sourceid) + _
		TABCHAR + iif(expanded, "expanded", "variable") + TABCHAR + fbSemanticModelNumber(first.macro_identity)
	fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + fbSemanticModelNumber(symbolid) + _
		TABCHAR + role + TABCHAR + fbSemanticModelEscape(payload))
	fbSemanticModelAppendDetail("H" + TABCHAR + "symbol" + TABCHAR + fbSemanticModelNumber(symbolid) + _
		TABCHAR + "symbol" + TABCHAR + fbSemanticModelNumber(symbolid) + TABCHAR + role + TABCHAR + "0")
	fbSemanticModelExportCoordinates("source-context", sourceid, role, first, last)
	fbSemanticModelMacroOrigin("symbol", symbolid, first.macro_identity, role)
end sub

sub fbSemanticModelFinishArrayMutations()
	if( fbSemanticModelFullEnabled() = FALSE ) then exit sub
	if( fbSemanticModelMacroProvenanceEnabled() = FALSE ) then exit sub
	dim as string owner = fbSemanticModelNumber(fbSemanticModelSymbolId(@symbGetGlobalNamespc()))
	dim as string role = "native-array-mutation-count:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity())
	fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + owner + TABCHAR + role + _
		TABCHAR + fbSemanticModelNumber(semantic_array_mutation_count))
end sub
'' end of tooling/semantic-array-mutations.bas
