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

private function hCRangeValue(byval n as ASTNODE ptr, byref interval as CINTEGERRANGE) as integer
	if( n = NULL orelse n->vector <> 0 ) then return FALSE
	if( hCRangeType(n->dtype) = FALSE ) then return FALSE
	select case as const n->class
	case AST_NODECLASS_CONST
		interval.lower.negative = FALSE
		interval.lower.magnitude = culngint(n->val.i)
		if( typeIsSigned(n->dtype) ) then
			if( n->val.i < 0 ) then
				interval.lower.negative = TRUE
				interval.lower.magnitude = 0ull - culngint(n->val.i)
			end if
			dim as CINTEGERRANGE declared_range
			hCRangeFull(n->dtype, declared_range)
			if( hCRangeCompare(interval.lower, declared_range.lower) < 0 orelse _
			    hCRangeCompare(interval.lower, declared_range.upper) > 0 ) then return FALSE
		else
			interval.lower.magnitude and= hCRangeMask(typeGetBits(n->dtype))
		end if
		interval.upper = interval.lower
		return TRUE
	case AST_NODECLASS_VAR
		var sym = n->sym
		if( sym = NULL orelse symbIsVar(sym) = FALSE ) then return FALSE
		if( symbIsRef(sym) or symbIsParamVarBydescOrByref(sym) ) then return FALSE
		if( symbIsThreadLocal(sym) or symbIsThreadLocalC(sym) ) then return FALSE
		if( symbGetArrayDimensions(sym) <> 0 orelse n->var_.ofs <> 0 ) then return FALSE
		if( typeGetDtAndPtrOnly(symbGetFullType(sym)) <> typeGetDtAndPtrOnly(n->dtype) ) then return FALSE
		hCRangeFull(n->dtype, interval)
		return TRUE
	case AST_NODECLASS_CONV
		if( n->r <> NULL ) then return FALSE
		if( hCRangeValue(n->l, interval) = FALSE ) then return FALSE
		return hCRangeConvert(interval, n->dtype)
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

private function hCRangeTruth(byval n as ASTNODE ptr, byref truth as integer) as integer
	dim as CINTEGERRANGE left_range, right_range
	if( hCRangeValue(n->l, left_range) = FALSE ) then return FALSE
	if( hCRangeValue(n->r, right_range) = FALSE ) then return FALSE
	var dtype = hCRangeCommonType(n->l->dtype, n->r->dtype)
	if( hCRangeConvert(left_range, dtype) = FALSE ) then return FALSE
	if( hCRangeConvert(right_range, dtype) = FALSE ) then return FALSE
	var lower_upper = hCRangeCompare(left_range.lower, right_range.upper)
	var upper_lower = hCRangeCompare(left_range.upper, right_range.lower)
	select case as const n->op.op
	case AST_OP_EQ, AST_OP_NE
		if( lower_upper > 0 or upper_lower < 0 ) then
			truth = FALSE
		elseif( lower_upper = 0 and upper_lower = 0 ) then
			truth = TRUE
		else
			return FALSE
		end if
		if( n->op.op = AST_OP_NE ) then truth = not truth
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
	if( n->op.options and AST_OPOPT_DOINVERSE ) then truth = not truth
	return TRUE
end function

'' -------------------------------------------------------------------------
'' Tree rewriting before local storage analysis
'' -------------------------------------------------------------------------

private sub hCRangeRememberLabel(byval labels as THASH ptr, byval sym as FBSYMBOL ptr)
	if( sym = NULL orelse symbIsLabel(sym) = FALSE ) then exit sub
	var key = str(culngint(sym))
	if( hashLookup(labels, strptr(key)) = NULL ) then
		hashAdd(labels, ZstrDup(strptr(key)), sym, hashHash(strptr(key)))
	end if
end sub

private sub hCRangeLabelReferences(byval n as ASTNODE ptr, byval labels as THASH ptr)
	if( n = NULL ) then exit sub
	if( n->class <> AST_NODECLASS_LABEL ) then hCRangeRememberLabel(labels, n->sym)
	select case as const n->class
	case AST_NODECLASS_BOP, AST_NODECLASS_UOP, AST_NODECLASS_BRANCH
		hCRangeRememberLabel(labels, n->op.ex)
	case AST_NODECLASS_JMPTB
		hCRangeRememberLabel(labels, n->jmptb.deflabel)
		for i as integer = 0 to n->jmptb.labelcount - 1
			hCRangeRememberLabel(labels, n->jmptb.labels[i])
		next
	case AST_NODECLASS_CALL
		'' Match the loader's profiling and string-copyback subtrees, which
		'' are not ordinary L/R children. Real label addresses must survive.
		hCRangeLabelReferences(n->call.profbegin, labels)
		hCRangeLabelReferences(n->call.profend, labels)
		var item = n->call.strtail
		while( item <> NULL )
			hCRangeLabelReferences(item->srctree, labels)
			item = item->prev
		wend
	end select
	hCRangeLabelReferences(n->l, labels)
	hCRangeLabelReferences(n->r, labels)
end sub

private sub hCRangePruneLabel(byval n as ASTNODE ptr, byval removed as THASH ptr, byval live as THASH ptr)
	if( n = NULL ) then exit sub
	if( n->class = AST_NODECLASS_LABEL andalso n->l = NULL andalso n->r = NULL ) then
		var key = str(culngint(n->sym))
		if( hashLookup(removed, strptr(key)) <> NULL andalso hashLookup(live, strptr(key)) = NULL ) then
			'' Only retire labels whose branch this pass removed. Keep the
			'' symbol's source-access facts intact for semantic observations.
			n->class = AST_NODECLASS_NOP
			n->dtype = FB_DATATYPE_VOID
			n->sym = NULL
		end if
	end if
	hCRangePruneLabel(n->l, removed, live)
	hCRangePruneLabel(n->r, removed, live)
end sub

private function hCRangeFold(byval n as ASTNODE ptr, byval removed as THASH ptr, byref removed_branches as integer) as integer
	if( n = NULL ) then return FALSE
	var changed = hCRangeFold(n->l, removed, removed_branches)
	changed or= hCRangeFold(n->r, removed, removed_branches)
	if( n->class <> AST_NODECLASS_BOP orelse n->vector <> 0 ) then return changed
	if( astOpIsRelational(n->op.op) = FALSE ) then return changed
	dim as integer truth
	if( hCRangeTruth(n, truth) = FALSE ) then return changed
	astDelTree(n->l)
	astDelTree(n->r)
	n->l = NULL
	n->r = NULL
	n->sym = NULL
	if( n->op.ex <> NULL ) then
		n->dtype = FB_DATATYPE_INVALID
		n->subtype = NULL
		if( truth ) then
			n->class = AST_NODECLASS_BRANCH
			n->op.op = AST_OP_JMP
			n->op.options = AST_OPOPT_ALLOCRES
		else
			hCRangeRememberLabel(removed, n->op.ex)
			removed_branches += 1
			n->class = AST_NODECLASS_NOP
		end if
	else
		'' astNewCONSTi normalizes BOOLEAN separately from BASIC integer -1.
		var value = astNewCONSTi(truth, n->dtype, n->subtype)
		n->class = AST_NODECLASS_CONST
		n->val = value->val
		astDelNode(value)
	end if
	return TRUE
end function

private function hCIntegerRangesBegin(byval first as ASTNODE ptr) as integer
	if( c_local_refs_complete = FALSE ) then return FALSE
	dim as integer changed = FALSE
	dim as integer removed_branches = 0
	dim as THASH removed
	hashInit(@removed, 32, TRUE)
	var n = first
	while( n <> NULL )
		changed or= hCRangeFold(n, @removed, removed_branches)
		n = n->next
	wend
	if( removed_branches <> 0 ) then
		dim as THASH live
		hashInit(@live, 32, TRUE)
		n = first
		while( n <> NULL )
			hCRangeLabelReferences(n, @live)
			n = n->next
		wend
		n = first
		while( n <> NULL )
			hCRangePruneLabel(n, @removed, @live)
			n = n->next
		wend
		hashEnd(@live)
	end if
	hashEnd(@removed)
	return changed
end function

'' end of backend/c/c-integer-ranges.bi
