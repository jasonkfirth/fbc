/'
	Project: FreeBASIC C backend
	File: backend/c/c-format-results.bi
	Purpose: Preserve literal format contracts across native translation calls.
	Responsibilities: Recognize libc translation signatures, retain their literal
	arguments and carry native format checking through evaluated result registers.
	This file intentionally does NOT contain: BASIC grammar or translation execution.
'/

dim shared as integer c_format_result_emitted

private sub hCFormatResultsInit()
	c_format_result_emitted = FALSE
end sub

private function hCFormatArgument(byval proc as FBSYMBOL ptr, byref is_plural as integer) as integer
	if( symbGetProcMode(proc) <> FB_FUNCMODE_CDECL ) then return -1
	if( symbGetIsParsed(proc) or symbGetIsFuncPtr(proc) ) then return -1
	var dtype = symbGetProcRealType(proc)
	if( typeGetPtrCnt(dtype) <> 1 orelse typeGetDtOnly(dtype) <> FB_DATATYPE_CHAR ) then return -1
	if( typeIsConstAt(dtype, 1) ) then return -1
	var identifier = symbGetMangledName(proc)
	if( identifier = NULL ) then return -1
	dim as integer first, count, number_index = -1, category_index = -1
	is_plural = FALSE
	select case *identifier
	case "gettext"
		first = 0: count = 1
	case "dgettext"
		first = 1: count = 2
	case "dcgettext"
		first = 1: count = 3
		category_index = 2
	case "ngettext"
		first = 0: count = 3
		number_index = 2
	case "dngettext"
		first = 1: count = 4
		number_index = 3
	case "dcngettext"
		first = 1: count = 5
		number_index = 3: category_index = 4
	case else
		return -1
	end select
	if( symbGetProcParams(proc) <> count ) then return -1
	'' Plural counts use C unsigned long. The 64-bit Unix targets are LP64,
	'' while Windows keeps a 32-bit C long on both pointer widths.
	var count_size = iif(fbIs64bit() andalso (env.target.options and FB_TARGETOPT_UNIX), 8, 4)
	var param = symbGetProcHeadParam(proc)
	for i as integer = 0 to count - 1
		if( param = NULL orelse symbGetParamMode(param) <> FB_PARAMMODE_BYVAL ) then return -1
		dtype = symbGetFullType(param)
		if( i = category_index ) then
			'' The category has the native signed C-int ABI.
			if( typeIsPtr(dtype) orelse typeGetClass(dtype) <> FB_DATACLASS_INTEGER ) then return -1
			if( typeGetSize(dtype) <> 4 orelse typeIsSigned(dtype) = FALSE ) then return -1
		elseif( i = number_index ) then
			if( typeIsPtr(dtype) orelse typeGetClass(dtype) <> FB_DATACLASS_INTEGER ) then return -1
			if( typeGetSize(dtype) <> count_size orelse typeIsSigned(dtype) ) then return -1
		else
			if( typeGetPtrCnt(dtype) <> 1 orelse typeGetDtOnly(dtype) <> FB_DATATYPE_CHAR ) then return -1
		end if
		param = symbGetNext(param)
	next
	is_plural = (number_index >= 0)
	return first
end function

private sub hCFormatResult _
	( byval vr as IRVREG ptr, byval literal as FBSYMBOL ptr, byval plural as FBSYMBOL ptr )
	if( vr = NULL orelse literal = NULL ) then exit sub
	if( irIsREG(vr) = FALSE ) then exit sub
	var value = exprLookup(vr->reg)
	assert(value <> NULL)
	var helper = "fb_FormatResult"
	if( c_format_result_emitted = FALSE ) then
		'' libc's gettext functions retain the argument types of their source
		'' format. GCC checks that contract through a format_arg call expression,
		'' but loses it when the evaluated translation is stored in a register.
		'' This identity helper carries the original literal beside that value.
		'' Its format argument is an address, so it re-evaluates no source call.
		var section = sectionGosub(0)
		var first = ctx.sections(0).bufferlen
		hWriteLine("static inline char* " + helper + "(char* value, const char* format) __attribute__((format_arg(2)));")
		hWriteLine("static inline char* " + helper + "(char* value, const char* format)")
		hWriteLine("{ (void)format; return value; }")
		hCLeafNativeSpan(helper, TRUE, first, ctx.sections(0).bufferlen)
		sectionReturn(section)
		c_format_result_emitted = TRUE
	end if
	hCLeafNativeReference(helper)
	var evaluated = exprFlush(value)
	var format = hCLiteralStorageName(literal)
	if( plural <> NULL ) then
		'' Clang checks only one of multiple format_arg attributes. Present both
		'' original contracts as a conditional format argument to the same helper.
		'' Its selector reads the evaluated result register, so no translation,
		'' count expression or source argument is evaluated again. The helper
		'' returns that exact result and never uses the selected literal at runtime.
		format = "(" + evaluated + " ? " + format + " : " + hCLiteralStorageName(plural) + ")"
	end if
	var text = helper + "(" + evaluated + ", " + format + ")"
	var result = exprNewTEXT(vr->dtype, vr->subtype, text)
	result->format_literal = literal
	result->format_plural = plural
	exprCache(vr->reg, result)
end sub

/' end of backend/c/c-format-results.bi '/
