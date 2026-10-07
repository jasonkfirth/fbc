'' Project: FreeBASIC compiler - BASIC intrinsic syntax
'' -----------------------------------------
''
'' File: parser/intrinsics/parser-quirk-array.bas
''
'' Purpose:
''
''     Parse quirk array statements (ERASE, SWAP) and functions (LBOUND, UBOUND)
''     parsing.
''
'' Responsibilities:
''
''     - parse the intrinsic statement or function family described below
''     - validate arguments before building AST and runtime calls
''
'' This file intentionally does NOT contain:
''
''     - the runtime implementation of those intrinsics
''

'' quirk array statements (ERASE, SWAP) and functions (LBOUND, UBOUND) parsing
''
'' chng: sep/2004 written [v1ctor]


#include once "core/fb.bi"
#include once "core/fbint.bi"
#include once "parser/parser.bi"
#include once "runtime/rtl.bi"
#include once "ast/ast.bi"
#include once "tooling/semantic-hooks.bi"
#include once "tooling/semantic-expressions.bi"

'' EraseStmt = ERASE ID (',' ID)*
function cEraseStmt() as integer

	'' ERASE
	lexSkipToken( LEXCHECK_POST_SUFFIX )

	do
		var expr = cVarOrDeref( FB_VAREXPROPT_NOARRAYCHECK )
		if( expr = NULL ) then
			errReport( FB_ERRMSG_EXPECTEDIDENTIFIER )
			hSkipUntil( CHAR_COMMA )
		else
			'' array?
			var s = astGetSymbol( expr )
			if( s <> NULL ) then
				if( symbIsArray( s ) = FALSE ) then
					s = NULL
				end if
			end if

			if( s = NULL ) then
				errReport( FB_ERRMSG_EXPECTEDARRAY )
				hSkipUntil( CHAR_COMMA )
			else
				if( typeIsConst( astGetFullType( expr ) ) ) then
					errReport( FB_ERRMSG_CONSTANTCANTBECHANGED )
				end if

				'' ERASE frees dynamic arrays (destruct only),
				'' but re-initializes static arrays (destruct and construct).
				if( symbGetIsDynamic( s ) ) then
					astAdd( rtlArrayErase( expr, TRUE, TRUE ) )
				else
					astAdd( rtlArrayClear( expr ) )
				end if
			end if
		end if

	'' ','?
	loop while( hMatch( CHAR_COMMA ) )

	function = TRUE
end function

private function hScopedSwap( ) as integer

	'' SWAP
	lexSkipToken( LEXCHECK_POST_SUFFIX )

	var l = cVarOrDeref( FB_VAREXPROPT_ISASSIGN )
	if( l = NULL ) then
		errReport( FB_ERRMSG_EXPECTEDIDENTIFIER )
		hSkipStmt( )
		return TRUE
	end if

	if( astIsConstant( l ) ) then
		errReport( FB_ERRMSG_CONSTANTCANTBECHANGED, TRUE )
	end if

	hMatchCOMMA( )

	var r = cVarOrDeref( FB_VAREXPROPT_ISASSIGN )
	if( r = NULL ) then
		errReport( FB_ERRMSG_EXPECTEDIDENTIFIER )
		astDelTree( l )
		hSkipStmt( )
		return TRUE
	end if

	if( astIsConstant( r ) ) then
		errReport( FB_ERRMSG_CONSTANTCANTBECHANGED, TRUE )
	end if
	'' SWAP consumes both source values and replaces both targets. Observe
	'' the original lvalues before string/runtime or temporary lowering.
	fbSemanticModelSetAccess( l, "read-write" )
	fbSemanticModelSetAccess( r, "read-write" )

	dim as integer ldtype = astGetDataType( l )
	dim as integer rdtype = astGetDataType( r )

	'' Maybe UDT extends Z|WSTRING? Check for string conversions...
	if( ldtype <> rdtype ) then
		if( ldtype = FB_DATATYPE_STRUCT ) then
			var sym = astGetSubType( l )
			if( symbGetUdtIsZstring( sym ) ) then
				if( rdtype = FB_DATATYPE_CHAR ) then
					astTryOvlStringCONV( l )
					ldtype = astGetDataType( l )
				end if
			elseif( symbGetUdtIsWstring( sym ) ) then
				if( rdtype = FB_DATATYPE_WCHAR ) then
					astTryOvlStringCONV( l )
					ldtype = astGetDataType( l )
				end if
			end if
		elseif( rdtype = FB_DATATYPE_STRUCT ) then
			var sym = astGetSubType( r )
			if( symbGetUdtIsZstring( sym ) ) then
				if( ldtype = FB_DATATYPE_CHAR ) then
					astTryOvlStringCONV( r )
					rdtype = astGetDataType( r )
				end if
			elseif( symbGetUdtIsWstring( sym ) ) then
				if( ldtype = FB_DATATYPE_WCHAR ) then
					astTryOvlStringCONV( r )
					rdtype = astGetDataType( r )
				end if
			end if
		end if
	end if

	if( ldtype <> rdtype and (ldtype = FB_DATATYPE_WCHAR or rdtype = FB_DATATYPE_WCHAR) and _
	    symbIsString(ldtype) and symbIsString(rdtype) ) then
		'' Capture both lvalues before converting either operand. A Unicode
		'' temporary preserves wide scalars and makes aliased expressions safe.
		dim as ASTNODE ptr before = NULL
		if( astHasSideFx(l) ) then before = astMakeRef(l)
		if( astHasSideFx(r) ) then before = astNewLINK(before, astMakeRef(r), AST_LINK_RETURN_NONE)
		var temporary = symbAddTempVar(FB_DATATYPE_USTRING)
		astDtorListAdd(temporary)
		var body = astNewLINK(astBuildTempVarClear(temporary), _
		                     rtlStrAssign(astNewVAR(temporary), rtlToUstr(astCloneTree(l))), AST_LINK_RETURN_NONE)
		body = astNewLINK(body, rtlStrAssign(l, rtlToUstr(astCloneTree(r))), AST_LINK_RETURN_NONE)
		body = astNewLINK(body, rtlStrAssign(r, astNewVAR(temporary)), AST_LINK_RETURN_NONE)
		astAdd(astNewLINK(before, body, AST_LINK_RETURN_NONE))
		return TRUE
	end if

	select case( ldtype )
	case FB_DATATYPE_STRING, FB_DATATYPE_USTRING, FB_DATATYPE_FIXSTR, FB_DATATYPE_CHAR
		select case rdtype
		case FB_DATATYPE_STRING, FB_DATATYPE_USTRING, FB_DATATYPE_FIXSTR, FB_DATATYPE_CHAR
			function = rtlStrSwap( l, r )
		case else
			errReport( FB_ERRMSG_TYPEMISMATCH )
		end select
		exit function

	case FB_DATATYPE_WCHAR
		if( rdtype = FB_DATATYPE_WCHAR ) then
			function = rtlWstrSwap( l, r )
		else
			errReport( FB_ERRMSG_TYPEMISMATCH )
		end if
		exit function
	end select

	'' Check whether a "raw" assignment (no operator overloads) would work.
	'' Must check both l = r and r = l due to inheritance with UDTs which
	'' can allow one but not the other (and perhaps there even are other
	'' cases with similar effect).
	if( (astCheckASSIGN( l, r, TRUE ) = FALSE) or _
	    (astCheckASSIGN( r, l, TRUE ) = FALSE) ) then
		errReport( FB_ERRMSG_TYPEMISMATCH )
		exit function
	end if

	if( (ldtype = FB_DATATYPE_STRUCT) or (rdtype = FB_DATATYPE_STRUCT) ) then
		'' This should all be guaranteed by the assignment check above
		assert( ldtype = FB_DATATYPE_STRUCT )
		assert( rdtype = FB_DATATYPE_STRUCT )
		assert( astGetSubtype( l ) = astGetSubtype( r ) )
		return rtlMemSwap( l, r )
	end if

	''
	'' For the ASM backend SWAP can be done with PUSH/POP, if...
	''
	'' - it's on integers or floats (structs handled above)
	''
	'' - neither side is a bitfield (for those we always have to use a
	''   temp var, to get the bitfield accesses built properly)
	''
	'' - both side's types have the same size, otherwise we may push 4
	''   bytes and pop 8, or similar.
	''
	'' - it's either both integer or both float, so we don't swap between
	''   integer and float this way. The ASSIGN converts differently than
	''   the POP, so you'd get different results depending on whether it's
	''   <SWAP i, f> or <SWAP f, i>.
	''
	dim as integer use_pushpop = TRUE
	use_pushpop and= (env.clopt.backend = FB_BACKEND_GAS)
	use_pushpop and= (typeGetSize( ldtype ) = typeGetSize( rdtype ))
	use_pushpop and= (typeGetClass( ldtype ) = typeGetClass( rdtype ))
	use_pushpop and= (astIsBITFIELD( l ) = FALSE)
	use_pushpop and= (astIsBITFIELD( r ) = FALSE)

	'' A scope to enclose the temp vars
	dim as ASTNODE ptr t = NULL

	'' Side effects? Then use references to be able to read/write...
	if( astHasSideFx( l ) ) then
		t = astNewLINK( t, astMakeRef( l ), AST_LINK_RETURN_NONE )
	end if

	if( astHasSideFx( r ) ) then
		t = astNewLINK( t, astMakeRef( r ), AST_LINK_RETURN_NONE )
	end if

	if( use_pushpop ) then
		'' push clone( l )
		t = astNewLINK( t, astNewSTACK( AST_OP_PUSH, astCloneTree( l ) ), AST_LINK_RETURN_NONE )

		'' l = clone( r )
		t = astNewLINK( t, astNewASSIGN( l, astCloneTree( r ) ), AST_LINK_RETURN_NONE )

		'' pop r
		t = astNewLINK( t, astNewSTACK( AST_OP_POP, r ), AST_LINK_RETURN_NONE )
	else
		'' var temp = clone( l )
		var temp = symbAddTempVar( astGetFullType( l ), astGetSubtype( l ) )
		t = astNewLINK( t, astNewASSIGN( astNewVAR( temp ), astCloneTree( l ) ), AST_LINK_RETURN_NONE )

		'' l = clone( r )
		t = astNewLINK( t, astNewASSIGN( l, astCloneTree( r ) ), AST_LINK_RETURN_NONE )

		'' r = temp
		t = astNewLINK( t, astNewASSIGN( r, astNewVAR( temp ) ), AST_LINK_RETURN_NONE )
	end if

	astAdd( t )
	function = TRUE
end function

'' SwapStmt = SWAP VarOrDeref ',' VarOrDeref
function cSwapStmt( ) as integer
	dim as ASTNODE ptr scopenode = any

	'' A scope to enclose the SWAP temp vars
	'' The scope must be created before parsing the lhs/rhs expressions
	'' given to SWAP, otherwise any temporaries they use would be destroyed
	'' too early by astScopeBegin() because that flushes the AST dtor list.
	scopenode = astScopeBegin( )

	function = hScopedSwap( )

	astScopeEnd( scopenode )
end function

'':::::
''cArrayFunct =   (LBOUND|UBOUND) '(' ID (',' Expression)? ')' .
''
function cArrayFunct(byval tk as FB_TOKEN) as ASTNODE ptr
	dim as ASTNODE ptr arrayexpr = any, dimexpr = any
	dim as FBSYMBOL ptr s = any
	dim as LEX_LOCATION dimension_start = any, dimension_end = any
	dim as longint dimension_nonphysical_start = 0, dimension_nonphysical_end = 0
	dim as integer dimension_is_explicit = FALSE

	function = NULL

	select case tk
	'' (LBOUND|UBOUND) '(' ID (',' Expression)? ')'
	case FB_TK_LBOUND, FB_TK_UBOUND
		dim as LEX_LOCATION source_start = lexGetCurrentLocation( )
		dim as longint nonphysical_start = lexGetNonphysicalTokenCount( )
		lexSkipToken( LEXCHECK_POST_SUFFIX )

		'' '('
		hMatchLPRNT( )

		'' ID
		arrayexpr = cVarOrDeref( FB_VAREXPROPT_NOARRAYCHECK )
		if( arrayexpr = NULL ) then
			errReport( FB_ERRMSG_EXPECTEDIDENTIFIER )
			'' error recovery: skip until next ')' and fake an expr
			hSkipUntil( CHAR_RPRNT, TRUE )
			return astNewCONSTi( 0 )
		end if

		'' array?
		s = astGetSymbol( arrayexpr )
		if( s <> NULL ) then
			if( symbIsArray( s ) = FALSE ) then
				s = NULL
			end if
		end if

		if( s = NULL ) then
			errReport( FB_ERRMSG_EXPECTEDARRAY, TRUE )
			'' error recovery: skip until next ')' and fake an expr
			hSkipUntil( CHAR_RPRNT, TRUE )
			return astNewCONSTi( 0 )
		end if

		'' (',' Expression)?
		if( hMatch( CHAR_COMMA ) ) then
			dimension_is_explicit = TRUE
			dimension_start = lexGetCurrentLocation( )
			dimension_nonphysical_start = lexGetNonphysicalTokenCount( )
			hMatchExpressionEx( dimexpr, FB_DATATYPE_INTEGER )
			dimension_end = lexGetLastLocation( )
			dimension_nonphysical_end = lexGetNonphysicalTokenCount( )
		else
			dimexpr = astNewCONSTi( 1 )
		end if

		'' ')'
		hMatchRPRNT( )

		dim as longint original_dimension = 0
		if( dimension_is_explicit ) then
			original_dimension = fbSemanticModelOriginalExpression _
				(dimexpr, dimension_start, dimension_nonphysical_start, _
				 dimension_end, dimension_nonphysical_end)
		end if
		dim as longint selected_dimension = 0
		dim as ASTNODE ptr selected_dimension_expr = NULL
		dim as ASTNODE ptr result
		if( fbSemanticModelEnabled( ) ) then
			result = astBuildArrayBound( arrayexpr, dimexpr, tk, @selected_dimension_expr )
		else
			result = astBuildArrayBound( arrayexpr, dimexpr, tk )
		end if
		if( selected_dimension_expr <> NULL ) then
			dim as LEX_LOCATION selected_source_start = dimension_start
			dim as LEX_LOCATION selected_source_end = dimension_end
			dim as longint selected_nonphysical_start = dimension_nonphysical_start
			dim as longint selected_nonphysical_end = dimension_nonphysical_end
			if( dimension_is_explicit = FALSE ) then
				selected_source_start = source_start
				'' The default dimension is generated, not the closing parenthesis.
				'' Anchor its nonphysical type fact to the intrinsic token so it stays
				'' distinct from the completed query expression.
				selected_source_end = source_start
				selected_source_start.is_physical = FALSE
				selected_source_end.is_physical = FALSE
				selected_nonphysical_start = nonphysical_start
				selected_nonphysical_end = nonphysical_start
			end if
			selected_dimension = fbSemanticModelSelectedArrayIndex _
				(selected_dimension_expr, selected_source_start, selected_source_end, _
				 selected_nonphysical_start, selected_nonphysical_end)
			astDelTree( selected_dimension_expr )
		end if
		fbSemanticModelArrayBound(result, s, tk, original_dimension, selected_dimension, source_start, nonphysical_start)
		function = result
	end select
end function

'' end of parser/intrinsics/parser-quirk-array.bas
