'' Project: FreeBASIC compiler - EXTERN policy observations
'' File: tooling/semantic-extern.bas
'' Purpose: Preserve actual source mangling scope transitions.
'' Responsibilities: Copy selections, native fallback outcomes and restoration order.
'' This file intentionally does NOT contain: EXTERN parsing or symbol spelling decisions.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-extern.bi"
#include once "tooling/semantic-constructs.bi"
#include once "tooling/semantic-source.bi"
#include once "parser/parser.bi"

'' The parser executes serially. Frames contain scalar observation identities,
'' never pointers into its compound stack. An unterminated native scope remains
'' open in the receipt; publication must not invent an END EXTERN for recovery.
private const EXTERN_DEPTH_LIMIT = 1024
private const EXTERN_EVENT_LIMIT = 1000000
private type SEMANTIC_EXTERN_FRAME
	construct as longint
	previous as integer
	selected as integer
end type
private dim shared as SEMANTIC_EXTERN_FRAME extern_frames(0 to EXTERN_DEPTH_LIMIT - 1)
private dim shared as integer extern_depth
private dim shared as longint extern_events, extern_begins, extern_ends

function fbSemanticModelExternEnabled( ) as integer
	if( fbSemanticModelTypedExpressionsEnabled( ) = FALSE or lex.ctx = NULL ) then return FALSE
	return (lex.ctx->semantic_probe = FALSE) and (fbGetIsPP( ) = FALSE)
end function

sub fbSemanticModelResetExtern( )
	extern_depth = 0
	extern_events = 0
	extern_begins = 0
	extern_ends = 0
end sub

private sub hProperty( byref key as const string, byref value as const string )
	fbSemanticModelAppendProvenance("K" + TABCHAR + "module" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelModuleIdentity( )) + TABCHAR + key + TABCHAR + fbSemanticModelEscape(value))
end sub

sub fbSemanticModelExternTransition _
	( byref kind as const string, byval construct as longint, byval previous as integer, _
	  byval entry_context as longint, byref source as LEX_LOCATION, _
	  byref spelling as const string, byval present as integer, byval recovered as integer )
	if( fbSemanticModelExternEnabled( ) = FALSE ) then exit sub
	if( construct <= 0 or entry_context < 0 or (entry_context = 0 and fbSemanticModelFullEnabled( )) or _
		extern_events >= EXTERN_EVENT_LIMIT or _
		previous < FB_MANGLING_BASIC or previous > FB_MANGLING_RTLIB or _
		parser.mangling < FB_MANGLING_BASIC or parser.mangling > FB_MANGLING_RTLIB or _
		len(spelling) > FB_MAXLITLEN ) then
		fbSemanticModelFailAt("invalid native EXTERN transition")
		exit sub
	end if
	select case kind
	case "begin"
		if( extern_depth >= EXTERN_DEPTH_LIMIT or (present = FALSE and len(spelling) > 0) ) then
			fbSemanticModelFailAt("native EXTERN scope or spelling limit")
			exit sub
		end if
		with extern_frames(extern_depth)
			.construct = construct
			.previous = previous
			.selected = parser.mangling
		end with
		extern_depth += 1
		extern_begins += 1
	case "end"
		if( extern_depth = 0 or present or recovered or len(spelling) > 0 ) then
			fbSemanticModelFailAt("native EXTERN restoration has no scope")
			exit sub
		end if
		with extern_frames(extern_depth - 1)
			if( .construct <> construct or .selected <> previous or .previous <> parser.mangling ) then
				fbSemanticModelFailAt("native EXTERN restoration disagrees with its scope")
				exit sub
			end if
		end with
		extern_depth -= 1
		extern_ends += 1
	case else
		fbSemanticModelFailAt("unknown native EXTERN transition")
		exit sub
	end select
	extern_events += 1
	dim as longint macro_identity, macro_start
	dim as integer macro_valid
	'' Compact output keeps internal expansion frames for truthful anchors,
	'' but does not publish their MI identities. Do not leak those private IDs.
	if( fbSemanticModelMacroProvenanceEnabled( ) and source.macro_identity > 0 ) then
		macro_identity = source.macro_identity
		macro_start = source.macro_start_unit
		macro_valid = abs(source.macro_unit_valid <> FALSE)
	end if
	dim as string value = kind + TABCHAR + fbSemanticModelNumber(construct) + TABCHAR + _
		fbSemanticModelNumber(previous) + TABCHAR + fbSemanticModelNumber(parser.mangling) + TABCHAR + _
		fbSemanticModelNumber(entry_context) + TABCHAR + fbSemanticModelNumber(fbSemanticModelCurrentContext( )) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelCurrentStatement( )) + TABCHAR + fbSemanticModelNumber(fbSemanticModelCurrentSource( )) + TABCHAR + _
		fbSemanticModelNumber(abs(present <> FALSE)) + TABCHAR + fbSemanticModelNumber(abs(recovered <> FALSE)) + TABCHAR + _
		fbSemanticModelEscape(spelling) + TABCHAR + fbSemanticModelEscape(source.source_file) + TABCHAR + _
		fbSemanticModelNumber(source.start_line) + TABCHAR + fbSemanticModelNumber(source.start_column) + TABCHAR + _
		fbSemanticModelNumber(source.end_line) + TABCHAR + fbSemanticModelNumber(source.end_column) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelLocationIsPhysical(source)) + TABCHAR + fbSemanticModelNumber(macro_identity) + TABCHAR + _
		fbSemanticModelNumber(macro_start) + TABCHAR + fbSemanticModelNumber(macro_valid)
	hProperty("extern-mangling:" + fbSemanticModelNumber(extern_events), value)
end sub

sub fbSemanticModelFinishExtern( byval commit as integer )
	if( commit and fbSemanticModelTypedExpressionsEnabled( ) ) then
		hProperty("extern-mangling-counts", fbSemanticModelNumber(extern_events) + TABCHAR + _
			fbSemanticModelNumber(extern_begins) + TABCHAR + fbSemanticModelNumber(extern_ends) + TABCHAR + fbSemanticModelNumber(extern_depth))
	end if
	fbSemanticModelResetExtern( )
end sub

'' end of tooling/semantic-extern.bas
