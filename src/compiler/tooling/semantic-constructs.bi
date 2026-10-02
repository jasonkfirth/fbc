'' Project: FreeBASIC compiler - semantic source constructs
'' File: tooling/semantic-constructs.bi
'' Purpose: Observe parser statement and compound boundaries.
'' Responsibilities: Define construct identities, owners, and fact associations.
'' This file intentionally does NOT contain: statement grammar or AST lowering.

#ifndef __FB_SEMANTIC_CONSTRUCTS_BI__
#define __FB_SEMANTIC_CONSTRUCTS_BI__

#include once "lexer/lex.bi"

declare sub fbSemanticModelResetConstructs( )
declare function fbSemanticModelStatementBegin( byref source as LEX_LOCATION, byval token as integer, byval token_class as integer ) as longint
declare sub fbSemanticModelStatementEnd( byval identity as longint, byref route as const string, byref ending as LEX_LOCATION, byval errors_before as integer )
declare function fbSemanticModelCurrentStatement( ) as longint
declare function fbSemanticModelConstructBegin( byval token as integer ) as longint
declare sub fbSemanticModelConstructEnd( byval identity as longint, byref ending as LEX_LOCATION )
declare sub fbSemanticModelAssociateStatement( byref domain as const string, byval subject as longint, byval statement as longint = 0 )
declare sub fbSemanticModelStatementOperation( byref operation as const string )

#endif

'' end of tooling/semantic-constructs.bi
