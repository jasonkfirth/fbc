/'
	Project: FreeBASIC C backend
	File: backend/c/c-integer-abs.bi
	Purpose: Emit integer ABS with BASIC's width and bit-pattern semantics.
	Responsibilities: Typed single-evaluation helpers, unsigned modular magnitude
	and signed-minimum wrap results without signed C negation.
	This file intentionally does NOT contain: Floating ABS or 128-bit lowering.
'/

private function hCIntegerAbsName(byval dtype as integer) as const zstring ptr
	select case as const typeGetSizeType(dtype)
	case FB_SIZETYPE_INT8
		ctx.usedbuiltins or= BUILTIN_ABS_I8 : return @"fb_c_abs_i8"
	case FB_SIZETYPE_UINT8
		ctx.usedbuiltins or= BUILTIN_ABS_U8 : return @"fb_c_abs_u8"
	case FB_SIZETYPE_INT16
		ctx.usedbuiltins or= BUILTIN_ABS_I16 : return @"fb_c_abs_i16"
	case FB_SIZETYPE_UINT16
		ctx.usedbuiltins or= BUILTIN_ABS_U16 : return @"fb_c_abs_u16"
	case FB_SIZETYPE_INT32
		ctx.usedbuiltins or= BUILTIN_ABS_I32 : return @"fb_c_abs_i32"
	case FB_SIZETYPE_UINT32
		ctx.usedbuiltins or= BUILTIN_ABS_U32 : return @"fb_c_abs_u32"
	case FB_SIZETYPE_INT64
		ctx.usedbuiltins or= BUILTIN_ABS_I64 : return @"fb_c_abs_i64"
	case FB_SIZETYPE_UINT64
		ctx.usedbuiltins or= BUILTIN_ABS_U64 : return @"fb_c_abs_u64"
	end select
	return NULL
end function

private sub hWriteIntegerAbs(byval dtype as integer)
	var typename = hEmitType(dtype, NULL)
	var unsignedtype = hEmitType(typeToUnsigned(dtype), NULL)
	var identifier = *hCIntegerAbsName(dtype)
	var size = typeGetSize(dtype)
	var zero = iif(size = 8, "0ull", "0u")
	hWriteLine("static inline " + typename + " " + identifier + "(" + typename + " value)", TRUE)
	hWriteLine("{", TRUE)
	if( typeIsSigned(dtype) ) then
		'' Keep magnitude arithmetic unsigned, then preserve the resulting bits.
		'' In particular, ABS of the signed minimum retains that minimum value.
		hWriteLine(unsignedtype + " bits = (" + unsignedtype + ")value;", TRUE)
		hWriteLine("if (value < 0) bits = (" + unsignedtype + ")(" + zero + " - bits);", TRUE)
		hWriteLine(typename + " result;", TRUE)
		hWriteLine("__builtin_memcpy(&result, &bits, sizeof(result));", TRUE)
		hWriteLine("return result;", TRUE)
	elseif( size >= 4 ) then
		'' The constant evaluator interprets 32/64-bit unsigned operands through
		'' the signed type of the same width. Smaller unsigned values fit LONG.
		var signbit = iif(size = 8, "0x8000000000000000ull", "0x80000000u")
		hWriteLine("if (value >= " + signbit + ") return " + zero + " - value;", TRUE)
		hWriteLine("return value;", TRUE)
	else
		hWriteLine("return value;", TRUE)
	end if
	hWriteLine("}", TRUE)
end sub

/' end of backend/c/c-integer-abs.bi '/
