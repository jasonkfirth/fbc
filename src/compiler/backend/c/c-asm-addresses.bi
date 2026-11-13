/'
	Project: FreeBASIC C backend
	File: backend/c/c-asm-addresses.bi
	Purpose: Preserve real storage addresses across Intel inline assembly.
	Responsibilities: Find assembly-visible parameter homes and format native
	address operands without introducing value copies around an assembly block.
	This file intentionally does NOT parse instruction effects or change the ABI.
'/

'' These symbols are borrowed only while their procedure is being emitted.
'' The list owns its nodes and is cleared before local symbols are released.
dim shared as TLIST c_asm_parameter_homes
dim shared as FBSYMBOL ptr c_asm_parameter_proc, c_asm_last_named
dim shared as integer c_asm_parameter_ready

private sub hCAsmHomesClear()
	if( c_asm_parameter_ready ) then listEnd(@c_asm_parameter_homes)
	c_asm_parameter_ready = FALSE
	c_asm_parameter_proc = NULL
	c_asm_last_named = NULL
end sub

private function hCAsmAddressOperands() as integer
	if( env.clopt.asmsyntax <> FB_ASMSYNTAX_INTEL ) then return FALSE
	select case fbGetCpuFamily()
	case FB_CPUFAMILY_X86, FB_CPUFAMILY_X86_64
		return TRUE
	end select
	return FALSE
end function

private function hCAsmStaticAddress(byval storage as FBSYMBOL ptr) as integer
	'' Private static storage has a stable native address. Automatic storage
	'' keeps a memory constraint: GCC may otherwise require a general register
	'' just to form its address at O0, while the assembly clobbers those registers.
	return symbIsLocal(storage) andalso symbIsStatic(storage) andalso _
	       (symbIsThreadLocal(storage) = FALSE)
end function

private function hCAsmParameterHome(byval proc as FBSYMBOL ptr, byval storage as FBSYMBOL ptr) as integer
	if( proc <> c_asm_parameter_proc or c_asm_parameter_ready = FALSE ) then return FALSE
	var item = cptr(FBSYMBOL ptr ptr, listGetHead(@c_asm_parameter_homes))
	while( item <> NULL )
		if( *item = storage ) then return TRUE
		item = listGetNext(item)
	wend
	return FALSE
end function

private sub hCAsmRememberParameterHome(byval storage as FBSYMBOL ptr)
	'' The existing reachable-storage walk calls this only for live ASM.
	'' An unreachable assembly statement must not create an unused C home.
	if( c_asm_parameter_proc = NULL or storage = NULL ) then exit sub
	if( symbIsVar(storage) = FALSE orelse symbIsParamVar(storage) = FALSE ) then exit sub
	if( storage = c_asm_last_named ) then exit sub
	var dtype = symbGetType(storage)
	'' Aggregate and managed-value parameter transport owns separate lifetime
	'' rules. Scalar values and pointer carriers can move directly on entry.
	if( typeIsPtr(dtype) = FALSE andalso _
	    (typeGetClass(dtype) = FB_DATACLASS_UDT or typeGetClass(dtype) = FB_DATACLASS_STRING) ) then exit sub
	if( hCAsmParameterHome(c_asm_parameter_proc, storage) ) then exit sub
	if( c_asm_parameter_ready = FALSE ) then
		listInit(@c_asm_parameter_homes, 8, sizeof(FBSYMBOL ptr))
		c_asm_parameter_ready = TRUE
	end if
	*cptr(FBSYMBOL ptr ptr, listNewNode(@c_asm_parameter_homes)) = storage
end sub

private sub hCAsmHomesBegin(byval proc as FBSYMBOL ptr)
	hCAsmHomesClear()
	if( proc = NULL ) then exit sub
	if( symbIsNaked(proc) or hCAsmAddressOperands() = FALSE ) then exit sub
	c_asm_parameter_proc = proc
	var tail = symbGetProcTailParam(proc)
	if( tail <> NULL andalso symbGetParamMode(tail) = FB_PARAMMODE_VARARG ) then
		'' __builtin_va_start must still name the last native fixed argument.
		if( tail->prev <> NULL ) then c_asm_last_named = symbGetParamVar(tail->prev)
	end if
end sub

private function hCAsmGCCAddressTemplate(byref code as string, byval first_address as integer) as string
	'' GCC's address modifier prints the selected memory addressing mode.
	'' Clang already prints an Intel address for an unmodified "p" operand;
	'' its %a modifier instead prints AT&T syntax even in an Intel template.
	dim as DZSTRING result
	DZstrZero(result)
	dim as integer i = 0
	while( i < len(code) )
		if( code[i] = asc("%") andalso i + 1 < len(code) ) then
			if( code[i + 1] = asc("%") ) then
				DZstrConcatAssign(result, "%%")
				i += 2
				continue while
			elseif( code[i + 1] >= asc("0") andalso code[i + 1] <= asc("9") ) then
				dim as integer operand = 0, j = i + 1
				while( j < len(code) andalso code[j] >= asc("0") andalso code[j] <= asc("9") )
					operand = operand * 10 + code[j] - asc("0")
					j += 1
				wend
				if( operand >= first_address ) then
					DZstrConcatAssign(result, "%a")
				else
					DZstrConcatAssign(result, "%")
				end if
				i += 1
				continue while
			end if
		end if
		DZstrConcatAssignC(result, code[i])
		i += 1
	wend
	if( result.data <> NULL ) then function = *result.data
	DZstrAllocate(result, 0)
end function

/' end of backend/c/c-asm-addresses.bi '/
