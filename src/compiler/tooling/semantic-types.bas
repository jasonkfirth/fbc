'' Project: FreeBASIC compiler - normalized semantic types
'' File: tooling/semantic-types.bas
'' Purpose: Expose compiler-selected types without requiring private bit masks.
'' Responsibilities: Preserve nominal identity, qualifiers, indirection, and width.
'' This file intentionally does NOT contain: structural type equivalence or layout guesses.

#include once "tooling/semantic-private.bi"

function fbSemanticModelTypeFact _
	( byref domain as const string, byval identity as longint, byval dtype as integer, _
	  byval subtype as FBSYMBOL ptr, byval storage_bytes as longint, byref role as const string ) as string
	if( fbSemanticModelFullEnabled( ) = FALSE ) then return ""
	if( (identity = 0) or (dtype = FB_DATATYPE_INVALID) ) then return ""
	dim as integer base = typeGetDtOnly(dtype), pointers = typeGetPtrCnt(dtype)
	if( (base < 0) or (base >= FB_DATATYPES) ) then
		fbSemanticModelFail( )
		return ""
	end if
	dim as string name = *symb_dtypeTB(base).name, qualifiers
	if( base = FB_DATATYPE_FIXSTR ) then name = "fixed-string"
	'' Position zero describes the current value; each additional position
	'' follows one dereference. Distinct nominal IDs remain distinct types.
	for position as integer = 0 to pointers
		qualifiers += iif(typeIsConstAt(dtype, position), "1", "0")
	next
	dim as string modifier
	if( typeHasMangleDt(dtype) ) then
		dim as integer mangle = typeGetMangleDt(dtype)
		if( (mangle >= 0) and (mangle < FB_DATATYPES) ) then modifier = *symb_dtypeTB(mangle).name
	end if
	dim as string size
	if( pointers > 0 ) then
		size = fbSemanticModelNumber(env.pointersize)
	elseif( base = FB_DATATYPE_STRUCT ) then
		if( subtype <> NULL ) then
			if( symbIsStruct(subtype) andalso (subtype->udt.retdtype <> FB_DATATYPE_INVALID) ) then size = fbSemanticModelNumber(symbCalcLen(dtype, subtype))
		end if
	elseif( base = FB_DATATYPE_FIXSTR ) then
		if( storage_bytes > 0 ) then size = fbSemanticModelNumber(storage_bytes)
	elseif( symb_dtypeTB(base).size > 0 ) then
		size = fbSemanticModelNumber(symb_dtypeTB(base).size)
	end if
	return "NT" + TABCHAR + domain + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR + role + _
		TABCHAR + name + TABCHAR + fbSemanticModelNumber(fbSemanticModelSymbolId(subtype)) + _
		TABCHAR + fbSemanticModelNumber(pointers) + TABCHAR + fbSemanticModelNumber(abs(typeIsRef(dtype) <> FALSE)) + _
		TABCHAR + qualifiers + TABCHAR + size + TABCHAR + modifier
end function

sub fbSemanticModelExportType _
	( byref domain as const string, byval identity as longint, byval dtype as integer, _
	  byval subtype as FBSYMBOL ptr, byval storage_bytes as longint, byref role as const string )
	dim as string fact = fbSemanticModelTypeFact(domain, identity, dtype, subtype, storage_bytes, role)
	if( len(fact) > 0 ) then fbSemanticModelAppendDetail(fact)
end sub

'' end of tooling/semantic-types.bas
