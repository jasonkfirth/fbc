'' Project: FreeBASIC compiler - logical serialization
'' File: parser/intrinsics/parser-quirk-serialization.bas
'' Purpose: Generate a logical traversal program using resolved compiler types.
'' Responsibilities: Validate supported ownership, encode target layout separately
'' from the portable schema, and lower opt-in intrinsics to runtime calls.
'' This file intentionally does NOT contain: file/network operations or runtime reflection.

#include once "core/fb.bi"
#include once "core/fbint.bi"
#include once "parser/parser.bi"
#include once "runtime/rtl.bi"
#include once "ast/ast.bi"

'' Limits also bound compiler work for deeply nested or enormous definitions.
const SERIAL_MAX_DEPTH = 32
const SERIAL_MAX_NODES = 4096
const SERIAL_MAX_SCHEMA = 65536
const SERIAL_MAX_STORAGE = 64ll * 1024 * 1024

'' -------------------------------------------------------------------------
'' Compiler-owned traversal program and portable schema
'' -------------------------------------------------------------------------

private function hUnsupported( byref detail as const string ) as integer
	errReportEx( FB_ERRMSG_INVALIDDATATYPES, "serialization: " + detail )
	return FALSE
end function

declare function hSerialType( byval dtype as integer, byval subtype as FBSYMBOL ptr, _
	byval bytes as longint, byval offset as longint, byref program as string, _
	byref schema as string, byval depth as integer, byref nodes as integer, _
	byval writing as integer ) as integer

'' Each instruction is ASCII: kind,size,offset,count,dimensions; then bounds
'' for inline arrays, followed by child instructions. Only the compiler emits
'' this program. Its host-dependent offsets/sizes are never put on the wire.
private sub hInstruction( byval kind as string, byval bytes as longint, _
	byval offset as longint, byval count as longint, byval dims as integer, _
	byref program as string )
	program += kind + "," + str(bytes) + "," + str(offset) + "," + _
		str(count) + "," + str(dims) + ";"
end sub

private function hSerialArray( byval sym as FBSYMBOL ptr, byval offset as longint, _
	byval descriptor as integer, byref program as string, byref schema as string, _
	byval depth as integer, byref nodes as integer, byval writing as integer ) as integer
	if( sym = NULL ) then return hUnsupported( "missing array type information" )
	if( symbIsArray(sym) = FALSE ) then return hUnsupported( "expected an array" )
	var dims = symbGetArrayDimensions( sym )
	var rank = dims
	if( (dims = -1) andalso descriptor ) then
		rank = 0
		dims = FB_MAXARRAYDIMS
	elseif( (dims < 1) or (dims > FB_MAXARRAYDIMS) ) then
		return hUnsupported( "invalid array dimension count" )
	end if
	nodes += 1
	if( (nodes > SERIAL_MAX_NODES) or (depth >= SERIAL_MAX_DEPTH) ) then
		return hUnsupported( "type exceeds traversal limits" )
	end if
	var bytes = symbGetSizeOf( sym )
	var kind = iif( descriptor, "R", "A" )
	dim as longint count = symbGetArrayElements(sym)
	if( descriptor ) then count = rank
	hInstruction( kind, bytes, offset, count, dims, program )
	schema += kind + str(rank) + "["
	if( symbGetIsDynamic(sym) = FALSE ) then
		for i as integer = 0 to dims - 1
			schema += str(symbArrayLbound(sym,i)) + ":" + str(symbArrayUbound(sym,i)) + ";"
		next
	end if
	if( descriptor = FALSE ) then
		for i as integer = 0 to dims - 1
			program += str(symbArrayLbound(sym,i)) + "," + str(symbArrayUbound(sym,i)) + ";"
		next
	end if
	if( hSerialType( symbGetFullType(sym), symbGetSubtype(sym), bytes, 0, _
					program, schema, depth + 1, nodes, writing ) = FALSE ) then return FALSE
	schema += "]"
	if( len(schema) > SERIAL_MAX_SCHEMA ) then return hUnsupported( "schema exceeds 64 KiB" )
	return TRUE
end function

private function hSerialType( byval dtype as integer, byval subtype as FBSYMBOL ptr, _
	byval bytes as longint, byval offset as longint, byref program as string, _
	byref schema as string, byval depth as integer, byref nodes as integer, _
	byval writing as integer ) as integer
	nodes += 1
	if( (depth >= SERIAL_MAX_DEPTH) or (nodes > SERIAL_MAX_NODES) ) then
		return hUnsupported( "type exceeds traversal limits" )
	end if
	if( typeIsPtr(dtype) ) then return hUnsupported( "raw pointers require an explicit application codec" )
	if( writing and typeIsConst(dtype) ) then return hUnsupported( "destination contains a CONST field" )
	if( (bytes < 0) or (bytes > SERIAL_MAX_STORAGE) ) then return hUnsupported( "type exceeds storage limits" )

	dim as string kind
	select case typeGetDtOnly(dtype)
	case FB_DATATYPE_BOOLEAN
		kind = "B"
	case FB_DATATYPE_BYTE, FB_DATATYPE_SHORT, FB_DATATYPE_LONG, FB_DATATYPE_LONGINT
		kind = "I"
	case FB_DATATYPE_UBYTE, FB_DATATYPE_USHORT, FB_DATATYPE_ULONG, FB_DATATYPE_ULONGINT
		kind = "U"
	case FB_DATATYPE_INTEGER
		kind = "N"
	case FB_DATATYPE_UINT
		kind = "V"
	case FB_DATATYPE_ENUM
		if( subtype = NULL ) then return hUnsupported( "missing enum definition" )
		hInstruction( "N", bytes, offset, 0, 0, program )
		schema += "E{"
		var element = symbGetEnumFirstElm(subtype)
		while( element )
			schema += *symbGetName(element) + ":" + str(symbGetConstInt(element)) + ";"
			if( len(schema) > SERIAL_MAX_SCHEMA ) then return hUnsupported( "schema exceeds 64 KiB" )
			element = symbGetEnumNextElm(element)
		wend
		schema += "}"
		return TRUE
	case FB_DATATYPE_SINGLE
		kind = "F"
	case FB_DATATYPE_DOUBLE
		kind = "D"
	case FB_DATATYPE_STRING
		kind = "S"
	case FB_DATATYPE_FIXSTR
		kind = "T"
	case FB_DATATYPE_STRUCT
		if( subtype = NULL ) then return hUnsupported( "missing UDT definition" )
		if( symbGetUDTIsUnion(subtype) or symbGetUDTHasAnonUnion(subtype) or (subtype->udt.base <> NULL) ) then
			return hUnsupported( "unions and inheritance have no automatic ownership model" )
		end if
		var member = symbGetUDTSymbTbHead(subtype)
		while( member )
			if( symbIsProc(member) ) then
				if( (member->attrib and FB_SYMBATTRIB_INTERNAL) = 0 ) then
					if( member->pattrib and (FB_PROCATTRIB_CONSTRUCTOR or FB_PROCATTRIB_DESTRUCTOR1 or _
											FB_PROCATTRIB_DESTRUCTOR0 or FB_PROCATTRIB_OPERATOR) ) then
						return hUnsupported( "user constructors, destructors and operators require an application codec" )
					end if
				end if
			end if
			member = member->next
		wend
		dim as integer count = 0
		var fld = symbUdtGetFirstField(subtype)
		while( fld )
			if( (fld->attrib and FB_SYMBATTRIB_DESCRIPTOR) = 0 ) then count += 1
			fld = symbUdtGetNextField(fld)
		wend
		hInstruction( "C", bytes, offset, count, 0, program )
		schema += "C{" '' Structural compatibility includes member names and order.
		fld = symbUdtGetFirstField(subtype)
		while( fld )
			if( (fld->attrib and FB_SYMBATTRIB_DESCRIPTOR) = 0 ) then
				if( symbFieldIsBitfield(fld) or (fld->attrib and _
					(FB_SYMBATTRIB_REF or FB_SYMBATTRIB_VIS_PRIVATE or FB_SYMBATTRIB_VIS_PROTECTED)) ) then
					return hUnsupported( "bitfields, references and non-public fields are unsupported: " + *symbGetName(fld) )
				end if
				schema += *symbGetName(fld) + "="
				if( symbIsArray(fld) ) then
					dim as longint ofs = fld->ofs
					if( symbGetIsDynamic(fld) ) then
						var desc = symbGetArrayDescriptor(fld)
						if( desc = NULL ) then return hUnsupported( "missing dynamic array descriptor" )
						ofs = desc->ofs
					end if
					if( hSerialArray(fld, ofs, symbGetIsDynamic(fld), program, schema, depth + 1, nodes, writing) = FALSE ) then return FALSE
				else
					if( hSerialType(symbGetFullType(fld), symbGetSubtype(fld), symbGetSizeOf(fld), _
						fld->ofs, program, schema, depth + 1, nodes, writing) = FALSE ) then return FALSE
				end if
				schema += ";"
				if( len(schema) > SERIAL_MAX_SCHEMA ) then return hUnsupported( "schema exceeds 64 KiB" )
			end if
			fld = symbUdtGetNextField(fld)
		wend
		schema += "}"
		return TRUE
	case else
		return hUnsupported( "this data type has no portable codec" )
	end select
	hInstruction( kind, bytes, offset, 0, 0, program )
	schema += kind
	'' Native INTEGER is normalized to 64 bits. Fixed-width numeric types and
	'' fixed strings keep their declared size in the portable schema.
	if( (kind <> "N") and (kind <> "V") and (kind <> "S") and (kind <> "B") ) then schema += str(bytes)
	return TRUE
end function

'' -------------------------------------------------------------------------
'' Opt-in intrinsic parsing and runtime lowering
'' -------------------------------------------------------------------------

function cSerializationFunct( byval tk as FB_TOKEN ) as ASTNODE ptr
	dim as ASTNODE ptr payload = NULL
	lexSkipToken( LEXCHECK_POST_SUFFIX )
	hMatchLPRNT( )
	if( tk = FB_TK_DESERIALIZE ) then
		hMatchExpressionEx( payload, FB_DATATYPE_STRING )
		hMatchCOMMA( )
	end if
	var value = cVarOrDeref( FB_VAREXPROPT_NOARRAYCHECK )
	'' The no-index variable parser leaves a root array's optional () for the
	'' caller, as it does for LBOUND. Array fields already consume their ().
	if( value <> NULL ) then
		if( astIsNIDXARRAY(value) and (lexGetToken( ) = CHAR_LPRNT) and (lexGetLookAhead(1) = CHAR_RPRNT) ) then
			lexSkipToken( )
			lexSkipToken( )
		end if
	end if
	hMatchRPRNT( )
	if( value = NULL ) then
		errReport( FB_ERRMSG_EXPECTEDIDENTIFIER )
		astDelTree(payload)
		return NULL
	end if
	if( (tk = FB_TK_DESERIALIZE) and typeIsConst(astGetFullType(value)) ) then
		errReport( FB_ERRMSG_CONSTANTCANTBECHANGED )
		astDelTree(value)
		astDelTree(payload)
		return NULL
	end if
	if( (tk = FB_TK_DESERIALIZE) and astIsCALL(value) ) then
		hUnsupported( "destination must be an addressable variable" )
		astDelTree(value)
		astDelTree(payload)
		return NULL
	end if
	dim as string program, schema
	dim as integer nodes = 0, ok
	var writing = (tk = FB_TK_DESERIALIZE)
	var isarray = astIsNIDXARRAY(value)
	if( isarray ) then
		ok = hSerialArray(astGetSymbol(value), 0, TRUE, program, schema, 0, nodes, writing)
	else
		if( astIsBITFIELD(value) ) then
			ok = hUnsupported( "bitfield expressions are unsupported" )
		else
			ok = hSerialType(astGetFullType(value), astGetSubType(value), _
				astSizeOf(value), 0, program, schema, 0, nodes, writing)
		end if
	end if
	if( len(schema) > SERIAL_MAX_SCHEMA ) then ok = hUnsupported( "schema exceeds 64 KiB" )
	if( ok = FALSE ) then
		astDelTree(value)
		astDelTree(payload)
		return NULL
	end if
	dim as integer idx
	dim as string procname
	if( tk = FB_TK_SERIALIZE ) then
		idx = iif(isarray, FB_RTL_IDX_SERIALIZEARRAY, FB_RTL_IDX_SERIALIZE)
		procname = iif(isarray, "fb_SerializeArray", "fb_Serialize")
	else
		idx = iif(isarray, FB_RTL_IDX_DESERIALIZEARRAY, FB_RTL_IDX_DESERIALIZE)
		procname = iif(isarray, "fb_DeserializeArray", "fb_Deserialize")
	end if
	var call_ = astNewCALL(rtlProcLookup(strptr(procname), idx))
	if( astNewARG(call_, value) = NULL ) then return NULL
	if( astNewARG(call_, astNewCONSTstr(schema)) = NULL ) then return NULL
	if( astNewARG(call_, astNewCONSTstr(program)) = NULL ) then return NULL
	if( payload ) then
		if( astNewARG(call_, payload) = NULL ) then return NULL
	end if
	return call_
end function

'' end of parser/intrinsics/parser-quirk-serialization.bas
