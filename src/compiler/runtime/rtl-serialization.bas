'' Project: FreeBASIC compiler - serialization runtime interface
'' File: runtime/rtl-serialization.bas
'' Purpose: Register transport-independent logical serialization calls.
'' Responsibilities: Describe value and array-descriptor argument conventions.
'' This file intentionally does NOT contain: type traversal or binary codec.

#include once "core/fb.bi"
#include once "core/fbint.bi"
#include once "runtime/rtl.bi"

'' This module owns the registration table. rtlInit registers its declarations
'' for each compiler session; callers only receive the resulting symbol handles.
dim shared as FB_RTL_PROCDEF funcdata(0 to ...) = _
{ _
	( @"fb_Serialize", NULL, FB_DATATYPE_STRING, FB_FUNCMODE_FBCALL, _
	  NULL, FB_RTL_OPT_NONE, 3, _
	  { (typeSetIsConst(FB_DATATYPE_VOID), FB_PARAMMODE_BYREF, FALSE), _
		(FB_DATATYPE_STRING, FB_PARAMMODE_BYREF, FALSE), _
		(FB_DATATYPE_STRING, FB_PARAMMODE_BYREF, FALSE) } ), _
	( @"fb_Deserialize", NULL, FB_DATATYPE_INTEGER, FB_FUNCMODE_FBCALL, _
	  NULL, FB_RTL_OPT_NONE, 4, _
	  { (FB_DATATYPE_VOID, FB_PARAMMODE_BYREF, FALSE), _
		(FB_DATATYPE_STRING, FB_PARAMMODE_BYREF, FALSE), _
		(FB_DATATYPE_STRING, FB_PARAMMODE_BYREF, FALSE), _
		(FB_DATATYPE_STRING, FB_PARAMMODE_BYREF, FALSE) } ), _
	( @"fb_SerializeArray", NULL, FB_DATATYPE_STRING, FB_FUNCMODE_FBCALL, _
	  NULL, FB_RTL_OPT_NONE, 3, _
	  { (typeSetIsConst(FB_DATATYPE_VOID), FB_PARAMMODE_BYDESC, FALSE), _
		(FB_DATATYPE_STRING, FB_PARAMMODE_BYREF, FALSE), _
		(FB_DATATYPE_STRING, FB_PARAMMODE_BYREF, FALSE) } ), _
	( @"fb_DeserializeArray", NULL, FB_DATATYPE_INTEGER, FB_FUNCMODE_FBCALL, _
	  NULL, FB_RTL_OPT_NONE, 4, _
	  { (FB_DATATYPE_VOID, FB_PARAMMODE_BYDESC, FALSE), _
		(FB_DATATYPE_STRING, FB_PARAMMODE_BYREF, FALSE), _
		(FB_DATATYPE_STRING, FB_PARAMMODE_BYREF, FALSE), _
		(FB_DATATYPE_STRING, FB_PARAMMODE_BYREF, FALSE) } ), _
	( NULL ) _
}

sub rtlSerializationModInit( )
	rtlAddIntrinsicProcs( @funcdata(0) )
end sub

'' end of runtime/rtl-serialization.bas
