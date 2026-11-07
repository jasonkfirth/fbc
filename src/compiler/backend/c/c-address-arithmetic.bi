'' Project: FreeBASIC compiler - C address arithmetic
'' File: backend/c/c-address-arithmetic.bi
'' Purpose: Convert pointer offsets and procedure comparisons to integer addresses.
'' Responsibilities: Preserve pointer types, element scaling and evaluation count.
'' This file intentionally does NOT contain: Rewrites of pointer-valued operations.

private function hCProcedureAddressValue(byval value as EXPRNODE ptr) as integer
	if( value = NULL ) then return FALSE
	if( typeIsPtr(value->dtype) = FALSE ) then return FALSE
	'' Pointer casts preserve a procedure address even when BASIC exposes it
	'' through ANY PTR. Stop at value-producing operations and bounded depth.
	for depth as integer = 0 to 63
		if( value = NULL ) then exit for
		if( typeGetDtAndPtrOnly(value->dtype) = typeAddrOf(FB_DATATYPE_FUNCTION) ) then return TRUE
		if( value->class <> EXPRCLASS_CAST ) then exit for
		value = value->l
	next
	return FALSE
end function

private function hCNativePointerOffsets _
	( byval address as EXPRNODE ptr, byval depth as integer = 0 ) as EXPRNODE ptr

	if( address = NULL or depth >= 64 ) then return address

	select case address->class
	case EXPRCLASS_CAST
		if( address->l <> NULL andalso typeIsPtr(address->dtype) andalso _
		    typeIsPtr(address->l->dtype) ) then
			address->l = hCNativePointerOffsets( address->l, depth + 1 )
		end if

	case EXPRCLASS_UOP
		if( address->op <> AST_OP_ADDROF or address->l = NULL ) then return address
		var member = address->l
		if( member->class <> EXPRCLASS_MEMBER or member->l = NULL or member->r = NULL ) then return address
		if( typeIsPtr(member->l->dtype) = FALSE or member->r->class <> EXPRCLASS_IMM ) then return address
		var dtype = address->dtype
		var subtype = address->subtype
		var offset = member->r->val.i
		var origin = member->l
		exprFreeTree( member->r )
		exprFreeNode( member )
		exprFreeNode( address )
		'' Taking the address of a field does not load that field.  Its stored
		'' byte displacement also works when the record origin is null.
		origin = hCNativePointerOffsets( origin, depth + 1 )
		origin = exprNewCAST( FB_DATATYPE_UINT, NULL, origin )
		if( offset <> 0 ) then
			origin = exprNewBOP( AST_OP_ADD, origin, exprNewIMMi(offset, FB_DATATYPE_UINT) )
		end if
		return exprNewCAST( dtype, subtype, origin )

	case EXPRCLASS_BOP
		if( address->op <> AST_OP_ADD andalso address->op <> AST_OP_SUB ) then return address
		if( address->l = NULL or address->r = NULL ) then return address

		dim as EXPRNODE ptr pointer_expr = NULL, offset_expr = NULL
		dim as integer pointer_on_left = FALSE
		if( typeIsPtr(address->l->dtype) andalso not typeIsPtr(address->r->dtype) ) then
			pointer_expr = address->l
			offset_expr = address->r
			pointer_on_left = TRUE
		elseif( address->op = AST_OP_ADD andalso typeIsPtr(address->r->dtype) andalso _
		        not typeIsPtr(address->l->dtype) ) then
			pointer_expr = address->r
			offset_expr = address->l
		else
			return address
		end if
		if( typeGetClass(offset_expr->dtype) <> FB_DATACLASS_INTEGER ) then return address

		var pointee = typeDeref( pointer_expr->dtype )
		if( typeIsPtr(pointee) = FALSE ) then
			select case typeGetDtOnly( pointee )
			case FB_DATATYPE_VOID, FB_DATATYPE_FUNCTION, FB_DATATYPE_FWDREF
				return address
			case FB_DATATYPE_STRUCT
				if( pointer_expr->subtype = NULL ) then return address
			end select
		end if
		dim as longint elementsize = symbCalcLen( pointee, pointer_expr->subtype )
		if( elementsize <= 0 ) then return address

		var dtype = address->dtype
		var subtype = address->subtype
		var op = address->op
		exprFreeNode( address )

		'' BASIC can form pointers from null or virtual origins without
		'' dereferencing them. C pointer addition does not permit that. The
		'' backend already uses native unsigned addresses for virtual array
		'' origins; use the same wrapping arithmetic for pointer offsets.
		'' Each original operand remains in the resulting expression once.
		pointer_expr = hCNativePointerOffsets( pointer_expr, depth + 1 )
		pointer_expr = exprNewCAST( FB_DATATYPE_UINT, NULL, pointer_expr )
		offset_expr = exprNewCAST( FB_DATATYPE_UINT, NULL, offset_expr )
		if( elementsize <> 1 ) then
			offset_expr = exprNewBOP( AST_OP_MUL, offset_expr, _
			                           exprNewIMMi(elementsize, FB_DATATYPE_UINT) )
		end if
		if( pointer_on_left ) then
			address = exprNewBOP( op, pointer_expr, offset_expr )
		else
			address = exprNewBOP( AST_OP_ADD, offset_expr, pointer_expr )
		end if
		return exprNewCAST( dtype, subtype, address )
	end select

	return address
end function

private function hCIntegerAddressValues _
	( byval value as EXPRNODE ptr, byval used_as_pointer as integer = FALSE, _
	  byval depth as integer = 0 ) as EXPRNODE ptr

	if( value = NULL or depth >= 64 ) then return value

	'' Only explicit pointer-to-integer conversions use this path. Pointer
	'' values passed to other code keep typed C arithmetic, so bounds warnings
	'' remain visible until their actual accesses can be checked separately.
	if( typeIsPtr(value->dtype) ) then used_as_pointer = TRUE
	select case value->class
	case EXPRCLASS_CAST
		if( value->l <> NULL ) then
			if( used_as_pointer = FALSE andalso typeIsPtr(value->dtype) = FALSE andalso _
			    typeGetClass(value->dtype) = FB_DATACLASS_INTEGER andalso _
			    typeIsPtr(value->l->dtype) ) then
				value->l = hCNativePointerOffsets( value->l )
			end if
			value->l = hCIntegerAddressValues( value->l, used_as_pointer, depth + 1 )
		end if

	case EXPRCLASS_ARRAYADDR
		'' Its formatter already uses integer addresses and requires the original
		'' storage/index tree to recover the declared array origin.

	case else
		if( value->l <> NULL ) then
			value->l = hCIntegerAddressValues( value->l, used_as_pointer, depth + 1 )
		end if
		if( value->r <> NULL ) then
			value->r = hCIntegerAddressValues( value->r, used_as_pointer, depth + 1 )
		end if
	end select

	return value
end function

'' end of backend/c/c-address-arithmetic.bi
