'' Project: FreeBASIC compiler - accepted primitive type occurrences
'' File: tooling/semantic-primitive-types.bas
'' Purpose: Preserve native primitive type selections at their original tokens.
'' Responsibilities: Selected types, physical/macro origins and module closure.
'' This file intentionally does NOT contain: type grammar, type inference or code emission.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-coordinates.bi"
#include once "tooling/semantic-source.bi"
#include once "tooling/semantic-constructs.bi"
#include once "tooling/semantic-macros.bi"
#include once "tooling/semantic-primitive-types.bi"

'' Like the other observers, this state belongs to the compiler's one active
'' module. Its lifecycle, not an individual parser call, resets the count.
const SEMANTIC_PRIMITIVE_TYPE_LIMIT as longint = 1000000
dim shared as longint accepted_primitive_type_count

sub fbSemanticModelResetPrimitiveTypes( )
	accepted_primitive_type_count = 0
end sub

sub fbSemanticModelFinishPrimitiveTypes( )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	dim as string row = "K" + TABCHAR + "symbol" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(@symbGetGlobalNamespc( ))) + _
		TABCHAR + "accepted-primitive-type-count:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )) + _
		TABCHAR + fbSemanticModelNumber(accepted_primitive_type_count)
	fbSemanticModelAppendDetail(row)
end sub

sub fbSemanticModelPrimitiveType _
	( byval dtype as integer, byval length as longint, byval is_fixlenstr as integer, _
	  byval token as integer, byref source as LEX_LOCATION )

	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( lex.ctx->semantic_probe ) then exit sub
	'' Physical primitive selections are useful with compact macro output too.
	'' LOC status still decides whether a spelling is editable; MR attachments
	'' remain controlled by the independent macro-provenance capability.
	'' Only cSymbolType's actual primitive branch is observed. An alias with
	'' the same final dtype has a different source identity and uses B/MR.
	select case token
	case FB_TK_ANY, FB_TK_BOOLEAN, FB_TK_BYTE, FB_TK_UBYTE, FB_TK_SHORT, FB_TK_USHORT, _
		FB_TK_INTEGER, FB_TK_UINT, FB_TK_LONG, FB_TK_ULONG, FB_TK_LONGINT, FB_TK_ULONGINT, _
		FB_TK_SINGLE, FB_TK_DOUBLE, FB_TK_STRING, FB_TK_USTRING, FB_TK_ZSTRING, FB_TK_WSTRING
	case else
		exit sub
	end select
	if( (dtype = FB_DATATYPE_INVALID) or (accepted_primitive_type_count >= SEMANTIC_PRIMITIVE_TYPE_LIMIT) ) then
		fbSemanticModelFailAt("primitive type selection is invalid or exceeds its bound")
		exit sub
	end if
	dim as longint statement = fbSemanticModelCurrentStatement( )
	'' Preprocessor type queries are not typed program statements. They have
	'' no ST owner and are outside this occurrence family, rather than errors.
	if( statement = 0 ) then exit sub
	dim as longint owner = fbSemanticModelSymbolId(@symbGetGlobalNamespc( ))
	dim as longint sourceid = fbSemanticModelCurrentSource( )
	if( (owner < 1) or (sourceid < 1) ) then
		fbSemanticModelFailAt("primitive type selection ownership is unavailable")
		exit sub
	end if
	accepted_primitive_type_count += 1
	'' Payload: statement, source context, original keyword ID, final dtype,
	'' and module-local ordinal. The detail identity also stays unique across
	'' modules which share the same canonical global namespace.
	'' Pointer/const bits and target-sized INTEGER remain compiler selections.
	'' One ordinal and a separate module count retain empty and nonempty sets.
	dim as longint detail = fbSemanticModelNextDetailIdentity( )
	dim as string role = "primitive-type-input:" + fbSemanticModelNumber(detail)
	dim as string payload = fbSemanticModelNumber(statement) + TABCHAR + fbSemanticModelNumber(sourceid) + _
		TABCHAR + fbSemanticModelNumber(token) + TABCHAR + fbSemanticModelNumber(dtype) + _
		TABCHAR + fbSemanticModelNumber(accepted_primitive_type_count)
	dim as string row = "K" + TABCHAR + "symbol" + TABCHAR + fbSemanticModelNumber(owner) + _
		TABCHAR + role + TABCHAR + fbSemanticModelEscape(payload)
	fbSemanticModelAppendDetail(row)
	fbSemanticModelExportCoordinates("source-context", sourceid, role, source, source)
	fbSemanticModelMacroOrigin("symbol", owner, source.macro_identity, role)

	'' Report the accepted type component, not the enclosing variable, array or
	'' ABI argument slot. A pointer's value width is separate from its pointee;
	'' dynamic strings likewise report their descriptor, not character storage.
	dim as integer pointers = typeGetPtrCnt(dtype)
	dim as string storage_kind = "value"
	if( pointers > 0 ) then
		storage_kind = "pointer"
	elseif( typeIsDynamicString(typeGetDtOnly(dtype)) ) then
		storage_kind = "string-descriptor"
	elseif( length <= 0 ) then
		storage_kind = "unsized"
	end if
	dim as longint display_length = length
	if( pointers > 0 ) then
		'' The formatter's length describes string elements, not pointer bytes.
		display_length = symb_dtypeTB(typeGetDtOnly(dtype)).size
	end if
	dim as string type_name = symbTypeToStr(dtype, NULL, display_length, is_fixlenstr)
	dim as string layout = type_name + TABCHAR + storage_kind + TABCHAR + _
		fbSemanticModelNumber(iif(length > 0, length, 0)) + TABCHAR + fbSemanticModelNumber(pointers)
	fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + fbSemanticModelNumber(owner) + _
		TABCHAR + "primitive-type-layout:" + fbSemanticModelNumber(detail) + TABCHAR + fbSemanticModelEscape(layout))
end sub

'' end of tooling/semantic-primitive-types.bas
