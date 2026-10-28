'' Project: FreeBASIC compiler - default type policy observations
'' File: tooling/semantic-default-types.bas
'' Purpose: Preserve native DEF initialization and actual range assignments.
'' Responsibilities: Initial table, native clamping, committed changes and provenance.
'' This file intentionally does NOT contain: DEF parsing or symbol lookup.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-default-types.bi"
#include once "tooling/semantic-constructs.bi"
#include once "tooling/semantic-source.bi"
#include once "parser/parser.bi"

'' Native default typing covers ASCII A through underscore, including the
'' punctuation after Z. This compatibility table is wider than the alphabet.
'' The getter only reads that table or the dialect fallback; it performs no
'' symbol lookup, parser callbacks or allocation. Module processing is serial.
private const DEFAULT_TYPE_LIMIT = 1000000
private dim shared as longint default_type_count
private dim shared as integer default_type_started

private function hKnownType( byval dtype as integer ) as integer
	select case dtype
	case FB_DATATYPE_BYTE, FB_DATATYPE_UBYTE, FB_DATATYPE_SHORT, FB_DATATYPE_USHORT, _
		FB_DATATYPE_INTEGER, FB_DATATYPE_UINT, FB_DATATYPE_LONG, FB_DATATYPE_ULONG, _
		FB_DATATYPE_LONGINT, FB_DATATYPE_ULONGINT, FB_DATATYPE_SINGLE, FB_DATATYPE_DOUBLE, FB_DATATYPE_STRING
		return TRUE
	end select
	return FALSE
end function

sub fbSemanticModelResetDefaultTypes( )
	default_type_count = 0
	default_type_started = FALSE
end sub

private sub hProperty( byref key as const string, byref value as const string )
	fbSemanticModelAppendProvenance("K" + TABCHAR + "module" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelModuleIdentity( )) + TABCHAR + key + TABCHAR + fbSemanticModelEscape(value))
end sub

sub fbSemanticModelBeginDefaultTypes( )
	if( fbSemanticModelTypedExpressionsEnabled( ) = FALSE ) then exit sub
	if( default_type_started ) then fbSemanticModelFailAt("default type policy initialized twice"): exit sub
	dim as string raw_types, wire_types
	dim as zstring * 2 default_name
	default_name[1] = 0
	for code as integer = asc("A") to asc("_") + 1
		'' The final slot records the native fallback outside the table.
		default_name[0] = iif(code > asc("_"), asc("@"), code)
		dim as integer dtype = symbGetDefType(@default_name)
		if( hKnownType(dtype) = FALSE ) then fbSemanticModelFailAt("invalid initial default type policy"): exit sub
		raw_types += TABCHAR + fbSemanticModelNumber(dtype)
		wire_types += TABCHAR + fbSemanticModelTypeNumber(dtype)
	next
	default_type_started = TRUE
	hProperty("native-default-type-initial", fbSemanticModelNumber(asc("A")) + TABCHAR + _
		fbSemanticModelNumber(asc("_")) + raw_types + wire_types)
end sub

sub fbSemanticModelDefaultTypeRange _
	( byval raw_first as integer, byval raw_last as integer, byval first as integer, byval last as integer, _
	  byval dtype as integer, byval changed as integer, byval entry_context as longint )
	if( changed ) then fbSemanticModelDefaultTypesChanged( )
	if( fbSemanticModelTypedExpressionsEnabled( ) = FALSE or lex.ctx = NULL ) then exit sub
	if( lex.ctx->semantic_probe or fbGetIsPP( ) ) then exit sub
	if( default_type_started = FALSE or default_type_count >= DEFAULT_TYPE_LIMIT or _
		first < asc("A") or last > asc("_") or first > last or hKnownType(dtype) = FALSE or _
		entry_context < 0 or (entry_context = 0 and fbSemanticModelFullEnabled( )) ) then
		fbSemanticModelFailAt("invalid native default type range")
		exit sub
	end if
	dim as LEX_LOCATION source = lexGetLastLocation( )
	dim as longint macro_identity, macro_start
	dim as integer macro_valid
	if( fbSemanticModelMacroProvenanceEnabled( ) and source.macro_identity > 0 ) then
		macro_identity = source.macro_identity
		macro_start = source.macro_start_unit
		macro_valid = abs(source.macro_unit_valid <> FALSE)
	end if
	default_type_count += 1
	dim as string value = fbSemanticModelNumber(raw_first) + TABCHAR + fbSemanticModelNumber(raw_last) + TABCHAR + _
		fbSemanticModelNumber(first) + TABCHAR + fbSemanticModelNumber(last) + TABCHAR + _
		fbSemanticModelNumber(dtype) + TABCHAR + fbSemanticModelTypeNumber(dtype) + TABCHAR + fbSemanticModelNumber(abs(changed <> FALSE)) + TABCHAR + _
		fbSemanticModelNumber(entry_context) + TABCHAR + fbSemanticModelNumber(fbSemanticModelCurrentContext( )) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelCurrentStatement( )) + TABCHAR + fbSemanticModelNumber(fbSemanticModelCurrentSource( )) + TABCHAR + _
		fbSemanticModelEscape(source.source_file) + TABCHAR + fbSemanticModelNumber(source.start_line) + TABCHAR + _
		fbSemanticModelNumber(source.start_column) + TABCHAR + fbSemanticModelNumber(source.end_line) + TABCHAR + _
		fbSemanticModelNumber(source.end_column) + TABCHAR + fbSemanticModelNumber(fbSemanticModelLocationIsPhysical(source)) + TABCHAR + _
		fbSemanticModelNumber(macro_identity) + TABCHAR + fbSemanticModelNumber(macro_start) + TABCHAR + fbSemanticModelNumber(macro_valid)
	hProperty("native-default-type:" + fbSemanticModelNumber(default_type_count), value)
end sub

sub fbSemanticModelFinishDefaultTypes( byval commit as integer )
	if( commit and fbSemanticModelTypedExpressionsEnabled( ) ) then
		if( default_type_started = FALSE ) then fbSemanticModelFailAt("default type policy has no initialization"): exit sub
		hProperty("native-default-type-count", fbSemanticModelNumber(default_type_count))
	end if
	fbSemanticModelResetDefaultTypes( )
end sub

'' end of tooling/semantic-default-types.bas
