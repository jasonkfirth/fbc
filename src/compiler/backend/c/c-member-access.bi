'' Project: FreeBASIC compiler - typed C member access
'' File: backend/c/c-member-access.bi
'' Purpose: Recover declared C member paths from typed constant IR offsets.
'' Responsibilities: Match field representation, nested records and fixed array
''     elements without consuming the base expression on an unsuccessful search.
'' This file intentionally does NOT contain: raw-memory casts or symbol mutation.

private function hCMemberTypeMatches _
	( byval dtype as integer, byval subtype as FBSYMBOL ptr, _
	  byval wanted as integer, byval wantedsubtype as FBSYMBOL ptr, _
	  byval address_only as integer = FALSE, _
	  byval read_value as integer = FALSE ) as integer

	if( (typeGetDtAndPtrOnly(dtype) = typeGetDtAndPtrOnly(wanted)) andalso _
	    (subtype = wantedsubtype) ) then return TRUE
	if( typeIsPtr(dtype) or typeIsPtr(wanted) ) then
		'' A byte copy transfers the stored pointer word without a typed load
		'' or conversion. Its address can therefore retain the declared field
		'' even when the copy temporary uses another object-pointer type.
		if( address_only andalso typeIsPtr(dtype) andalso typeIsPtr(wanted) ) then return TRUE
		'' FBSTRING uses C char*, while BASIC byte text may use uint8*.
		if( typeGetPtrCnt(dtype) <> 1 or typeGetPtrCnt(wanted) <> 1 ) then return FALSE
		var actualbase = typeGetDtOnly(dtype)
		var wantedbase = typeGetDtOnly(wanted)
		if( read_value andalso wantedbase = FB_DATATYPE_VOID ) then
			'' C gives void* and character pointers the same representation.
			'' Load the declared slot before converting its value; this does not
			'' make pointer-to-pointer storage interchangeable (C11 6.2.5p28).
			if( subtype <> NULL or wantedsubtype <> NULL ) then return FALSE
			if( typeHasMangleDt(dtype) or typeHasMangleDt(wanted) ) then return FALSE
			if( typeIsConstAt(dtype, 1) andalso (typeIsConstAt(wanted, 1) = FALSE) ) then return FALSE
			select case actualbase
			case FB_DATATYPE_BYTE, FB_DATATYPE_UBYTE, FB_DATATYPE_CHAR
				return TRUE
			end select
		end if
		return ((actualbase = FB_DATATYPE_CHAR) or (actualbase = FB_DATATYPE_UBYTE)) andalso _
		       ((wantedbase = FB_DATATYPE_CHAR) or (wantedbase = FB_DATATYPE_UBYTE))
	end if
	'' Integer aliases can share representation. Floating/integer bit casts cannot.
	if( typeGetClass(dtype) <> FB_DATACLASS_INTEGER ) then return FALSE
	if( typeGetClass(wanted) <> FB_DATACLASS_INTEGER ) then return FALSE
	return typeGetSizeType(dtype) = typeGetSizeType(wanted)
end function

private function hFindCMemberPath _
	( byval dtype as integer, byval subtype as FBSYMBOL ptr, _
	  byval offset as longint, byval wanted as integer, _
	  byval wantedsubtype as FBSYMBOL ptr, byref path as string, _
	  byref actualdtype as integer, byref actualsubtype as FBSYMBOL ptr, _
	  byval depth as integer = 0, byval address_only as integer = FALSE, _
	  byval read_value as integer = FALSE ) as integer

	'' The type model rejects recursive by-value records. Keep an independent
	'' limit here so malformed internal metadata cannot recurse indefinitely.
	if( offset < 0 or depth >= 64 ) then return FALSE
	if( offset = 0 andalso hCMemberTypeMatches(dtype, subtype, wanted, wantedsubtype, address_only, read_value) ) then
		actualdtype = dtype
		actualsubtype = subtype
		return TRUE
	end if
	if( typeIsPtr(dtype) ) then return FALSE

	select case typeGetDtOnly(dtype)
	case FB_DATATYPE_STRING, FB_DATATYPE_USTRING
		'' The emitted FBSTRING contract is data, length, allocation size,
		'' each occupying one native pointer-sized slot on supported targets.
		if( offset >= env.pointersize * 3 ) then return FALSE
		if( offset mod env.pointersize <> 0 ) then return FALSE
		var slot = offset \ env.pointersize
		dim as integer memberdtype = FB_DATATYPE_INTEGER
		if( slot = 0 ) then memberdtype = typeAddrOf(FB_DATATYPE_CHAR)
		if( hCMemberTypeMatches(memberdtype, NULL, wanted, wantedsubtype, address_only, read_value) = FALSE ) then return FALSE
		select case slot
		case 0: path += ".data"
		case 1: path += ".len"
		case 2: path += ".size"
		end select
		actualdtype = memberdtype
		actualsubtype = NULL
		return TRUE

	case FB_DATATYPE_STRUCT
		if( subtype = NULL ) then return FALSE
		'' Bitfield records are emitted as byte bodies, not named C fields.
		'' Keep packed records on the qualified byte-copy access path.
		if( symbGetUdtHasBitfield(subtype) ) then return FALSE
		if( symbGetUDTAlign(subtype) <> 0 andalso address_only = FALSE ) then return FALSE
		if( offset >= symbGetSizeOf(subtype) ) then return FALSE
		var fld = symbUdtGetFirstField(subtype)
		while( fld <> NULL )
			if( symbIsDynamic(fld) = FALSE ) then
				var localoffset = offset - symbGetOfs(fld)
				var extent = symbGetRealSize(fld)
				if( localoffset >= 0 andalso localoffset < extent ) then
					var fielddtype = symbGetFullType(fld)
					var fieldsubtype = symbGetSubtype(fld)
					var elementsize = symbGetSizeOf(fld)
					var element = 0ll
					var tail = "." + hGetCFieldId(fld)
					if( symbGetArrayDimensions(fld) > 0 ) then
						if( elementsize <= 0 ) then return FALSE
						element = localoffset \ elementsize
						localoffset mod= elementsize
						var remaining = symbGetArrayElements(fld)
						for dimension as integer = 0 to symbGetArrayDimensions(fld) - 1
							var count = symbArrayUbound(fld, dimension) - symbArrayLbound(fld, dimension) + 1
							if( count <= 0 ) then return FALSE
							remaining \= count
							if( remaining <= 0 ) then return FALSE
							tail += "[" + str(element \ remaining) + "]"
							element mod= remaining
						next
					end if
					'' Raw fixed strings have an extra C array dimension. Keep
					'' their byte access path, while admitting ordinary text
					'' pointers by retaining the pointer level in this test.
					select case typeGetDtAndPtrOnly(fielddtype)
					case FB_DATATYPE_FIXSTR, FB_DATATYPE_CHAR, FB_DATATYPE_WCHAR
					case else
						if( hFindCMemberPath(fielddtype, fieldsubtype, localoffset, wanted, _
						    wantedsubtype, tail, actualdtype, actualsubtype, depth + 1, address_only, _
					    (read_value andalso (symbIsRef(fld) = FALSE))) ) then
							path += tail
							return TRUE
						end if
					end select
				end if
			end if
			fld = symbUdtGetNextField(fld)
		wend
	end select
	return FALSE
end function

private function exprTryCMember _
	( byval base_expr as EXPRNODE ptr, byval offset as longint, _
	  byval wanted as integer, byval wantedsubtype as FBSYMBOL ptr, _
	  byval address_only as integer = FALSE, _
	  byval read_value as integer = FALSE ) as EXPRNODE ptr

	'' Low-level IR may have converted a declared record pointer to bytes.
	'' Recover its declared origin through pointer casts, but never through
	'' integer address arithmetic or an incompatible field representation.
	var origin = base_expr
	do
		select case origin->class
		case EXPRCLASS_CAST
			'' Pointer casts also encode the final access type, for example
			'' int64* around a record's byte address. Recover the declared
			'' origin and require the complete field representation to match.
			if( typeIsPtr(origin->dtype) = FALSE ) then exit do
			if( typeIsPtr(origin->l->dtype) = FALSE ) then
				'' Only the byte-pointer BOP shape can preserve a declared origin;
				'' an arbitrary integer-to-pointer cast cannot identify a field.
				if( origin->l->class <> EXPRCLASS_BOP ) then exit do
				if( typeIsPtr(origin->l->l->dtype) = FALSE ) then exit do
			end if
		case EXPRCLASS_BOP
			if( (origin->op <> AST_OP_ADD) and (origin->op <> AST_OP_SUB) ) then exit do
			if( origin->r->class <> EXPRCLASS_IMM ) then exit do
			if( typeGetClass(origin->r->dtype) <> FB_DATACLASS_INTEGER ) then exit do
			if( typeGetPtrCnt(origin->l->dtype) <> 1 ) then exit do
			select case typeGetDtOnly(origin->l->dtype)
			case FB_DATATYPE_BYTE, FB_DATATYPE_UBYTE, FB_DATATYPE_CHAR
			case else: exit do
			end select
			'' This addition was performed on bytes, so its constant is a
			'' displacement, not a count of record elements.
			var delta = origin->r->val.i
			if( origin->op = AST_OP_SUB ) then
				if( delta = (-9223372036854775807ll - 1ll) ) then return NULL
				delta = -delta
			end if
			if( delta > 0 ) then
				if( offset > 9223372036854775807ll - delta ) then return NULL
			elseif( delta < 0 ) then
				if( offset < (-9223372036854775807ll - 1ll) - delta ) then return NULL
			end if
			offset += delta
		case else: exit do
		end select
		origin = origin->l
	loop
	var dtype = origin->dtype
	var subtype = origin->subtype
	var is_pointer = typeIsPtr(dtype)
	if( is_pointer ) then
		if( typeGetPtrCnt(dtype) <> 1 ) then return NULL
		dtype = typeDeref(dtype)
	end if
	select case typeGetDtOnly(dtype)
	case FB_DATATYPE_STRUCT, FB_DATATYPE_STRING, FB_DATATYPE_USTRING
	case else: return NULL
	end select
	'' A typed pointer is a view, not proof of the pointee's declared storage.
	'' Keep the new value conversion on actual object symbols; reference and
	'' imported symbols may also name indirect storage.
	var read_declared_value = read_value andalso (is_pointer = FALSE)
	if( origin->class <> EXPRCLASS_SYM ) then read_declared_value = FALSE
	if( read_declared_value ) then
		if( symbIsVar(origin->sym) = FALSE ) then
			read_declared_value = FALSE
		elseif( symbIsRef(origin->sym) or symbIsParamVarBydescOrByref(origin->sym) or symbIsImport(origin->sym) ) then
			read_declared_value = FALSE
		end if
	end if
	dim as string path
	dim as integer actualdtype
	dim as FBSYMBOL ptr actualsubtype
	if( hFindCMemberPath(dtype, subtype, offset, wanted, wantedsubtype, _
	    path, actualdtype, actualsubtype, 0, address_only, read_declared_value) = FALSE ) then return NULL
	if( len(path) = 0 and is_pointer = FALSE ) then return NULL
	if( is_pointer ) then
		if( len(hCAlignmentViewName(dtype, subtype)) = 0 ) then return NULL
	end if
	if( is_pointer and len(path) > 0 ) then path = "->" + mid(path, 2)
	var result = exprNew(EXPRCLASS_MEMBER, actualdtype, actualsubtype)
	result->text = ZstrDup(strptr(path))
	if( is_pointer ) then result->member_view = C_MEMBER_VIEW_VALUE
	'' Drop erased-type wrappers only after a complete path was proved.
	do while( base_expr <> origin )
		var next_expr = base_expr->l
		if( base_expr->r <> NULL ) then exprFreeTree(base_expr->r)
		exprFreeNode(base_expr)
		base_expr = next_expr
	loop
	result->l = origin
	'' Retain the byte displacement for the existing alignment analysis;
	'' this metadata child is not evaluated in the emitted C expression.
	result->r = exprNewIMMi(offset)
	return result
end function

'' end of backend/c/c-member-access.bi
