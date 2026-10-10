'' Project: FreeBASIC compiler - semantic comment observations
'' File: tooling/semantic-comments.bas
'' Purpose: Export compiler-accepted physical line-comment markers.
'' Responsibilities: Checked identities, original coordinates and context links.
'' This file intentionally does NOT contain: a second lexer or comment text copies.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-comments.bi"
#include once "tooling/semantic-coordinates.bi"
#include once "preprocessor/pp.bi"

'' Only the module lifecycle owns this counter. Records use the exporter's
'' bounded detail identities and transaction buffer; no comment text is kept.
dim shared as longint accepted_comment_count

sub fbSemanticModelResetComments( )
	accepted_comment_count = 0
end sub

sub fbSemanticModelFinishComments( )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	dim as string row = "K" + TABCHAR + "symbol" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(@symbGetGlobalNamespc( ))) + _
		TABCHAR + "accepted-line-comment-count:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )) + _
		TABCHAR + fbSemanticModelNumber(accepted_comment_count)
	fbSemanticModelAppendDetail(row)
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

'' end of tooling/semantic-comments.bas
