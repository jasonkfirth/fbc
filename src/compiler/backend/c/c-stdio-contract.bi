'' Project: FreeBASIC compiler - C stdio declarations
'' File: backend/c/c-stdio-contract.bi
'' Purpose: Expose the existing stream pointee type to C builtin declarations.
'' Responsibilities: Find one consistent global FILE binding before final output.
'' This file intentionally does NOT contain: stream layouts, header inclusion,
''     procedure renaming, new ABI conversions or diagnostic suppression.

'' Scratch state for one synchronous symbol-table walk. Keep owned C spelling,
'' rather than borrowing symbols after the callback has returned.
dim shared as string c_stdio_file_type
dim shared as integer c_stdio_file_conflict

private sub hCStdioObserveType( byval dtype as integer, byval subtype as FBSYMBOL ptr )

	if( typeGetPtrCnt(dtype) <> 1 ) then
		c_stdio_file_conflict = TRUE
		exit sub
	end if
	dim as string spelling
	select case typeGetDtOnly(dtype)
	case FB_DATATYPE_STRUCT
		if( subtype = NULL orelse symbIsLocal(subtype) ) then
			c_stdio_file_conflict = TRUE
			exit sub
		end if
		'' The tag can be forward-declared before the generated record body.
		spelling = hGetUdtName(subtype)
	case FB_DATATYPE_FWDREF, FB_DATATYPE_VOID
		'' Unresolved pointees already use void* in C procedure declarations.
		spelling = "void"
	case else
		c_stdio_file_conflict = TRUE
		exit sub
	end select
	if( len(c_stdio_file_type) = 0 ) then
		c_stdio_file_type = spelling
	elseif( c_stdio_file_type <> spelling ) then
		c_stdio_file_conflict = TRUE
	end if
end sub

private sub hCStdioObserveProc( byval proc as FBSYMBOL ptr )

	if( symbGetIsFuncPtr(proc) ) then exit sub
	if( (symbGetIsAccessed(proc) = FALSE) andalso _
	    (symbGetProcIsEmitted(proc) = FALSE) ) then exit sub
	var rawname = symbGetMangledName(proc)
	if( rawname = NULL ) then exit sub
	dim as integer fileparam = 0
	select case *rawname
	case "fopen", "freopen", "tmpfile", "popen", "fdopen"
		hCStdioObserveType(symbGetProcRealType(proc), symbGetProcRealSubtype(proc))
		if( *rawname = "freopen" ) then fileparam = 3
	case "fclose", "fflush", "feof", "ferror", "clearerr", "fgetc", "getc", _
	     "getc_unlocked", "fileno", "fgetpos", "fsetpos", "ftell", "fseek", _
	     "rewind", "setbuf", "setvbuf", "fscanf", "fprintf", "vfprintf", _
	     "vfscanf", "flockfile", "funlockfile", "ftrylockfile", "getw", _
	     "fgetwc", "getwc", "fwide", "fwprintf", "vfwprintf", "fwscanf", "vfwscanf"
		fileparam = 1
	case "fputc", "fputs", "putc", "putc_unlocked", "ungetc", "putw", _
	     "fputwc", "putwc", "ungetwc", "fputws"
		fileparam = 2
	case "fgets", "fgetws"
		fileparam = 3
	case "fread", "fwrite"
		fileparam = 4
	case else
		exit sub
	end select
	if( symbGetProcMode(proc) <> FB_FUNCMODE_CDECL ) then
		c_stdio_file_conflict = TRUE
		exit sub
	end if
	if( fileparam = 0 ) then exit sub
	'' Argument positions follow the C stdio interfaces, excluding any hidden
	'' ABI parameters. A different source contract must keep its diagnostics.
	var param = symbGetProcHeadParam(proc)
	for i as integer = 1 to fileparam - 1
		if( param = NULL ) then exit for
		param = symbGetNext(param)
	next
	if( param = NULL orelse symbGetParamMode(param) <> FB_PARAMMODE_BYVAL ) then
		c_stdio_file_conflict = TRUE
		exit sub
	end if
	hCStdioObserveType(symbGetFullType(param), symbGetSubtype(param))
end sub

private function hCStdioDeclaration( ) as string

	c_stdio_file_type = ""
	c_stdio_file_conflict = FALSE
	symbForEachGlobal(FB_SYMBCLASS_PROC, @hCStdioObserveProc)
	dim as string declaration
	if( len(c_stdio_file_type) > 0 andalso c_stdio_file_conflict = FALSE ) then
		'' Clang resolves stdio builtin prototypes through the FILE typedef.
		'' Define that name from the same pointee used by the BASIC declarations.
		'' With inconsistent views, leave the original declarations and warnings.
		declaration = "typedef " + c_stdio_file_type + " FILE"
		if( c_stdio_file_type <> "void" ) then
			'' Match the existing alignment-one pointee views in the procedure
			'' declarations. This typedef leaves the record's own layout intact.
			declaration += " __attribute__((aligned(1)))"
		end if
		declaration += ";" + NEWLINE
	end if
	c_stdio_file_type = ""
	c_stdio_file_conflict = FALSE
	return declaration
end function

'' end of backend/c/c-stdio-contract.bi
