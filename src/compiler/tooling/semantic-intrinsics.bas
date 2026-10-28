'' Project: FreeBASIC compiler - semantic intrinsic dispatch
'' File: tooling/semantic-intrinsics.bas
'' Purpose: Preserve native grammar selections before intrinsic lowering erases them.
'' Responsibilities: Copy parser contexts, return outcomes and bounded root snapshots.
'' This file intentionally does NOT contain: intrinsic resolution or runtime effect inference.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-intrinsics.bi"
#include once "tooling/semantic-constructs.bi"
#include once "tooling/semantic-source.bi"
#include once "diagnostics/error.bi"
#include once "parser/parser.bi"

'' Parser callbacks run serially. Completion order is the observation order;
'' a nested intrinsic can finish before the enclosing grammar returns. No
'' node pointer survives this callback, and no expression is evaluated here.
private const INTRINSIC_DISPATCH_LIMIT = 100000
private dim shared as longint intrinsic_dispatch_count

sub fbSemanticModelResetIntrinsics( )
	intrinsic_dispatch_count = 0
end sub

private sub hIntrinsicProperty( byref key as const string, byref value as const string )
	fbSemanticModelAppendProvenance("K" + TABCHAR + "module" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelModuleIdentity( )) + TABCHAR + key + TABCHAR + fbSemanticModelEscape(value))
end sub

sub fbSemanticModelIntrinsicBegin( byref site as SEMANTIC_INTRINSIC_SITE )
	site.enabled = FALSE
	if( fbSemanticModelEnabled( ) = FALSE ) then exit sub
	if( lex.ctx = NULL ) then exit sub
	if( lex.ctx->semantic_probe or fbGetIsPP( ) ) then exit sub
	if( lex.ctx->head = NULL ) then
		fbSemanticModelFailAt("intrinsic observation has no lexer token")
		exit sub
	end if
	site.errors_before = errGetCount( )
	site.statement = fbSemanticModelCurrentStatement( )
	site.configuration = fbSemanticModelCurrentContext( )
	'' Reading the already-fetched token does not create identifier hints or
	'' perform extra lexer queries while the native grammar is being observed.
	site.source = lex.ctx->head->source
	site.enabled = TRUE
end sub

private function hIntrinsicPoint( byref source as LEX_LOCATION ) as string
	dim as integer physical = source.is_physical and source.raw_valid and _
		(source.raw_start_line > 0) and (source.raw_start_column >= 0)
	return fbSemanticModelEscape(source.source_file) + TABCHAR + fbSemanticModelNumber(source.source_context) + TABCHAR + _
		fbSemanticModelNumber(source.start_line) + TABCHAR + fbSemanticModelNumber(source.start_column) + TABCHAR + _
		fbSemanticModelNumber(iif(physical, source.raw_start_line, 0)) + TABCHAR + _
		fbSemanticModelNumber(iif(physical, source.raw_start_column, 0)) + TABCHAR + fbSemanticModelNumber(abs(physical))
end function

sub fbSemanticModelIntrinsicEnd( byref site as SEMANTIC_INTRINSIC_SITE, _
	byref kind as const string, byref route as const string, byval token as integer, _
	byval accepted as integer, byval node as ASTNODE ptr )
	if( site.enabled = FALSE ) then exit sub
	'' Unknown tokens can pass through the generic dispatchers. Only an
	'' actual switch arm names a route; fallthrough is not a selected builtin.
	if( len(route) = 0 ) then exit sub
	if( fbSemanticModelEnabled( ) = FALSE ) then exit sub
	if( (intrinsic_dispatch_count >= INTRINSIC_DISPATCH_LIMIT) or _
		(token < 0) or (token > FB_TK_THREAD) or (site.errors_before < 0) or _
		(errGetCount( ) < site.errors_before) ) then
		fbSemanticModelFailAt("invalid native intrinsic dispatch observation")
		exit sub
	end if
	if( (kind <> "statement") and (kind <> "function") ) then
		fbSemanticModelFailAt("unknown native intrinsic dispatch kind")
		exit sub
	end if
	if( (kind = "statement" and node <> NULL) or _
		(kind = "function" and ((node <> NULL) <> (accepted <> FALSE))) ) then
		fbSemanticModelFailAt("contradictory native intrinsic dispatch result")
		exit sub
	end if
	dim as integer root_class = 0
	dim as longint dtype = FB_DATATYPE_INVALID
	if( node <> NULL ) then
		root_class = node->class
		dtype = culng(node->dtype)
		if( (root_class < 0) or (root_class >= AST_CLASSES) ) then
			fbSemanticModelFailAt("invalid native intrinsic result class")
			exit sub
		end if
	end if
	dim as string outcome = "declined"
	if( accepted ) then outcome = "accepted"
	if( errGetCount( ) <> site.errors_before ) then outcome = "recovered"
	dim as LEX_LOCATION ending = lexGetLastLocation( )
	intrinsic_dispatch_count += 1
	hIntrinsicProperty("native-intrinsic-dispatch:" + fbSemanticModelNumber(intrinsic_dispatch_count), _
		kind + TABCHAR + route + TABCHAR + fbSemanticModelNumber(token) + TABCHAR + _
		fbSemanticModelNumber(abs(accepted <> FALSE)) + TABCHAR + fbSemanticModelNumber(site.errors_before) + TABCHAR + _
		fbSemanticModelNumber(errGetCount( )) + TABCHAR + outcome + TABCHAR + fbSemanticModelNumber(site.statement) + TABCHAR + _
		fbSemanticModelNumber(site.configuration) + TABCHAR + fbSemanticModelNumber(abs(node <> NULL)) + TABCHAR + _
		fbSemanticModelNumber(root_class) + TABCHAR + fbSemanticModelTypeNumber(dtype) + TABCHAR + _
		hIntrinsicPoint(site.source) + TABCHAR + hIntrinsicPoint(ending))
end sub

sub fbSemanticModelFinishIntrinsics( byval commit as integer )
	if( commit and fbSemanticModelEnabled( ) ) then
		hIntrinsicProperty("native-intrinsic-dispatch-count", fbSemanticModelNumber(intrinsic_dispatch_count))
	end if
end sub

'' end of tooling/semantic-intrinsics.bas
