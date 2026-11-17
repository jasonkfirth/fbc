/'
Project: FreeBASIC C backend local storage
File: backend/c/c-overaligned-locals.bi
Purpose: Preserve automatic-object alignment above the Wii EABI stack boundary.
Responsibilities: Allocate once at procedure entry and retain typed local access.
This file intentionally does NOT change record layout, heap allocation or other targets.
'/

type COVERALIGNEDLOCAL
	previous as COVERALIGNEDLOCAL ptr
	identifier as string
end type

'' This list owns only C macro names for the current procedure. Symbol and
'' type ownership stays with the frontend; names are released at procedure end.
dim shared c_overaligned_locals as COVERALIGNEDLOCAL ptr

private function hCEmitOveralignedLocal _
	( byval sym as FBSYMBOL ptr, byval dtype as integer, _
	  byval subtype as FBSYMBOL ptr, byref identifier as string, _
	  byval alignment as integer ) as integer

	if( env.clopt.target <> FB_COMPTARGET_WII orelse alignment <= 8 ) then return FALSE
	if( ctx.currproc = NULL orelse symbIsNaked(ctx.currproc) ) then return FALSE
	if( symbIsLocal(sym) = FALSE orelse symbIsStatic(sym) orelse symbIsExtern(sym) orelse symbIsParamVar(sym) ) then return FALSE
	'' Error captures have their own physical storage and qualified spellings.
	if( identifier <> hGetCId(sym) ) then return FALSE
	if( alignment <= 0 orelse (alignment and (alignment - 1)) <> 0 ) then
		errReportEx(FB_ERRMSG_INTERNAL, "C automatic storage alignment")
		return TRUE
	end if
	var size = symbGetRealSize(sym)
	if( size <= 0 ) then
		errReportEx(FB_ERRMSG_INTERNAL, "C automatic storage extent")
		return TRUE
	end if

	var entry = new COVERALIGNEDLOCAL
	if( entry = NULL ) then
		errReportEx(FB_ERRMSG_INTERNAL, "C automatic storage metadata allocation")
		return TRUE
	end if
	entry->identifier = identifier
	entry->previous = c_overaligned_locals
	c_overaligned_locals = entry
	var suffix = *symbUniqueId(TRUE)
	var storage = "fb_c_aligned_storage_" + suffix
	var typename = "fb_c_aligned_type_" + suffix

	'' The Wii stack ABI guarantees eight-byte alignment. GCC can leave even
	'' explicitly aligned nested records on that boundary at -O0. Its aligned
	'' allocation builtin supplies genuinely aligned, untyped storage instead.
	'' Hoist the allocation to the procedure header: loops and GOTO must never
	'' repeat it or bypass initialization of its address. The local typedef can
	'' still use a BASIC type declared inside a nested scope. The builtin's
	'' alignment argument is in bits. A typed lvalue macro needs no block-local
	'' pointer initialization that a source jump could bypass.
	assert(ctx.section >= 3)
	var section = sectionGosub(2)
	hWriteLine("void *const " + storage + " = __builtin_alloca_with_align(" + _
	           str(size) + "u, " + str(clngint(alignment) * 8) + "u);", TRUE)
	sectionReturn(section)
	hWriteLine("typedef " + hEmitType(dtype, subtype, TRUE) + " " + typename + hEmitArrayDecl(sym) + ";", TRUE)
	hWriteLine("#undef " + identifier, TRUE)
	hWriteLine("#define " + identifier + " (*((" + typename + "*)" + storage + "))", TRUE)
	return TRUE
end function

private sub hCEndOveralignedLocals()
	while( c_overaligned_locals <> NULL )
		var entry = c_overaligned_locals
		hWriteLine("#undef " + entry->identifier)
		c_overaligned_locals = entry->previous
		delete entry
	wend
end sub

'' end of backend/c/c-overaligned-locals.bi
