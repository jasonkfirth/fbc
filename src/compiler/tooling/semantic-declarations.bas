'' Project: FreeBASIC compiler - accepted source declarations
'' File: tooling/semantic-declarations.bas
'' Purpose: Preserve the order in which source declaration receipts are accepted.
'' Responsibilities: Copied symbol/context identities and bounded module closure.
'' This file intentionally does NOT contain: lexical sorting or generated declarations.

#include once "tooling/semantic-private.bi"
#include once "parser/parser.bi"
#include once "tooling/semantic-constructs.bi"
#include once "tooling/semantic-source.bi"
#include once "tooling/semantic-declarations.bi"

'' Modules are processed serially. This counter orders observations, not native
'' statement numbers, which can coincide after FBLite declaration movement.
private const DECLARATION_LIMIT = 1000000
private dim shared as longint declaration_count

sub fbSemanticModelResetDeclarations( )
	declaration_count = 0
end sub

private sub hProperty( byref key as const string, byref value as const string )
	fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(@symbGetGlobalNamespc( ))) + TABCHAR + _
		key + TABCHAR + fbSemanticModelEscape(value))
end sub

sub fbSemanticModelAcceptedDeclaration( byval sym as FBSYMBOL ptr, _
	byref kind as const string, byval receipt as longint )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	'' Compact exports retain an internal macro map without publishing MI/MO.
	'' Its native identities cannot become dangling public expansion links.
	if( (kind = "macro-point") and (fbSemanticModelMacroProvenanceEnabled( ) = FALSE) ) then exit sub
	if( (sym = NULL) or (receipt <= 0) or (declaration_count >= DECLARATION_LIMIT) ) then
		fbSemanticModelFailAt("invalid accepted declaration receipt")
		exit sub
	end if
	if( (kind <> "binding") and (kind <> "declaration") and (kind <> "macro-point") ) then
		fbSemanticModelFailAt("unknown accepted declaration route")
		exit sub
	end if

	'' Named formals can produce both a binding and a DCL receipt. Preserve both
	'' observations explicitly. Their acceptance order need not be token order:
	'' a procedure header can accept its formals before its own procedure name.
	declaration_count += 1
	hProperty("accepted-source-declaration:" + fbSemanticModelNumber(declaration_count), _
		kind + TABCHAR + fbSemanticModelNumber(receipt) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(sym)) + TABCHAR + fbSemanticModelNumber(sym->class) + TABCHAR + _
		fbSemanticModelNumber(parser.stmt.cnt) + TABCHAR + fbSemanticModelNumber(fbSemanticModelCurrentStatement( )) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelCurrentSource( )) + TABCHAR + fbSemanticModelNumber(fbSemanticModelCurrentContext( )))
end sub

sub fbSemanticModelFinishDeclarations( byval commit as integer )
	if( commit and fbSemanticModelFullEnabled( ) ) then
		hProperty("accepted-source-declaration-count:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )), _
			fbSemanticModelNumber(declaration_count))
	end if
	fbSemanticModelResetDeclarations( )
end sub

'' end of tooling/semantic-declarations.bas
