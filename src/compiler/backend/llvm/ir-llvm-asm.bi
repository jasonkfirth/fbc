'' Project: FreeBASIC compiler - LLVM inline assembly
'' -----------------------------------------
''
'' File: backend/llvm/ir-llvm-asm.bi
''
'' Purpose:
''
''     Emit one LLVM asm call for a consecutive sequence of x86 instructions.
''
'' Responsibilities:
''
''     - translate resolved BASIC symbols to LLVM constraint operands
''     - preserve memory access and physical register clobber contracts
''     - normalize Intel operand qualifiers and escape LLVM strings
''
'' This file intentionally does NOT contain:
''
''     - assembly parsing, BASIC control flow, or external tool invocation
''

#ifndef __FB_IR_LLVM_ASM_BI__
#define __FB_IR_LLVM_ASM_BI__

private function hAsmMemoryPlaceholders( byref text as string ) as string
	'' LLVM's memory operand already includes brackets and a size qualifier.
	'' Remove the redundant brackets around a plain placeholder, just as the
	'' C emitter does for Clang. Preserve other assembly punctuation.
	dim as string result
	dim as integer i = 1
	while( i <= len( text ) )
		if( mid( text, i, 2 ) = "[$" ) then
			var j = i + 2
			while( j <= len( text ) )
				var ch = asc( mid( text, j, 1 ) )
				if( (ch < asc("0")) or (ch > asc("9")) ) then exit while
				j += 1
			wend
			if( (j > i + 2) andalso (mid( text, j, 1 ) = "]") ) then
				result += mid( text, i + 1, j - i - 1 )
				i = j + 1
				continue while
			end if
		end if
		result += mid( text, i, 1 )
		i += 1
	wend
	return result
end function

private sub hAsmClobbers( byref constraints as string )
	'' BASIC asm has no explicit clobber list. Assume general registers and
	'' vector registers may change. The user must restore stack/frame pointers.
	if( fbGetCpuFamily() = FB_CPUFAMILY_X86_64 ) then
		constraints += ",~{rax},~{rbx},~{rcx},~{rdx},~{rsi},~{rdi}"
		for i as integer = 8 to 15
			constraints += ",~{r" & i & "}"
		next
	else
		constraints += ",~{eax},~{ebx},~{ecx},~{edx},~{esi},~{edi}"
	end if
	for i as integer = 0 to iif( fbIs64bit(), 15, 7 )
		constraints += ",~{xmm" & i & "}"
		next
	for i as integer = 0 to 7
		constraints += ",~{mm" & i & "}"
		next
end sub

private sub _emitAsmLine( byval asmtokenhead as ASTASMTOK ptr )
	dim as string asmtext, operands, constraints, escaped
	dim as integer operandcount = 0, lea = FALSE, linestart = TRUE

	var n = asmtokenhead
	while( n <> NULL )
		select case( n->type )
		case AST_ASMTOK_TEXT
			var text = *n->text
			if( text = NEWLINE ) then
				linestart = TRUE
				lea = FALSE
			elseif( linestart andalso (len(trim(text)) > 0) ) then
				lea = (lcase(text) = "lea")
				linestart = FALSE
			end if
			select case( lcase(text) )
			case "ptr", "offset", "byte", "word", "dword", "qword", _
			     "tbyte", "fword", "xmmword", "ymmword", "zmmword"
				text = lcase( text )
			end select
			'' LEA already selects an address; LLVM rejects redundant OFFSET.
			if( (lea = FALSE) or (text <> "offset") ) then asmtext += text

		case AST_ASMTOK_SYMB
			if( operandcount > 0 ) then
				operands += ", "
				constraints += ","
			end if
			if( symbIsProc( n->sym ) ) then
				asmtext += "${" & operandcount & ":c}"
				var ptrtype = hEmitType( typeAddrOf( FB_DATATYPE_FUNCTION ), n->sym )
				operands += ptrtype + " " + hEmitProcName( n->sym )
				constraints += "i"
			else
				'' Passing an address in a general register can overwrite RAX
				'' before a user instruction stores it. Let LLVM select the memory
				'' address directly. elementtype is required with opaque pointers.
				asmtext += "$" & operandcount
				var dtype = hEmitSymType( n->sym )
				operands += dtype + "* elementtype(" + dtype + ") " + hEmitVarName( n->sym )
				constraints += "*m"
			end if
			operandcount += 1
		end select
		n = n->next
	wend

	asmtext = hAsmMemoryPlaceholders( asmtext )
	'' LLVM IR strings use hexadecimal escapes, not C string escapes.
	for i as integer = 1 to len( asmtext )
		var ch = asc( mid( asmtext, i, 1 ) )
		select case( ch )
		case 10, 13, 34, 92
			escaped += $"\" + hex( ch, 2 )
		case else
			escaped += chr( ch )
		end select
	next
	if( operandcount > 0 ) then constraints += ","
	constraints += "~{cc},~{memory}"
	hAsmClobbers( constraints )
	hWriteLine( "call void asm sideeffect alignstack inteldialect """ + escaped + """, """ + _
		constraints + """( " + operands + " )" )
end sub

#endif

'' end of backend/llvm/ir-llvm-asm.bi
