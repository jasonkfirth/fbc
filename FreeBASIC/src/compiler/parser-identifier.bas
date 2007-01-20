''	FreeBASIC - 32-bit BASIC Compiler.
''	Copyright (C) 2004-2007 The FreeBASIC development team.
''
''	This program is free software; you can redistribute it and/or modify
''	it under the terms of the GNU General Public License as published by
''	the Free Software Foundation; either version 2 of the License, or
''	(at your option) any later version.
''
''	This program is distributed in the hope that it will be useful,
''	but WITHOUT ANY WARRANTY; without even the implied warranty of
''	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
''	GNU General Public License for more details.
''
''	You should have received a copy of the GNU General Public License
''	along with this program; if not, write to the Free Software
''	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA.


'' complex (scoped with namespace and/or class) identifiers parsing
''
'' chng: may/2006 written [v1ctor]


#include once "inc\fb.bi"
#include once "inc\fbint.bi"
#include once "inc\parser.bi"

'':::::
private sub hSkipSymbol( )

	do
		lexSkipToken( LEXCHECK_NOPERIOD )

    	'' '.'?
    	if( lexGetToken( ) <> CHAR_DOT ) then
    		exit do
    	end if

    	select case as const lexGetClass()
    	case FB_TKCLASS_IDENTIFIER, FB_TKCLASS_KEYWORD, FB_TKCLASS_QUIRKWD

    	case else
    		exit do
    	end select
	loop

end sub

'':::::
private function hGlobalId _
	( _
		byval options as FB_IDOPT = FB_IDOPT_SHOWERROR _
	) as FBSYMCHAIN ptr

	function = NULL

    '' another '.'?
    if( lexGetLookAhead( 1, LEXCHECK_NOPERIOD ) = CHAR_DOT ) then
    	'' skip the first '.'
    	lexSkipToken( LEXCHECK_NOPERIOD )

    else
    	'' inside a WITH block, a single '.' is ambiguous..
    	if( parser.stmt.with.sym <> NULL ) then
    		exit function
    	end if
    end if

    if( (options and FB_IDOPT_ISDECL) <> 0 ) then
    	'' different name spaces?
    	if( symbIsGlobalNamespc( ) = FALSE ) then
    		if( errReport( FB_ERRMSG_DECLOUTSIDENAMESPC ) = FALSE ) then
    			exit function
    		end if
    	end if
    end if

    '' skip the '.'
    lexSkipToken( LEXCHECK_NOPERIOD )

    '' not an ID?
    select case lexGetClass( )
    case FB_TKCLASS_IDENTIFIER, FB_TKCLASS_QUIRKWD

    case else
    	if( (options and FB_IDOPT_SHOWERROR) <> 0 ) then
    		errReport( FB_ERRMSG_EXPECTEDIDENTIFIER )
    	end if
    	exit function
    end select

	function = symbLookupAt( @symbGetGlobalNamespc( ), _
							 lexGetText( ), _
							 FALSE, _
							 TRUE )

end function

'':::::
#macro hCheckDecl _
	( _
		base_parent, _
		parent, _
		chain_, _
		options _
	)

    if( (options and FB_IDOPT_SHOWERROR) <> 0 ) then
    	'' declaration?
    	if( (options and FB_IDOPT_ISDECL) <> 0 ) then
    		if( base_parent <> NULL ) then
    			'' different parents?
    			if( symbGetParent( base_parent ) <> symbGetCurrentNamespc( ) ) then
    				errReport( FB_ERRMSG_DECLOUTSIDENAMESPC )
    				return NULL
    			end if
			end if

		'' not a decl..
		else
			'' check for ambiguous access (dup symbols in different imported namespaces)
			if( chain_ <> NULL ) then
				'' same symbol found in more than on hash tb?
				if( chain_->next <> NULL ) then
					dim as FBSYMBOL ptr ns = symbGetNamespace( chain_->sym )
					'' first symbol declared in other namespace?
					if( ns <> parent ) then
						'' not a parent namespace?
						if( symbIsChildOf( iif( parent <> NULL, _
									   	    	parent, _
									   	    	symbGetCurrentNamespc( ) ), _
											ns ) = FALSE ) then
							'' more than one imported symbol
							errReport( FB_ERRMSG_AMBIGUOUSSYMBOLACCESS )
							'' (don't return NULL or a new variable would be implicitly created)
						end if
					end if
				end if
    		end if
    	end if
    end if

#endmacro

'':::::
'' Identifier	= (ID{namespace|class} '.')* ID
''				|  ID ('.' ID)* .
''
function cIdentifier _
	( _
		byref base_parent as FBSYMBOL ptr, _
		byval options as FB_IDOPT _
	) as FBSYMCHAIN ptr

    dim as FBSYMCHAIN ptr chain_ = any
    dim as FBSYMBOL ptr parent = any

    base_parent = NULL

    chain_ = lexGetSymChain( )

	if( fbLangOptIsSet( FB_LANG_OPT_NAMESPC ) = FALSE ) then
	    return chain_
	end if

    if( chain_ = NULL ) then
    	'' '.'?
    	if( lexGetToken( ) <> CHAR_DOT ) then
    		return NULL
    	end if

    	chain_ = hGlobalId( options )
    	if( chain_ = NULL ) then
          	if( (options and FB_IDOPT_SHOWERROR) <> 0 ) then
          		errReportUndef( FB_ERRMSG_UNDEFINEDSYMBOL, lexGetText( ) )
    		else
    			hSkipSymbol( )
           	end if

    		return NULL
    	end if
    end if

    parent = NULL

    do
    	dim as FBSYMBOL ptr sym = chain_->sym

    	select case as const symbGetClass( sym )
    	case FB_SYMBCLASS_NAMESPACE, FB_SYMBCLASS_CLASS

    	case FB_SYMBCLASS_STRUCT
    		if( (options and FB_IDOPT_ALLOWSTRUCT) = 0 ) then
    			exit do
    		end if

    		'' ordinary struct?
    		if( symbGetIsUnique( sym ) = FALSE ) then
    			exit do
    		end if

    	case FB_SYMBCLASS_TYPEDEF
            '' typedef of a TYPE/CLASS?
            select case symbGetType( sym )
            case FB_DATATYPE_STRUCT
    			if( (options and FB_IDOPT_ALLOWSTRUCT) = 0 ) then
    				exit do
    			end if

            	sym = symbGetSubtype( sym )

    			'' ordinary struct?
    			if( symbGetIsUnique( sym ) = FALSE ) then
    				exit do
    			end if

            'case FB_DATATYPE_CLASS
            	'' ...

            case else
            	exit do
            end select

    	case else
    		exit do
    	end select

    	'' '.'?
    	if( lexGetLookAhead( 1, LEXCHECK_NOPERIOD ) <> CHAR_DOT ) then
    		'' if it's a namespace, the '.' is obligatory, the
    		'' namespace itself isn't a composite type
    		if( symbGetClass( sym ) = FB_SYMBCLASS_NAMESPACE ) then
    			'' skip id
    			lexSkipToken( LEXCHECK_NOPERIOD )

    			if( (options and FB_IDOPT_DONTCHKPERIOD) <> 0 ) then
    				exit do
    			end if

    			if( (options and FB_IDOPT_SHOWERROR) <> 0 ) then
    				if( errReport( FB_ERRMSG_EXPECTEDPERIOD ) = FALSE ) then
    					return NULL
    				end if
    			end if
    		end if

    		exit do
    	end if

    	'' skip id
    	lexSkipToken( LEXCHECK_NOPERIOD )

    	'' skip '.'
    	lexSkipToken( LEXCHECK_NOPERIOD )

    	parent = sym

    	if( base_parent = NULL ) then
    		base_parent = parent
    	end if

    	'' ID
    	select case as const lexGetClass( )
    	case FB_TKCLASS_IDENTIFIER, FB_TKCLASS_QUIRKWD

    	case FB_TKCLASS_OPERATOR, FB_TKCLASS_KEYWORD
    		if( (options and FB_IDOPT_ISOPERATOR ) <> 0 ) then
    			exit do
    		end if

    		if( (options and FB_IDOPT_SHOWERROR) <> 0 ) then
    			errReport( FB_ERRMSG_EXPECTEDIDENTIFIER )
    		end if

    		return NULL

    	case else
    		if( (options and FB_IDOPT_SHOWERROR) <> 0 ) then
    			errReport( FB_ERRMSG_EXPECTEDIDENTIFIER )
    		end if

    		return NULL
    	end select

    	'' look up
    	chain_ = symbLookupAt( parent, lexGetText( ), FALSE )
    	if( chain_ = NULL ) then
          	if( (options and FB_IDOPT_SHOWERROR) <> 0 ) then
          		errReportUndef( FB_ERRMSG_UNDEFINEDSYMBOL, lexGetText( ) )
    		else
    			hSkipSymbol( )
           	end if

    	    return NULL
    	end if

    	'' check access to non-static members
    	if( (options and FB_IDOPT_CHECKSTATIC) <> 0 ) then
    		'' struct or class?
    		select case symbGetClass( parent )
    		case FB_SYMBCLASS_STRUCT, FB_SYMBCLASS_CLASS
    			'' for each symbol (because dups..)
    			dim as FBSYMCHAIN ptr iter = chain_
    			do
        			dim as FBSYMBOL ptr sym = iter->sym
        			do
        				'' field, never static..
        				if( symbGetClass( sym ) = FB_SYMBCLASS_FIELD ) then
							errReport( FB_ERRMSG_ACCESSTONONSTATICMEMBER )
        					goto exit_check
        				end if

        				sym = sym->hash.next
        			loop while( sym <> NULL )

    				iter = symbChainGetNext( iter )
    			loop while( iter <> NULL )
exit_check:
    		end select
    	end if
    loop

	''
	hCheckDecl( base_parent, parent, chain_, options )

	function = chain_

end function

'':::::
'' ParentId		= ID{namespace|class} ('.' ID{namespace|class})* .
''
function cParentId _
	( _
		byval options as FB_IDOPT _
	) as FBSYMBOL ptr

    dim as FBSYMCHAIN ptr chain_ = any
    dim as FBSYMBOL ptr parent = any, base_parent = any

    base_parent = NULL

	if( fbLangOptIsSet( FB_LANG_OPT_NAMESPC ) = FALSE ) then
	    return NULL
	end if

    chain_ = lexGetSymChain( )
    if( chain_ = NULL ) then
    	'' '.'?
    	if( lexGetToken( ) = CHAR_DOT ) then
    		chain_ = hGlobalId( )
    	end if
    end if

    parent = NULL

    do while( chain_ <> NULL )

    	select case as const symbGetClass( chain_->sym )
    	case FB_SYMBCLASS_NAMESPACE, FB_SYMBCLASS_CLASS
    		parent = chain_->sym

    	case FB_SYMBCLASS_STRUCT
    		if( (options and FB_IDOPT_ALLOWSTRUCT) = 0 ) then
    			exit do
    		end if

    		'' ordinary struct?
    		if( symbGetIsUnique( chain_->sym ) = FALSE ) then
    			exit do
    		end if

    		parent = chain_->sym

    	case FB_SYMBCLASS_TYPEDEF
            dim as FBSYMBOL ptr sym = any

            '' typedef of a TYPE/CLASS?
            select case symbGetType( chain_->sym )
            case FB_DATATYPE_STRUCT
    			if( (options and FB_IDOPT_ALLOWSTRUCT) = 0 ) then
    				exit do
    			end if

    			sym = symbGetSubtype( chain_->sym )

    			'' ordinary struct?
    			if( symbGetIsUnique( sym ) = FALSE ) then
    				exit do
    			end if

			'case FB_DATATYPE_CLASS
				'' ...

    		case else
    			exit do
    		end select

    		parent = sym

    	case else
    		exit do
    	end select

    	'' '.'?
    	if( lexGetLookAhead( 1, LEXCHECK_NOPERIOD ) <> CHAR_DOT ) then
    		'' skip id
    		lexSkipToken( LEXCHECK_NOPERIOD )

    		if( (options and FB_IDOPT_DONTCHKPERIOD) <> 0 ) then
    			exit do
    		end if

    		if( errReport( FB_ERRMSG_EXPECTEDPERIOD ) = FALSE ) then
    			return NULL
    		else
    			exit do
    		end if
    	end if

    	'' skip id
    	lexSkipToken( LEXCHECK_NOPERIOD )

    	'' skip '.'
    	lexSkipToken( LEXCHECK_NOPERIOD )

    	if( base_parent = NULL ) then
    		base_parent = parent
    	end if

    	'' ID
    	select case as const lexGetClass( )
    	case FB_TKCLASS_IDENTIFIER, FB_TKCLASS_QUIRKWD

    	case FB_TKCLASS_OPERATOR, FB_TKCLASS_KEYWORD
    		if( (options and FB_IDOPT_ISOPERATOR ) <> 0 ) then
    			exit do
    		end if

    		if( errReport( FB_ERRMSG_EXPECTEDIDENTIFIER ) = FALSE ) then
    			return NULL
    		else
    			exit do
    		end if

    	case else
    		if( errReport( FB_ERRMSG_EXPECTEDIDENTIFIER ) = FALSE ) then
    			return NULL
    		else
    			exit do
    		end if
    	end select

    	chain_ = symbLookupAt( parent, lexGetText( ), FALSE )
    loop

	''
	hCheckDecl( base_parent, parent, chain_, options )

	function = parent

end function


