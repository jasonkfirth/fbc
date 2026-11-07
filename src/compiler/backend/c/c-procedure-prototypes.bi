'' Project: FreeBASIC compiler - C procedure declarations
'' File: backend/c/c-procedure-prototypes.bi
'' Purpose: Track declarations already emitted in the C translation unit.
'' Responsibilities: Keep owned names and exact declaration text per module.
'' This file intentionally does NOT contain: General ABI compatibility inference for different declarations.

type CPROTOTYPEENTRY as CPROTOTYPEENTRY_
type CPROTOTYPEENTRY_
	name as zstring ptr
	declaration as zstring ptr
	next_same as CPROTOTYPEENTRY ptr
end type

dim shared as TLIST c_prototype_entries
dim shared as THASH c_prototype_names
dim shared as integer c_prototype_ready
dim shared as integer c_prototype_startup_on_header

private sub hCPrototypeFinish( )
	if( c_prototype_ready = FALSE ) then exit sub
	hashEnd( @c_prototype_names )
	var entry = cptr(CPROTOTYPEENTRY ptr, listGetHead(@c_prototype_entries))
	while( entry <> NULL )
		ZstrFree( entry->name )
		ZstrFree( entry->declaration )
		entry = listGetNext( entry )
	wend
	listEnd( @c_prototype_entries )
	c_prototype_ready = FALSE
	c_prototype_startup_on_header = FALSE
end sub

private sub hCPrototypeInit( )
	hCPrototypeFinish( )
	listInit( @c_prototype_entries, 32, sizeof(CPROTOTYPEENTRY), LIST_FLAGS_CLEAR )
	hashInit( @c_prototype_names, 128, FALSE )
	c_prototype_ready = TRUE
end sub

private function hCPrototypeHasName( byref identifier as string ) as integer
	if( c_prototype_ready = FALSE or len(identifier) = 0 ) then return FALSE
	return (hashLookup(@c_prototype_names, strptr(identifier)) <> NULL)
end function

private function hCPrototypeMatches _
	( byref identifier as string, byref declaration as string ) as integer

	if( c_prototype_ready = FALSE or len(identifier) = 0 ) then return FALSE
	var entry = cptr(CPROTOTYPEENTRY ptr, hashLookup(@c_prototype_names, strptr(identifier)))
	while( entry <> NULL )
		if( *entry->declaration = declaration ) then return TRUE
		entry = entry->next_same
	wend
	return FALSE
end function

private sub hCPrototypeRemember _
	( byref identifier as string, byref declaration as string )

	if( c_prototype_ready = FALSE or len(identifier) = 0 ) then exit sub
	if( hCPrototypeMatches(identifier, declaration) ) then exit sub
	var first = cptr(CPROTOTYPEENTRY ptr, hashLookup(@c_prototype_names, strptr(identifier)))
	var entry = cptr(CPROTOTYPEENTRY ptr, listNewNode(@c_prototype_entries))
	if( entry = NULL ) then exit sub
	entry->name = ZstrDup( strptr(identifier) )
	entry->declaration = ZstrDup( strptr(declaration) )
	if( entry->name = NULL or entry->declaration = NULL ) then
		if( entry->name <> NULL ) then ZstrFree( entry->name )
		if( entry->declaration <> NULL ) then ZstrFree( entry->declaration )
		listDelNode( @c_prototype_entries, entry )
		exit sub
	end if
	if( first = NULL ) then
		hashAdd( @c_prototype_names, entry->name, entry, hashHash(entry->name) )
	else
		entry->next_same = first->next_same
		first->next_same = entry
	end if
end sub

'' An explicit declaration may name the compiler's implicit entry object.
'' Its argv spelling otherwise uses an equivalent alignment typedef. Canonicalize
'' only exact BASIC/native identities; incompatible aliases still reach C.
private function hCPrototypeCanonicalEntry(byval proc as FBSYMBOL ptr) as FBSYMBOL ptr
	var entry = env.main.proc
	if( entry = NULL orelse proc = entry ) then return proc
	if( symbGetIsMainProc(entry) = FALSE ) then return proc
	if( symbIsPrivate(proc) or symbGetIsFuncPtr(proc) or symbIsMethod(proc) or symbIsReturnByref(proc) ) then return proc
	if( symbGetFullType(proc) <> symbGetFullType(entry) or symbGetSubtype(proc) <> symbGetSubtype(entry) ) then return proc
	if( symbGetProcRealType(proc) <> symbGetProcRealType(entry) or symbGetProcRealSubtype(proc) <> symbGetProcRealSubtype(entry) ) then return proc
	if( symbGetProcMode(proc) <> FB_FUNCMODE_CDECL or symbGetProcMode(entry) <> FB_FUNCMODE_CDECL ) then return proc
	if( symbGetProcParams(proc) <> 2 or symbGetProcParams(entry) <> 2 ) then return proc
	var proc_name = symbGetMangledName(proc)
	var entry_name = symbGetMangledName(entry)
	if( proc_name = NULL or entry_name = NULL ) then return proc
	if( *proc_name <> *entry_name ) then return proc
	if( hGetCId(proc) <> hGetCId(entry) ) then return proc
	var param = symbGetProcHeadParam(proc)
	var entry_param = symbGetProcHeadParam(entry)
	while( param <> NULL and entry_param <> NULL )
		if( symbGetParamMode(param) <> FB_PARAMMODE_BYVAL or symbGetParamMode(entry_param) <> FB_PARAMMODE_BYVAL ) then return proc
		if( symbGetFullType(param) <> symbGetFullType(entry_param) or symbGetSubtype(param) <> symbGetSubtype(entry_param) ) then return proc
		param = param->next
		entry_param = entry_param->next
	wend
	if( param <> NULL or entry_param <> NULL ) then return proc
	return entry
end function

'' end of backend/c/c-procedure-prototypes.bi
