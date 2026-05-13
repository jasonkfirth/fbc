'' FreeBASIC compiler parser helpers
''
'' File: parser-helper.bas
''
'' Purpose:
''   Shared parser helper routines used by multiple parser modules.
''
'' Responsibilities:
''   - common token skipping and error recovery helpers
''   - shared declaration and type-use validation helpers
''   - common array-bound helper code used by variable and type declarations
''
'' This file intentionally does NOT contain:
''   - grammar-specific parser entry points
''   - parser context initialization
''   - lexer or AST ownership logic
''

#include once "fb.bi"
#include once "fbint.bi"
#include once "parser.bi"
#include once "ast.bi"

'':::::
sub hSkipUntil _
	( _
		byval token as integer, _
		byval doeat as integer, _
		byval flags as LEXCHECK, _
		byval stop_on_comma as integer _
	)

	dim as integer prntcnt

	prntcnt = 0
	do
		select case as const lexGetToken( flags )
		case FB_TK_EOL
			exit do

		case FB_TK_STMTSEP, FB_TK_COMMENT, FB_TK_REM
			'' anything but EOL? exit..
			if( token <> FB_TK_EOL ) then
				exit do
			end if

		case FB_TK_EOF
			exit sub

		'' '('?
		case CHAR_LPRNT
			if( token = CHAR_LPRNT ) then
				exit do
			end if

			prntcnt += 1

		case CHAR_LBRACE
			prntcnt += 1

		'' ')'?
		case CHAR_RPRNT
			'' inside parentheses?
			if( prntcnt > 0 ) then
				prntcnt -= 1
			else
				if( token = CHAR_RPRNT ) then
					exit do
				end if
			end if

		case CHAR_RBRACE
			'' inside braces?
			if( prntcnt > 0 ) then
				prntcnt -= 1
			else
				if( token = CHAR_RBRACE ) then
					exit do
				end if
			end if

		'' ','?
		case CHAR_COMMA
			'' skip until ','?
			if( (token = CHAR_COMMA) or stop_on_comma ) then
				'' not inside parentheses?
				if( prntcnt = 0 ) then
					exit do
				end if
			end if

		case else
			'' token found? exit..
			if( lexGetToken( flags ) = token ) then
				exit do
			end if

		end select

		lexSkipToken( flags )
	loop

	'' skip token?
	if( doeat ) then
		'' same?
		if( token = lexGetToken( flags ) ) then
			lexSkipToken( flags )
		end if
	end if

end sub

'':::::
sub hSkipCompound _
	( _
		byval for_token as integer, _
		byval until_token as integer, _
		byval flags as LEXCHECK _
	)

	dim as integer cnt, iscomment

	if( until_token = INVALID ) then
		until_token = for_token
	end if

	cnt = 0
	iscomment = FALSE
	do
		select case lexGetToken( flags )
		case FB_TK_EOF
			exit sub

		case FB_TK_EOL
			iscomment = FALSE

		case FB_TK_COMMENT, FB_TK_REM
			iscomment = TRUE

		case FB_TK_END
			if( iscomment = FALSE ) then
				if( lexGetLookAhead( 1, flags ) = until_token ) then
					lexSkipToken( flags )

					if( cnt > 0 ) then
						cnt -= 1
					end if

					if( cnt = 0 ) then
						exit do
					end if
				end if
			end if

		case for_token
			if( iscomment = FALSE ) then
				cnt += 1
			end if

		end select

		lexSkipToken( flags )
	loop

	lexSkipToken( flags )

end sub

'':::::
function hMatchExpr _
	( _
		byval dtype as integer _
	) as ASTNODE ptr

	dim as ASTNODE ptr expr

	expr = cExpression( )
	if( expr = NULL ) then
		errReport( FB_ERRMSG_EXPECTEDEXPRESSION )
		'' error recovery: fake an expr
		if( dtype = FB_DATATYPE_INVALID ) then
			return NULL
		end if

		expr = astNewCONSTz( dtype )
	end if

	function = expr

end function

'':::::
sub hDisallowStaticAttrib( byref attrib as FB_SYMBATTRIB, byref pattrib as FB_PROCATTRIB )
	if( (attrib and FB_SYMBATTRIB_STATIC) <> 0 ) then
		errReport( FB_ERRMSG_MEMBERCANTBESTATIC )
		attrib and= not FB_SYMBATTRIB_STATIC
	end if
end sub

'':::::
sub hDisallowVirtualCtor( byref attrib as FB_SYMBATTRIB, byref pattrib as FB_PROCATTRIB )
	'' Constructors cannot be virtual (they initialize the vptr
	'' needed for virtual calls, chicken-egg problem)
	if( pattrib and (FB_PROCATTRIB_ABSTRACT or FB_PROCATTRIB_VIRTUAL) ) then
		if( pattrib and FB_PROCATTRIB_ABSTRACT ) then
			errReport( FB_ERRMSG_ABSTRACTCTOR )
		else
			errReport( FB_ERRMSG_VIRTUALCTOR )
		end if
		pattrib and= not (FB_PROCATTRIB_ABSTRACT or FB_ERRMSG_VIRTUALCTOR)
	end if
end sub

'':::::
sub hDisallowAbstractDtor( byref attrib as FB_SYMBATTRIB, byref pattrib as FB_PROCATTRIB )
	'' Destructors cannot be abstract; they need to have a body to ensure
	'' that base and field destructors are called.
	if( pattrib and FB_PROCATTRIB_ABSTRACT ) then
		errReport( FB_ERRMSG_ABSTRACTDTOR )
		pattrib and= not FB_PROCATTRIB_ABSTRACT
	end if
end sub

'':::::
sub hDisallowConstCtorDtor( byval tk as integer, byref attrib as FB_SYMBATTRIB, byref pattrib as FB_PROCATTRIB )
	'' It doesn't make sense for ctors/dtors to be CONST. It's a ctor's
	'' purpose to initialize an object and it couldn't do that if it used
	'' a CONST This. And as for dtors, they need to be able to destroy all
	'' objects, CONST or not. It doesn't matter whether the dtor modifies
	'' the object in the process since it's dead afterwards anyways.
	if( attrib and FB_SYMBATTRIB_CONST ) then
		errReport( iif( tk = FB_TK_CONSTRUCTOR, _
			FB_ERRMSG_CONSTCTOR, FB_ERRMSG_CONSTDTOR ) )
		attrib and= not FB_SYMBATTRIB_CONST
	end if
end sub

'':::::
sub hComplainIfAbstractClass _
	( _
		byval dtype as integer, _
		byval subtype as FBSYMBOL ptr _
	)

	if( typeGetDtAndPtrOnly( dtype ) = FB_DATATYPE_STRUCT ) then
		if( symbCompGetAbstractCount( subtype ) > 0 ) then
			errReport( FB_ERRMSG_OBJECTOFABSTRACTCLASS )
		end if
	end if

end sub

'':::::
sub hMaybeComplainTypeUsage _
	( _
		byref dtype as integer, _
		byref subtype as FBSYMBOL ptr, _
		byref lgt as longint _
	)

	'' check access to structs and if the check fails, fake a
	'' a type for error recovery

	select case typeGetDtAndPtrOnly( dtype )
	case FB_DATATYPE_STRUCT

		'' Check visibility of the symbol type
		if( symbCheckAccessStruct( subtype ) = FALSE ) then
			errReport( FB_ERRMSG_ILLEGALMEMBERACCESS )
			'' error recovery: fake a type
			dtype = FB_DATATYPE_INTEGER
			subtype = NULL
			lgt = typeGetSize( dtype )
		end if

	end select

end sub

'':::::
sub hComplainAboutConstDynamicArray( byval sym as FBSYMBOL ptr )
	'' Disallow const dynamic arrays, they could never be assigned,
	'' since dynamic arrays aren't allowed to have initializers.
	if( typeIsConst( symbGetFullType( sym ) ) ) then
		errReport( FB_ERRMSG_DYNAMICARRAYSCANTBECONST )
	end if
end sub

'':::::
sub hSymbolType _
	( _
		byref dtype as integer, _
		byref subtype as FBSYMBOL ptr, _
		byref lgt as longint, _
		byval is_byref as integer, _
		byval is_extends as integer _
	)

	dim as integer options = FB_SYMBTYPEOPT_DEFAULT
	if( is_byref ) then
		options and= not FB_SYMBTYPEOPT_CHECKSTRPTR
		options or= FB_SYMBTYPEOPT_ISBYREF
	end if
	if( is_extends ) then
		options and= not FB_SYMBTYPEOPT_CHECKSTRPTR
	end if

	'' parse the symbol type (INTEGER, STRING, etc...)
	if( cSymbolType( dtype, subtype, lgt, , options ) = FALSE ) then
		errReport( FB_ERRMSG_EXPECTEDIDENTIFIER )
		'' error recovery: fake a type
		dtype = FB_DATATYPE_INTEGER
		subtype = NULL
		lgt = typeGetSize( dtype )
	end if

	'' ANY?
	if( dtype = FB_DATATYPE_VOID ) then
		errReport( FB_ERRMSG_INVALIDDATATYPES )
		'' error recovery: fake a type
		dtype = typeAddrOf( dtype )
		subtype = NULL
		lgt = typeGetSize( dtype )
	end if

	'' ANY alias "modifier" but no pointer level?
	if( typeHasMangleDt( dtype ) and (typeGetDtAndPtrOnly( dtype ) = FB_DATATYPE_VOID) ) then
		errReport( FB_ERRMSG_INVALIDDATATYPES )
		'' error recovery: fake a type
		dtype = typeAddrOf( dtype )
		subtype = NULL
		lgt = typeGetSize( dtype )
	end if

end sub

'':::::
function hCheckScope() as integer
	if( parser.scope > FB_MAINSCOPE ) then
		if( fbIsModLevel( ) = FALSE ) then
			errReport( FB_ERRMSG_ILLEGALINSIDEASUB )
		else
			errReport( FB_ERRMSG_ILLEGALINSIDEASCOPE )
		end if
		function = FALSE
	else
		function = TRUE
	end if
end function

'':::::
private function hExprTbIsConst _
	( _
		byval dimensions as integer, _
		exprTB() as ASTNODE ptr _
	) as integer

	for i as integer = 0 to dimensions-1
		if( astIsCONST( exprTB(i, 0) ) = FALSE ) then
			return FALSE
		elseif( exprTB(i, 1) = NULL ) then
			'' do nothing, allow NULL expression here for ellipsis
		elseif( astIsCONST( exprTB(i, 1) ) = FALSE ) then
			return FALSE
		end if
	next

	function = TRUE
end function

'':::::
private sub hMakeArrayDimTB _
	( _
		byval dimensions as integer, _
		exprTB() as ASTNODE ptr, _
		dTB() as FBARRAYDIM _
	)

	for i as integer = 0 to dimensions-1
		dim as ASTNODE ptr expr = any

		'' lower bound
		dTB(i).lower = astConstFlushToInt( exprTB(i, 0) )

		'' upper bound
		expr = exprTB(i, 1)
		if( expr = NULL ) then
			'' if a null expr is found, that means it was an ellipsis for the
			'' upper bound, so we set a special upper value, and CONTINUE in
			'' order to skip the check
			dTB(i).upper = FB_ARRAYDIM_UNKNOWN
		else
			dTB(i).upper = astConstFlushToInt( expr )

			'' Besides the upper < lower case, also complain about FB_ARRAYDIM_UNKNOWN being
			'' specified, otherwise we'd think ellipsis was given...
			if( (dTB(i).upper < dTB(i).lower) or (dTB(i).upper = FB_ARRAYDIM_UNKNOWN) ) then
				errReport( FB_ERRMSG_INVALIDSUBSCRIPT )
				dTB(i).lower = 0
				dTB(i).upper = 0
			end if
		end if
	next

end sub

'':::::
sub hMaybeConvertExprTb2DimTb _
	( _
		byref attrib as FB_SYMBATTRIB, _
		byval dimensions as integer, _
		exprTB() as ASTNODE ptr, _
		dTB() as FBARRAYDIM _
	)

	'' if subscripts are constants, convert exprTB to dimTB
	if( hExprTbIsConst( dimensions, exprTB() ) ) then
		'' only if not explicitly dynamic (ie: not REDIM, COMMON)
		if( (attrib and FB_SYMBATTRIB_DYNAMIC) = 0 ) then
			hMakeArrayDimTB( dimensions, exprTB(), dTB() )
		end if
	else
		'' Non-constant array bounds, must be dynamic
		attrib or= FB_SYMBATTRIB_DYNAMIC
	end if

end sub

'':::::
sub hComplainAboutEllipsis _
	( _
		byval dimensions as integer, _
		exprTB() as ASTNODE ptr, _
		byval errmsg as integer _
	)

	'' Disallow ellipsis dimensions (nicer than "ellipsis requires initializer" +
	'' "cannot initialize dynamic array" errors)
	for i as integer = 0 to dimensions - 1
		if( exprTB(i,1) = NULL ) then
			errReport( errmsg )
			'' Error recovery: Allow further use of the exprTB() as if there were no ellipsis
			exprTB(i,1) = astNewCONSTi( 0 )
		end if
	next

end sub

'' end of parser-helper.bas
