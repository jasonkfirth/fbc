'' Project: FreeBASIC compiler - UTF-8 runtime call lowering
'' File: runtime/rtl-ustring.bas
'' Purpose: Declare native USTRING services and build explicit conversions.
'' Responsibilities: Register UTF-8 intrinsics and retain descriptor ownership.
'' This file intentionally does NOT contain: decoding or parser grammar.

#include once "core/fb.bi"
#include once "core/fbint.bi"
#include once "ast/ast.bi"
#include once "runtime/rtl.bi"

'' -------------------------------------------------------------------------
'' USTRING runtime procedure definitions
'' -------------------------------------------------------------------------
dim shared as FB_RTL_PROCDEF ufuncdata(0 to ...) = _
{ _
	( _
		@FB_RTL_USTRFROMBYTES, NULL, _
		FB_DATATYPE_USTRING, FB_FUNCMODE_FBCALL, _
		NULL, FB_RTL_OPT_NOQB, _
		2, _
		{ _
			( typeSetIsConst( FB_DATATYPE_VOID ), FB_PARAMMODE_BYREF, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_INTEGER ), FB_PARAMMODE_BYVAL, FALSE ) _
		} _
	), _
	( _
		@FB_RTL_USTRFROMWSTR, NULL, _
		FB_DATATYPE_USTRING, FB_FUNCMODE_FBCALL, _
		NULL, FB_RTL_OPT_NOQB, _
		1, _
		{ _
			( typeAddrOf( typeSetIsConst( FB_DATATYPE_WCHAR ) ), FB_PARAMMODE_BYVAL, FALSE ) _
		} _
	), _
	( _
		@FB_RTL_USTRTOWSTR, NULL, _
		typeAddrOf( FB_DATATYPE_WCHAR ), FB_FUNCMODE_FBCALL, _
		NULL, FB_RTL_OPT_NOQB, _
		1, _
		{ _
			( typeSetIsConst( FB_DATATYPE_STRING ), FB_PARAMMODE_BYREF, FALSE ) _
		} _
	), _
	( _
		@FB_RTL_USTRLEN, NULL, _
		FB_DATATYPE_INTEGER, FB_FUNCMODE_FBCALL, _
		NULL, FB_RTL_OPT_NOQB, _
		2, _
		{ _
			( typeSetIsConst( FB_DATATYPE_VOID ), FB_PARAMMODE_BYREF, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_INTEGER ), FB_PARAMMODE_BYVAL, FALSE ) _
		} _
	), _
	( _
		@FB_RTL_USTRMID, NULL, _
		FB_DATATYPE_USTRING, FB_FUNCMODE_FBCALL, _
		NULL, FB_RTL_OPT_NOQB, _
		3, _
		{ _
			( typeSetIsConst( FB_DATATYPE_STRING ), FB_PARAMMODE_BYREF, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_INTEGER ), FB_PARAMMODE_BYVAL, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_INTEGER ), FB_PARAMMODE_BYVAL, FALSE ) _
		} _
	), _
	( _
		@FB_RTL_USTRASC, NULL, _
		FB_DATATYPE_ULONG, FB_FUNCMODE_FBCALL, _
		NULL, FB_RTL_OPT_NOQB, _
		2, _
		{ _
			( typeSetIsConst( FB_DATATYPE_STRING ), FB_PARAMMODE_BYREF, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_INTEGER ), FB_PARAMMODE_BYVAL, FALSE ) _
		} _
	), _
	( _
		@FB_RTL_USTRFILL1, NULL, _
		FB_DATATYPE_USTRING, FB_FUNCMODE_FBCALL, _
		NULL, FB_RTL_OPT_NOQB, _
		2, _
		{ _
			( typeSetIsConst( FB_DATATYPE_INTEGER ), FB_PARAMMODE_BYVAL, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_INTEGER ), FB_PARAMMODE_BYVAL, FALSE ) _
		} _
	), _
	( _
		@FB_RTL_USTRFILL2, NULL, _
		FB_DATATYPE_USTRING, FB_FUNCMODE_FBCALL, _
		NULL, FB_RTL_OPT_NOQB, _
		2, _
		{ _
			( typeSetIsConst( FB_DATATYPE_INTEGER ), FB_PARAMMODE_BYVAL, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_STRING ), FB_PARAMMODE_BYREF, FALSE ) _
		} _
	), _
	( _
		@FB_RTL_USTRASSIGNMID, NULL, _
		FB_DATATYPE_VOID, FB_FUNCMODE_FBCALL, _
		NULL, FB_RTL_OPT_NOQB, _
		4, _
		{ _
			( FB_DATATYPE_USTRING, FB_PARAMMODE_BYREF, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_INTEGER ), FB_PARAMMODE_BYVAL, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_INTEGER ), FB_PARAMMODE_BYVAL, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_STRING ), FB_PARAMMODE_BYREF, FALSE ) _
		} _
	), _
	( _
		@FB_RTL_USTRLSET, NULL, _
		FB_DATATYPE_VOID, FB_FUNCMODE_FBCALL, _
		NULL, FB_RTL_OPT_NOQB, _
		2, _
		{ _
			( FB_DATATYPE_USTRING, FB_PARAMMODE_BYREF, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_STRING ), FB_PARAMMODE_BYREF, FALSE ) _
		} _
	), _
	( _
		@FB_RTL_USTRRSET, NULL, _
		FB_DATATYPE_VOID, FB_FUNCMODE_FBCALL, _
		NULL, FB_RTL_OPT_NOQB, _
		2, _
		{ _
			( FB_DATATYPE_USTRING, FB_PARAMMODE_BYREF, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_STRING ), FB_PARAMMODE_BYREF, FALSE ) _
		} _
	), _
	( _
		@FB_RTL_USTRINIT, NULL, _
		FB_DATATYPE_USTRING, FB_FUNCMODE_FBCALL, _
		NULL, FB_RTL_OPT_NOQB, _
		5, _
		{ _
			( FB_DATATYPE_VOID, FB_PARAMMODE_BYREF, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_INTEGER ), FB_PARAMMODE_BYVAL, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_VOID ), FB_PARAMMODE_BYREF, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_INTEGER ), FB_PARAMMODE_BYVAL, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ) _
		} _
	), _
	( _
		@FB_RTL_USTRASSIGN, NULL, _
		FB_DATATYPE_USTRING, FB_FUNCMODE_FBCALL, _
		NULL, FB_RTL_OPT_NOQB, _
		5, _
		{ _
			( FB_DATATYPE_VOID, FB_PARAMMODE_BYREF, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_INTEGER ), FB_PARAMMODE_BYVAL, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_VOID ), FB_PARAMMODE_BYREF, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_INTEGER ), FB_PARAMMODE_BYVAL, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ) _
		} _
	), _
	( _
		@FB_RTL_USTRCONCATASSIGN, NULL, _
		FB_DATATYPE_USTRING, FB_FUNCMODE_FBCALL, _
		NULL, FB_RTL_OPT_NOQB, _
		5, _
		{ _
			( FB_DATATYPE_VOID, FB_PARAMMODE_BYREF, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_INTEGER ), FB_PARAMMODE_BYVAL, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_VOID ), FB_PARAMMODE_BYREF, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_INTEGER ), FB_PARAMMODE_BYVAL, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ) _
		} _
	), _
	( _
		@FB_RTL_USTRCONCAT, NULL, _
		FB_DATATYPE_USTRING, FB_FUNCMODE_FBCALL, _
		NULL, FB_RTL_OPT_NOQB, _
		5, _
		{ _
			( FB_DATATYPE_USTRING, FB_PARAMMODE_BYREF, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_VOID ), FB_PARAMMODE_BYREF, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_INTEGER ), FB_PARAMMODE_BYVAL, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_VOID ), FB_PARAMMODE_BYREF, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_INTEGER ), FB_PARAMMODE_BYVAL, FALSE ) _
		} _
	), _
	( _
		@FB_RTL_USTRCHR, NULL, _
		FB_DATATYPE_USTRING, FB_FUNCMODE_CDECL, _
		NULL, FB_RTL_OPT_NOQB, _
		2, _
		{ _
			( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_VOID ), FB_PARAMMODE_VARARG, FALSE ) _
		} _
	), _
	( _
		@FB_RTL_USTRINSTR, NULL, _
		FB_DATATYPE_INTEGER, FB_FUNCMODE_FBCALL, _
		NULL, FB_RTL_OPT_NOQB, _
		3, _
		{ _
			( typeSetIsConst( FB_DATATYPE_INTEGER ), FB_PARAMMODE_BYVAL, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_STRING ), FB_PARAMMODE_BYREF, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_STRING ), FB_PARAMMODE_BYREF, FALSE ) _
		} _
	), _
	( _
		@FB_RTL_USTRINSTRANY, NULL, _
		FB_DATATYPE_INTEGER, FB_FUNCMODE_FBCALL, _
		NULL, FB_RTL_OPT_NOQB, _
		3, _
		{ _
			( typeSetIsConst( FB_DATATYPE_INTEGER ), FB_PARAMMODE_BYVAL, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_STRING ), FB_PARAMMODE_BYREF, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_STRING ), FB_PARAMMODE_BYREF, FALSE ) _
		} _
	), _
	( _
		@FB_RTL_USTRINSTRREV, NULL, _
		FB_DATATYPE_INTEGER, FB_FUNCMODE_FBCALL, _
		NULL, FB_RTL_OPT_NOQB, _
		3, _
		{ _
			( typeSetIsConst( FB_DATATYPE_STRING ), FB_PARAMMODE_BYREF, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_STRING ), FB_PARAMMODE_BYREF, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_INTEGER ), FB_PARAMMODE_BYVAL, FALSE ) _
		} _
	), _
	( _
		@FB_RTL_USTRINSTRREVANY, NULL, _
		FB_DATATYPE_INTEGER, FB_FUNCMODE_FBCALL, _
		NULL, FB_RTL_OPT_NOQB, _
		3, _
		{ _
			( typeSetIsConst( FB_DATATYPE_STRING ), FB_PARAMMODE_BYREF, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_STRING ), FB_PARAMMODE_BYREF, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_INTEGER ), FB_PARAMMODE_BYVAL, FALSE ) _
		} _
	), _
	( _
		@FB_RTL_USTRTRIM, NULL, _
		FB_DATATYPE_USTRING, FB_FUNCMODE_FBCALL, _
		NULL, FB_RTL_OPT_NOQB, _
		1, _
		{ _
			( typeSetIsConst( FB_DATATYPE_STRING ), FB_PARAMMODE_BYREF, FALSE ) _
		} _
	), _
	( _
		@FB_RTL_USTRTRIMEX, NULL, _
		FB_DATATYPE_USTRING, FB_FUNCMODE_FBCALL, _
		NULL, FB_RTL_OPT_NOQB, _
		2, _
		{ _
			( typeSetIsConst( FB_DATATYPE_STRING ), FB_PARAMMODE_BYREF, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_STRING ), FB_PARAMMODE_BYREF, FALSE ) _
		} _
	), _
	( _
		@FB_RTL_USTRTRIMANY, NULL, _
		FB_DATATYPE_USTRING, FB_FUNCMODE_FBCALL, _
		NULL, FB_RTL_OPT_NOQB, _
		2, _
		{ _
			( typeSetIsConst( FB_DATATYPE_STRING ), FB_PARAMMODE_BYREF, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_STRING ), FB_PARAMMODE_BYREF, FALSE ) _
		} _
	), _
	( _
		@FB_RTL_USTRLTRIM, NULL, _
		FB_DATATYPE_USTRING, FB_FUNCMODE_FBCALL, _
		NULL, FB_RTL_OPT_NOQB, _
		1, _
		{ _
			( typeSetIsConst( FB_DATATYPE_STRING ), FB_PARAMMODE_BYREF, FALSE ) _
		} _
	), _
	( _
		@FB_RTL_USTRLTRIMEX, NULL, _
		FB_DATATYPE_USTRING, FB_FUNCMODE_FBCALL, _
		NULL, FB_RTL_OPT_NOQB, _
		2, _
		{ _
			( typeSetIsConst( FB_DATATYPE_STRING ), FB_PARAMMODE_BYREF, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_STRING ), FB_PARAMMODE_BYREF, FALSE ) _
		} _
	), _
	( _
		@FB_RTL_USTRLTRIMANY, NULL, _
		FB_DATATYPE_USTRING, FB_FUNCMODE_FBCALL, _
		NULL, FB_RTL_OPT_NOQB, _
		2, _
		{ _
			( typeSetIsConst( FB_DATATYPE_STRING ), FB_PARAMMODE_BYREF, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_STRING ), FB_PARAMMODE_BYREF, FALSE ) _
		} _
	), _
	( _
		@FB_RTL_USTRRTRIM, NULL, _
		FB_DATATYPE_USTRING, FB_FUNCMODE_FBCALL, _
		NULL, FB_RTL_OPT_NOQB, _
		1, _
		{ _
			( typeSetIsConst( FB_DATATYPE_STRING ), FB_PARAMMODE_BYREF, FALSE ) _
		} _
	), _
	( _
		@FB_RTL_USTRRTRIMEX, NULL, _
		FB_DATATYPE_USTRING, FB_FUNCMODE_FBCALL, _
		NULL, FB_RTL_OPT_NOQB, _
		2, _
		{ _
			( typeSetIsConst( FB_DATATYPE_STRING ), FB_PARAMMODE_BYREF, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_STRING ), FB_PARAMMODE_BYREF, FALSE ) _
		} _
	), _
	( _
		@FB_RTL_USTRRTRIMANY, NULL, _
		FB_DATATYPE_USTRING, FB_FUNCMODE_FBCALL, _
		NULL, FB_RTL_OPT_NOQB, _
		2, _
		{ _
			( typeSetIsConst( FB_DATATYPE_STRING ), FB_PARAMMODE_BYREF, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_STRING ), FB_PARAMMODE_BYREF, FALSE ) _
		} _
	), _
	( _
		@FB_RTL_USTRUCASE, NULL, _
		FB_DATATYPE_USTRING, FB_FUNCMODE_FBCALL, _
		NULL, FB_RTL_OPT_NOQB, _
		2, _
		{ _
			( typeSetIsConst( FB_DATATYPE_STRING ), FB_PARAMMODE_BYREF, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, TRUE, 0 ) _
		} _
	), _
	( _
		@FB_RTL_USTRLCASE, NULL, _
		FB_DATATYPE_USTRING, FB_FUNCMODE_FBCALL, _
		NULL, FB_RTL_OPT_NOQB, _
		2, _
		{ _
			( typeSetIsConst( FB_DATATYPE_STRING ), FB_PARAMMODE_BYREF, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, TRUE, 0 ) _
		} _
	), _
	( _
		@"left", @"fb_UStrLeft", _
		FB_DATATYPE_USTRING, FB_FUNCMODE_FBCALL, _
		NULL, FB_RTL_OPT_OVER or FB_RTL_OPT_NOQB, _
		2, _
		{ _
			( typeSetIsConst( FB_DATATYPE_USTRING ), FB_PARAMMODE_BYREF, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_INTEGER ), FB_PARAMMODE_BYVAL, FALSE ) _
		} _
	), _
	( _
		@"right", @"fb_UStrRight", _
		FB_DATATYPE_USTRING, FB_FUNCMODE_FBCALL, _
		NULL, FB_RTL_OPT_OVER or FB_RTL_OPT_NOQB, _
		2, _
		{ _
			( typeSetIsConst( FB_DATATYPE_USTRING ), FB_PARAMMODE_BYREF, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_INTEGER ), FB_PARAMMODE_BYVAL, FALSE ) _
		} _
	), _
	( _
		@FB_RTL_USTRINDEXGET, NULL, _
		FB_DATATYPE_ULONG, FB_FUNCMODE_FBCALL, _
		NULL, FB_RTL_OPT_NOQB, _
		2, _
		{ _
			( typeSetIsConst( FB_DATATYPE_STRING ), FB_PARAMMODE_BYREF, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_INTEGER ), FB_PARAMMODE_BYVAL, FALSE ) _
		} _
	), _
	( _
		@FB_RTL_USTRINDEXSET, NULL, _
		FB_DATATYPE_VOID, FB_FUNCMODE_FBCALL, _
		NULL, FB_RTL_OPT_NOQB, _
		3, _
		{ _
			( FB_DATATYPE_USTRING, FB_PARAMMODE_BYREF, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_INTEGER ), FB_PARAMMODE_BYVAL, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_INTEGER ), FB_PARAMMODE_BYVAL, FALSE ) _
		} _
	), _
	( _
		@FB_RTL_USTRBYTES, NULL, _
		FB_DATATYPE_STRING, FB_FUNCMODE_FBCALL, _
		NULL, FB_RTL_OPT_NOQB, _
		1, _
		{ _
			( typeSetIsConst( FB_DATATYPE_STRING ), FB_PARAMMODE_BYREF, FALSE ) _
		} _
	), _
	( _
		@FB_RTL_USTRFILELINEINPUT, NULL, _
		FB_DATATYPE_LONG, FB_FUNCMODE_FBCALL, _
		NULL, FB_RTL_OPT_NOQB, _
		4, _
		{ _
			( FB_DATATYPE_LONG, FB_PARAMMODE_BYVAL, FALSE ), _
			( FB_DATATYPE_VOID, FB_PARAMMODE_BYREF, FALSE ), _
			( FB_DATATYPE_INTEGER, FB_PARAMMODE_BYVAL, FALSE ), _
			( FB_DATATYPE_LONG, FB_PARAMMODE_BYVAL, FALSE ) _
		} _
	), _
	( _
		@FB_RTL_USTRINPUT, NULL, _
		FB_DATATYPE_LONG, FB_FUNCMODE_FBCALL, _
		NULL, FB_RTL_OPT_NOQB, _
		3, _
		{ _
			( FB_DATATYPE_VOID, FB_PARAMMODE_BYREF, FALSE ), _
			( FB_DATATYPE_INTEGER, FB_PARAMMODE_BYVAL, FALSE ), _
			( FB_DATATYPE_LONG, FB_PARAMMODE_BYVAL, FALSE ) _
		} _
	), _
	( _
		@FB_RTL_USTRDATAREAD, NULL, _
		FB_DATATYPE_VOID, FB_FUNCMODE_FBCALL, _
		NULL, FB_RTL_OPT_NOQB, _
		3, _
		{ _
			( FB_DATATYPE_VOID, FB_PARAMMODE_BYREF, FALSE ), _
			( FB_DATATYPE_INTEGER, FB_PARAMMODE_BYVAL, FALSE ), _
			( FB_DATATYPE_LONG, FB_PARAMMODE_BYVAL, FALSE ) _
		} _
	), _
	( _
		@FB_RTL_USTRPRINT, NULL, _
		FB_DATATYPE_VOID, FB_FUNCMODE_FBCALL, _
		NULL, FB_RTL_OPT_NOQB, _
		3, _
		{ _
			( FB_DATATYPE_LONG, FB_PARAMMODE_BYVAL, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_STRING ), FB_PARAMMODE_BYREF, FALSE ), _
			( FB_DATATYPE_LONG, FB_PARAMMODE_BYVAL, FALSE ) _
		} _
	), _
	( _
		@FB_RTL_USTRWRITE, NULL, _
		FB_DATATYPE_VOID, FB_FUNCMODE_FBCALL, _
		NULL, FB_RTL_OPT_NOQB, _
		3, _
		{ _
			( FB_DATATYPE_LONG, FB_PARAMMODE_BYVAL, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_STRING ), FB_PARAMMODE_BYREF, FALSE ), _
			( FB_DATATYPE_LONG, FB_PARAMMODE_BYVAL, FALSE ) _
		} _
	), _
	( _
		@FB_RTL_USTRPRINTUSGINIT, NULL, _
		FB_DATATYPE_LONG, FB_FUNCMODE_FBCALL, _
		NULL, FB_RTL_OPT_NOQB, _
		1, _
		{ _
			( typeSetIsConst( FB_DATATYPE_USTRING ), FB_PARAMMODE_BYREF, FALSE ) _
		} _
	), _
	( _
		@FB_RTL_USTRLPRINTUSGINIT, NULL, _
		FB_DATATYPE_LONG, FB_FUNCMODE_FBCALL, _
		@rtlPrinter_cb, FB_RTL_OPT_NOQB, _
		1, _
		{ _
			( typeSetIsConst( FB_DATATYPE_USTRING ), FB_PARAMMODE_BYREF, FALSE ) _
		} _
	), _
	( _
		@FB_RTL_USTRPRINTUSG, NULL, _
		FB_DATATYPE_LONG, FB_FUNCMODE_FBCALL, _
		NULL, FB_RTL_OPT_NOQB, _
		3, _
		{ _
			( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_USTRING ), FB_PARAMMODE_BYREF, FALSE ), _
			( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ) _
		} _
	), _
	( NULL ) _
}

'' -------------------------------------------------------------------------
'' Intrinsic registration and conversion lowering
'' -------------------------------------------------------------------------
sub rtlUstringModInit( )
	rtlAddIntrinsicProcs( @ufuncdata(0) )
end sub

function rtlToUstr( byval expr as ASTNODE ptr ) as ASTNODE ptr
	dim as ASTNODE ptr proc
	dim as integer dtype = astGetDataType( expr )
	if( dtype = FB_DATATYPE_USTRING ) then return expr
	if( dtype = FB_DATATYPE_WCHAR ) then
		proc = astNewCALL( PROCLOOKUP( USTRFROMWSTR ) )
		if( astNewARG( proc, expr ) = NULL ) then return NULL
	else
		if( typeGetClass( dtype ) <> FB_DATACLASS_STRING and dtype <> FB_DATATYPE_CHAR ) then
			expr = rtlToStr( expr, FALSE )
			if( expr = NULL ) then return NULL
			dtype = astGetDataType( expr )
		end if
		var length = rtlCalcStrLen( expr, dtype )
		proc = astNewCALL( PROCLOOKUP( USTRFROMBYTES ) )
		if( astNewARG( proc, expr ) = NULL ) then return NULL
		if( astNewARG( proc, astNewCONSTi( length ) ) = NULL ) then return NULL
	end if
	return proc
end function

function rtlUStrToBytes( byval expr as ASTNODE ptr ) as ASTNODE ptr
	var proc = astNewCALL( PROCLOOKUP( USTRBYTES ) )
	if( astNewARG( proc, expr ) = NULL ) then return NULL
	return proc
end function

'' -------------------------------------------------------------------------
'' Writable USTRING argument lowering
'' -------------------------------------------------------------------------
function rtlUStrPrepareWrite( byref dst as ASTNODE ptr, byref before as ASTNODE ptr ) as ASTNODE ptr
	before = NULL
	if( astGetDataType( dst ) <> FB_DATATYPE_USTRING ) then return NULL
	if( astHasSideFx( dst ) ) then before = astMakeRef( dst )
	return rtlStrAssign( astCloneTree( dst ), astCloneTree( dst ) )
end function

function rtlUStrFinishWrite( byval before as ASTNODE ptr, byval proc as ASTNODE ptr, byval after as ASTNODE ptr ) as ASTNODE ptr
	if( after = NULL ) then return proc
	return astNewLINK( before, astNewLINK( proc, after, AST_LINK_RETURN_LEFT ), AST_LINK_RETURN_RIGHT )
end function

'' end of runtime/rtl-ustring.bas
