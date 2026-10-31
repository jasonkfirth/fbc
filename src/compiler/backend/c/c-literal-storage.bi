/'
	Project: FreeBASIC C backend
	File: backend/c/c-literal-storage.bi
	Purpose: Give addressable string literals stable, correctly qualified storage.
	Responsibilities: Pool each literal symbol once, retain writable addresses
	required by native consumers, and omit data used only by pruned procedures.
	This file intentionally does NOT contain: Source type changes, runtime copies,
	procedure ABI changes, or native diagnostic suppression.
'/

type CLITERALSTORAGE
	sym as FBSYMBOL ptr
	identifier as zstring ptr
	declaration as DZSTRING
	declaration_prefix as uinteger
	declaration_at as uinteger
	references as uinteger
	defined as integer
	mutable_address as integer
end type

'' Literal symbols belong to the module's global table and survive procedure
'' flushing. The pool owns its C names and declarations until output closes.
dim shared as TLIST c_literal_storage
dim shared as THASH c_literal_symbols, c_literal_names
dim shared as integer c_literal_storage_ready
dim shared as uinteger c_literal_name_min, c_literal_name_max

private sub hCLiteralStorageFinish()
	if( c_literal_storage_ready = FALSE ) then exit sub
	var entry = cptr(CLITERALSTORAGE ptr, listGetHead(@c_literal_storage))
	while( entry <> NULL )
		ZstrFree(entry->identifier)
		DZstrAllocate(entry->declaration, 0)
		entry = listGetNext(entry)
	wend
	hashEnd(@c_literal_names)
	hashEnd(@c_literal_symbols)
	listEnd(@c_literal_storage)
	c_literal_storage_ready = FALSE
end sub

private sub hCLiteralStorageInit()
	hCLiteralStorageFinish()
	listInit(@c_literal_storage, 32, sizeof(CLITERALSTORAGE), LIST_FLAGS_CLEAR)
	hashInit(@c_literal_symbols, 256, TRUE)
	hashInit(@c_literal_names, 256, TRUE)
	c_literal_name_min = 0
	c_literal_name_max = 0
	c_literal_storage_ready = TRUE
end sub

private function hCLiteralStorageGet(byval sym as FBSYMBOL ptr) as CLITERALSTORAGE ptr
	assert(c_literal_storage_ready)
	var key = str(culngint(sym))
	var entry = cptr(CLITERALSTORAGE ptr, hashLookup(@c_literal_symbols, strptr(key)))
	if( entry <> NULL ) then return entry
	entry = listNewNode(@c_literal_storage)
	entry->sym = sym
	var identifier = hGetCId(sym)
	entry->identifier = ZstrDup(strptr(identifier))
	if( c_literal_name_min = 0 or len(identifier) < c_literal_name_min ) then c_literal_name_min = len(identifier)
	if( len(identifier) > c_literal_name_max ) then c_literal_name_max = len(identifier)
	hashAdd(@c_literal_symbols, ZstrDup(strptr(key)), entry, hashHash(strptr(key)))
	hashAdd(@c_literal_names, ZstrDup(strptr(identifier)), entry, hashHash(strptr(identifier)))
	return entry
end function

private sub hCLiteralMutableCast(byval dtype as integer, byval value as EXPRNODE ptr)
	if( typeIsPtr(dtype) = FALSE orelse typeIsConstAt(dtype, 1) ) then exit sub
	if( typeGetDtOnly(dtype) = FB_DATATYPE_FUNCTION ) then exit sub
	while( value <> NULL )
		select case as const value->class
		case EXPRCLASS_CAST
			if( typeIsPtr(value->dtype) = FALSE ) then exit sub
			value = value->l
		case EXPRCLASS_UOP
			if( value->op <> AST_OP_ADDROF ) then exit sub
			value = value->l
		case EXPRCLASS_SYM
			if( symbGetIsLiteral(value->sym) ) then
				var entry = hCLiteralStorageGet(value->sym)
				entry->mutable_address = TRUE
			end if
			exit sub
		case else
			exit sub
		end select
	wend
end sub

private function hCLiteralStorageName(byval sym as FBSYMBOL ptr) as string
	var entry = hCLiteralStorageGet(sym)
	if( entry->defined = FALSE ) then
		dim as string initializer
		if( symbGetType(sym) = FB_DATATYPE_WCHAR ) then
			'' The builder consumes the internal wide encoding exactly once.
			hBuildWstrLit(initializer, symbGetVarLitTextW(sym), symbGetWstrLength(sym) + 1)
			if( hUseNumericWstrLit() ) then
				'' A compound literal is an expression with block lifetime. Its
				'' element list also initializes our module-owned array directly.
				var brace = instr(initializer, "{")
				assert(brace > 0)
				initializer = mid(initializer, brace)
			end if
		else
			hBuildStrLit(initializer, hUnescape(symbGetVarLitText(sym)), symbGetStrLength(sym) + 1, 0)
		end if
		var prefix = "static const " + hEmitType(typeUnsetIsConst(symbGetFullType(sym)), NULL) + " "
		var declaration = prefix + *entry->identifier + "[]"
		'' Keep the assembler identity available to the existing ASM emitter.
		var object_name = hGetMangledNameForASM(sym, TRUE)
		hAppendCObjectAlias(declaration, object_name)
		declaration += " = " + initializer + ";"
		DZstrAssign(entry->declaration, strptr(declaration))
		entry->declaration_prefix = len(prefix)
		var section = sectionGosub(0)
		hWriteLine(declaration, TRUE)
		sectionReturn(section)
		entry->defined = TRUE
	end if
	return *entry->identifier
end function

private function hCLiteralIdentifierChar(byval ch as ubyte, byval first as integer) as integer
	if( ch = asc("_") or (ch >= asc("A") and ch <= asc("Z")) or _
	    (ch >= asc("a") and ch <= asc("z")) ) then return TRUE
	return (first = FALSE) and (ch >= asc("0")) and (ch <= asc("9"))
end function

private sub hCLiteralStoragePrepare(byval buffer as ubyte ptr, byval bytes as uinteger)
	if( listGetHead(@c_literal_storage) = NULL ) then exit sub
	'' Reachability pruning has finished. Count native C identifier tokens,
	'' excluding comments and quoted text, so dead helpers cannot strand data.
	'' Declarations are blanked in place to preserve recorded header offsets.
	dim as uinteger offset = 0
	while( offset < bytes )
		var ch = buffer[offset]
		if( ch = asc("""") or ch = asc("'") ) then
			offset += 1
			while( offset < bytes )
				var current = buffer[offset]
				offset += 1
				if( current = ch ) then exit while
				if( current = asc("\") and offset < bytes ) then offset += 1
			wend
		elseif( ch = asc("/") andalso offset + 1 < bytes andalso _
		        (buffer[offset + 1] = asc("/") or buffer[offset + 1] = asc("*")) ) then
			var block_comment = (buffer[offset + 1] = asc("*"))
			offset += 2
			while( offset < bytes )
				if( block_comment ) then
					if( buffer[offset] = asc("*") andalso offset + 1 < bytes andalso buffer[offset + 1] = asc("/") ) then
						offset += 2
						exit while
					end if
				elseif( buffer[offset] = 10 or buffer[offset] = 13 ) then
					exit while
				end if
				offset += 1
			wend
		elseif( hCLiteralIdentifierChar(ch, TRUE) ) then
			var start = offset
			offset += 1
			while( offset < bytes andalso hCLiteralIdentifierChar(buffer[offset], FALSE) )
				offset += 1
			wend
			if( offset - start < c_literal_name_min or offset - start > c_literal_name_max ) then continue while
			var identifier = space(offset - start)
			memcpy(strptr(identifier), buffer + start, len(identifier))
			var entry = cptr(CLITERALSTORAGE ptr, hashLookup(@c_literal_names, strptr(identifier)))
			if( entry <> NULL ) then
				entry->references += 1
				if( start >= entry->declaration_prefix ) then
					var beginning = start - entry->declaration_prefix
					var length = cuint(entry->declaration.len)
					if( length <= bytes - beginning andalso memcmp(buffer + beginning, entry->declaration.data, length) = 0 ) then
						entry->declaration_at = beginning + 1
					end if
				end if
			end if
		else
			offset += 1
		end if
	wend
	var entry = cptr(CLITERALSTORAGE ptr, listGetHead(@c_literal_storage))
	while( entry <> NULL )
		if( entry->defined ) then
			if( entry->declaration_at = 0 ) then
				'' A missing declaration invalidates the recorded byte range.
				'' Report the internal failure before forming or writing a pointer.
				errReportEx(FB_ERRMSG_INTERNAL, __FUNCTION__)
				exit sub
			end if
			var declaration = buffer + entry->declaration_at - 1
			if( entry->references <= 1 ) then
				memset(declaration, asc(" "), entry->declaration.len)
			else
				if( entry->mutable_address ) then
					'' Source constness still controls expression and ABI views.
					'' A requested mutable address needs writable backing bytes,
					'' just as in the native backend's literal data section.
					memset(declaration + len("static "), asc(" "), len("const"))
				end if
				fbSemanticModelEmissionName(entry->sym, "c", *entry->identifier)
			end if
		end if
		entry = listGetNext(entry)
	wend
end sub

/' end of backend/c/c-literal-storage.bi '/
