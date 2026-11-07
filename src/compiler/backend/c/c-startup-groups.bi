/'
Project: FreeBASIC C backend
File: backend/c/c-startup-groups.bi
Purpose: Share runtime startup registrations without changing source callback identities.
Responsibilities: Own per-unit groups, preserve priority/order and record wrapper calls.
This file intentionally does NOT reorder modules or replace NetBSD finalizer entries.
'/

type CSTARTUPCALL as CSTARTUPCALL_
type CSTARTUPCALL_
	proc as FBSYMBOL ptr
	identifier as zstring ptr
	previous_call as CSTARTUPCALL ptr
	next_call as CSTARTUPCALL ptr
end type

type CSTARTUPGROUP
	priority as integer
	is_dtor as integer
	count as integer
	has_noreturn as integer
	first_call as CSTARTUPCALL ptr
	last_call as CSTARTUPCALL ptr
end type

'' The compiler emits modules serially. These pools own copied C names until
'' the current unit is written; global procedure symbols remain borrowed.
dim shared as TLIST c_startup_groups, c_startup_calls
dim shared as integer c_startup_ready

private sub hCStartupFinish()
	if( c_startup_ready = FALSE ) then exit sub
	var entry = cptr(CSTARTUPCALL ptr, listGetHead(@c_startup_calls))
	while( entry <> NULL )
		ZstrFree(entry->identifier)
		entry = listGetNext(entry)
	wend
	listEnd(@c_startup_calls)
	listEnd(@c_startup_groups)
	c_startup_ready = FALSE
end sub

private sub hCStartupInit()
	hCStartupFinish()
	listInit(@c_startup_groups, 8, sizeof(CSTARTUPGROUP), LIST_FLAGS_CLEAR)
	listInit(@c_startup_calls, 32, sizeof(CSTARTUPCALL), LIST_FLAGS_CLEAR)
	c_startup_ready = TRUE
end sub

private function hCStartupGrouped(byval proc as FBSYMBOL ptr) as integer
	if( c_startup_ready = FALSE or proc = NULL ) then return FALSE
	'' MinGW ordering is qualified for GCC and Clang. Other loaders
	'' keep their original per-callback registrations.
	if( env.clopt.target <> FB_COMPTARGET_WIN32 ) then return FALSE
	if( (proc->stats and (FB_SYMBSTATS_GLOBALCTOR or FB_SYMBSTATS_GLOBALDTOR)) = 0 ) then return FALSE
	if( symbIsNaked(proc) ) then return FALSE
	return TRUE
end function

private function hCStartupPriority(byval proc as FBSYMBOL ptr) as integer
	var priority = symbGetProcPriority(proc)
	'' GNU startup attributes use 65535 when no priority is written.
	if( priority = 0 ) then priority = 65535
	return priority
end function

private function hCStartupToken(byval is_dtor as integer, byval priority as integer) as string
	return "fb_c_startup_attr_" + iif(is_dtor, "d_", "c_") + str(priority)
end function

private function hCStartupWrapper(byval startup_group as CSTARTUPGROUP ptr) as string
	return "fb_c_startup_" + iif(startup_group->is_dtor, "d_", "c_") + str(startup_group->priority)
end function

private function hCStartupAttribute(byval startup_group as CSTARTUPGROUP ptr) as string
	var result = "__attribute__((" + iif(startup_group->is_dtor, "destructor", "constructor")
	if( startup_group->priority <> 65535 ) then result += "(" + str(startup_group->priority) + ")"
	return result + "))"
end function

private function hCStartupCombine(byval startup_group as CSTARTUPGROUP ptr) as integer
	'' Keep singleton registrations unchanged. A non-returning callback must
	'' also keep its original registration: grouping later callbacks would
	'' introduce unreachable calls and obscure their startup observations.
	return (startup_group->count > 1) andalso (startup_group->has_noreturn = FALSE)
end function

private function hCStartupAttributeToken(byval proc as FBSYMBOL ptr) as string
	return hCStartupToken(symbGetIsGlobalDtor(proc), hCStartupPriority(proc))
end function

private sub hCStartupAdd(byval proc as FBSYMBOL ptr)
	if( hCStartupGrouped(proc) = FALSE ) then exit sub
	var entry = cptr(CSTARTUPCALL ptr, listGetHead(@c_startup_calls))
	while( entry <> NULL )
		if( entry->proc = proc ) then exit sub
		entry = listGetNext(entry)
	wend
	var priority = hCStartupPriority(proc)
	var is_dtor = symbGetIsGlobalDtor(proc)
	var startup_group = cptr(CSTARTUPGROUP ptr, listGetHead(@c_startup_groups))
	while( startup_group <> NULL )
		if( startup_group->priority = priority and startup_group->is_dtor = is_dtor ) then exit while
		startup_group = listGetNext(startup_group)
	wend
	if( startup_group = NULL ) then
		startup_group = listNewNode(@c_startup_groups)
		if( startup_group = NULL ) then
			errReportEx(FB_ERRMSG_INTERNAL, "C startup group allocation")
			exit sub
		end if
		startup_group->priority = priority
		startup_group->is_dtor = is_dtor
	end if
	entry = listNewNode(@c_startup_calls)
	if( entry = NULL ) then
		errReportEx(FB_ERRMSG_INTERNAL, "C startup call allocation")
		exit sub
	end if
	entry->proc = proc
	var identifier = hGetCId(proc)
	entry->identifier = ZstrDup(strptr(identifier))
	if( entry->identifier = NULL ) then
		errReportEx(FB_ERRMSG_INTERNAL, "C startup name allocation")
		exit sub
	end if
	entry->previous_call = startup_group->last_call
	if( startup_group->last_call <> NULL ) then
		startup_group->last_call->next_call = entry
	else
		startup_group->first_call = entry
	end if
	startup_group->last_call = entry
	startup_group->count += 1
	startup_group->has_noreturn or= hProcDoesNotReturn(proc)
end sub

private sub hCStartupWriteCalls(byval startup_group as CSTARTUPGROUP ptr, byval reverse_order as integer)
	var entry = iif(reverse_order, startup_group->last_call, startup_group->first_call)
	while( entry <> NULL )
		hWriteLine(*entry->identifier + "();", TRUE)
		entry = iif(reverse_order, entry->previous_call, entry->next_call)
	wend
end sub

private sub hCStartupEmit()
	var startup_group = cptr(CSTARTUPGROUP ptr, listGetHead(@c_startup_groups))
	while( startup_group <> NULL )
		var grouped = hCStartupCombine(startup_group)
		var wrapper_name = hCStartupWrapper(startup_group)
		if( grouped ) then
			hWriteLine("static void " + wrapper_name + "(void) " + hCStartupAttribute(startup_group) + ";", TRUE)
			hWriteLine("static void " + wrapper_name + "(void)", TRUE)
			hWriteLine("{", TRUE)
			sectionIndent()
			'' GCC emits source-order MinGW .ctors entries and its CRT walks
			'' backwards. Clang reverses those entries before the CRT sees them.
			'' Other init_array targets use forward constructors/reverse finalizers.
			hWriteLine("#if defined(_WIN32) && !defined(__clang__)", TRUE)
			hCStartupWriteCalls(startup_group, (startup_group->is_dtor = FALSE))
			hWriteLine("#else", TRUE)
			hCStartupWriteCalls(startup_group, startup_group->is_dtor)
			hWriteLine("#endif", TRUE)
			sectionUnindent()
			hWriteLine("}", TRUE)
		end if
		var entry = startup_group->first_call
		while( entry <> NULL )
			var kind = iif(startup_group->is_dtor, "destructor", "constructor")
			var priority = symbGetProcPriority(entry->proc)
			if( grouped ) then
				fbSemanticModelStartupEmission(entry->proc, kind, "c", "wrapper-call", _
					iif(priority = 0, "", str(priority)), wrapper_name, *entry->identifier + "()")
			else
				fbSemanticModelStartupEmission(entry->proc, kind, "c", "declaration-attribute", _
					iif(priority = 0, "", str(priority)), "function-attribute", _
					hCStartupAttribute(startup_group))
			end if
			entry = entry->next_call
		wend
		startup_group = listGetNext(startup_group)
	wend
end sub

private function hCStartupWritePrelude() as integer
	var startup_group = cptr(CSTARTUPGROUP ptr, listGetHead(@c_startup_groups))
	while( startup_group <> NULL )
		var attribute = hCStartupAttribute(startup_group)
		if( hCStartupCombine(startup_group) ) then
			'' Registration used to retain every callback's object symbol.
			'' Keep that property when direct wrapper calls can be inlined.
			attribute = "__attribute__((used))"
		end if
		var line_text = "#define " + hCStartupToken(startup_group->is_dtor, startup_group->priority) + " " + attribute + NEWLINE
		if( hCAbiWriteChunk(cptr(const ubyte ptr, strptr(line_text)), len(line_text)) = FALSE ) then return FALSE
		startup_group = listGetNext(startup_group)
	wend
	return TRUE
end function

'' end of backend/c/c-startup-groups.bi
