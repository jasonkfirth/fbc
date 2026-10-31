/'
	Project: FreeBASIC C backend
	File: backend/c/c-va-pointer-cursors.bi
	Purpose: Give builtin pointer va_list operations a correctly typed cursor.
	Responsibilities: Evaluate raw slot addresses once, transfer pointer bytes to
	naturally aligned native storage, and write back after cursor consumption.
	This file intentionally does NOT contain: Record layout changes, copies of
	struct/array va_list objects, or native diagnostic suppression.
'/

type CVAPOINTERCURSOR
	address_name as string
	value_name as string
	writable as integer
end type

private function hCVaPointerOperand _
	(byval vreg as IRVREG ptr, byref cursor as CVAPOINTERCURSOR, _
	 byval read_value as integer, byval write_value as integer) as EXPRNODE ptr
	var value = exprNewVREG(vreg, TRUE, write_value)
	'' The parser normalizes these IR operands to VA_LIST. Its source
	'' mangling marker no longer describes the native storage form here.
	select case fbGetBackendValistType()
	case FB_CVA_LIST_BUILTIN_POINTER, FB_CVA_LIST_BUILTIN_VOID_POINTER
	case else
		return value
	end select
	if( typeGetDtAndPtrOnly(vreg->dtype) <> FB_DATATYPE_VA_LIST ) then return value
	if( value->class <> EXPRCLASS_UOP orelse value->op <> AST_OP_DEREF ) then return value
	if( value->l = NULL ) then return value
	var address = value->l
	if( address->class = EXPRCLASS_CAST ) then
		if( address->l = NULL ) then return value
		address = address->l
	end if
	'' A direct native cursor object already has the right declared storage.
	'' An indirect pointer can instead designate a BASIC record's void* slot,
	'' even when the pointer itself has the native C cursor-pointer type.
	if( address->class = EXPRCLASS_UOP andalso address->op = AST_OP_ADDROF ) then
		var object_value = address->l
		if( object_value <> NULL andalso object_value->class = EXPRCLASS_SYM ) then
			var sym = object_value->sym
			if( sym <> NULL andalso symbIsVar(sym) ) then
				if( symbIsRef(sym) = FALSE and symbIsParamVarBydescOrByref(sym) = FALSE and _
				    symbGetArrayDimensions(sym) = 0 ) then
					select case symbGetValistType(symbGetFullType(sym), symbGetSubtype(sym))
					case FB_CVA_LIST_BUILTIN_POINTER, FB_CVA_LIST_BUILTIN_VOID_POINTER
						return value
					end select
				end if
			end if
		end if
	end if

	'' BASIC record fields retain their ordinary object-pointer storage type.
	'' A native va_list can instead be char*. Accessing that slot through a
	'' va_list* cast both loses alignment information and aliases another
	'' pointer-object type. Pointer va_list targets need only their cursor word;
	'' struct and array targets keep their native lifetime/copy rules above.
	if( value->l->class = EXPRCLASS_CAST ) then
		value->l->l = NULL
	else
		value->l = NULL
	end if
	exprFreeTree(value)
	cursor.address_name = hCUnalignedTempName()
	cursor.value_name = hCUnalignedTempName()
	cursor.writable = write_value
	var address_dtype = typeAddrOf(FB_DATATYPE_VOID)
	if( write_value = FALSE ) then address_dtype = typeAddrOf(typeSetIsConst(FB_DATATYPE_VOID))
	var native_type = hEmitType(typeUnsetIsConst(vreg->dtype), vreg->subtype, TRUE)
	hWriteDeclaration(hEmitType(address_dtype, NULL, TRUE) + " " + cursor.address_name + ";")
	hWriteDeclaration(native_type + " " + cursor.value_name + ";")
	'' The target classifier promises one pointer-sized cursor. Keep that
	'' representation constraint checked by the native compiler as well.
	hWriteDeclaration("enum { " + cursor.value_name + "_size = 1 / ((sizeof(" + native_type + _
		") == sizeof(void*)) ? 1 : 0) };")
	hWriteLine(cursor.address_name + " = " + exprFlush(exprNewCAST(address_dtype, NULL, address)) + ";")
	if( read_value ) then
		hWriteLine("__builtin_memcpy(&" + cursor.value_name + ", " + cursor.address_name + _
			", sizeof(" + cursor.value_name + "));")
	end if
	'' va_start and va_copy destinations may be uninitialized. They must not
	'' acquire an input load merely because their storage uses a byte bridge.
	return exprNewTEXT(typeUnsetIsConst(vreg->dtype), vreg->subtype, strptr(cursor.value_name))
end function

private sub hCVaPointerWriteback(byref cursor as CVAPOINTERCURSOR)
	if( len(cursor.address_name) = 0 or cursor.writable = FALSE ) then exit sub
	hWriteLine("__builtin_memcpy(" + cursor.address_name + ", &" + cursor.value_name + _
		", sizeof(" + cursor.value_name + "));")
end sub

/' end of backend/c/c-va-pointer-cursors.bi '/
