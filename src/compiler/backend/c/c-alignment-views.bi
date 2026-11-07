'' Project: FreeBASIC compiler - C alignment views
'' File: backend/c/c-alignment-views.bi
'' Purpose: Keep declared member paths when a record pointer may be unaligned.
'' Responsibilities: Remember forward/completed declaration scopes and emit compatible alignment-one
''     typedefs without changing the original record layout or BASIC symbols.
'' This file intentionally does NOT contain: public parameter or by-value type definitions.

'' Local symbol pool addresses can be reused. The newest declaration owns
'' the current type's scope and alignment alias.
private function hCLatestViewType( byval dtype as integer, byval subtype as FBSYMBOL ptr ) as CALIGNVIEW ptr
	var entry = cptr(CALIGNVIEW ptr, listGetHead(@ctx.alignmentviews))
	dim as CALIGNVIEW ptr result = NULL
	while( entry <> NULL )
		if( entry->dtype = dtype and entry->subtype = subtype ) then result = entry
		entry = listGetNext(entry)
	wend
	return result
end function

private sub hCRememberViewType( byval dtype as integer, byval subtype as FBSYMBOL ptr )
	var entry = cptr(CALIGNVIEW ptr, listNewNode(@ctx.alignmentviews))
	entry->dtype = dtype
	entry->subtype = subtype
	entry->section = ctx.section
	entry->rawproc = (dtype = FB_DATATYPE_FUNCTION) andalso (c_pointer_raw_types <> 0)
end sub

'' Procedure-pointer typedefs emitted during a record declaration may use
'' natural pointer types until the record is complete. The newest matching
'' entry wins because a local symbol's pool address can be reused.
private function hCRawProcedureType( byval subtype as FBSYMBOL ptr ) as integer
	if( subtype = NULL ) then return FALSE
	if( symbIsProc(subtype) = FALSE orelse symbGetIsFuncPtr(subtype) = FALSE ) then return FALSE
	var entry = cptr(CALIGNVIEW ptr, listGetHead(@ctx.alignmentviews))
	dim as integer result = FALSE
	while( entry <> NULL )
		if( entry->dtype = FB_DATATYPE_FUNCTION and entry->subtype = subtype ) then result = entry->rawproc
		entry = listGetNext(entry)
	wend
	return result
end function

private function hCAlignmentViewName _
	( byval dtype as integer, byval subtype as FBSYMBOL ptr ) as string

	dtype = typeGetDtOnly(dtype)
	if( dtype = FB_DATATYPE_USTRING ) then dtype = FB_DATATYPE_STRING
	'' Ensure a complete declaration exists before referring to its fields.
	var original = hEmitType(dtype, subtype, FALSE)
	var entry = hCLatestViewType(dtype, subtype)
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
