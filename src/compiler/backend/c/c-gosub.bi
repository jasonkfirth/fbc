'' Project: FreeBASIC C backend
'' File: backend/c/c-gosub.bi
'' Purpose: Lower procedure-local GOSUB calls without setjmp/longjmp.
'' Responsibilities: Allocate return identifiers, retain runtime API roots and
''     dispatch RETURN within the same C activation.
'' This file intentionally does NOT change ERROR continuation addresses.

'' State belongs to one emitted procedure. Runtime context storage remains the
'' compiler's existing one-pointer GOSUB symbol, including recursive calls.
dim shared as integer c_gosub_enabled, c_gosub_sites, c_gosub_returns, c_gosub_has_returns
dim shared as string c_gosub_context

private sub hCGosubInit( )
	c_gosub_enabled = FALSE
	c_gosub_sites = 0
	c_gosub_returns = FALSE
	c_gosub_has_returns = FALSE
	c_gosub_context = ""
end sub

private sub hCGosubBegin( byval proc as FBSYMBOL ptr, byval body as ASTNODE ptr = NULL )
	hCGosubInit( )
	if( symbIsNaked(proc) ) then exit sub
	if( symbGetProcStatGosub(proc) = FALSE ) then exit sub
	c_gosub_enabled = TRUE
	'' Outlined handlers borrow the same owner context, which may live
	'' in captured storage. Each emitted C function owns its dispatch labels.
	var symbol = symbGetProcGosubSym(proc)
	if( hCErrCapture(symbol) >= 0 ) then
		c_gosub_context = hCErrSymbolText(symbol)
	else
		c_gosub_context = hGetCId(symbol)
	end if
	'' GOSUB RETURN nodes are top-level statements in the completed body.
	'' RETURN label only discards a frame, so it needs no return-site label.
	var statement = body
	if( statement = NULL ) then statement = astGetProc()->l
	while( statement <> NULL )
		if( statement->class = AST_NODECLASS_BRANCH andalso statement->op.op = AST_OP_RET andalso statement->op.ex = NULL ) then
			c_gosub_has_returns = TRUE
		end if
		statement = statement->next
	wend
end sub

private function hCGosubAPI( byval spelling as const zstring ptr, byval index as integer ) as string
	var proc = rtlProcLookup(spelling, index)
	if( proc = NULL ) then
		errReportEx(FB_ERRMSG_INTERNAL, "C GOSUB runtime declaration")
		return ""
	end if
	symbSetIsAccessed(proc)
	var identifier = hGetCId(proc)
	hCLeafNativeReference(identifier)
	return identifier
end function

private sub hCGosubCall( byval label as FBSYMBOL ptr )
	assert(c_gosub_enabled)
	'' Identifiers use the same fixed 32-bit protocol on every target.
	'' Keep the compiler's signed counter below that common limit as well.
	if( c_gosub_sites = &h7fffffff ) then
		errReportEx(FB_ERRMSG_INTERNAL, "too many C GOSUB return sites")
		exit sub
	end if
	c_gosub_sites += 1
	var push_name = hCGosubAPI(@FB_RTL_GOSUBPUSHTARGET, FB_RTL_IDX_GOSUBPUSHTARGET)
	var end_name = hCGosubAPI(@FB_RTL_END, FB_RTL_IDX_END)
	hWriteLine("{")
	hWriteLine("int32 fb_c_gosub_status = " + push_name + "(&" + c_gosub_context + ", " + str(c_gosub_sites) + "u);")
	hWriteLine("if (fb_c_gosub_status != 0) " + end_name + "(fb_c_gosub_status);")
	hWriteLine("}")
	hWriteLine("goto " + hGetCId(label) + ";")
	if( c_gosub_has_returns ) then hWriteLine("fb_c_gosub_return_" + str(c_gosub_sites) + ":;")
end sub

private sub _emitReturn( byval bytestopop as integer )
	'' This IR operation is the local GOSUB return, not a C function return.
	assert(bytestopop = 0)
	assert(c_gosub_enabled)
	c_gosub_returns = TRUE
	var return_name = hCGosubAPI(@FB_RTL_GOSUBRETURNTARGET, FB_RTL_IDX_GOSUBRETURNTARGET)
	hWriteLine("fb_c_gosub_target = " + return_name + "(&" + c_gosub_context + ");")
	hWriteLine("goto fb_c_gosub_dispatch;")
end sub

private sub hCGosubEnd( )
	if( c_gosub_returns = FALSE ) then exit sub
	var end_name = hCGosubAPI(@FB_RTL_END, FB_RTL_IDX_END)
	var section = sectionGosub(2)
	hWriteLine("uint32 fb_c_gosub_target = 0;")
	hCErrMoveOwnerPrologue()
	'' Header placement also covers void fallthrough and non-returning bodies.
	hWriteLine("goto fb_c_gosub_body;")
	hWriteLine("fb_c_gosub_dispatch:;")
	hWriteLine("switch (fb_c_gosub_target) {")
	for site as integer = 1 to c_gosub_sites
		hWriteLine("case " + str(site) + "u: goto fb_c_gosub_return_" + str(site) + ";")
	next
	hWriteLine("default: " + end_name + "(" + str(FB_RTERROR_RETURNWITHOUTGOSUB) + ");")
	hWriteLine("}")
	hWriteLine("fb_c_gosub_body:;")
	sectionReturn(section)
end sub

'' end of backend/c/c-gosub.bi
