'' Project: FreeBASIC compiler - structured semantic diagnostics
'' File: tooling/semantic-diagnostics.bas
'' Purpose: Retain diagnostics actually reported by the compiler.
'' Responsibilities: Preserve severity, code, text, source, and semantic context.
'' This file intentionally does NOT contain: diagnostic policy or console parsing.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-source.bi"
#include once "tooling/semantic-constructs.bi"
#include once "parser/parser.bi"

sub fbSemanticModelDiagnostic _
	( byref severity as const string, byval code as integer, byval line_number as integer, _
	  byval message as const zstring ptr, byval detail as const zstring ptr, byval custom_text as const zstring ptr )
	if( fbSemanticModelEnabled( ) = FALSE ) then exit sub
	dim as string text, extra, custom
	if( message <> NULL ) then text = *message
	if( detail <> NULL ) then extra = *detail
	if( custom_text <> NULL ) then custom = *custom_text
	dim as longint identity = fbSemanticModelNextDetailIdentity( )
	'' Reporting must not ask the lexer to scan another token. A diagnostic
	'' point is informational, even when its line resembles a source range.
	fbSemanticModelAppendProvenance("DI" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR + severity + _
		TABCHAR + fbSemanticModelNumber(code) + TABCHAR + fbSemanticModelEscape(text) + TABCHAR + _
		fbSemanticModelEscape(extra) + TABCHAR + fbSemanticModelEscape(custom) + TABCHAR + _
		fbSemanticModelEscape(env.inf.name) + TABCHAR + fbSemanticModelNumber(line_number) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelCurrentSource( )) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelCurrentStatement( )) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(parser.currproc)))
end sub

'' end of tooling/semantic-diagnostics.bas
