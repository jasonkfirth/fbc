'' Project: FreeBASIC compiler - C alignment views
'' File: backend/c/c-alignment-views.bi
'' Purpose: Keep declared member paths when a record pointer may be unaligned.
'' Responsibilities: Remember declaration scopes and emit compatible alignment-one
''     typedefs without changing the original record layout or BASIC symbols.
'' This file intentionally does NOT change public parameter or by-value types.

private sub hCRememberViewType( byval dtype as integer, byval subtype as FBSYMBOL ptr )
	var entry = cptr(CALIGNVIEW ptr, listNewNode(@ctx.alignmentviews))
	entry->dtype = dtype
	entry->subtype = subtype
	entry->section = ctx.section
end sub

private function hCAlignmentViewName _
	( byval dtype as integer, byval subtype as FBSYMBOL ptr ) as string

	dtype = typeGetDtOnly(dtype)
	if( dtype = FB_DATATYPE_USTRING ) then dtype = FB_DATATYPE_STRING
	'' Ensure a complete declaration exists before referring to its fields.
	var original = hEmitType(dtype, subtype, FALSE)
	var entry = cptr(CALIGNVIEW ptr, listGetHead(@ctx.alignmentviews))
	while( entry <> NULL )
		if( entry->dtype = dtype and entry->subtype = subtype ) then exit while
		entry = listGetNext(entry)
	wend
	if( entry = NULL ) then return ""
	dim as string aliasname
	if( dtype = FB_DATATYPE_STRING ) then
		aliasname = "fb_c_align1_string"
	else
		aliasname = "fb_c_align1_" + hGetUdtId(subtype)
	end if
	if( entry->emitted = FALSE ) then
		'' Use the original type's actual declaration section. A local type can
		'' have been forced outward for static storage or generated procedures.
		if( entry->section > ctx.section ) then return ""
		var previous = sectionGosub(entry->section)
		hWriteLine("typedef " + original + " " + aliasname + " __attribute__((aligned(1)));", TRUE)
		sectionReturn(previous)
		entry->emitted = TRUE
	end if
	return aliasname
end function

private function hCMemberViewIsAggregate( byval n as EXPRNODE ptr ) as integer
	if( n->class <> EXPRCLASS_MEMBER or n->member_view = 0 ) then return FALSE
	if( typeIsPtr(n->dtype) ) then return FALSE
	select case typeGetDtOnly(n->dtype)
	case FB_DATATYPE_STRUCT, FB_DATATYPE_STRING, FB_DATATYPE_USTRING
		return TRUE
	case FB_DATATYPE_INTEGER128, FB_DATATYPE_UINTEGER128
		'' The 32-bit C representation uses a two-word record.
		return fbGetBits() = 32
	end select
	return FALSE
end function

'' end of backend/c/c-alignment-views.bi
