'' Project: FreeBASIC compiler - pragma history
'' File: tooling/semantic-pragmas.bas
'' Purpose: Preserve actual pragma stack and option/callback decisions.
'' Responsibilities: Bounded nested scopes, copied actions and terminal coverage.
'' This file intentionally does NOT contain: pragma parsing or option setters.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-pragmas.bi"
#include once "tooling/semantic-constructs.bi"
#include once "tooling/semantic-source.bi"
#include once "parser/parser.bi"

'' Compilation is serial. Frames contain only observation IDs and selected
'' indices; strings are copied into the writer before lexer storage changes.
private const PRAGMA_EVENT_LIMIT = 1000000
private const PRAGMA_SCOPE_LIMIT = 1024
private type SEMANTIC_PRAGMA_FRAME
	identity as longint
	option_index as integer
end type
private dim shared as SEMANTIC_PRAGMA_FRAME pragma_frames(0 to PRAGMA_SCOPE_LIMIT - 1)
private dim shared as integer pragma_depth
private dim shared as longint pragma_events, pragma_begins, pragma_ends

private function hEnabled( ) as integer
	if( fbSemanticModelTypedExpressionsEnabled( ) = FALSE or lex.ctx = NULL ) then return FALSE
	return lex.ctx->semantic_probe = FALSE
end function

sub fbSemanticModelResetPragmas( )
	pragma_depth = 0
	pragma_events = 0
	pragma_begins = 0
	pragma_ends = 0
end sub

private sub hProperty( byref key as const string, byref value as const string )
	fbSemanticModelAppendProvenance("K" + TABCHAR + "module" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelModuleIdentity( )) + TABCHAR + key + TABCHAR + fbSemanticModelEscape(value))
end sub

private sub hEvent( byref kind as const string, byref value as const string )
	if( pragma_depth = 0 or pragma_events >= PRAGMA_EVENT_LIMIT ) then
		fbSemanticModelFailAt("pragma event has no scope or exceeds its bound")
		exit sub
	end if
	pragma_events += 1
	hProperty("native-pragma:" + fbSemanticModelNumber(pragma_events), kind + TABCHAR + _
		fbSemanticModelNumber(pragma_frames(pragma_depth - 1).identity) + TABCHAR + value)
end sub

function fbSemanticModelPragmaBegin( byref requested as const string ) as longint
	if( hEnabled( ) = FALSE ) then return 0
	if( pragma_depth >= PRAGMA_SCOPE_LIMIT or len(requested) > FB_MAXLITLEN ) then
		fbSemanticModelFailAt("pragma scope or spelling limit")
		return 0
	end if
	dim as longint parent, macro_identity, macro_start
	dim as integer macro_valid
	if( pragma_depth ) then parent = pragma_frames(pragma_depth - 1).identity
	pragma_begins += 1
	dim as longint identity = pragma_begins
	pragma_frames(pragma_depth).identity = identity
	pragma_frames(pragma_depth).option_index = -1
	pragma_depth += 1
	dim as LEX_LOCATION source = lex.ctx->head->source
	if( fbSemanticModelMacroProvenanceEnabled( ) and source.macro_identity > 0 ) then
		macro_identity = source.macro_identity
		macro_start = source.macro_start_unit
		macro_valid = abs(source.macro_unit_valid <> FALSE)
	end if
	hEvent("begin", fbSemanticModelNumber(parent) + TABCHAR + fbSemanticModelEscape(requested) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelCurrentStatement( )) + TABCHAR + fbSemanticModelNumber(fbSemanticModelCurrentContext( )) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelCurrentSource( )) + TABCHAR + fbSemanticModelEscape(source.source_file) + TABCHAR + _
		fbSemanticModelNumber(source.start_line) + TABCHAR + fbSemanticModelNumber(source.start_column) + TABCHAR + _
		fbSemanticModelNumber(source.end_line) + TABCHAR + fbSemanticModelNumber(source.end_column) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelLocationIsPhysical(source)) + TABCHAR + fbSemanticModelNumber(macro_identity) + TABCHAR + _
		fbSemanticModelNumber(macro_start) + TABCHAR + fbSemanticModelNumber(macro_valid))
	return identity
end function

sub fbSemanticModelPragmaSelect( byval option_index as integer, byval group as integer, byval option_code as integer, byval flags as integer, byref mode as const string, byref spelling as const string )
	if( hEnabled( ) = FALSE ) then exit sub
	if( pragma_depth = 0 or option_index < 0 or option_index > 6 or group < 0 or group > 3 or flags < 0 or flags > 31 ) then
		fbSemanticModelFailAt("invalid native pragma selection"): exit sub
	end if
	if( pragma_frames(pragma_depth - 1).option_index <> -1 or (mode <> "set" and mode <> "push" and mode <> "pop") ) then
		fbSemanticModelFailAt("repeated or invalid pragma selection"): exit sub
	end if
	pragma_frames(pragma_depth - 1).option_index = option_index
	hEvent("select", fbSemanticModelNumber(option_index) + TABCHAR + fbSemanticModelNumber(group) + TABCHAR + _
		fbSemanticModelNumber(option_code) + TABCHAR + fbSemanticModelNumber(flags) + TABCHAR + mode + TABCHAR + fbSemanticModelEscape(spelling))
end sub

private function hSelected( byval option_index as integer ) as integer
	if( pragma_depth > 0 ) then return pragma_frames(pragma_depth - 1).option_index = option_index
	return FALSE
end function

sub fbSemanticModelPragmaUnknown( byref spelling as const string )
	if( hEnabled( ) = FALSE ) then exit sub
	if( hSelected(-1) = FALSE ) then fbSemanticModelFailAt("unknown pragma already has a selection"): exit sub
	hEvent("unknown", fbSemanticModelEscape(spelling))
end sub

sub fbSemanticModelPragmaStack( byref kind as const string, byval option_index as integer, byval before as integer, byval after as integer, byval accepted as integer, byval value as longint )
	if( hEnabled( ) = FALSE ) then exit sub
	if( hSelected(option_index) = FALSE or before < 0 or before > FB_MAXPRAGMARECLEVEL or after < 0 or after > FB_MAXPRAGMARECLEVEL or (kind <> "push" and kind <> "pop") ) then
		fbSemanticModelFailAt("invalid native pragma stack action"): exit sub
	end if
	hEvent(kind, fbSemanticModelNumber(option_index) + TABCHAR + fbSemanticModelNumber(before) + TABCHAR + _
		fbSemanticModelNumber(after) + TABCHAR + fbSemanticModelNumber(abs(accepted <> FALSE)) + TABCHAR + fbSemanticModelNumber(value))
end sub

sub fbSemanticModelPragmaApply( byval option_index as integer, byval requested as longint, byval supplied as longint, byval before as longint, byval after as longint, byval entry_context as longint )
	if( hEnabled( ) = FALSE ) then exit sub
	if( hSelected(option_index) = FALSE ) then fbSemanticModelFailAt("pragma application has no native selection"): exit sub
	hEvent("apply", fbSemanticModelNumber(option_index) + TABCHAR + fbSemanticModelNumber(requested) + TABCHAR + _
		fbSemanticModelNumber(supplied) + TABCHAR + fbSemanticModelNumber(before) + TABCHAR + fbSemanticModelNumber(after) + TABCHAR + _
		fbSemanticModelNumber(entry_context) + TABCHAR + fbSemanticModelNumber(fbSemanticModelCurrentContext( )))
end sub

sub fbSemanticModelPragmaCallback( byval option_index as integer, byref kind as const string )
	if( hEnabled( ) = FALSE ) then exit sub
	if( hSelected(option_index) = FALSE or (kind <> "once" and kind <> "reserve") ) then fbSemanticModelFailAt("invalid native pragma callback"): exit sub
	hEvent("callback", fbSemanticModelNumber(option_index) + TABCHAR + kind)
end sub

sub fbSemanticModelPragmaReserve( byref role as const string, byval spelling as const zstring ptr, byval accepted as integer, byval sym as FBSYMBOL ptr )
	if( hEnabled( ) = FALSE ) then exit sub
	if( hSelected(3) = FALSE or spelling = NULL or (role <> "asm" and role <> "extern" and role <> "symbol") ) then fbSemanticModelFailAt("invalid pragma reservation"): exit sub
	dim as longint symbol_id
	if( sym <> NULL ) then
		if( accepted = FALSE or role <> "symbol" or sym->class <> FB_SYMBCLASS_RESERVED ) then fbSemanticModelFailAt("pragma reservation has another symbol class"): exit sub
		if( fbSemanticModelFullEnabled( ) ) then symbol_id = fbSemanticModelSymbolId(sym)
	end if
	hEvent("reserve", role + TABCHAR + fbSemanticModelEscape(*spelling) + TABCHAR + _
		fbSemanticModelNumber(abs(accepted <> FALSE)) + TABCHAR + fbSemanticModelNumber(symbol_id))
end sub

sub fbSemanticModelPragmaEnd( byval identity as longint, byref reason as const string, byval errors_before as integer, byval errors_after as integer )
	if( identity = 0 or hEnabled( ) = FALSE ) then exit sub
	if( pragma_depth = 0 ) then fbSemanticModelFailAt("pragma close has no scope"): exit sub
	if( pragma_frames(pragma_depth - 1).identity <> identity ) then fbSemanticModelFailAt("pragma close has another scope"): exit sub
	hEvent("end", reason + TABCHAR + fbSemanticModelNumber(errors_before) + TABCHAR + fbSemanticModelNumber(errors_after) + TABCHAR + fbSemanticModelNumber(fbSemanticModelCurrentContext( )))
	pragma_depth -= 1
	pragma_ends += 1
end sub

sub fbSemanticModelFinishPragmas( byval commit as integer )
	if( commit and fbSemanticModelTypedExpressionsEnabled( ) ) then
		hProperty("native-pragma-counts", fbSemanticModelNumber(pragma_events) + TABCHAR + fbSemanticModelNumber(pragma_begins) + TABCHAR + _
			fbSemanticModelNumber(pragma_ends) + TABCHAR + fbSemanticModelNumber(pragma_depth) + TABCHAR + fbSemanticModelNumber(FB_MAXPRAGMARECLEVEL))
	end if
	fbSemanticModelResetPragmas( )
end sub

'' end of tooling/semantic-pragmas.bas
