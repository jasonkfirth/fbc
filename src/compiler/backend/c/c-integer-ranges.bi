/'
	Project: FreeBASIC C backend
	File: backend/c/c-integer-ranges.bi
	Purpose: Fold comparisons determined by scalar integer ranges.
	Responsibilities: Track value-preserving casts, apply C integer promotions,
	and retain the source tree's boolean and conditional-branch conventions.
	This file intentionally does NOT contain: pointer or floating-point proofs,
	indirect-load elimination, value propagation between statements, or lint flags.
'/

'' -------------------------------------------------------------------------
'' Integer bounds and conversions
'' -------------------------------------------------------------------------

type CRANGEBOUND
	negative as integer
	magnitude as ulongint
end type

type CINTEGERRANGE
	lower as CRANGEBOUND
	upper as CRANGEBOUND
end type

'' Signed magnitude covers INT64_MIN through UINT64_MAX without signed overflow
'' or a native __int128 dependency in the compiler itself. Zero is nonnegative.
private function hCRangeCompare(byref a as CRANGEBOUND, byref b as CRANGEBOUND) as integer
	if( a.negative <> b.negative ) then return iif(a.negative, -1, 1)
	dim as integer order = 0
	if( a.magnitude < b.magnitude ) then order = -1
	if( a.magnitude > b.magnitude ) then order = 1
	return iif(a.negative, -order, order)
end function

private function hCRangeType(byval dtype as integer) as integer
	select case as const typeGetDtAndPtrOnly(dtype)
	case FB_DATATYPE_BYTE, FB_DATATYPE_UBYTE, FB_DATATYPE_SHORT, FB_DATATYPE_USHORT, _
	     FB_DATATYPE_LONG, FB_DATATYPE_ULONG, FB_DATATYPE_INTEGER, FB_DATATYPE_UINT, _
	     FB_DATATYPE_LONGINT, FB_DATATYPE_ULONGINT
		return TRUE
	end select
	return FALSE
end function

private function hCRangeMask(byval bits as integer) as ulongint
	if( bits = 64 ) then return &hffffffffffffffffull
	return (1ull shl bits) - 1ull
end function

private sub hCRangeFull(byval dtype as integer, byref interval as CINTEGERRANGE)
	interval.lower.negative = FALSE
	interval.lower.magnitude = 0
	interval.upper.negative = FALSE
	if( typeIsSigned(dtype) ) then
		interval.lower.negative = TRUE
		interval.lower.magnitude = 1ull shl (typeGetBits(dtype) - 1)
		interval.upper.magnitude = interval.lower.magnitude - 1ull
	else
		interval.upper.magnitude = hCRangeMask(typeGetBits(dtype))
	end if
end sub

private function hCRangeConvert(byref interval as CINTEGERRANGE, byval dtype as integer) as integer
	dim as CINTEGERRANGE target
	hCRangeFull(dtype, target)
	if( hCRangeCompare(interval.lower, target.lower) >= 0 andalso _
	    hCRangeCompare(interval.upper, target.upper) <= 0 ) then return TRUE
	'' Out-of-range conversion to a signed type is implementation-defined.
	'' Do not turn that conversion into a portable range proof.
	if( typeIsSigned(dtype) ) then return FALSE
	if( hCRangeCompare(interval.lower, interval.upper) = 0 ) then
		var magnitude = interval.lower.magnitude
		if( interval.lower.negative ) then magnitude = 0ull - magnitude
		interval.lower.negative = FALSE
		interval.lower.magnitude = magnitude and hCRangeMask(typeGetBits(dtype))
		interval.upper = interval.lower
	else
		'' Unsigned casts wrap modulo 2^width. A wrapped interval may be split;
		'' retaining the complete destination range is conservative in that case.
		interval = target
	end if
	return TRUE
end function

'' -------------------------------------------------------------------------
'' Operand ranges and native C promotions
'' -------------------------------------------------------------------------

private function hCRangeConstant(byval dtype as integer, byval value as longint, byref interval as CINTEGERRANGE) as integer
	interval.lower.negative = FALSE
	interval.lower.magnitude = culngint(value)
	if( typeIsSigned(dtype) ) then
		if( value < 0 ) then
			interval.lower.negative = TRUE
			interval.lower.magnitude = 0ull - culngint(value)
		end if
		dim as CINTEGERRANGE declared_range
		hCRangeFull(dtype, declared_range)
		if( hCRangeCompare(interval.lower, declared_range.lower) < 0 orelse _
		    hCRangeCompare(interval.lower, declared_range.upper) > 0 ) then return FALSE
	else
		interval.lower.magnitude and= hCRangeMask(typeGetBits(dtype))
	end if
	interval.upper = interval.lower
	return TRUE
end function

private function hCRangeValue(byval n as ASTNODE ptr, byref interval as CINTEGERRANGE, byval retain_operands as integer = FALSE) as integer
	if( n = NULL orelse n->vector <> 0 ) then return FALSE
	if( hCRangeType(n->dtype) = FALSE ) then return FALSE
	select case as const n->class
	case AST_NODECLASS_CONST
		return hCRangeConstant(n->dtype, n->val.i, interval)
	case AST_NODECLASS_VAR
		var sym = n->sym
		if( sym = NULL orelse symbIsVar(sym) = FALSE ) then return FALSE
		if( retain_operands ) then
			hCRangeFull(n->dtype, interval)
			return TRUE
		end if
		if( symbIsRef(sym) or symbIsParamVarBydescOrByref(sym) ) then return FALSE
		if( symbIsThreadLocal(sym) or symbIsThreadLocalC(sym) ) then return FALSE
		if( symbGetArrayDimensions(sym) <> 0 orelse n->var_.ofs <> 0 ) then return FALSE
		if( typeGetDtAndPtrOnly(symbGetFullType(sym)) <> typeGetDtAndPtrOnly(n->dtype) ) then return FALSE
		hCRangeFull(n->dtype, interval)
		return TRUE
	case AST_NODECLASS_FIELD, AST_NODECLASS_DEREF, AST_NODECLASS_IDX
		'' Range-only callers must retain the complete load and its address.
		if( retain_operands = FALSE ) then return FALSE
		hCRangeFull(n->dtype, interval)
		return TRUE
	case AST_NODECLASS_CONV
		if( n->r <> NULL ) then return FALSE
		if( hCRangeValue(n->l, interval, retain_operands) = FALSE ) then return FALSE
		return hCRangeConvert(interval, n->dtype)
	case AST_NODECLASS_LINK
		'' A nested folded comparison retains reads on the left and yields
		'' its fixed value on the right. Preserve that whole sequence again
		'' if an enclosing comparison can also be resolved.
		if( retain_operands = FALSE or n->link.ret <> AST_LINK_RETURN_RIGHT ) then return FALSE
		return hCRangeValue(n->r, interval, TRUE)
	end select
	'' Calls, opaque text, dereferences, and other address computations retain
	'' their evaluation even if their apparent result type has a limited range.
	return FALSE
end function

private function hCRangeCommonType(byval ldtype as integer, byval rdtype as integer) as integer
	dim as integer lbits = typeGetBits(ldtype)
	dim as integer rbits = typeGetBits(rdtype)
	dim as integer lsigned = typeIsSigned(ldtype)
	dim as integer rsigned = typeIsSigned(rdtype)
	'' The emitted int8/int16 types promote to signed int32, including uint16.
	'' Thereafter a wider signed type can represent every value of uint32;
	'' equal-width signed/unsigned operands instead select the unsigned type.
	if( lbits < 32 ) then
		lbits = 32
		lsigned = TRUE
	end if
	if( rbits < 32 ) then
		rbits = 32
		rsigned = TRUE
	end if
	var bits = iif(lbits > rbits, lbits, rbits)
	var is_signed = lsigned
	if( lsigned <> rsigned ) then
		if( lsigned ) then
			is_signed = (lbits > rbits)
		else
			is_signed = (rbits > lbits)
		end if
	end if
	if( bits = 64 ) then return iif(is_signed, FB_DATATYPE_LONGINT, FB_DATATYPE_ULONGINT)
	return iif(is_signed, FB_DATATYPE_LONG, FB_DATATYPE_ULONG)
end function

private function hCRangeIntervalsTruth _
	( byval op as integer, byval ldtype as integer, byval rdtype as integer, _
	  byref left_range as CINTEGERRANGE, byref right_range as CINTEGERRANGE, _
	  byref truth as integer ) as integer
	var dtype = hCRangeCommonType(ldtype, rdtype)
	if( hCRangeConvert(left_range, dtype) = FALSE ) then return FALSE
	if( hCRangeConvert(right_range, dtype) = FALSE ) then return FALSE
	var lower_upper = hCRangeCompare(left_range.lower, right_range.upper)
	var upper_lower = hCRangeCompare(left_range.upper, right_range.lower)
	select case as const op
	case AST_OP_EQ, AST_OP_NE
		if( lower_upper > 0 or upper_lower < 0 ) then
			truth = FALSE
		elseif( lower_upper = 0 and upper_lower = 0 ) then
			truth = TRUE
		else
			return FALSE
		end if
		if( op = AST_OP_NE ) then truth = not truth
	case AST_OP_LT
		if( upper_lower < 0 ) then
			truth = TRUE
		elseif( lower_upper >= 0 ) then
			truth = FALSE
		else
			return FALSE
		end if
	case AST_OP_LE
		if( upper_lower <= 0 ) then
			truth = TRUE
		elseif( lower_upper > 0 ) then
			truth = FALSE
		else
			return FALSE
		end if
	case AST_OP_GT
		if( lower_upper > 0 ) then
			truth = TRUE
		elseif( upper_lower <= 0 ) then
			truth = FALSE
		else
			return FALSE
		end if
	case AST_OP_GE
		if( lower_upper >= 0 ) then
			truth = TRUE
		elseif( upper_lower < 0 ) then
			truth = FALSE
		else
			return FALSE
		end if
	case else
		return FALSE
	end select
	return TRUE
end function

private function hCRangeTruth(byval n as ASTNODE ptr, byref truth as integer, byval retain_operands as integer = FALSE) as integer
	dim as CINTEGERRANGE left_range, right_range
	if( hCRangeValue(n->l, left_range, retain_operands) = FALSE ) then return FALSE
	if( hCRangeValue(n->r, right_range, retain_operands) = FALSE ) then return FALSE
	if( hCRangeIntervalsTruth(n->op.op, n->l->dtype, n->r->dtype, left_range, right_range, truth) = FALSE ) then return FALSE
	if( n->op.options and AST_OPOPT_DOINVERSE ) then truth = not truth
	return TRUE
end function

'' end of backend/c/c-integer-ranges.bi
