'' Project: FreeBASIC compiler - semantic sidecar
'' File: tooling/semantic-private.bi
'' Purpose: Share the exporter contract between its serialization modules.
'' Responsibilities: Expose checked staging, identities, and metadata traversal.
'' This file intentionally does NOT contain: parser or backend interfaces.

#include once "core/fbint.bi"
#include once "ast/ast.bi"
#include once "tooling/semantic-hooks.bi"
#include once "lexer/lex.bi"

declare function fbSemanticModelFullEnabled( ) as integer
declare function fbSemanticModelNumber( byval value as longint ) as string
declare function fbSemanticModelEscape( byref value as string ) as string
declare sub fbSemanticModelAppendDetail( byref value as string )
declare sub fbSemanticModelFail( )
declare function fbSemanticModelNextVisit( ) as ulongint
declare function fbSemanticModelVisitSymbol _
	( byval sym as FBSYMBOL ptr, byval visit as ulongint ) as integer
declare function fbSemanticModelSymbolOrigin( byval sym as FBSYMBOL ptr ) as string
declare function fbSemanticModelParameterVariable _
	( byval param as FBSYMBOL ptr, byval variables_live as integer ) as longint
declare function fbSemanticModelTypeKind( byval sym as FBSYMBOL ptr ) as string
declare function fbSemanticModelOperatorCode( byval op as integer ) as string
declare sub fbSemanticModelExportVariable( byval sym as FBSYMBOL ptr )
declare sub fbSemanticModelExportContext( byref filename as string )
declare sub fbSemanticModelExportInitializer _
	( byval sym as FBSYMBOL ptr, byval root as ASTNODE ptr, _
	  byref kind as const string )
declare sub fbSemanticModelExportSymbols _
	( byval head as FBSYMBOL ptr )
declare sub fbSemanticModelExportNodeDetails _
	( byval node as ASTNODE ptr, byval nodeid as longint )
declare function fbSemanticModelExportTree _
	( byval root as ASTNODE ptr, byval parentid as longint, _
	  byref next_node_id as longint, byval source_line as integer, _
	  byval filename as zstring ptr ) as longint
declare sub fbSemanticModelExportValue _
	( byref domain as const string, byval identity as longint, _
	  byval dtype as integer, byval value as FBVALUE ptr )
declare function fbSemanticModelFormatValue _
	( byval dtype as integer, byval value as FBVALUE ptr, _
	  byref value_kind as string, byref value_text as string ) as integer

'' end of tooling/semantic-private.bi
