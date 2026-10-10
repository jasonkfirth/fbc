'' Project: FreeBASIC compiler - original member declaration names
'' File: tooling/semantic-member-names.bas
'' Purpose: Preserve accepted field/constant case before symbol canonicalization.
'' Responsibilities: Native UDT ownership, source/macro origins and module counts.
'' This file intentionally does NOT contain: a parser, layout changes or naming policy.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-member-names.bi"
#include once "tooling/semantic-coordinates.bi"
#include once "tooling/semantic-source.bi"
#include once "tooling/semantic-constructs.bi"
#include once "tooling/semantic-macros.bi"

'' Export is serial and module-scoped. The compiler lifecycle owns this count;
'' each receipt shares the existing bounded detail identity space.
dim shared as longint accepted_member_name_count

sub fbSemanticModelResetMemberNames( )
	accepted_member_name_count = 0
end sub

sub fbSemanticModelFinishMemberNames( )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( fbSemanticModelMacroProvenanceEnabled( ) = FALSE ) then exit sub

	dim as string row = "K" + TABCHAR + "symbol" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(@symbGetGlobalNamespc( ))) + _
		TABCHAR + "accepted-member-name-count:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )) + _
		TABCHAR + fbSemanticModelNumber(accepted_member_name_count)
	fbSemanticModelAppendDetail(row)
end sub

sub fbSemanticModelMemberDeclarationSpelling _
	( byval sym as FBSYMBOL ptr, byval spelling as const zstring ptr, _
	  byref source as LEX_LOCATION )

	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( fbSemanticModelMacroProvenanceEnabled( ) = FALSE ) then exit sub
	if( lex.ctx->semantic_probe or (sym = NULL) or (spelling = NULL) ) then exit sub
	if( (sym->class <> FB_SYMBCLASS_FIELD) and (sym->class <> FB_SYMBCLASS_CONST) ) then exit sub
	dim as FBSYMBOL ptr parent = symbGetParent(sym)
	if( parent = NULL ) then exit sub
	if( parent->class <> FB_SYMBCLASS_STRUCT ) then exit sub
	if( (len(*spelling) < 1) or (len(*spelling) > FB_MAXNAMELEN) or _
	    (ucase(*spelling) <> ucase(*symbGetName(sym))) ) then
		fbSemanticModelFailAt("invalid member declaration spelling")
		exit sub
	end if
	dim as longint symbolid = fbSemanticModelSymbolId(sym)
	dim as longint statement = fbSemanticModelCurrentStatement( )
	dim as longint sourceid = fbSemanticModelCurrentSource( )
	if( (symbolid = 0) or (statement = 0) or (sourceid = 0) ) then
		fbSemanticModelFailAt("unavailable member declaration spelling ownership")
		exit sub
	end if
	dim as longint identity = fbSemanticModelNextDetailIdentity( )
	if( identity < 1 ) then exit sub
	accepted_member_name_count += 1
	'' Even a wholly generated field has an accepted parser buffer. Keep its
	'' actual expansion identity; a macro invocation is an anchor, not its name.
	dim as string role = "member-declaration-spelling:" + fbSemanticModelNumber(identity)
	dim as string kind = iif(sym->class = FB_SYMBCLASS_FIELD, "field", "constant")
	dim as string payload = fbSemanticModelNumber(statement) + TABCHAR + fbSemanticModelNumber(sourceid) + _
		TABCHAR + kind + TABCHAR + *spelling
	fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + fbSemanticModelNumber(symbolid) + _
		TABCHAR + role + TABCHAR + fbSemanticModelEscape(payload))
	fbSemanticModelExportCoordinates("source-context", sourceid, role, source, source)
	fbSemanticModelMacroOrigin("symbol", symbolid, source.macro_identity, role)
end sub

'' end of tooling/semantic-member-names.bas
