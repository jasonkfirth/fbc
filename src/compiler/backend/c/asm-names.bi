'' Project: FreeBASIC compiler - C inline assembly
'' File: backend/c/asm-names.bi
'' Purpose: Keep assembly-local names unique when a C optimizer copies a template.
'' Responsibilities: Record raw name ownership before ASM nodes are consumed,
''     preserve references across separate regions, and localize contained labels.
'' This file intentionally does NOT contain: AST mutation or assembler execution.

'' The emitter walks one procedure at a time. These tables borrow no AST storage
'' and are released at procedure/module end, including a subsequent empty unit.
dim shared as TSTRSET asmnames
dim shared as integer asmnamesactive, asmnamesopaque

const C_ASM_MULTIPLE_REGIONS = -1
enum C_ASM_NAME_PASS
	C_ASM_NAME_RECORD
	C_ASM_NAME_DEFINE
	C_ASM_NAME_EMIT
end enum

private function hAsmNameChar( byval ch as ubyte, byval first as integer ) as integer
	select case ch
	case asc("A") to asc("Z"), asc("a") to asc("z"), asc("_"), asc("."), asc("$"), asc("@")
		return TRUE
	case asc("0") to asc("9")
		return (first = FALSE)
	end select
	return FALSE
end function

private function hAsmNamesText _
	( _
		byref code as string, _
		byref names as TSTRSET, _
		byval pass as C_ASM_NAME_PASS, _
		byval region as integer = 0 _
	) as string

	dim as DZSTRING result
	DZstrZero( result )
	dim as integer i = 1, length = len( code ), linestart = TRUE
	while( i <= length )
		var first = i
		var ch = code[i - 1]
		if( ch = asc("""") or ch = asc("'") ) then
			'' Quoted assembler data is opaque, including escaped quotes.
			var quotechar = ch
			i += 1
			while( i <= length )
				ch = code[i - 1]
				i += 1
				if( ch = asc("\") ) then
					if( i <= length ) then i += 1
				elseif( ch = quotechar ) then
					exit while
				end if
			wend
			linestart = FALSE
		elseif( ch = asc("#") or (ch = asc("/") andalso i < length andalso code[i] = asc("/")) ) then
			while( i <= length andalso code[i - 1] <> 10 )
				i += 1
			wend
		elseif( ch = asc("%") ) then
			'' C operand placeholders are not assembler identifiers.
			i += 1
			while( i <= length andalso hAsmNameChar(code[i - 1], FALSE) )
				i += 1
			wend
			if( i <= length andalso code[i - 1] = asc("=") ) then i += 1
			linestart = FALSE
		elseif( hAsmNameChar(ch, TRUE) ) then
			i += 1
			while( i <= length andalso hAsmNameChar(code[i - 1], FALSE) )
				i += 1
			wend
			var word = mid( code, first, i - first )
			var afterword = i
			while( afterword <= length andalso (code[afterword - 1] = 32 or code[afterword - 1] = 9) )
				afterword += 1
			wend
			var definition = linestart andalso afterword <= length andalso code[afterword - 1] = asc(":")
			select case pass
			case C_ASM_NAME_RECORD
				if( linestart ) then
					select case lcase( word )
					case ".global", ".globl", ".weak", ".macro"
						asmnamesopaque = TRUE
					end select
				end if
				'' An instruction/directive position is not a local-name reference,
				'' even when a newer mnemonic is absent from the keyword table.
				var ownership = iif(linestart andalso definition = FALSE, C_ASM_MULTIPLE_REGIONS, region)
				var item = cptr(TSTRSETITEM ptr, hashLookup(@names.hash, strptr(word)))
				if( item = NULL ) then
					strsetAdd( @names, word, ownership )
				elseif( item->userdata <> ownership ) then
					item->userdata = C_ASM_MULTIPLE_REGIONS
				end if
			case C_ASM_NAME_DEFINE
				if( definition andalso word <> "." andalso parserIsInlineAsmKeyword(strptr(word)) = FALSE ) then
					var item = cptr(TSTRSETITEM ptr, hashLookup(@asmnames.hash, strptr(word)))
					if( item <> NULL andalso item->userdata <> C_ASM_MULTIPLE_REGIONS ) then
						strsetAdd( @names, word, 0 )
					end if
				end if
			case C_ASM_NAME_EMIT
				DZstrConcatAssign( result, word )
				if( hashLookup(@names.hash, strptr(word)) <> NULL ) then
					'' %= is unique per C asm instance, including an inlined copy.
					DZstrConcatAssign( result, "_fbasm_%=" )
				end if
			end select
			linestart = definition
			if( pass = C_ASM_NAME_EMIT ) then continue while
		else
			i += 1
			select case ch
			case 10, asc(";")
				linestart = TRUE
			case 9, 13, 32, asc(":")
				'' Keep a label's following instruction at the statement start.
			case else
				linestart = FALSE
			end select
		end if
		if( pass = C_ASM_NAME_EMIT ) then
			DZstrConcatAssign( result, mid(code, first, i - first) )
		end if
	wend
	if( result.data <> NULL ) then function = *result.data
	DZstrAllocate( result, 0 )
end function

private sub hAsmEndNames( )
	if( asmnamesactive ) then strsetEnd( @asmnames )
	asmnamesactive = FALSE
	asmnamesopaque = FALSE
end sub

private sub hAsmBeginNames( byval proc as FBSYMBOL ptr )
	hAsmEndNames( )
	if( symbIsNaked(proc) or env.clopt.asmsyntax <> FB_ASMSYNTAX_INTEL ) then exit sub
	select case fbGetCpuFamily( )
	case FB_CPUFAMILY_X86, FB_CPUFAMILY_X86_64
	case else
		exit sub
	end select
	var n = astGetProc()->l
	dim as integer region = 0, inregion = FALSE
	while( n )
		select case n->class
		case AST_NODECLASS_ASM
			dim as integer basiclabel = FALSE
			var scan = n->asm.tokhead
			while( scan )
				if( scan->type = AST_ASMTOK_SYMB andalso scan->sym <> NULL ) then
					if( scan->sym->class = FB_SYMBCLASS_LABEL ) then basiclabel = TRUE
				end if
				scan = scan->next
			wend
			if( basiclabel ) then inregion = FALSE
			if( asmnamesactive = FALSE ) then
				strsetInit( @asmnames, 16 )
				asmnamesactive = TRUE
			end if
			if( inregion = FALSE ) then region += 1
			inregion = TRUE
			'' Replace resolved BASIC operands with punctuation: only raw ASM
			'' names belong to this table; BASIC labels already use asm goto.
			dim as DZSTRING rawtext
			DZstrZero( rawtext )
			var token = n->asm.tokhead
			while( token )
				if( token->type = AST_ASMTOK_TEXT ) then
					DZstrConcatAssign( rawtext, *token->text )
				else
					DZstrConcatAssign( rawtext, "?" )
				end if
				token = token->next
			wend
			if( rawtext.data <> NULL ) then
				var code = *rawtext.data
				hAsmNamesText( code, asmnames, C_ASM_NAME_RECORD, region )
			end if
			DZstrAllocate( rawtext, 0 )
			if( basiclabel ) then inregion = FALSE
		case AST_NODECLASS_DBG, AST_NODECLASS_LIT
			'' Source annotations do not end a consecutive assembly region.
		case else
			inregion = FALSE
		end select
		n = n->next
	wend
end sub

private function hAsmLocalizeNames( byref code as string ) as string
	if( asmnamesactive = FALSE or asmnamesopaque ) then return code
	dim as TSTRSET labels
	strsetInit( @labels, 16 )
	hAsmNamesText( code, labels, C_ASM_NAME_DEFINE )
	function = hAsmNamesText( code, labels, C_ASM_NAME_EMIT )
	strsetEnd( @labels )
end function

'' end of backend/c/asm-names.bi
