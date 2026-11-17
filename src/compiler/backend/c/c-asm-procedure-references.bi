/'
	Project: FreeBASIC C backend
	File: backend/c/c-asm-procedure-references.bi
	Purpose: Classify exposed procedure references and checked register effects.
	Responsibilities: Check a bounded Intel instruction subset and local labels
	before pruning, and describe its general-register effects for native emission.
	This file intentionally does NOT render instructions or own native storage.
'/

'' The emitter processes one procedure at a time. Outside a checked body,
'' raw assembly retains the module-wide conservative reachability policy.
dim shared c_asm_procedure_refs_complete as integer

enum CASMX86GPR
	C_ASM_GPR_EAX = 1 shl 0
	C_ASM_GPR_EBX = 1 shl 1
	C_ASM_GPR_ECX = 1 shl 2
	C_ASM_GPR_EDX = 1 shl 3
	C_ASM_GPR_ESI = 1 shl 4
	C_ASM_GPR_EDI = 1 shl 5
end enum

private function hCAsmReferenceToken(byval token as ASTASMTOK ptr) as ASTASMTOK ptr
	'' The ASM parser preserves whitespace as text tokens. Skip only spaces
	'' and tabs, leaving quoted text and instruction separators unclassified.
	while( token <> NULL )
		if( token->type <> AST_ASMTOK_TEXT orelse token->text = NULL ) then exit while
		var text = *token->text
		var blank = TRUE
		for i as integer = 0 to len(text)-1
			if( text[i] <> 9 and text[i] <> 32 ) then
				blank = FALSE
				exit for
			end if
		next
		if( blank = FALSE ) then exit while
		token = token->next
	wend
	return token
end function

private function hCAsmLocalLabel(byval token as ASTASMTOK ptr) as string
	token = hCAsmReferenceToken(token)
	if( token = NULL orelse token->type <> AST_ASMTOK_TEXT ) then return ""
	if( token->text = NULL ) then return ""
	var text = *token->text
	if( len(text) = 0 ) then return ""
	for i as integer = 0 to len(text)-1
		var character = text[i]
		if( (character >= asc("a") and character <= asc("z")) or _
		    (character >= asc("A") and character <= asc("Z")) or character = asc("_") ) then
			continue for
		end if
		if( i = 0 or character < asc("0") or character > asc("9") ) then return ""
	next
	var separator = hCAsmReferenceToken(token->next)
	if( separator = NULL orelse separator->type <> AST_ASMTOK_TEXT ) then return ""
	if( separator->text = NULL orelse *separator->text <> ":" ) then return ""
	if( hCAsmReferenceToken(separator->next) <> NULL ) then return ""
	return text
end function

private function hCAsmKnownOpcode(byref text as const string) as integer
	'' These ordinary instructions cannot define or manufacture native names.
	'' Calls, prefixes, macros and directives keep the opaque fallback.
	'' CPU queries have fixed register effects. Stack and x87 control-word
	'' operands still pass through the same resolved-symbol checks below.
	select case text
	case "mov", "lea", "add", "sub", "inc", "dec", "neg", "not", _
	     "and", "or", "xor", "shl", "shr", "sar", "imul", "cmp", "test", _
	     "jmp", "je", "jz", "jne", "jnz", "js", "jns", "jl", "jle", _
	     "jg", "jge", "jb", "jbe", "ja", "jae", "jc", "jnc", _
	     "fld", "fst", "fstp", "fist", "fistp", "fadd", "faddp", _
	     "fiadd", "fsub", "fsubp", "fmul", "fmulp", "fdiv", "fdivp", _
	     "finit", "fninit", "fnstcw", "fldcw", "push", "pop", _
	     "cpuid", "xgetbv", "nop"
		return TRUE
	end select
	return FALSE
end function

private function hCAsmRegister(byref text as const string) as integer
	select case text
	case "eax", "ebx", "ecx", "edx", "esi", "edi", "esp", "ebp", _
	     "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rsp", "rbp", _
	     "ax", "bx", "cx", "dx", "si", "di", "sp", "bp", _
	     "al", "ah", "bl", "bh", "cl", "ch", "dl", "dh", "st"
		return TRUE
	end select
	return FALSE
end function

private function hCAsmHexSuffix(byval token as ASTASMTOK ptr) as integer
	if( token = NULL orelse token->type <> AST_ASMTOK_TEXT ) then return FALSE
	if( token->text = NULL ) then return FALSE
	var text = lcase(*token->text)
	if( len(text) < 2 orelse text[0] <> asc("x") ) then return FALSE
	for i as integer = 1 to len(text)-1
		if( text[i] >= asc("0") and text[i] <= asc("9") ) then continue for
		if( text[i] >= asc("a") and text[i] <= asc("f") ) then continue for
		return FALSE
	next
	return TRUE
end function

private function hCAsmX86RegisterMask(byref text as const string) as integer
	'' Any named part of a general register conservatively affects its full
	'' native register. Stack/frame ownership keeps the emitter's ABI rules.
	select case text
	case "eax", "ax", "al", "ah"
		return C_ASM_GPR_EAX
	case "ebx", "bx", "bl", "bh"
		return C_ASM_GPR_EBX
	case "ecx", "cx", "cl", "ch"
		return C_ASM_GPR_ECX
	case "edx", "dx", "dl", "dh"
		return C_ASM_GPR_EDX
	case "esi", "si"
		return C_ASM_GPR_ESI
	case "edi", "di"
		return C_ASM_GPR_EDI
	end select
	return 0
end function

private function hCAsmX86GprMask(byval token as ASTASMTOK ptr, byref needs_frame as integer) as integer
	needs_frame = FALSE
	if( fbGetCpuFamily() <> FB_CPUFAMILY_X86 or c_asm_procedure_refs_complete = FALSE ) then return -1
	dim as integer mask = 0, line_begin = TRUE, stack_changed = FALSE, automatic_memory = FALSE
	while( token <> NULL )
		token = hCAsmReferenceToken(token)
		if( token = NULL ) then exit while
		if( token->type = AST_ASMTOK_SYMB ) then
			'' A procedure address can participate in a manually constructed
			'' call. Its register effects require the conservative native path.
			if( token->sym = NULL orelse token->sym->class = FB_SYMBCLASS_PROC ) then return -1
			if( token->sym->class = FB_SYMBCLASS_VAR ) then
				if( symbIsLocal(token->sym) andalso symbIsStatic(token->sym) = FALSE ) then automatic_memory = TRUE
			end if
		elseif( token->type = AST_ASMTOK_TEXT andalso token->text <> NULL ) then
			var text = lcase(*token->text)
			if( text = NEWLINE ) then
				line_begin = TRUE
			elseif( len(trim(text)) > 0 ) then
				if( line_begin ) then
					var following = hCAsmReferenceToken(token->next)
					if( following <> NULL andalso following->type = AST_ASMTOK_TEXT andalso _
					    following->text <> NULL andalso *following->text = ":" ) then
						'' The procedure proof already confines this label to the
						'' current joined template. Its name has no register effect.
						line_begin = FALSE
						token = following->next
						continue while
					end if
					'' Keep an explicit effect whitelist independent of the name-
					'' reference whitelist, so adding an opcode cannot omit an
					'' implicit register write from its native clobber contract.
					select case text
					case "cpuid"
						mask or= C_ASM_GPR_EAX or C_ASM_GPR_EBX or C_ASM_GPR_ECX or C_ASM_GPR_EDX
					case "xgetbv"
						mask or= C_ASM_GPR_EAX or C_ASM_GPR_ECX or C_ASM_GPR_EDX
					case "imul"
						'' The one-operand form writes EDX:EAX. Other forms can
						'' retain those extra clobbers while operands add their roots.
						mask or= C_ASM_GPR_EAX or C_ASM_GPR_EDX
					case "push", "pop"
						stack_changed = TRUE
					case "jmp", "je", "jz", "jne", "jnz", "js", "jns", "jl", "jle", _
					     "jg", "jge", "jb", "jbe", "ja", "jae", "jc", "jnc"
						'' Only a complete local-label operand stays inside this
						'' effect proof. Register/immediate targets can leave it.
						if( following = NULL orelse following->type <> AST_ASMTOK_TEXT ) then return -1
						if( following->text = NULL ) then return -1
						var destination = lcase(*following->text)
						if( len(destination) = 0 orelse hCAsmRegister(destination) ) then return -1
						var first = destination[0]
						if( (first < asc("a") or first > asc("z")) and first <> asc("_") ) then return -1
						var tail = hCAsmReferenceToken(following->next)
						if( tail <> NULL ) then
							if( tail->type <> AST_ASMTOK_TEXT orelse tail->text = NULL orelse *tail->text <> NEWLINE ) then return -1
						end if
					case "mov", "lea", "add", "sub", "inc", "dec", "neg", "not", _
					     "and", "or", "xor", "shl", "shr", "sar", "cmp", "test", _
					     "fld", "fst", "fstp", "fist", "fistp", "fadd", "faddp", _
					     "fiadd", "fsub", "fsubp", "fmul", "fmulp", "fdiv", "fdivp", _
					     "finit", "fninit", "fnstcw", "fldcw", "nop"
					case else
						return -1
					end select
					line_begin = FALSE
				else
					mask or= hCAsmX86RegisterMask(text)
				end if
			end if
		end if
		token = token->next
	wend
	'' A frame-address input keeps automatic memory references stable while
	'' PUSH/POP temporarily change ESP inside the joined native template.
	needs_frame = stack_changed and automatic_memory
	return mask
end function

private function hCAsmProcedureLineComplete _
	( byval token as ASTASMTOK ptr, byref labels as THASH ) as integer

	token = hCAsmReferenceToken(token)
	if( token = NULL ) then return TRUE
	if( len(hCAsmLocalLabel(token)) > 0 ) then return TRUE
	if( token->type <> AST_ASMTOK_TEXT orelse token->text = NULL ) then return FALSE
	var opcode = lcase(*token->text)
	if( hCAsmKnownOpcode(opcode) = FALSE ) then return FALSE
	token = hCAsmReferenceToken(token->next)
	while( token <> NULL )
		select case token->type
		case AST_ASMTOK_SYMB
			if( token->sym = NULL ) then return FALSE
			select case token->sym->class
			case FB_SYMBCLASS_PROC, FB_SYMBCLASS_VAR
			case FB_SYMBCLASS_LABEL
				'' ASM GOTO breaks the joined template in astLoadASM(). Keep
				'' that form opaque instead of borrowing another template's labels.
				return FALSE
			case else
				return FALSE
			end select
		case AST_ASMTOK_TEXT
			if( token->text = NULL ) then return FALSE
			var text = *token->text
			var lowertext = lcase(text)
			select case lowertext
			case "[", "]", "(", ")", "+", "-", "*", ","
			case "byte", "word", "dword", "qword", "tbyte"
				'' A size word is a modifier only in this explicit memory form.
				'' Otherwise it could be the native alias of a private procedure.
				var qualifier = hCAsmReferenceToken(token->next)
				if( qualifier = NULL orelse qualifier->type <> AST_ASMTOK_TEXT ) then return FALSE
				if( qualifier->text = NULL orelse lcase(*qualifier->text) <> "ptr" ) then return FALSE
				var opening = hCAsmReferenceToken(qualifier->next)
				if( opening = NULL orelse opening->type <> AST_ASMTOK_TEXT ) then return FALSE
				if( opening->text = NULL orelse *opening->text <> "[" ) then return FALSE
				token = qualifier
			case else
				if( hCAsmRegister(lowertext) = FALSE ) then
					var local_label = FALSE
					if( labels.list <> NULL ) then local_label = (hashLookup(@labels, strptr(text)) <> NULL)
					if( local_label = FALSE ) then
						if( len(text) = 0 ) then return FALSE
						'' The lexer separates an assembler 0x literal into 0 and
						'' an adjacent text suffix. Resolved symbols and whitespace
						'' cannot complete that literal and keep their existing rules.
						if( text = "0" andalso hCAsmHexSuffix(token->next) ) then
							token = token->next
						else
							for i as integer = 0 to len(text)-1
								if( text[i] < asc("0") or text[i] > asc("9") ) then return FALSE
							next
						end if
					end if
				end if
			end select
		case else
			return FALSE
		end select
		token = hCAsmReferenceToken(token->next)
	wend
	return TRUE
end function

private function hCAsmProcedureTreeComplete _
	( byval n as ASTNODE ptr, byref labels as THASH ) as integer

	if( n = NULL ) then return TRUE
	if( n->class = AST_NODECLASS_ASM ) then
		if( hCAsmProcedureLineComplete(n->asm.tokhead, labels) = FALSE ) then return FALSE
	end if
	return hCAsmProcedureTreeComplete(n->l, labels) andalso hCAsmProcedureTreeComplete(n->r, labels)
end function

private function hCAsmProcedureGroupComplete _
	( byval first as ASTNODE ptr, byval following as ASTNODE ptr ) as integer

	'' astLoadASM joins adjacent Intel ASM lines across source annotations.
	'' A raw label is safe to resolve only inside that same native template.
	'' BASIC control flow between templates can discard one independently.
	dim labels as THASH
	var n = first
	while( n <> following )
		if( n->class = AST_NODECLASS_ASM ) then
			var identifier = hCAsmLocalLabel(n->asm.tokhead)
			if( len(identifier) > 0 ) then
				'' Fixed buckets bound initial allocation, not the label count.
				if( labels.list = NULL ) then hashInit(@labels, 32, TRUE)
				if( hashLookup(@labels, strptr(identifier)) = NULL ) then
					hashAdd(@labels, ZstrDup(strptr(identifier)), cptr(any ptr, 1), hashHash(strptr(identifier)))
				end if
			end if
		end if
		n = n->next
	wend
	var complete = TRUE
	n = first
	while( n <> following )
		if( hCAsmProcedureTreeComplete(n, labels) = FALSE ) then
			complete = FALSE
			exit while
		end if
		n = n->next
	wend
	if( labels.list <> NULL ) then hashEnd(@labels)
	return complete
end function

private function hCAsmProcedureReferencesComplete(byval first as ASTNODE ptr) as integer
	if( env.clopt.asmsyntax <> FB_ASMSYNTAX_INTEL ) then return FALSE
	select case fbGetCpuFamily()
	case FB_CPUFAMILY_X86, FB_CPUFAMILY_X86_64
	case else
		return FALSE
	end select
	dim empty_labels as THASH
	var n = first
	while( n <> NULL )
		if( n->class = AST_NODECLASS_ASM ) then
			var following = n->next
			while( following <> NULL )
				select case following->class
				case AST_NODECLASS_ASM, AST_NODECLASS_DBG, AST_NODECLASS_LIT
				case else
					exit while
				end select
				following = following->next
			wend
			if( hCAsmProcedureGroupComplete(n, following) = FALSE ) then return FALSE
			n = following
			continue while
		end if
		'' An ASM node nested in another expression has no adjacent template
		'' proof. Resolved symbols remain usable; unknown names stay opaque.
		if( hCAsmProcedureTreeComplete(n, empty_labels) = FALSE ) then return FALSE
		n = n->next
	wend
	return TRUE
end function

'' end of backend/c/c-asm-procedure-references.bi
