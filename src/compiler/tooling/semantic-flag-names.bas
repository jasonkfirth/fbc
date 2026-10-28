'' Project: FreeBASIC compiler - stable semantic flag vocabulary
'' File: tooling/semantic-flag-names.bas
'' Purpose: Decode initialized compiler flags with their class-specific meanings.
'' Responsibilities: Stable names, native definitions and explicit unmapped bits.
'' This file intentionally does NOT contain: sidecar I/O or reads of symbol unions.

#include once "core/fb.bi"
#include once "core/fbint.bi"
#include once "tooling/semantic-flag-names.bi"

private type SEMANTIC_FLAG_DEFINITION
	group_id as integer
	bits as ulong
	flag_name as const zstring ptr
	classes as ulong
end type

'' A zero class mask applies to every live symbol class. Reused status bits
'' have separate entries with disjoint class masks; they are never synonyms.
private dim shared as const SEMANTIC_FLAG_DEFINITION flag_definitions(0 to ...) = { _
	(0, FB_SYMBATTRIB_SHARED, @"shared", 0), _
	(0, FB_SYMBATTRIB_STATIC, @"static", 0), _
	(0, FB_SYMBATTRIB_DYNAMIC, @"dynamic", 0), _
	(0, FB_SYMBATTRIB_COMMON, @"common", 0), _
	(0, FB_SYMBATTRIB_EXTERN, @"external", 0), _
	(0, FB_SYMBATTRIB_PUBLIC, @"public", 0), _
	(0, FB_SYMBATTRIB_PRIVATE, @"private", 0), _
	(0, FB_SYMBATTRIB_LOCAL, @"local", 0), _
	(0, FB_SYMBATTRIB_EXPORT, @"exported", 0), _
	(0, FB_SYMBATTRIB_IMPORT, @"imported", 0), _
	(0, FB_SYMBATTRIB_LITERAL, @"literal", 0), _
	(0, FB_SYMBATTRIB_CONST, @"const", 0), _
	(0, FB_SYMBATTRIB_TEMP, @"temporary", (1UL shl FB_SYMBCLASS_VAR)), _
	(0, FB_SYMBATTRIB_DESCRIPTOR, @"array-descriptor", (1UL shl FB_SYMBCLASS_VAR) or (1UL shl FB_SYMBCLASS_STRUCT) or (1UL shl FB_SYMBCLASS_FIELD)), _
	(0, FB_SYMBATTRIB_PARAMVARBYDESC, @"parameter-variable-bydesc", (1UL shl FB_SYMBCLASS_VAR)), _
	(0, FB_SYMBATTRIB_PARAMVARBYVAL, @"parameter-variable-byval", (1UL shl FB_SYMBCLASS_VAR)), _
	(0, FB_SYMBATTRIB_PARAMVARBYREF, @"parameter-variable-byref", (1UL shl FB_SYMBCLASS_VAR)), _
	(0, FB_SYMBATTRIB_FUNCRESULT, @"function-result", (1UL shl FB_SYMBCLASS_VAR)), _
	(0, FB_SYMBATTRIB_REF, @"reference-storage", (1UL shl FB_SYMBCLASS_VAR) or (1UL shl FB_SYMBCLASS_FIELD)), _
	(0, FB_SYMBATTRIB_INSTANCEPARAM, @"instance-parameter", (1UL shl FB_SYMBCLASS_VAR) or (1UL shl FB_SYMBCLASS_PARAM)), _
	(0, FB_SYMBATTRIB_SUFFIXED, @"type-suffixed", 0), _
	(0, FB_SYMBATTRIB_VIS_PRIVATE, @"member-private", 0), _
	(0, FB_SYMBATTRIB_VIS_PROTECTED, @"member-protected", 0), _
	(0, FB_SYMBATTRIB_INTERNAL, @"internal", 0), _
	(0, FB_SYMBATTRIB_ANONYMOUS, @"anonymous", 0), _
	(0, FB_SYMBATTRIB_THREADLOCAL, @"thread-local", (1UL shl FB_SYMBCLASS_VAR)), _
	(0, FB_SYMBATTRIB_THREADLOCALC, @"c-thread-local", (1UL shl FB_SYMBCLASS_VAR)), _
	(1, FB_PROCATTRIB_OVERLOADED, @"overloaded", (1UL shl FB_SYMBCLASS_PROC)), _
	(1, FB_PROCATTRIB_METHOD, @"instance-method", (1UL shl FB_SYMBCLASS_PROC)), _
	(1, FB_PROCATTRIB_CONSTRUCTOR, @"constructor", (1UL shl FB_SYMBCLASS_PROC)), _
	(1, FB_PROCATTRIB_DESTRUCTOR1, @"complete-destructor", (1UL shl FB_SYMBCLASS_PROC)), _
	(1, FB_PROCATTRIB_OPERATOR, @"operator", (1UL shl FB_SYMBCLASS_PROC)), _
	(1, FB_PROCATTRIB_PROPERTY, @"property", (1UL shl FB_SYMBCLASS_PROC)), _
	(1, FB_PROCATTRIB_STATICLOCALS, @"static-locals", (1UL shl FB_SYMBCLASS_PROC)), _
	(1, FB_PROCATTRIB_NAKED, @"naked", (1UL shl FB_SYMBCLASS_PROC)), _
	(1, FB_PROCATTRIB_VIRTUAL, @"virtual", (1UL shl FB_SYMBCLASS_PROC)), _
	(1, FB_PROCATTRIB_ABSTRACT, @"abstract", (1UL shl FB_SYMBCLASS_PROC)), _
	(1, FB_PROCATTRIB_NOTHISCONSTNESS, @"ignore-this-constness", (1UL shl FB_SYMBCLASS_PROC)), _
	(1, FB_PROCATTRIB_RETURNBYREF, @"return-by-reference", (1UL shl FB_SYMBCLASS_PROC)), _
	(1, FB_PROCATTRIB_DESTRUCTOR0, @"deleting-destructor", (1UL shl FB_SYMBCLASS_PROC)), _
	(1, FB_PROCATTRIB_SFXLIB, @"sound-intrinsic", (1UL shl FB_SYMBCLASS_PROC)), _
	(1, FB_PROCATTRIB_GFXLIB, @"graphics-intrinsic", (1UL shl FB_SYMBCLASS_PROC)), _
	(2, FB_SYMBSTATS_ACCESSED, @"accessed", 0), _
	(2, FB_SYMBSTATS_DECLARED, @"declared", 0), _
	(2, FB_SYMBSTATS_RTL, @"runtime", 0), _
	(2, FB_SYMBSTATS_THROWABLE, @"throwable", 0), _
	(2, FB_SYMBSTATS_PARSED, @"parsed", 0), _
	(2, FB_SYMBSTATS_HASALIAS, @"has-alias", 0), _
	(2, FB_SYMBSTATS_CANTDUP, @"cannot-duplicate", 0), _
	(2, FB_SYMBSTATS_CANTUNDEF, @"cannot-undefine", 0), _
	(2, FB_SYMBSTATS_EMITTED, @"emitted", 0), _
	(2, FB_SYMBSTATS_BEINGEMITTED, @"being-emitted", 0), _
	(2, FB_SYMBSTATS_CTORINITED, @"constructor-initialized", (1UL shl FB_SYMBCLASS_PROC)), _
	(2, FB_SYMBSTATS_MAINPROC, @"main-procedure", (1UL shl FB_SYMBCLASS_PROC)), _
	(2, FB_SYMBSTATS_MODLEVELPROC, @"module-procedure", (1UL shl FB_SYMBCLASS_PROC)), _
	(2, FB_SYMBSTATS_FUNCPTR, @"procedure-pointer-type", (1UL shl FB_SYMBCLASS_PROC)), _
	(2, FB_SYMBSTATS_GLOBALCTOR, @"global-constructor", (1UL shl FB_SYMBCLASS_PROC)), _
	(2, FB_SYMBSTATS_GLOBALDTOR, @"global-destructor", (1UL shl FB_SYMBCLASS_PROC)), _
	(2, FB_SYMBSTATS_CANBECLONED, @"call-cloneable", (1UL shl FB_SYMBCLASS_PROC)), _
	(2, FB_SYMBSTATS_UTF8ARGS, @"utf8-arguments", (1UL shl FB_SYMBCLASS_PROC)), _
	(2, FB_SYMBSTATS_NORETURN, @"no-return", (1UL shl FB_SYMBCLASS_PROC)), _
	(2, FB_SYMBSTATS_PROCEMITTED, @"procedure-emitted", (1UL shl FB_SYMBCLASS_PROC)), _
	(2, FB_SYMBSTATS_EXCLPARENT, @"exclude-mangling-parent", (1UL shl FB_SYMBCLASS_PROC)), _
	(2, FB_SYMBSTATS_IMPLICIT, @"implicit-variable", (1UL shl FB_SYMBCLASS_VAR)), _
	(2, FB_SYMBSTATS_RTTITABLE, @"rtti-storage", (1UL shl FB_SYMBCLASS_VAR)), _
	(2, FB_SYMBSTATS_VTABLE, @"vtable-storage", (1UL shl FB_SYMBCLASS_VAR)), _
	(2, FB_SYMBSTATS_JUMPTB, @"jump-table-storage", (1UL shl FB_SYMBCLASS_VAR)), _
	(2, FB_SYMBSTATS_TEMPORARY, @"temporary-lifetime", (1UL shl FB_SYMBCLASS_VAR)), _
	(2, FB_SYMBSTATS_DONTINIT, @"do-not-initialize", (1UL shl FB_SYMBCLASS_VAR) or (1UL shl FB_SYMBCLASS_FIELD) or (1UL shl FB_SYMBCLASS_PARAM)), _
	(2, FB_SYMBSTATS_ARGV, @"argv-parameter", (1UL shl FB_SYMBCLASS_VAR) or (1UL shl FB_SYMBCLASS_PARAM)), _
	(2, FB_SYMBSTATS_CANREDEFINE, @"redefinable-constant", (1UL shl FB_SYMBCLASS_CONST)), _
	(2, FB_SYMBSTATS_FIXLENSTR, @"fixed-length-string-alias", (1UL shl FB_SYMBCLASS_TYPEDEF)), _
	(2, FB_SYMBSTATS_HASRTTI, @"has-rtti", (1UL shl FB_SYMBCLASS_STRUCT)), _
	(2, FB_SYMBSTATS_UNUSEDVTABLE, @"unused-vtable", (1UL shl FB_SYMBCLASS_VAR)), _
	(2, FB_SYMBSTATS_UNIONFIELD, @"union-field", (1UL shl FB_SYMBCLASS_FIELD)), _
	(2, FB_SYMBSTATS_DECLAREDASANY, @"declared-as-any", (1UL shl FB_SYMBCLASS_PARAM)), _
	(3, FB_UDTOPT_ISUNION, @"union", (1UL shl FB_SYMBCLASS_STRUCT)), _
	(3, FB_UDTOPT_ISANON, @"anonymous", (1UL shl FB_SYMBCLASS_STRUCT)), _
	(3, FB_UDTOPT_HASPTRFIELD, @"has-pointer-field", (1UL shl FB_SYMBCLASS_STRUCT)), _
	(3, FB_UDTOPT_HASCTORFIELD, @"has-constructor-field", (1UL shl FB_SYMBCLASS_STRUCT)), _
	(3, FB_UDTOPT_HASDTORFIELD, @"has-destructor-field", (1UL shl FB_SYMBCLASS_STRUCT)), _
	(3, FB_UDTOPT_HASRECBYVALPARAM, @"recursive-byval-parameter", (1UL shl FB_SYMBCLASS_STRUCT)), _
	(3, FB_UDTOPT_HASRECBYVALRES, @"recursive-byval-result", (1UL shl FB_SYMBCLASS_STRUCT)), _
	(3, FB_UDTOPT_HASGETPROPERTY, @"get-property", (1UL shl FB_SYMBCLASS_STRUCT)), _
	(3, FB_UDTOPT_HASSETPROPERTY, @"set-property", (1UL shl FB_SYMBCLASS_STRUCT)), _
	(3, FB_UDTOPT_HASIDXGETPROPERTY, @"index-get-property", (1UL shl FB_SYMBCLASS_STRUCT)), _
	(3, FB_UDTOPT_HASIDXSETPROPERTY, @"index-set-property", (1UL shl FB_SYMBCLASS_STRUCT)), _
	(3, FB_UDTOPT_HASKWDFIELD, @"has-keyword-field", (1UL shl FB_SYMBCLASS_STRUCT)), _
	(3, FB_UDTOPT_HASINITEDFIELD, @"has-initialized-field", (1UL shl FB_SYMBCLASS_STRUCT)), _
	(3, FB_UDTOPT_HASANONUNION, @"has-anonymous-union", (1UL shl FB_SYMBCLASS_STRUCT)), _
	(3, FB_UDTOPT_HASSTATICVAR, @"has-static-variable", (1UL shl FB_SYMBCLASS_STRUCT)), _
	(3, FB_UDTOPT_HASBITFIELD, @"has-bitfield", (1UL shl FB_SYMBCLASS_STRUCT)), _
	(3, FB_UDTOPT_ISWSTRING, @"wstring-wrapper", (1UL shl FB_SYMBCLASS_STRUCT)), _
	(3, FB_UDTOPT_ISZSTRING, @"zstring-wrapper", (1UL shl FB_SYMBCLASS_STRUCT)), _
	(3, FB_UDTOPT_HASNESTED, @"nested-type", (1UL shl FB_SYMBCLASS_STRUCT)), _
	(3, FB_UDTOPT_HASZEROEDFIELD, @"has-zeroed-field", (1UL shl FB_SYMBCLASS_STRUCT)), _
	(3, FB_UDTOPT_HASFILLEDFIELD, @"has-filled-field", (1UL shl FB_SYMBCLASS_STRUCT)), _
	(4, FB_PROCSTATS_RETURNUSED, @"return-used", (1UL shl FB_SYMBCLASS_PROC)), _
	(4, FB_PROCSTATS_ASSIGNUSED, @"result-assignment-used", (1UL shl FB_SYMBCLASS_PROC)), _
	(4, FB_PROCSTATS_GOSUBUSED, @"gosub-used", (1UL shl FB_SYMBCLASS_PROC)) _
}

private function hGroupName( byval group_id as integer ) as string
	select case group_id
	case 0: return "attributes"
	case 1: return "procedure-attributes"
	case 2: return "status"
	case 3: return "aggregate-options"
	case 4: return "procedure-status"
	end select
	return ""
end function

private function hClassName( byval symbol_class as integer ) as string
	select case symbol_class
	case FB_SYMBCLASS_VAR: return "variable"
	case FB_SYMBCLASS_CONST: return "constant"
	case FB_SYMBCLASS_PROC: return "procedure"
	case FB_SYMBCLASS_PARAM: return "parameter"
	case FB_SYMBCLASS_DEFINE: return "define"
	case FB_SYMBCLASS_KEYWORD: return "keyword"
	case FB_SYMBCLASS_LABEL: return "label"
	case FB_SYMBCLASS_NAMESPACE: return "namespace"
	case FB_SYMBCLASS_ENUM: return "enum"
	case FB_SYMBCLASS_STRUCT: return "type"
	case FB_SYMBCLASS_CLASS: return "class"
	case FB_SYMBCLASS_FIELD: return "field"
	case FB_SYMBCLASS_TYPEDEF: return "typedef"
	case FB_SYMBCLASS_FWDREF: return "forward-type"
	case FB_SYMBCLASS_SCOPE: return "scope"
	case FB_SYMBCLASS_RESERVED: return "reserved"
	case FB_SYMBCLASS_NSIMPORT: return "namespace-import"
	end select
	return ""
end function

function fbSemanticFlagDefinitionCount( ) as integer
	return ubound(flag_definitions) + 1
end function

function fbSemanticFlagDefinition( byval ordinal as integer, byref group_name as string, _
	byref flag_name as string, byref bits as ulong, byref classes as string ) as integer
	group_name = ""
	flag_name = ""
	bits = 0
	classes = ""
	if( (ordinal < 0) or (ordinal > ubound(flag_definitions)) ) then return FALSE
	with flag_definitions(ordinal)
		group_name = hGroupName(.group_id)
		flag_name = *.flag_name
		bits = .bits
		if( .classes = 0 ) then
			classes = "*"
		else
			for symbol_class as integer = FB_SYMBCLASS_VAR to FB_SYMBCLASS_NSIMPORT
				if( (.classes and (1UL shl symbol_class)) <> 0 ) then
					if( len(classes) > 0 ) then classes += ","
					classes += hClassName(symbol_class)
				end if
			next
		end if
	end with
	return TRUE
end function

function fbSemanticNamedFlags( byval group_id as integer, byval symbol_class as integer, _
	byval bits as ulong, byref unmapped as ulong ) as string
	unmapped = bits
	if( (symbol_class < FB_SYMBCLASS_VAR) or (symbol_class > FB_SYMBCLASS_NSIMPORT) ) then return ""
	dim as string result
	dim as ulong known_bits = 0
	for ordinal as integer = 0 to ubound(flag_definitions)
		with flag_definitions(ordinal)
			if( (.group_id = group_id) and _
				((.classes = 0) or ((.classes and (1UL shl symbol_class)) <> 0)) ) then
				known_bits or= .bits
				if( (bits and .bits) <> 0 ) then
					if( len(result) > 0 ) then result += ","
					result += *.flag_name
				end if
			end if
		end with
	next
	unmapped = bits and not known_bits
	return result
end function

'' end of tooling/semantic-flag-names.bas

