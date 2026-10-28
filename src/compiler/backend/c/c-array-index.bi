'' Project: FreeBASIC compiler - typed C array indexing
'' File: backend/c/c-array-index.bi
'' Purpose: Recover physical element indices before integer address lowering.
'' Responsibilities: Preserve one evaluation, native-width index wrapping and
''     declared element/member storage for fixed one-dimensional C arrays.
'' This file intentionally does NOT change descriptors, other ranks or byte views.

private function hCArrayIndexImmediate _
	( byval n as EXPRNODE ptr, byref value as longint ) as integer

	if( n = NULL ) then return FALSE
	if( typeGetClass(n->dtype) <> FB_DATACLASS_INTEGER or typeIsPtr(n->dtype) ) then return FALSE
	if( n->class = EXPRCLASS_IMM ) then
		value = n->val.i
		return TRUE
	end if
	'' Shift amounts are masked by _emitBop before reaching this tree.
	if( n->class = EXPRCLASS_BOP andalso n->op = AST_OP_AND ) then
		dim as longint left_value, right_value
		if( hCArrayIndexImmediate(n->l, left_value) andalso _
		    hCArrayIndexImmediate(n->r, right_value) ) then
			value = left_value and right_value
			return TRUE
		end if
	end if
	return FALSE
end function

private function hCArrayUnscaledIndex _
	( byval n as EXPRNODE ptr, byval element_size as longint ) as EXPRNODE ptr

	'' The query borrows a node. It leaves the original tree intact on failure.
	if( n = NULL ) then return NULL
	if( typeGetClass(n->dtype) <> FB_DATACLASS_INTEGER or typeIsPtr(n->dtype) ) then return NULL
	if( typeGetSize(n->dtype) <> env.pointersize ) then return NULL
	if( element_size = 1 ) then return n
	while( n->class = EXPRCLASS_CAST )
		if( typeGetClass(n->l->dtype) <> FB_DATACLASS_INTEGER or typeIsPtr(n->l->dtype) ) then return NULL
		if( typeGetSize(n->l->dtype) <> env.pointersize ) then return NULL
		n = n->l
	wend
	if( n->class <> EXPRCLASS_BOP ) then return NULL
	dim as longint value
	select case n->op
	case AST_OP_MUL
		if( hCArrayIndexImmediate(n->r, value) andalso value = element_size ) then return n->l
		if( hCArrayIndexImmediate(n->l, value) andalso value = element_size ) then return n->r
	case AST_OP_SHL
		if( hCArrayIndexImmediate(n->r, value) ) then
			'' Positive signed 64-bit element sizes cannot exceed 2^62 here.
			if( value >= 0 andalso value <= 62 ) then
				if( (1ull shl value) = culngint(element_size) ) then return n->l
			end if
		end if
	end select
	return NULL
end function

private sub hCArrayFreeScale( byval n as EXPRNODE ptr, byval kept as EXPRNODE ptr )
	if( n = NULL or n = kept ) then exit sub
	hCArrayFreeScale(n->l, kept)
	hCArrayFreeScale(n->r, kept)
	exprFreeNode(n)
end sub

private function exprTryCArrayIndex _
	( byval sym as FBSYMBOL ptr, byref byte_index as EXPRNODE ptr, _
	  byval offset as longint, byval wanted as integer, _
	  byval wantedsubtype as FBSYMBOL ptr, byval allow_onepast as integer = FALSE, _
	  byval read_value as integer = FALSE ) as EXPRNODE ptr

	if( symbIsVar(sym) = FALSE or symbIsCArray(sym) = FALSE ) then return NULL
	if( symbGetArrayDimensions(sym) <> 1 or symbGetOfs(sym) <> 0 ) then return NULL
	var dtype = symbGetFullType(sym)
	var subtype = symbGetSubtype(sym)
	if( symbIsValistStructArray(dtype, subtype) ) then return NULL
	if( typeIsPtr(dtype) = FALSE ) then
		select case typeGetDtOnly(dtype)
		case FB_DATATYPE_FIXSTR, FB_DATATYPE_CHAR, FB_DATATYPE_WCHAR
			return NULL
		end select
	end if
	var element_size = symbGetSizeOf(sym)
	if( element_size <= 0 ) then return NULL
	dim as EXPRNODE ptr index_expr = NULL
	var displacement = offset
	if( byte_index <> NULL ) then
		index_expr = hCArrayUnscaledIndex(byte_index, element_size)
		if( index_expr = NULL ) then return NULL
		'' The IR offset already includes the descriptor's byte bias. Remove it
		'' modulo 64 bits, without overflowing while subtracting signed offsets.
		displacement = clngint(culngint(offset) - culngint(symbGetArrayDiff(sym)))
	end if
	var element_delta = displacement \ element_size
	var field_offset = displacement mod element_size
	if( field_offset < 0 ) then
		field_offset += element_size
		element_delta -= 1
	end if
	if( byte_index = NULL ) then
		var count = symbGetArrayElements(sym)
		if( element_delta < 0 or element_delta > count ) then return NULL
		if( element_delta = count ) then
			if( allow_onepast = FALSE or field_offset <> 0 ) then return NULL
		end if
	end if
	dim as string path
	dim as integer actualdtype
	dim as FBSYMBOL ptr actualsubtype
	if( hFindCMemberPath(dtype, subtype, field_offset, wanted, wantedsubtype, _
	    path, actualdtype, actualsubtype, 0, FALSE, read_value) = FALSE ) then return NULL

	if( byte_index = NULL ) then
		index_expr = exprNewIMMi(element_delta)
	else
		hCArrayFreeScale(byte_index, index_expr)
		byte_index = NULL
		'' C indexing uses the physical origin. Unsigned native-width subtraction
		'' also handles INTEGER_MIN bounds without forming their signed negation.
		index_expr = exprNewCAST(FB_DATATYPE_UINT, NULL, index_expr)
		var adjustment = clngint(culngint(element_delta) - culngint(symbArrayLbound(sym, 0)))
		if( adjustment > 0 ) then
			index_expr = exprNewBOP(AST_OP_ADD, index_expr, exprNewIMMi(adjustment, FB_DATATYPE_UINT))
		elseif( adjustment < 0 ) then
			var magnitude = clngint(0ull - culngint(adjustment))
			index_expr = exprNewBOP(AST_OP_SUB, index_expr, exprNewIMMi(magnitude, FB_DATATYPE_UINT))
		end if
	end if
	var origin = exprNew(EXPRCLASS_SYM, FB_DATATYPE_INVALID, NULL)
	origin->sym = sym
	var result = exprNew(EXPRCLASS_INDEX, dtype, subtype)
	result->sym = sym
	result->l = origin
	result->r = index_expr
	if( len(path) > 0 ) then
		var member = exprNew(EXPRCLASS_MEMBER, actualdtype, actualsubtype)
		member->l = result
		member->r = exprNewIMMi(field_offset)
		member->text = ZstrDup(strptr(path))
		result = member
	end if
	return result
end function

private function hCArrayStorageOrigin _
	( byval storage as EXPRNODE ptr, byref offset as longint ) as EXPRNODE ptr

	offset = 0
	while( storage->class = EXPRCLASS_MEMBER )
		'' A pointer member path addresses the pointee, not the pointer's array slot.
		if( typeIsPtr(storage->l->dtype) ) then return NULL
		offset += storage->r->val.i
		storage = storage->l
	wend
	if( storage->class = EXPRCLASS_INDEX ) then return storage
	return NULL
end function

private function exprNewCAddress( byval storage as EXPRNODE ptr ) as EXPRNODE ptr
	if( hCMemberViewIsAggregate(storage) ) then
		storage->member_view = C_MEMBER_VIEW_STORAGE
		return exprNewUOP(AST_OP_ADDROF, storage)
	end if
	dim as longint offset
	var origin = hCArrayStorageOrigin(storage, offset)
	if( origin <> NULL andalso origin->r->class <> EXPRCLASS_IMM ) then
		'' BASIC permits forming a virtual index address outside the allocation.
		'' Keep its native integer address contract when the index is not known
		'' to be in bounds. The owned index tree still occurs only once in C.
		var result = exprNew(EXPRCLASS_ARRAYADDR, typeAddrOf(storage->dtype), storage->subtype)
		result->l = storage
		return result
	end if
	return exprNewUOP(AST_OP_ADDROF, storage)
end function

private function hCFindRecordArray _
	( byval subtype as FBSYMBOL ptr, byval offset as longint, _
	  byval byte_index as EXPRNODE ptr, byval wanted as integer, _
	  byval wantedsubtype as FBSYMBOL ptr, byref array_path as string, _
	  byref member_path as string, byref element_delta as longint, _
	  byref field_offset as longint, byref actualdtype as integer, _
	  byref actualsubtype as FBSYMBOL ptr, byval depth as integer = 0 ) as FBSYMBOL ptr

	'' Only ordinary owned C record storage proves the array's alignment.
	'' Packed, bitfield, pointer and nonzero-bound cases keep the byte path.
	if( subtype = NULL or depth >= 64 or offset < 0 ) then return NULL
	if( symbGetUdtHasBitfield(subtype) or symbGetUDTAlign(subtype) <> 0 ) then return NULL
	if( offset >= symbGetSizeOf(subtype) ) then return NULL
	var fld = symbUdtGetFirstField(subtype)
	while( fld <> NULL )
		if( symbIsDynamic(fld) = FALSE andalso symbIsRef(fld) = FALSE ) then
			var localoffset = offset - symbGetOfs(fld)
			if( localoffset >= 0 andalso localoffset < symbGetRealSize(fld) ) then
				var dtype = symbGetFullType(fld)
				var child = symbGetSubtype(fld)
				if( typeIsConstAt(dtype, 0) ) then return NULL
				if( symbGetArrayDimensions(fld) = 1 ) then
					if( symbArrayLbound(fld, 0) <> 0 ) then return NULL
					select case typeGetDtAndPtrOnly(dtype)
					case FB_DATATYPE_FIXSTR, FB_DATATYPE_CHAR, FB_DATATYPE_WCHAR
						return NULL
					end select
					var element_size = symbGetSizeOf(fld)
					var alignment = typeCalcNaturalAlign(dtype, child)
					if( element_size <= 0 or alignment <= 0 ) then return NULL
					if( symbGetOfs(fld) mod alignment <> 0 or element_size mod alignment <> 0 ) then return NULL
					if( typeCalcNaturalAlign(FB_DATATYPE_STRUCT, subtype) < alignment ) then return NULL
					if( hCArrayUnscaledIndex(byte_index, element_size) = NULL ) then return NULL
					field_offset = localoffset mod element_size
					if( hFindCMemberPath(dtype, child, field_offset, wanted, wantedsubtype, _
					    member_path, actualdtype, actualsubtype) = FALSE ) then return NULL
					element_delta = localoffset \ element_size
					array_path = "." + hGetCFieldId(fld)
					return fld
				elseif( symbGetArrayDimensions(fld) = 0 andalso _
				        typeGetDtAndPtrOnly(dtype) = FB_DATATYPE_STRUCT ) then
					var result = hCFindRecordArray(child, localoffset, byte_index, wanted, wantedsubtype, _
						array_path, member_path, element_delta, field_offset, actualdtype, actualsubtype, depth + 1)
					if( result <> NULL ) then
						array_path = "." + hGetCFieldId(fld) + array_path
						return result
					end if
				end if
			end if
		end if
		fld = symbUdtGetNextField(fld)
	wend
	return NULL
end function

private function exprTryCRecordArrayIndex _
	( byval base_expr as EXPRNODE ptr, byref byte_index as EXPRNODE ptr, _
	  byval offset as longint, byval wanted as integer, _
	  byval wantedsubtype as FBSYMBOL ptr ) as EXPRNODE ptr

	if( byte_index = NULL or base_expr->class <> EXPRCLASS_SYM ) then return NULL
	if( typeGetDtAndPtrOnly(base_expr->dtype) <> FB_DATATYPE_STRUCT ) then return NULL
	if( typeIsConstAt(base_expr->dtype, 0) ) then return NULL
	if( symbIsVar(base_expr->sym) = FALSE or symbIsExtern(base_expr->sym) ) then return NULL
	if( symbGetArrayDimensions(base_expr->sym) <> 0 ) then return NULL
	'' A constant byte displacement may still be attached to the scaled
	'' index. Borrow that scale only; the original tree is unchanged unless
	'' the declared array, target representation and alignment all match.
	if( typeIsPtr(byte_index->dtype) or typeGetClass(byte_index->dtype) <> FB_DATACLASS_INTEGER ) then return NULL
	if( typeGetSize(byte_index->dtype) <> env.pointersize ) then return NULL
	var scale = byte_index
	dim as longint correction
	if( scale->class = EXPRCLASS_BOP ) then
		if( scale->op = AST_OP_ADD andalso hCArrayIndexImmediate(scale->r, correction) ) then
			offset = clngint(culngint(offset) + culngint(correction))
			scale = scale->l
		elseif( scale->op = AST_OP_ADD andalso hCArrayIndexImmediate(scale->l, correction) ) then
			offset = clngint(culngint(offset) + culngint(correction))
			scale = scale->r
		elseif( scale->op = AST_OP_SUB andalso hCArrayIndexImmediate(scale->r, correction) ) then
			offset = clngint(culngint(offset) - culngint(correction))
			scale = scale->l
		end if
	end if

	dim as string array_path, member_path
	dim as longint element_delta, field_offset
	dim as integer actualdtype
	dim as FBSYMBOL ptr actualsubtype
	var fld = hCFindRecordArray(base_expr->subtype, offset, scale, wanted, wantedsubtype, _
		array_path, member_path, element_delta, field_offset, actualdtype, actualsubtype)
	if( fld = NULL ) then return NULL
	var index_expr = hCArrayUnscaledIndex(scale, symbGetSizeOf(fld))
	assert(index_expr <> NULL)
	'' The proof is complete before any owned node is transferred or freed.
	hCArrayFreeScale(byte_index, index_expr)
	byte_index = NULL
	index_expr = exprNewCAST(FB_DATATYPE_UINT, NULL, index_expr)
	if( element_delta <> 0 ) then
		index_expr = exprNewBOP(AST_OP_ADD, index_expr, exprNewIMMi(element_delta, FB_DATATYPE_UINT))
	end if
	var array_expr = exprNew(EXPRCLASS_MEMBER, symbGetFullType(fld), symbGetSubtype(fld))
	array_expr->l = base_expr
	array_expr->r = exprNewIMMi(symbGetOfs(fld))
	array_expr->text = ZstrDup(strptr(array_path))
	var result = exprNew(EXPRCLASS_INDEX, symbGetFullType(fld), symbGetSubtype(fld))
	result->sym = fld
	result->l = array_expr
	result->r = index_expr
	if( len(member_path) > 0 ) then
		var member = exprNew(EXPRCLASS_MEMBER, actualdtype, actualsubtype)
		member->l = result
		member->r = exprNewIMMi(field_offset)
		member->text = ZstrDup(strptr(member_path))
		result = member
	end if
	return result
end function

private function exprTryCRecordArrayAddress _
	( byval dtype as integer, byval subtype as FBSYMBOL ptr, _
	  byval value as EXPRNODE ptr ) as EXPRNODE ptr

	'' Follow pointer conversions only. An integer barrier or a conversion
	'' discarding const remains on its existing diagnostic path.
	var address = value
	while( address->class = EXPRCLASS_CAST )
		if( typeIsPtr(address->dtype) = FALSE or typeIsPtr(address->l->dtype) = FALSE ) then return NULL
		if( typeIsConstAt(address->l->dtype, 1) andalso typeIsConstAt(address->dtype, 1) = FALSE ) then return NULL
		address = address->l
	wend
	if( address->class <> EXPRCLASS_BOP or address->op <> AST_OP_ADD ) then return NULL
	if( typeIsPtr(address->l->dtype) = FALSE or typeIsPtr(address->r->dtype) ) then return NULL
	if( typeIsConstAt(address->dtype, 1) andalso typeIsConstAt(dtype, 1) = FALSE ) then return NULL
	var origin = address->l
	while( origin->class = EXPRCLASS_CAST )
		if( typeIsPtr(origin->dtype) = FALSE or typeIsPtr(origin->l->dtype) = FALSE ) then return NULL
		if( typeIsConstAt(origin->l->dtype, 1) andalso typeIsConstAt(origin->dtype, 1) = FALSE ) then return NULL
		origin = origin->l
	wend
	if( origin->class <> EXPRCLASS_UOP or origin->op <> AST_OP_ADDROF ) then return NULL
	if( origin->l->class <> EXPRCLASS_SYM ) then return NULL
	var byte_index = address->r
	var storage = exprTryCRecordArrayIndex(origin->l, byte_index, 0, typeDeref(dtype), subtype)
	if( storage = NULL ) then return NULL
	'' The successful query transfers the root and index subtrees. Detach
	'' those borrowed links before retiring the obsolete byte-address shell.
	origin->l = NULL
	address->r = NULL
	exprFreeTree(value)
	return exprNewCAST(dtype, subtype, exprNewCAddress(storage))
end function

'' end of backend/c/c-array-index.bi
