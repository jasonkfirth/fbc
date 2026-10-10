'' Project: FreeBASIC compiler - semantic comment observations
'' File: tooling/semantic-comments.bi
'' Purpose: Preserve comment markers accepted by the parser or preprocessor.
'' Responsibilities: Define the serial, non-owning observation interface.
'' This file intentionally does NOT contain: comment parsing or lint policy.

#ifndef __FB_SEMANTIC_COMMENTS_BI__
#define __FB_SEMANTIC_COMMENTS_BI__

#include once "lexer/lex.bi"

declare sub fbSemanticModelAcceptComment _
	( byval token as integer, byref source as LEX_LOCATION )
declare sub fbSemanticModelResetComments( )
declare sub fbSemanticModelFinishComments( )

#endif

'' end of tooling/semantic-comments.bi
