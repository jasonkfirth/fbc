'' Project: FreeBASIC compiler - C wide-string calls
'' File: backend/c/c-wide-libc.bi
'' Purpose: Preserve BASIC pointer alignment when calling native wide-string APIs.
'' Responsibilities: Validate libc signatures and emit checked native/byte-copy paths.
'' This file intentionally does NOT contain: Procedure-address changes or locale conversion.

private function hCWideLibcName(byval proc as FBSYMBOL ptr) as string
	if( symbGetProcMode(proc) <> FB_FUNCMODE_CDECL ) then return ""
	if( symbGetIsParsed(proc) or symbGetIsFuncPtr(proc) ) then return ""
	var rawname = symbGetMangledName(proc)
	if( rawname = NULL ) then return ""
	var compare = (*rawname = "wcsncmp")
	var find_char = (*rawname = "wcschr")
	if( compare = FALSE andalso find_char = FALSE andalso *rawname <> "wcslen" ) then return ""
	if( symbGetProcParams(proc) <> iif(compare, 3, iif(find_char, 2, 1)) ) then return ""
	'' These are the native CRT wchar_t bindings, not BASIC's unsigned
	'' four-byte WSTRING representation or a user-defined same-name function.
	dim as integer unit_type = FB_DATATYPE_LONG
	select case env.clopt.target
	case FB_COMPTARGET_WIN32, FB_COMPTARGET_CYGWIN
		unit_type = FB_DATATYPE_USHORT
	case FB_COMPTARGET_DOS, FB_COMPTARGET_ANDROID
		return ""
	end select
	var result_type = typeGetDtAndPtrOnly(symbGetProcRealType(proc))
	if( result_type <> iif(compare, FB_DATATYPE_LONG, iif(find_char, typeAddrOf(unit_type), FB_DATATYPE_UINT)) ) then return ""
	if( find_char andalso typeIsConstAt(symbGetProcRealType(proc), 1) ) then return ""
	var param = symbGetProcHeadParam(proc)
	for index as integer = 0 to iif(compare, 2, iif(find_char, 1, 0))
		if( param = NULL orelse symbGetParamMode(param) <> FB_PARAMMODE_BYVAL ) then return ""
		var dtype = symbGetFullType(param)
		if( compare andalso index = 2 ) then
			if( typeGetDtAndPtrOnly(dtype) <> FB_DATATYPE_UINT ) then return ""
		elseif( find_char andalso index = 1 ) then
			if( typeGetDtAndPtrOnly(dtype) <> unit_type ) then return ""
		else
			if( typeGetDtAndPtrOnly(dtype) <> typeAddrOf(unit_type) ) then return ""
			if( typeIsConstAt(dtype, 1) = FALSE ) then return ""
		end if
		param = symbGetNext(param)
	next
	var identifier = "fb_c_" + *rawname
	if( hCPrototypeHasName(identifier) = FALSE ) then
		'' Declare the original entry first. The fast path still calls that
		'' entry, and taking its address must keep the native address identity.
		hMaybeEmitProcProto(proc)
		var section = sectionGosub(0)
		var first = ctx.sections(0).bufferlen
		var unit = identifier + "_unit"
		hWriteLine("typedef __WCHAR_TYPE__ " + unit + " __attribute__((aligned(1)));", TRUE)
		var header = "static inline " + iif(compare, "int ", iif(find_char, unit + "* ", "__SIZE_TYPE__ ")) + identifier + "(const " + unit + "* first"
		if( compare ) then header += ", const " + unit + "* second, __SIZE_TYPE__ count"
		if( find_char ) then header += ", __WCHAR_TYPE__ wanted"
		header += ")"
		hWriteLine(header, TRUE)
		hWriteLine("{", TRUE)
		hWriteLine("__UINTPTR_TYPE__ a = (__UINTPTR_TYPE__)first;", TRUE)
		if( compare ) then
			hWriteLine("__UINTPTR_TYPE__ b = (__UINTPTR_TYPE__)second;", TRUE)
		elseif( find_char = FALSE ) then
			hWriteLine("__SIZE_TYPE__ length = 0;", TRUE)
		end if
		var addresses = iif(compare, "(a | b)", "a")
		hWriteLine("if ((" + addresses + " & (__alignof__(__WCHAR_TYPE__) - 1u)) == 0)", TRUE)
		var fast_call = *rawname + "((const __WCHAR_TYPE__*)__builtin_assume_aligned(first, __alignof__(__WCHAR_TYPE__))"
		if( compare ) then fast_call += ", (const __WCHAR_TYPE__*)__builtin_assume_aligned(second, __alignof__(__WCHAR_TYPE__)), count"
		if( find_char ) then fast_call += ", wanted"
		hWriteLine("return " + fast_call + ");", TRUE)
		hWriteLine(iif(compare, "while (count != 0) {", "for (;;) {"), TRUE)
		hWriteLine("__WCHAR_TYPE__ left;", TRUE)
		if( compare ) then hWriteLine("__WCHAR_TYPE__ right;", TRUE)
		'' BASIC pointers are machine addresses. Keep their byte stepping in
		'' that integer domain and copy a complete unit into aligned storage.
		'' No native wide lvalue is formed at a possibly unaligned address.
		hWriteLine("__builtin_memcpy(&left, (const void*)a, sizeof(left));", TRUE)
		if( compare ) then
			hWriteLine("__builtin_memcpy(&right, (const void*)b, sizeof(right));", TRUE)
			hWriteLine("if (left != right) return left < right ? -1 : 1;", TRUE)
			hWriteLine("if (left == 0) return 0;", TRUE)
			hWriteLine("b += sizeof(right);", TRUE)
			hWriteLine("--count;", TRUE)
		elseif( find_char ) then
			'' The native API returns the original address, including when the
			'' sought unit is the terminator. Keep that address in packed storage.
			hWriteLine("if (left == wanted) return (" + unit + "*)a;", TRUE)
			hWriteLine("if (left == 0) return 0;", TRUE)
		else
			hWriteLine("if (left == 0) return length;", TRUE)
			hWriteLine("++length;", TRUE)
		end if
		hWriteLine("a += sizeof(left);", TRUE)
		hWriteLine("}", TRUE)
		if( compare ) then hWriteLine("return 0;", TRUE)
		hWriteLine("}", TRUE)
		hCPrototypeRemember(identifier, header + ";")
		hCLeafNativeSpan(identifier, TRUE, first, ctx.sections(0).bufferlen)
		sectionReturn(section)
	end if
	hCLeafNativeReference(identifier)
	return identifier
end function

'' end of backend/c/c-wide-libc.bi
