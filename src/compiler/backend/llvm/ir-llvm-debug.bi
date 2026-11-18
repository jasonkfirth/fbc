'' Project: FreeBASIC compiler - LLVM backend
'' -----------------------------------------
''
'' File: backend/llvm/ir-llvm-debug.bi
''
'' Purpose:
''     Describe BASIC source files, procedures, locations and variables to LLVM.
''
'' Responsibilities:
''     - own and deduplicate metadata records for the current output module
''     - attach source locations and variable addresses to the emitted IR
''
'' This file intentionally does NOT contain:
''     - instruction lowering or target-specific debug section formats

type IRLLVMDEBUGCONTEXT
	records as IRLLVMOUTPUTBUFFER
	interned as TSTRSET
	next_id as integer
	unit_id as integer
	subprogram as integer
	proc_file as integer
	proc_line as integer
	file_id as integer
	line_number as integer
	location as integer
end type

dim shared as IRLLVMDEBUGCONTEXT llvm_dbg
dim shared as integer llvm_dbg_scopes()
dim shared as integer llvm_dbg_depth

private function hDbgRef( byval id as integer ) as string
	if( id = 0 ) then return "null"
	return "!" & id
end function

private sub hDbgWrite( byref line_text as const string )
	hAppendOutputBuffer( @llvm_dbg.records, strptr( line_text ), len( line_text ) )
	hAppendOutputBuffer( @llvm_dbg.records, strptr( NEWLINE ), len( NEWLINE ) )
end sub

private function hDbgRecord( byref definition as const string, byval unique as integer = FALSE ) as integer
	if( unique = FALSE ) then
		var item = cptr( TSTRSETITEM ptr, hashLookup( @llvm_dbg.interned.hash, strptr( definition ) ) )
		if( item ) then return item->userdata
	end if
	llvm_dbg.next_id += 1
	var id = llvm_dbg.next_id
	hDbgWrite( hDbgRef( id ) + " = " + definition )
	if( unique = FALSE ) then strsetAdd( @llvm_dbg.interned, definition, id )
	return id
end function

private function hDbgQuote( byref value as const string ) as string
	const LLVM_ESCAPE = 92  '' backslash in IR strings
	var result = """"
	for i as integer = 0 to len( value ) - 1
		var c = value[i]
		if( c < 32 orelse c >= 127 orelse c = asc( """") orelse c = LLVM_ESCAPE ) then
			result += chr( LLVM_ESCAPE ) + hex( c, 2 )
		else
			result += chr( c )
		end if
	next
	return result + """"
end function

private function hDbgFile( byval filename as const zstring ptr ) as integer
	var pathname = *filename
	var key = "file:" + pathname
	var cached = cptr( TSTRSETITEM ptr, hashLookup( @llvm_dbg.interned.hash, strptr( key ) ) )
	if( cached ) then return cached->userdata
	'' Use one absolute name. For DWARF 4, GNU as accepts a single
	'' filename in .file; separate directory/name operands require DWARF 5.
	var absolute_path = (pathname[0] = asc( "/" ))
#ifdef __FB_WIN32__
	absolute_path = absolute_path orelse (pathname[0] = 92) orelse _
	    ((len( pathname ) > 1) andalso (pathname[1] = asc( ":" )))
#endif
	if( absolute_path = FALSE ) then pathname = curdir( ) + "/" + pathname
	var file_id = hDbgRecord( "!DIFile(filename: " + hDbgQuote( pathname ) + ", directory: """")" )
	strsetAdd( @llvm_dbg.interned, key, file_id )
	return file_id
end function

private sub hDbgBegin( )
	if( env.clopt.debuginfo = FALSE ) then exit sub
	strsetInit( @llvm_dbg.interned, 64 )
	llvm_dbg.next_id = 0
	llvm_dbg.location = 0
	var source_file = hDbgFile( @env.inf.name )
	var optimized = iif( fbGetOption( FB_COMPOPT_OPTIMIZELEVEL ) > 0, "true", "false" )
	'' GDB understands C expression syntax for BASIC's shared scalar types.
	llvm_dbg.unit_id = hDbgRecord( "distinct !DICompileUnit(language: DW_LANG_C, file: " + _
	    hDbgRef( source_file ) + ", producer: " + hDbgQuote( FB_SIGN ) + _
	    ", isOptimized: " + optimized + ", runtimeVersion: 0, emissionKind: FullDebug)", TRUE )
	var version = hDbgRecord( "!{i32 2, !""Debug Info Version"", i32 3}" )
	var dwarf_version = hDbgRecord( "!{i32 2, !""Dwarf Version"", i32 4}" )
	hDbgWrite( "!llvm.dbg.cu = !{" + hDbgRef( llvm_dbg.unit_id ) + "}" )
	hDbgWrite( "!llvm.module.flags = !{" + hDbgRef( version ) + ", " + hDbgRef( dwarf_version ) + "}" )
end sub

private sub hDbgEnd( )
	if( llvm_dbg.unit_id = 0 ) then exit sub
	strsetEnd( @llvm_dbg.interned )
	if( llvm_dbg.records.buffer ) then deallocate( llvm_dbg.records.buffer )
	llvm_dbg.records.buffer = NULL
	llvm_dbg.records.bufferlen = 0
	llvm_dbg.records.buffercapacity = 0
	llvm_dbg.unit_id = 0
	llvm_dbg.location = 0
	llvm_dbg.subprogram = 0
	erase llvm_dbg_scopes
end sub

private function hDbgScope( ) as integer
	if( llvm_dbg_depth > 0 ) then return llvm_dbg_scopes(llvm_dbg_depth - 1)
	return llvm_dbg.subprogram
end function

private sub hDbgLocation( )
	var scope_id = hDbgScope( )
	if( llvm_dbg.file_id <> llvm_dbg.proc_file ) then
		scope_id = hDbgRecord( "!DILexicalBlockFile(scope: " + hDbgRef( scope_id ) + _
		    ", file: " + hDbgRef( llvm_dbg.file_id ) + ", discriminator: 0)" )
	end if
	llvm_dbg.location = hDbgRecord( "!DILocation(line: " & llvm_dbg.line_number & _
	    ", scope: " + hDbgRef( scope_id ) + ")" )
end sub

declare function hDbgStorageType( byval sym as FBSYMBOL ptr ) as integer

private function hDbgRecordType( byval subtype as FBSYMBOL ptr ) as integer
	if( subtype = NULL ) then return 0
	#ifdef __FB_64BIT__
	var key = "udt:" & culngint( subtype )
	#else
	var key = "udt:" & culngint( cuint( subtype ) )
	#endif
	var cached = cptr( TSTRSETITEM ptr, hashLookup( @llvm_dbg.interned.hash, strptr( key ) ) )
	if( cached ) then return cached->userdata

	'' Reserve the type before emitting fields. A pointer field may point
	'' back to this record, including through a different record's fields.
	llvm_dbg.next_id += 1
	var record_id = llvm_dbg.next_id
	strsetAdd( @llvm_dbg.interned, key, record_id )
	var members = "!{"
	var fld = symbUdtGetFirstField( subtype )
	while( fld )
		if( (symbGetName( fld ) <> NULL) andalso (symbIsBitfield( fld ) = FALSE) andalso _
		    (symbIsDescriptor( fld ) = FALSE) ) then
			var type_id = hDbgStorageType( fld )
			if( type_id > 0 ) then
				var member_id = hDbgRecord( "!DIDerivedType(tag: DW_TAG_member, name: " + _
				    hDbgQuote( *symbGetName( fld ) ) + ", scope: " + hDbgRef( record_id ) + _
				    ", baseType: " + hDbgRef( type_id ) + ", size: " & symbGetFieldBitLength( fld ) & _
				    ", offset: " & symbGetFieldBitOffset( fld ) & ")" )
				if( len( members ) > 2 ) then members += ", "
				members += hDbgRef( member_id )
			end if
		end if
		fld = symbUdtGetNextField( fld )
	wend

	var tag_name = iif( symbGetUDTIsUnion( subtype ), "DW_TAG_union_type", "DW_TAG_structure_type" )
	var display_name = ""
	if( symbGetName( subtype ) ) then display_name = *symbGetName( subtype )
	hDbgWrite( hDbgRef( record_id ) + " = distinct !DICompositeType(tag: " + tag_name + _
	    ", name: " + hDbgQuote( display_name ) + ", size: " & (subtype->lgt * 8) & _
	    ", elements: " + hDbgRef( hDbgRecord( members + "}" ) ) + ")" )
	return record_id
end function

private function hDbgType( byval dtype as integer, byval subtype as FBSYMBOL ptr ) as integer
	dtype = typeGetDtAndPtrOnly( dtype )
	if( typeIsPtr( dtype ) ) then
		return hDbgRecord( "!DIDerivedType(tag: DW_TAG_pointer_type, baseType: " + _
		    hDbgRef( hDbgType( typeDeref( dtype ), subtype ) ) + _
		    ", size: " & typeGetBits( dtype ) & ")" )
	end if
	var type_encoding = "DW_ATE_unsigned"
	select case typeGetClass( dtype )
	case FB_DATACLASS_INTEGER
		if( dtype = FB_DATATYPE_BOOLEAN ) then
			type_encoding = "DW_ATE_boolean"
		elseif( typeIsSigned( dtype ) ) then
			type_encoding = "DW_ATE_signed"
		end if
	case FB_DATACLASS_FPOINT
		type_encoding = "DW_ATE_float"
	case FB_DATACLASS_UDT
		if( dtype = FB_DATATYPE_STRUCT ) then return hDbgRecordType( subtype )
		return 0
	case else
		return 0
	end select
	return hDbgRecord( "!DIBasicType(name: " + hDbgQuote( *symbGetDatatypeInfo( dtype ).name ) + _
	    ", size: " & typeGetBits( dtype ) & ", encoding: " + type_encoding + ")" )
end function

private function hDbgStorageType( byval sym as FBSYMBOL ptr ) as integer
	if( symbIsDynamic( sym ) ) then return 0
	var dtype = symbGetType( sym )
	var type_id = hDbgType( dtype, symbGetSubtype( sym ) )
	if( type_id = 0 ) then return 0
	var dims = symbGetArrayDimensions( sym )
	if( dims = 0 ) then
		'' These fields occupy more than one code unit, even without BASIC
		'' array dimensions. Their debugger representation needs a string type.
		if( (dtype = FB_DATATYPE_CHAR orelse dtype = FB_DATATYPE_WCHAR) andalso _
		    symbGetSizeOf( sym ) > typeGetSize( dtype ) ) then return 0
		return type_id
	end if
	if( dims < 0 ) then return 0
	var ranges = "!{"
	for i as integer = 0 to dims - 1
		var lower = symbArrayLbound( sym, i )
		var count = symbArrayUbound( sym, i ) - lower + 1
		if( i > 0 ) then ranges += ", "
		ranges += hDbgRef( hDbgRecord( "!DISubrange(count: " & count & ", lowerBound: " & lower & ")" ) )
	next
	return hDbgRecord( "!DICompositeType(tag: DW_TAG_array_type, baseType: " + hDbgRef( type_id ) + _
	    ", size: " & (symbGetRealSize( sym ) * 8) & _
	    ", elements: " + hDbgRef( hDbgRecord( ranges + "}" ) ) + ")" )
end function

private function hDbgProcBegin( byval proc as FBSYMBOL ptr ) as string
	if( llvm_dbg.unit_id = 0 ) then return ""
	var filename = symbGetProcIncFile( proc )
	if( filename = NULL ) then filename = @env.inf.name
	llvm_dbg.proc_file = hDbgFile( filename )
	llvm_dbg.proc_line = proc->proc.ext->dbg.iniline
	if( llvm_dbg.proc_line < 1 ) then llvm_dbg.proc_line = 1
	llvm_dbg.file_id = llvm_dbg.proc_file
	llvm_dbg.line_number = 0
	llvm_dbg_depth = 0

	var types = "!{" + hDbgRef( hDbgType( symbGetType( proc ), symbGetSubtype( proc ) ) )
	var param = symbGetProcHeadParam( proc )
	while( param )
		types += ", " + hDbgRef( hDbgType( symbGetType( param ), symbGetSubtype( param ) ) )
		param = param->next
	wend
	var type_id = hDbgRecord( "!DISubroutineType(types: " + hDbgRef( hDbgRecord( types + "}" ) ) + ")" )
	var display_name = *symbGetMangledName( proc )
	if( symbGetName( proc ) ) then display_name = *symbGetName( proc )
	llvm_dbg.subprogram = hDbgRecord( "distinct !DISubprogram(name: " + hDbgQuote( display_name ) + _
	    ", linkageName: " + hDbgQuote( *symbGetMangledName( proc ) ) + _
	    ", scope: " + hDbgRef( llvm_dbg.proc_file ) + ", file: " + hDbgRef( llvm_dbg.proc_file ) + _
	    ", line: " & llvm_dbg.proc_line & ", type: " + hDbgRef( type_id ) + _
	    ", spFlags: DISPFlagDefinition, unit: " + hDbgRef( llvm_dbg.unit_id ) + ")", TRUE )
	hDbgLocation( )
	return " !dbg " + hDbgRef( llvm_dbg.subprogram )
end function

private sub hDbgSourceLine( byval line_number as integer, byval filename as const zstring ptr )
	if( llvm_dbg.subprogram = 0 ) then exit sub
	llvm_dbg.file_id = llvm_dbg.proc_file
	if( filename ) then llvm_dbg.file_id = hDbgFile( filename )
	llvm_dbg.line_number = line_number
	hDbgLocation( )
end sub

private sub hDbgScopeBegin( )
	if( llvm_dbg.subprogram = 0 ) then exit sub
	var parent_id = hDbgScope( )
	if( llvm_dbg_depth > ubound( llvm_dbg_scopes ) ) then
		redim preserve llvm_dbg_scopes(0 to llvm_dbg_depth + 15)
	end if
	llvm_dbg_scopes(llvm_dbg_depth) = hDbgRecord( "distinct !DILexicalBlock(scope: " + _
	    hDbgRef( parent_id ) + ", file: " + hDbgRef( llvm_dbg.file_id ) + _
	    ", line: " & llvm_dbg.line_number & ")", TRUE )
	llvm_dbg_depth += 1
	hDbgLocation( )
end sub

private sub hDbgScopeEnd( )
	if( llvm_dbg_depth = 0 ) then exit sub
	llvm_dbg_depth -= 1
	hDbgLocation( )
end sub

private sub hDbgDeclare( byval sym as FBSYMBOL ptr, byval ordinal as integer = 0 )
	if( llvm_dbg.subprogram = 0 ) then exit sub
	if( symbGetName( sym ) = NULL orelse symbIsTemp( sym ) orelse _
	    symbGetIsImplicit( sym ) orelse symbIsDescriptor( sym ) orelse _
	    symbIsParamVarByDesc( sym ) ) then exit sub
	var type_id = hDbgStorageType( sym )
	if( type_id = 0 ) then exit sub

	var declaration = "!DILocalVariable(name: " + hDbgQuote( *symbGetName( sym ) )
	var line_number = llvm_dbg.line_number
	if( ordinal > 0 ) then
		declaration += ", arg: " & ordinal
		line_number = llvm_dbg.proc_line
	end if
	var var_id = hDbgRecord( declaration + ", scope: " + hDbgRef( hDbgScope( ) ) + _
	    ", file: " + hDbgRef( llvm_dbg.file_id ) + ", line: " & line_number & _
	    ", type: " + hDbgRef( type_id ) + ")", TRUE )
	dim as integer dtype
	dim as FBSYMBOL ptr subtype
	symbGetRealType( sym, dtype, subtype )
	var expression = "!DIExpression()"
	if( symbIsParamVarByRef( sym ) orelse symbIsRef( sym ) ) then _
		expression = "!DIExpression(DW_OP_deref)"
	hWriteLine( "call void @llvm.dbg.declare(metadata " + hEmitType( typeAddrOf( dtype ), subtype ) + _
	    " " + hEmitVarName( sym ) + ", metadata " + hDbgRef( var_id ) + ", metadata " + expression + ")" )
end sub

private sub hDbgDeclareArg( byval proc as FBSYMBOL ptr, byval sym as FBSYMBOL ptr )
	if( llvm_dbg.subprogram = 0 ) then exit sub
	var ordinal = 1
	var param = symbGetProcHeadParam( proc )
	while( param )
		if( symbGetParamVar( param ) = sym ) then
			hDbgDeclare( sym, ordinal )
			exit sub
		end if
		param = param->next
		ordinal += 1
	wend
end sub

private function hDbgInstructionAttachment( ) as string
	if( llvm_dbg.location = 0 ) then return ""
	return ", !dbg " + hDbgRef( llvm_dbg.location )
end function

'' end of backend/llvm/ir-llvm-debug.bi
