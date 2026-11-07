'' Project: FreeBASIC compiler - semantic comment observations
'' File: tooling/semantic-comments.bas
'' Purpose: Export compiler-accepted physical comment observations.
'' Responsibilities: Line markers, block ranges, original coordinates and closure.
'' This file intentionally does NOT contain: a second lexer or comment text copies.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-comments.bi"
#include once "tooling/semantic-coordinates.bi"
#include once "preprocessor/pp.bi"

'' Only the module lifecycle owns this counter. Records use the exporter's
'' bounded detail identities and transaction buffer; no comment text is kept.
dim shared as longint accepted_comment_count, accepted_block_comment_count

sub fbSemanticModelResetComments( )
	accepted_comment_count = 0
	accepted_block_comment_count = 0
end sub

sub fbSemanticModelFinishComments( )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	dim as string row = "K" + TABCHAR + "symbol" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(@symbGetGlobalNamespc( ))) + _
		TABCHAR + "accepted-line-comment-count:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )) + _
		TABCHAR + fbSemanticModelNumber(accepted_comment_count)
	fbSemanticModelAppendDetail(row)
	fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(@symbGetGlobalNamespc())) + TABCHAR + _
		"accepted-block-comment-count:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity()) + TABCHAR + _
		fbSemanticModelNumber(accepted_block_comment_count))
end sub

sub fbSemanticModelAcceptComment _
	( byval token as integer, byref source as LEX_LOCATION )

	'' The compiler is serial. Detached/EVAL probes and inactive text are not
	'' accepted program comments. Replacement characters have no editable marker
	'' in the original revision; their definitions are observed while read.
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( lex.ctx->semantic_probe or pp.skipping ) then exit sub
	if( (source.raw_valid = FALSE) or (source.source_context < 1) or _
	    (source.macro_identity <> 0) ) then exit sub

	dim as string kind
	select case token
	case FB_TK_COMMENT: kind = "apostrophe"
	case FB_TK_REM: kind = "rem"
	case else
		fbSemanticModelFailAt("accepted comment has no comment marker token")
		exit sub
	end select

	dim as longint identity = fbSemanticModelNextDetailIdentity( )
	if( identity < 1 ) then exit sub
	accepted_comment_count += 1
	dim as string row = "COM" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelCurrentContext( )) + TABCHAR + kind
	fbSemanticModelAppendDetail(row)
	fbSemanticModelExportCoordinates("comment", identity, "marker", source, source)
end sub

'' Keep block comments as namespace-owned observations. Existing COM records
'' retain their line-marker contract; old line-comment policies do not acquire
'' a new marker kind. Coordinates include both native delimiters.
sub fbSemanticModelAcceptBlockComment(byref first as LEX_LOCATION, byref last as LEX_LOCATION)
	if( fbSemanticModelFullEnabled() = FALSE ) then exit sub
	if( lex.ctx->semantic_probe or pp.skipping ) then exit sub
	if( first.raw_valid = FALSE or last.raw_valid = FALSE or first.source_context < 1 or _
	    first.source_context <> last.source_context or first.macro_identity <> 0 ) then exit sub
	dim as longint identity = fbSemanticModelNextDetailIdentity()
	if( identity < 1 ) then exit sub
	dim as longint owner = fbSemanticModelSymbolId(@symbGetGlobalNamespc())
	dim as string role = "accepted-block-comment:" + fbSemanticModelNumber(identity)
	dim as string coordinates = fbSemanticModelCoordinateFact(first, last)
	if( len(coordinates) = 0 ) then
		fbSemanticModelFailAt("accepted block comment has no source coordinates"): exit sub
	end if
	fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + fbSemanticModelNumber(owner) + _
		TABCHAR + role + TABCHAR + fbSemanticModelEscape(coordinates))
	accepted_block_comment_count += 1
end sub

'' end of tooling/semantic-comments.bas
