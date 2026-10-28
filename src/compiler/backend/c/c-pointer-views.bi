'' Project: FreeBASIC compiler - C pointer views
'' File: backend/c/c-pointer-views.bi
'' Purpose: Represent BASIC object pointers without a natural-alignment promise.
'' Responsibilities: Own scoped C typedefs and preserve pointee types, qualifiers
''     and pointer depth. GCC/Clang lower accesses using the actual alignment.
'' This file intentionally does NOT contain: changes to record layout, by-value objects,
''     function-pointer targets, builtin va_list types or machine-level ABI.

private sub hCClearPointerViews( byval firstsection as integer )
	var entry = cptr(CPOINTERVIEW ptr, listGetHead(@c_pointer_views))
	while( entry <> NULL )
		var following = cptr(CPOINTERVIEW ptr, listGetNext(entry))
		if( entry->section >= firstsection ) then
			DZstrAllocate(entry->original, 0)
			DZstrAllocate(entry->aliasname, 0)
			listDelNode(@c_pointer_views, entry)
		end if
		entry = following
	wend
end sub

private function hCPointerViewSection _
	( byval dtype as integer, byval subtype as FBSYMBOL ptr ) as integer

	if( subtype = NULL ) then return 0
	select case typeGetDtOnly(dtype)
	case FB_DATATYPE_STRUCT, FB_DATATYPE_ENUM, FB_DATATYPE_FUNCTION
	case else: return 0
	end select
	var declsection = -1
	var entry = cptr(CALIGNVIEW ptr, listGetHead(@ctx.alignmentviews))
	while( entry <> NULL )
		'' A local symbol's pool address may have been reused. The newest
		'' declaration is authoritative; never retain ownership of that symbol.
		if( entry->dtype = typeGetDtOnly(dtype) and entry->subtype = subtype ) then declsection = entry->section
		entry = listGetNext(entry)
	wend
	assert(declsection >= 0 and declsection <= ctx.section)
	return declsection
end function

private function hCPointerViewName _
	( byref original as string, byval declsection as integer ) as string

	var entry = cptr(CPOINTERVIEW ptr, listGetHead(@c_pointer_views))
	while( entry <> NULL )
		if( entry->section = declsection andalso *entry->original.data = original ) then return *entry->aliasname.data
		entry = listGetNext(entry)
	wend
	c_pointer_view_id += 1
	var aliasname = "fb_c_pointer_view_" + str(c_pointer_view_id)
	var previous = sectionGosub(declsection)
	'' A typedef can lower alignment without changing the compatible object
	'' type. Applying this to complete pointees preserves sizeof and field
	'' offsets; the pointer object itself keeps its ordinary ABI alignment.
	hWriteLine("typedef " + original + " " + aliasname + " __attribute__((aligned(1)));", TRUE)
	sectionReturn(previous)
	entry = listNewNode(@c_pointer_views)
	DZstrAssign(entry->original, strptr(original))
	DZstrAssign(entry->aliasname, strptr(aliasname))
	entry->section = declsection
	return aliasname
end function

'' end of backend/c/c-pointer-views.bi
