'' Project: FreeBASIC compiler - C storage alignment
'' File: backend/c/c-alignment.bi
'' Purpose: Prove address alignment before emitting typed scalar access.
'' Responsibilities: Distinguish stored values from storage addresses and retain
''     unknown low bits through casts, members, array indices and arithmetic.
'' This file intentionally does NOT change BASIC pointer or array descriptors.

declare function hExprConstOffsetMod _
	( byval n as EXPRNODE ptr, byval align as integer ) as integer

private function hCStorageOffsetMod _
	( byval n as EXPRNODE ptr, byval align as integer ) as integer

	select case as const( n->class )
	case EXPRCLASS_SYM
		var sym = n->sym
		if( sym = NULL ) then return -1
		if( symbIsVar(sym) = FALSE and symbGetClass(sym) <> FB_SYMBCLASS_PARAM ) then return -1
		if( symbIsRef(sym) or symbIsImport(sym) ) then return -1
		if( symbGetClass(sym) = FB_SYMBCLASS_PARAM ) then
			if( symbGetParamMode(sym) <> FB_PARAMMODE_BYVAL ) then return -1
		end if
		'' A byte buffer has only byte alignment, even at offset zero. Its
		'' actual stack/linker placement may be stronger but is not a contract.
		if( typeCalcNaturalAlign(symbGetFullType(sym), symbGetSubtype(sym)) >= align ) then return 0

	case EXPRCLASS_MEMBER
		dim as integer origin_mod
		if( typeIsPtr(n->l->dtype) ) then
			origin_mod = hExprConstOffsetMod(n->l, align)
		else
			origin_mod = hCStorageOffsetMod(n->l, align)
		end if
		if( origin_mod < 0 ) then return -1
		return hNormalizeOffsetMod(origin_mod + n->r->val.i, align)

	case EXPRCLASS_INDEX
		var origin_mod = hCStorageOffsetMod(n->l, align)
		if( origin_mod < 0 ) then return -1
		var stride = symbGetSizeOf(n->l->sym) mod align
		if( stride = 0 ) then return origin_mod
		var index_mod = hExprConstOffsetMod(n->r, align)
		if( index_mod < 0 ) then return -1
		return hNormalizeOffsetMod(origin_mod + index_mod * stride, align)

	case EXPRCLASS_UOP
		if( n->op = AST_OP_DEREF ) then return hExprConstOffsetMod(n->l, align)
	end select
	return -1
end function

private function hExprConstOffsetMod _
	( byval n as EXPRNODE ptr, byval align as integer ) as integer

	'' -1 means unknown, not zero. In particular, a pointer variable's stored
	'' value does not inherit the alignment of the variable's own storage.
	select case as const( n->class )
	case EXPRCLASS_IMM
		if( typeGetClass(n->dtype) = FB_DATACLASS_INTEGER ) then
			return hNormalizeOffsetMod(n->val.i, align)
		end if

	case EXPRCLASS_SYM
		if( symbIsCArray(n->sym) ) then return hCStorageOffsetMod(n, align)

	case EXPRCLASS_ARRAYADDR
		return hCStorageOffsetMod(n->l, align)

	case EXPRCLASS_CAST
		'' Integer/pointer casts preserve low bits; conversion to BOOLEAN does not.
		if( typeGetDtAndPtrOnly(n->dtype) <> FB_DATATYPE_BOOLEAN ) then
			return hExprConstOffsetMod(n->l, align)
		end if

	case EXPRCLASS_UOP
		if( n->op = AST_OP_ADDROF ) then return hCStorageOffsetMod(n->l, align)

	case EXPRCLASS_BOP
		var left_mod = hExprConstOffsetMod(n->l, align)
		var right_mod = hExprConstOffsetMod(n->r, align)
		select case as const( n->op )
		case AST_OP_MUL, AST_OP_AND
			'' A known multiple/mask can prove zero low bits despite an unknown
			'' other operand. Natural scalar alignments are powers of two.
			if( left_mod = 0 or right_mod = 0 ) then return 0
		case AST_OP_SHL
			dim as longint amount
			if( hCArrayIndexImmediate(n->r, amount) ) then
				if( amount >= 0 andalso amount < 64 ) then
					if( (1ull shl amount) mod culngint(align) = 0 ) then return 0
				end if
			end if
		end select
		if( left_mod < 0 or right_mod < 0 ) then return -1
		select case as const( n->op )
		case AST_OP_ADD
			return hNormalizeOffsetMod(left_mod + right_mod, align)
		case AST_OP_SUB
			return hNormalizeOffsetMod(left_mod - right_mod, align)
		case AST_OP_MUL
			return hNormalizeOffsetMod(left_mod * right_mod, align)
		case AST_OP_AND
			return left_mod and right_mod
		end select
	end select
	return -1
end function

private function hCUnalignedAddress( byval addr as EXPRNODE ptr ) as EXPRNODE ptr
	'' A byte-copy path must not first form a misaligned typed pointer. Retire
	'' only outer pointer conversions; integer casts preserve native wrapping.
	while( addr->class = EXPRCLASS_CAST andalso typeIsPtr(addr->dtype) )
		var child = addr->l
		exprFreeNode(addr)
		addr = child
	wend
	return addr
end function

private function hCUnalignedTempName() as string
	'' Nested copy expressions need distinct locals. This counter belongs to
	'' C emission, so it does not consume BASIC symbol/model identities.
	ctx.unalignedid += 1
	assert(ctx.unalignedid <> 0)
	return "fb_c_unaligned_" + str(ctx.unalignedid)
end function

'' end of backend/c/c-alignment.bi
