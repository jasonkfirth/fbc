/'
	Project: FreeBASIC C backend
	File: backend/c/c-abi-parameters.bi
	Purpose: Express ignored formal parameters required by a calling contract.
	Responsibilities: Own per-output names and check unused parameters against
	the complete emitted procedure body.
	This file intentionally does NOT contain: procedure type, call operand or BASIC diagnostic changes.
'/

type CABIPARAM
	name as zstring ptr
	insert_at as uinteger
	emit_unused as integer
	next_param as CABIPARAM ptr
end type

type CABIPROC
	name as zstring ptr
	header as DZSTRING
	parameters as CABIPARAM ptr
	next_definition as CABIPROC ptr
	header_at as uinteger
	emit_header as integer
	semantic_signature as longint
end type

private const C_ABI_C23_BEGIN = !"#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L\n"
private const C_ABI_C23_ELSE = !"\n#else\n"
private const C_ABI_C23_END = !"\n#endif\n"

'' These pools own C spellings, not symbols. Local symbols may already have
'' expired when the final output observes global callback initializers.
dim shared as TLIST c_abi_procs, c_abi_params
dim shared as THASH c_abi_names
dim shared as CABIPROC ptr c_abi_definitions, c_abi_definition_tail
dim shared as integer c_abi_ready

private sub hCAbiFinish( )
	if( c_abi_ready = FALSE ) then exit sub
	hashEnd(@c_abi_names)
	var param = cptr(CABIPARAM ptr, listGetHead(@c_abi_params))
	while( param <> NULL )
		ZstrFree(param->name)
		param = listGetNext(param)
	wend
	var proc = cptr(CABIPROC ptr, listGetHead(@c_abi_procs))
	while( proc <> NULL )
		ZstrFree(proc->name)
		DZstrAllocate(proc->header, 0)
		proc = listGetNext(proc)
	wend
	listEnd(@c_abi_params)
	listEnd(@c_abi_procs)
	c_abi_definitions = NULL
	c_abi_definition_tail = NULL
	c_abi_ready = FALSE
end sub

private sub hCAbiInit( )
	hCAbiFinish( )
	'' Match the existing procedure pool block size; the pools grow as needed.
	listInit(@c_abi_procs, 32, sizeof(CABIPROC), LIST_FLAGS_CLEAR)
	listInit(@c_abi_params, 32, sizeof(CABIPARAM), LIST_FLAGS_CLEAR)
	hashInit(@c_abi_names, 128, FALSE) '' Chained buckets do not cap procedure count.
	c_abi_ready = TRUE
end sub

private function hCAbiGet( byval sym as FBSYMBOL ptr ) as CABIPROC ptr
	if( c_abi_ready = FALSE ) then return NULL
	var identifier = hGetCId(sym)
	if( len(identifier) = 0 ) then return NULL
	var entry = cptr(CABIPROC ptr, hashLookup(@c_abi_names, strptr(identifier)))
	if( entry = NULL ) then
		entry = listNewNode(@c_abi_procs)
		entry->name = ZstrDup(strptr(identifier))
		hashAdd(@c_abi_names, entry->name, entry, hashHash(entry->name))
	end if
	return entry
end function

private sub hCAbiParameter _
	( byref first as CABIPARAM ptr, byref last as CABIPARAM ptr, _
	  byval param as FBSYMBOL ptr, byval insert_at as uinteger )

	if( c_abi_ready = FALSE orelse c_local_refs_complete = FALSE ) then exit sub
	var storage = symbGetParamVar(param)
	if( storage = NULL ) then exit sub
	if( symbIsParamVar(storage) = FALSE ) then exit sub
	'' Access flags can describe reads removed with private dead stores.
	'' The late body check keeps every name still needed by emitted C.
	var identifier = hGetCId(storage)
	if( len(identifier) = 0 ) then exit sub
	var entry = cptr(CABIPARAM ptr, listNewNode(@c_abi_params))
	entry->name = ZstrDup(strptr(identifier))
	entry->insert_at = insert_at
	if( last <> NULL ) then
		last->next_param = entry
	else
		first = entry
	end if
	last = entry
end sub

private sub hCAbiDefinition _
	( byval sym as FBSYMBOL ptr, byref header as string, _
	  byval prefix_length as uinteger, byval first as CABIPARAM ptr, _
	  byval semantic_signature as longint )

	if( first = NULL ) then exit sub
	var entry = hCAbiGet(sym)
	if( entry = NULL ) then exit sub
	'' Different BASIC signatures can have the same C name. Keep the first
	'' optional annotation record; linking it again would cycle the list.
	'' The unchanged definitions still reach the C compiler for diagnosis.
	if( entry->header.data <> NULL ) then
		if( semantic_signature <> 0 ) then fbSemanticModelBackendAbiEnd(semantic_signature, header)
		exit sub
	end if
	assert(entry->header.data = NULL)
	DZstrAssign(entry->header, strptr(header))
	entry->parameters = first
	entry->semantic_signature = semantic_signature
	var param = first
	while( param <> NULL )
		param->insert_at += prefix_length
		assert(param->insert_at <= len(header))
		param = param->next_param
	wend
	if( c_abi_definition_tail <> NULL ) then
		c_abi_definition_tail->next_definition = entry
	else
		c_abi_definitions = entry
	end if
	c_abi_definition_tail = entry
end sub

'' -------------------------------------------------------------------------
'' Bounded inspection of the final C buffer
'' -------------------------------------------------------------------------

private function hCAbiFind _
	( byval buffer as const ubyte ptr, byval size as uinteger, _
	  byval begin_at as uinteger, byval needle as const ubyte ptr, _
	  byval needle_length as uinteger, byref found as uinteger ) as integer

	if( begin_at > size orelse needle_length = 0 ) then return FALSE
	if( needle_length > size - begin_at ) then return FALSE
	var last = size - needle_length
	var position = begin_at
	do
		if( buffer[position] = needle[0] ) then
			if( memcmp(buffer + position, needle, needle_length) = 0 ) then
				found = position
				return TRUE
			end if
		end if
		if( position = last ) then exit do
		position += 1
	loop
	return FALSE
end function

private function hCAbiIdentifierByte( byval value as ubyte ) as integer
	select case value
	case asc("a") to asc("z"), asc("A") to asc("Z"), asc("0") to asc("9"), asc("_"), asc("$"), 128 to 255
		return TRUE
	end select
	return FALSE
end function

private function hCAbiHasReference _
	( byval buffer as const ubyte ptr, byval body_last as uinteger, _
	  byval body_first as uinteger, byval identifier as const ubyte ptr, _
	  byval name_length as uinteger ) as integer

	'' Short C ABI names must not match parts of unrelated identifiers.
	'' Strings and comments may conservatively retain a name; they cannot
	'' cause a required parameter name to be removed.
	var search_at = body_first
	dim as uinteger position
	while( hCAbiFind(buffer, body_last, search_at, identifier, name_length, position) )
		var after_name = position + name_length
		if( ((position = body_first) orelse hCAbiIdentifierByte(buffer[position - 1]) = FALSE) andalso _
		    ((after_name = body_last) orelse hCAbiIdentifierByte(buffer[after_name]) = FALSE) ) then return TRUE
		search_at = after_name
	wend
	return FALSE
end function

private sub hCAbiPrepare( byval buffer as const ubyte ptr, byval size as uinteger )
	dim as uinteger search_at = 0
	var entry = c_abi_definitions
	while( entry <> NULL )
		'' Removing a name leaves the complete calling signature intact for
		'' both private direct calls and externally visible procedures.
		var header_length = cuint(len(*entry->header.data))
		dim as uinteger position
		var found = hCAbiFind(buffer, size, search_at, cptr(const ubyte ptr, entry->header.data), _
		                     header_length, position)
		'' Read-only ABI wrappers can be written after later procedure bodies.
		'' Retain the forward scan, then check earlier bodies if it misses.
		if( found = FALSE andalso search_at > 0 ) then
			found = hCAbiFind(buffer, size, 0, cptr(const ubyte ptr, entry->header.data), _
			                  header_length, position)
		end if
		if( found ) then
			'' Definitions are emitted in source order at column zero. A
			'' prototype has no formal names, and cannot match this header.
			var body_first = position + header_length
			while( body_first < size )
				select case buffer[body_first]
				case asc(!"\t"), asc(!"\n"), asc(!"\r"), asc(" ")
					body_first += 1
				case else
					exit while
				end select
			wend
			if( ((position = 0) orelse (buffer[position - 1] = asc(!"\n"))) andalso _
			    (body_first < size) andalso (buffer[body_first] = asc("{")) ) then
				dim as uinteger body_last
				'' Nested scopes retain indentation. The outer procedure
				'' brace is the first closing brace at column zero. Opaque
				'' assembly is excluded before inspecting this buffer.
				if( hCAbiFind(buffer, size, body_first + 1, cptr(const ubyte ptr, strptr(!"\n}")), _
				              2, body_last) andalso (body_last + 2 < size) ) then
					if( buffer[body_last + 2] = asc(!"\n") orelse buffer[body_last + 2] = asc(!"\r") ) then
						entry->header_at = position
						var param = entry->parameters
						while( param <> NULL )
							var name_length = cuint(len(*param->name))
							if( (param->insert_at >= name_length) andalso _
							    (param->insert_at <= header_length) ) then
								var name_at = param->insert_at - name_length
								if( memcmp(cptr(const ubyte ptr, entry->header.data) + name_at, _
								           param->name, name_length) = 0 ) then
									if( hCAbiHasReference(buffer, body_last, body_first + 1, _
									                      cptr(const ubyte ptr, param->name), name_length) = FALSE ) then
										param->emit_unused = TRUE
										entry->emit_header = TRUE
									end if
								end if
							end if
							param = param->next_param
						wend
						search_at = body_last + 2
					end if
				end if
			end if
		end if
		entry = entry->next_definition
	wend
end sub

private function hCAbiUnnamedHeader( byval entry as CABIPROC ptr ) as string
	'' The C23 definition keeps every argument type and removes only names
	'' absent from the complete emitted body after private-store removal.
	dim as string header
	dim as uinteger cursor = 0
	var param = entry->parameters
	while( param <> NULL )
		if( param->emit_unused ) then
			var name_at = param->insert_at - cuint(len(*param->name))
			assert(name_at >= cursor)
			header += mid(*entry->header.data, cursor + 1, name_at - cursor)
			cursor = param->insert_at
		end if
		param = param->next_param
	wend
	header += mid(*entry->header.data, cursor + 1)
	return header
end function

private sub hCAbiSemanticHeaders( )
	var entry = c_abi_definitions
	while( entry <> NULL )
		if( entry->semantic_signature <> 0 ) then
			dim as string header
			if( entry->emit_header ) then
				header = hCAbiUnnamedHeader(entry)
			else
				header = *entry->header.data
			end if
			fbSemanticModelBackendAbiEnd(entry->semantic_signature, header)
		end if
		entry = entry->next_definition
	wend
end sub

'' -------------------------------------------------------------------------
'' Final output
'' -------------------------------------------------------------------------

private function hCAbiWriteChunk( byval buffer as const ubyte ptr, byval size as uinteger ) as integer
	if( size = 0 ) then return TRUE
	return (put(#env.outf.num, , *buffer, size) = 0)
end function

private function hCAbiWriteOutput _
	( byval buffer as const ubyte ptr, byval size as uinteger ) as integer

	if( c_abi_ready = FALSE ) then
		return hCAbiWriteChunk(buffer, size)
	end if
	'' Candidate parameters already passed the procedure-local assembly
	'' check. An opaque body elsewhere cannot refer to their local names.
	if( env.clopt.debuginfo = FALSE ) then hCAbiPrepare(buffer, size)
	hCAbiSemanticHeaders( )
	'' GNU11 requires parameter names in definitions. C23 permits an unnamed
	'' parameter, so emit both headers and let the C compiler select its path.
	'' The body and the ABI are identical in both paths. No warning attribute is
	'' needed, and the legacy path still exposes its unused-parameter warnings.
	dim as uinteger cursor = 0
	var entry = c_abi_definitions
	while( entry <> NULL )
		if( entry->emit_header ) then
			var position = entry->header_at
			var header_length = cuint(len(*entry->header.data))
			assert(position >= cursor andalso position <= size)
			assert(header_length <= size - position)
			if( hCAbiWriteChunk(buffer + cursor, position - cursor) = FALSE ) then return FALSE
			if( hCAbiWriteChunk(cptr(const ubyte ptr, strptr(C_ABI_C23_BEGIN)), len(C_ABI_C23_BEGIN)) = FALSE ) then return FALSE
			var unnamed_header = hCAbiUnnamedHeader(entry)
			if( hCAbiWriteChunk(cptr(const ubyte ptr, strptr(unnamed_header)), len(unnamed_header)) = FALSE ) then return FALSE
			if( hCAbiWriteChunk(cptr(const ubyte ptr, strptr(C_ABI_C23_ELSE)), len(C_ABI_C23_ELSE)) = FALSE ) then return FALSE
			if( hCAbiWriteChunk(buffer + position, header_length) = FALSE ) then return FALSE
			if( hCAbiWriteChunk(cptr(const ubyte ptr, strptr(C_ABI_C23_END)), len(C_ABI_C23_END)) = FALSE ) then return FALSE
			cursor = position + header_length
		end if
		entry = entry->next_definition
	wend
	return hCAbiWriteChunk(buffer + cursor, size - cursor)
end function

'' end of backend/c/c-abi-parameters.bi
