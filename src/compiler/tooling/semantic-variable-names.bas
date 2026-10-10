'' Project: FreeBASIC compiler - original variable declaration names
'' File: tooling/semantic-variable-names.bas
'' Purpose: Retain accepted case before BASIC symbol names become canonical.
'' Responsibilities: Native ownership, physical/macro origins and complete counts.
'' This file intentionally does NOT contain: a declaration parser or naming policy.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-variable-names.bi"
#include once "tooling/semantic-coordinates.bi"
#include once "tooling/semantic-source.bi"
#include once "tooling/semantic-constructs.bi"
#include once "tooling/semantic-macros.bi"

'' The compiler exports one module at a time. Only its lifecycle resets this
'' count; each accepted input uses the existing bounded detail identity space.
dim shared as longint accepted_variable_name_count

sub fbSemanticModelResetVariableNames( )
	accepted_variable_name_count = 0
end sub

sub fbSemanticModelFinishVariableNames( )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( fbSemanticModelMacroProvenanceEnabled( ) = FALSE ) then exit sub
	dim as string row = "K" + TABCHAR + "symbol" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(@symbGetGlobalNamespc( ))) + _
		TABCHAR + "accepted-variable-name-count:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )) + _
		TABCHAR + fbSemanticModelNumber(accepted_variable_name_count)
	fbSemanticModelAppendDetail(row)
end sub

sub fbSemanticModelVariableDeclarationSpelling _
	( byval sym as FBSYMBOL ptr, byval spelling as const zstring ptr, _
	  byref kind as const string, byref source as LEX_LOCATION )

	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	'' Compact exports cannot retain generated declaration ownership. Do not
	'' claim a complete spelling set when some accepted inputs have no origin.
	if( fbSemanticModelMacroProvenanceEnabled( ) = FALSE ) then exit sub
	if( lex.ctx->semantic_probe or (sym = NULL) or (spelling = NULL) ) then exit sub
	if( (sym->class <> FB_SYMBCLASS_VAR) or (len(*spelling) < 1) or _
	    (len(*spelling) > FB_MAXNAMELEN) or (ucase(*spelling) <> ucase(*symbGetName(sym))) ) then
		fbSemanticModelFailAt("invalid variable declaration spelling")
		exit sub
	end if
	if( (kind <> "declared") and (kind <> "inferred") and (kind <> "implicit") ) then
		fbSemanticModelFailAt("invalid variable declaration spelling kind")
		exit sub
	end if
	dim as longint symbolid = fbSemanticModelSymbolId(sym)
	dim as longint statement = fbSemanticModelCurrentStatement( )
	dim as longint sourceid = fbSemanticModelCurrentSource( )
	if( (symbolid = 0) or (statement = 0) or (sourceid = 0) ) then
		fbSemanticModelFailAt("unavailable variable declaration spelling ownership")
		exit sub
	end if
	dim as longint identity = fbSemanticModelNextDetailIdentity( )
	if( identity < 1 ) then exit sub
	accepted_variable_name_count += 1
	'' Record the accepted parser buffer, including expanded names. A physical
	'' spelling may be absent after expansion; the actual macro origin remains.
	dim as string role = "variable-declaration-spelling:" + fbSemanticModelNumber(identity)
	dim as string payload = fbSemanticModelNumber(statement) + TABCHAR + fbSemanticModelNumber(sourceid) + _
		TABCHAR + kind + TABCHAR + *spelling
	fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + fbSemanticModelNumber(symbolid) + _
		TABCHAR + role + TABCHAR + fbSemanticModelEscape(payload))
	fbSemanticModelExportCoordinates("source-context", sourceid, role, source, source)
	fbSemanticModelMacroOrigin("symbol", symbolid, source.macro_identity, role)
end sub

'' end of tooling/semantic-variable-names.bas
