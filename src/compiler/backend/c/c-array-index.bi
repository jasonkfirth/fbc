'' Project: FreeBASIC compiler - typed C array indexing
'' File: backend/c/c-array-index.bi
'' Purpose: Recover physical element indices before integer address lowering.
'' Responsibilities: Preserve one evaluation, native-width index wrapping and
''     declared element/member storage for fixed one-dimensional C arrays.
'' This file intentionally does NOT contain: descriptor updates, multidimensional indexing or byte views.

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

'' end of backend/c/c-array-index.bi
