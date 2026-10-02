'' Project: FreeBASIC compiler - semantic sidecar
'' File: tooling/semantic-symbols.bas
'' Purpose: Export resolved signatures, layouts, values, and symbol relationships.
'' Responsibilities: Walk live symbol tables and describe compiler-owned metadata.
'' This file intentionally does NOT contain: name resolution or source parsing.

#include once "tooling/semantic-private.bi"
#include once "support/strings/hlp-str.bi"
#include once "support/hlp.bi"
#include once "crt/mem.bi"

'' One million decoded units cap literal/token formatting at eight MiB even
'' on a wide host; the shared transaction buffer imposes the overall limit.
private const SEMANTIC_MAX_LITERAL_UNITS = 1048576

'' -------------------------------------------------------------------------
'' Stable symbol vocabulary
'' -------------------------------------------------------------------------

private function hSymbolClass(byval sym as FBSYMBOL ptr) as string
	select case sym->class
	case FB_SYMBCLASS_VAR: return "variable"
	case FB_SYMBCLASS_CONST: return "constant"
	case FB_SYMBCLASS_PROC: return "procedure"
	case FB_SYMBCLASS_PARAM: return "parameter"
	case FB_SYMBCLASS_DEFINE: return "define"
	case FB_SYMBCLASS_KEYWORD: return "keyword"
	case FB_SYMBCLASS_LABEL: return "label"
	case FB_SYMBCLASS_NAMESPACE: return "namespace"
	case FB_SYMBCLASS_ENUM: return "enum"
	case FB_SYMBCLASS_STRUCT: return iif(symbGetUDTIsUnion(sym), "union", "type")
	case FB_SYMBCLASS_CLASS: return "class"
	case FB_SYMBCLASS_FIELD: return "field"
	case FB_SYMBCLASS_TYPEDEF: return "typedef"
	case FB_SYMBCLASS_FWDREF: return "forward-type"
	case FB_SYMBCLASS_SCOPE: return "scope"
	case FB_SYMBCLASS_RESERVED: return "reserved"
	case FB_SYMBCLASS_NSIMPORT: return "namespace-import"
	case else: return "unknown"
	end select
end function

private function hVisibility(byval sym as FBSYMBOL ptr) as string
	if( sym->attrib and FB_SYMBATTRIB_VIS_PRIVATE ) then return "private"
	if( sym->attrib and FB_SYMBATTRIB_VIS_PROTECTED ) then return "protected"
	if( sym->attrib and FB_SYMBATTRIB_PRIVATE ) then return "private"
	return "public"
end function

private function hStorage(byval sym as FBSYMBOL ptr) as string
	if( sym->class = FB_SYMBCLASS_PARAM ) then return "parameter"
	if( symbIsField(sym) ) then return "field"
	if( symbIsConst(sym) ) then return "constant"
	if( sym->attrib and FB_SYMBATTRIB_EXTERN ) then return "external"
	if( sym->attrib and FB_SYMBATTRIB_STATIC ) then return "static"
	if( sym->attrib and FB_SYMBATTRIB_LOCAL ) then return "local"
	if( sym->attrib and FB_SYMBATTRIB_SHARED ) then return "shared"
	if( symbIsVar(sym) or symbIsProc(sym) ) then return "global"
	return "none"
end function

private sub hSymbolNumber(byval symbolid as longint, byref key as const string, byval value as longint)
	fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + _
		fbSemanticModelNumber(symbolid) + TABCHAR + key + TABCHAR + fbSemanticModelNumber(value))
end sub

private function hTypeName(byval sym as FBSYMBOL ptr) as string
	'' Type declarations carry their own identity, whereas values carry it
	'' in subtype. The ordinary formatter requires a subtype for UDTs.
	select case sym->class
	case FB_SYMBCLASS_STRUCT, FB_SYMBCLASS_ENUM, FB_SYMBCLASS_FWDREF
		if( sym->id.name <> NULL ) then return *sym->id.name
		return ""
	case FB_SYMBCLASS_NAMESPACE, FB_SYMBCLASS_SCOPE, FB_SYMBCLASS_NSIMPORT, _
		FB_SYMBCLASS_DEFINE, FB_SYMBCLASS_KEYWORD, FB_SYMBCLASS_RESERVED, FB_SYMBCLASS_LABEL
		return ""
	end select
	if( sym->typ and FB_DATATYPE_INVALID ) then return ""
	if( (typeGetDtOnly(sym->typ) = FB_DATATYPE_STRUCT) and (sym->subtype = NULL) ) then return ""
	if( (typeGetDtOnly(sym->typ) = FB_DATATYPE_FUNCTION) and (sym->subtype = NULL) ) then return ""
	return symbTypeToStr(sym->typ, sym->subtype)
end function

private function hParamMode(byval mode as integer) as string
	select case mode
	case FB_PARAMMODE_BYVAL: return "byval"
	case FB_PARAMMODE_BYREF: return "byref"
	case FB_PARAMMODE_BYDESC: return "bydesc"
	case FB_PARAMMODE_VARARG: return "vararg"
	case else: return "unknown"
	end select
end function

private function hCallingConvention(byval mode as integer) as string
	select case mode
	case FB_FUNCMODE_STDCALL: return "stdcall"
	case FB_FUNCMODE_STDCALL_MS: return "stdcall-ms"
	case FB_FUNCMODE_CDECL: return "cdecl"
	case FB_FUNCMODE_PASCAL: return "pascal"
	case FB_FUNCMODE_THISCALL: return "thiscall"
	case FB_FUNCMODE_FASTCALL: return "fastcall"
	case else: return "unknown"
	end select
end function

sub fbSemanticModelExportContext(byref filename as string)
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	dim as string target = fbGetTargetId( )
	dim as string architecture = *fbGetFbcArch( )
	dim as string language = fbGetLangName(env.clopt.lang)
	dim as string backend = fbGetBackendName(env.clopt.backend)
	'' The established fbIsHostBigEndian() accessor reads the selected
	'' CPU policy. Like the symbol datatype table, it describes the target.
	dim as string byte_order = iif(fbIsHostBigEndian( ), "big", "little")
	fbSemanticModelAppendDetail("Q" + TABCHAR + fbSemanticModelEscape(filename) + _
		TABCHAR + fbSemanticModelEscape(target) + TABCHAR + architecture + _
		TABCHAR + language + TABCHAR + backend + TABCHAR + _
		fbSemanticModelNumber(env.pointersize) + TABCHAR + byte_order + TABCHAR + _
		fbSemanticModelNumber(fbGetTargetWcharSize( )) + TABCHAR + _
		fbSemanticModelNumber(env.lang.integerkeyworddtype))
	for dtype as integer = 0 to FB_DATATYPES - 1
		dim as string type_name = *symb_dtypeTB(dtype).name
		if( dtype = FB_DATATYPE_FIXSTR ) then type_name = "fixed-string"
		dim as string type_class, size_text, alignment_text
		select case symb_dtypeTB(dtype).class
		case FB_DATACLASS_INTEGER: type_class = "integer"
		case FB_DATACLASS_FPOINT: type_class = "float"
		case FB_DATACLASS_STRING: type_class = "string"
		case FB_DATACLASS_UDT: type_class = "aggregate"
		case FB_DATACLASS_PROC: type_class = "procedure"
		case else: type_class = "unknown"
		end select
		if( symb_dtypeTB(dtype).size >= 0 ) then size_text = fbSemanticModelNumber(symb_dtypeTB(dtype).size)
		if( (symb_dtypeTB(dtype).size > 0) and _
			(dtype <> FB_DATATYPE_XMMWORD) and _
			(symb_dtypeTB(dtype).class <> FB_DATACLASS_UDT) and _
			(symb_dtypeTB(dtype).class <> FB_DATACLASS_PROC) and _
			(symb_dtypeTB(dtype).class <> FB_DATACLASS_UNKNOWN) ) then
			'' XMMWORD is an internal register type. The scalar layout helper
			'' accepts source types aligned to at most eight bytes, not XMMWORD.
			alignment_text = fbSemanticModelNumber(typeCalcNaturalAlign(dtype, NULL))
		end if
		fbSemanticModelAppendDetail("Y" + TABCHAR + fbSemanticModelNumber(dtype) + _
			TABCHAR + fbSemanticModelEscape(type_name) + TABCHAR + type_class + _
			TABCHAR + size_text + TABCHAR + alignment_text + TABCHAR + _
			fbSemanticModelNumber(abs(symb_dtypeTB(dtype).signed <> FALSE)))
	next
end sub

'' -------------------------------------------------------------------------
'' Exact constant values and identity relationships
'' -------------------------------------------------------------------------

sub fbSemanticModelExportRelation _
	( byref domain as const string, byval identity as longint, _
	  byval target as FBSYMBOL ptr, byref kind as const string, _
	  byval ordinal as longint )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	dim as longint targetid = fbSemanticModelSymbolId(target)
	if( (identity = 0) or (targetid = 0) ) then exit sub
	fbSemanticModelAppendDetail("H" + TABCHAR + domain + TABCHAR + _
		fbSemanticModelNumber(identity) + TABCHAR + "symbol" + TABCHAR + _
		fbSemanticModelNumber(targetid) + TABCHAR + kind + TABCHAR + _
		fbSemanticModelNumber(ordinal))
end sub

function fbSemanticModelFormatValue _
	( byval dtype as integer, byval value as FBVALUE ptr, _
	  byref value_kind as string, byref value_text as string ) as integer
	value_kind = ""
	value_text = ""
	if( (fbSemanticModelFullEnabled( ) = FALSE) or (value = NULL) ) then return FALSE
	select case typeGetDtAndPtrOnly(dtype)
	case FB_DATATYPE_STRING, FB_DATATYPE_USTRING, FB_DATATYPE_FIXSTR, FB_DATATYPE_CHAR, FB_DATATYPE_WCHAR
		dim as FBSYMBOL ptr literal = value->s
		if( (literal = NULL) or (literal = cast(FBSYMBOL ptr, INVALID)) ) then return FALSE
		if( symbIsVar(literal) = FALSE ) then return FALSE
		if( (literal->attrib and FB_SYMBATTRIB_LITERAL) = 0 ) then return FALSE
		dim as integer text_length
		if( symbGetType(literal) = FB_DATATYPE_WCHAR ) then
			if( literal->var_.littextw = NULL ) then return FALSE
			dim as wstring ptr text = hUnescapeW(literal->var_.littextw, text_length)
			if( (text_length < 0) or (text_length > SEMANTIC_MAX_LITERAL_UNITS) ) then
				fbSemanticModelFail( )
				return FALSE
			end if
			value_kind = "wide-units"
			value_text = space(text_length * 8)
			if( len(value_text) <> text_length * 8 ) then
				fbSemanticModelFail( )
				return FALSE
			end if
			for index as integer = 0 to text_length - 1
				mid(value_text, index * 8 + 1, 8) = hex(culng((*text)[index]), 8)
			next
		else
			if( literal->var_.littext = NULL ) then return FALSE
			dim as zstring ptr text = hUnescape(literal->var_.littext, text_length)
			if( (text_length < 0) or (text_length > SEMANTIC_MAX_LITERAL_UNITS) ) then
				fbSemanticModelFail( )
				return FALSE
			end if
			value_kind = "bytes"
			value_text = space(text_length * 2)
			if( len(value_text) <> text_length * 2 ) then
				fbSemanticModelFail( )
				return FALSE
			end if
			for index as integer = 0 to text_length - 1
				mid(value_text, index * 2 + 1, 2) = hex(cubyte((*text)[index]), 2)
			next
		end if
	case FB_DATATYPE_SINGLE, FB_DATATYPE_DOUBLE
		'' IEEE bits avoid locale, decimal rounding, and loss of negative zero.
		'' The existing helper applies the compiler host's DOUBLE word policy.
		value_kind = "float64-bits"
		value_text = hFloatToHex(value->f, FB_DATATYPE_DOUBLE)
	case else
		if( typeGetClass(dtype) <> FB_DATACLASS_INTEGER ) then return FALSE
		if( typeIsSigned(dtype) ) then
			value_kind = "signed"
			value_text = fbSemanticModelNumber(value->i)
		else
			value_kind = "unsigned"
			value_text = ltrim(str(culngint(value->i)))
		end if
	end select
	return TRUE
end function

sub fbSemanticModelExportValue _
	( byref domain as const string, byval identity as longint, _
	  byval dtype as integer, byval value as FBVALUE ptr )
	dim as string value_kind, value_text
	if( fbSemanticModelFormatValue(dtype, value, value_kind, value_text) = FALSE ) then exit sub
	fbSemanticModelAppendDetail("C" + TABCHAR + domain + TABCHAR + _
		fbSemanticModelNumber(identity) + TABCHAR + value_kind + TABCHAR + value_text)
end sub

'' -------------------------------------------------------------------------
'' Symbol snapshots
'' -------------------------------------------------------------------------

private sub hExportArray(byval sym as FBSYMBOL ptr, byval symbolid as longint)
	dim as integer rank = symbGetArrayDimensions(sym)
	if( rank = 0 ) then exit sub
	if( (rank < -1) or (rank > FB_MAXARRAYDIMS) ) then
		fbSemanticModelFail( )
		exit sub
	end if
	dim as string prefix = "A" + TABCHAR + fbSemanticModelNumber(symbolid) + _
		TABCHAR + fbSemanticModelNumber(rank) + TABCHAR
	fbSemanticModelExportRelation("symbol", symbolid, sym->var_.array.desc, "array-descriptor")
	fbSemanticModelExportRelation("symbol", symbolid, sym->var_.array.desctype, "array-descriptor-type")
	fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + _
		fbSemanticModelNumber(symbolid) + TABCHAR + "array-elements" + TABCHAR + _
		fbSemanticModelNumber(sym->var_.array.elements))
	'' Dynamic arrays have a known or unknown rank, but their runtime bounds
	'' are not constants. Do not publish descriptor allocation defaults as bounds.
	if( (rank < 0) or symbGetIsDynamic(sym) or _
		(sym->var_.array.dimtb = NULL) ) then
		fbSemanticModelAppendDetail(prefix + "-1" + TABCHAR + "runtime" + TABCHAR + TABCHAR)
		return
	end if
	for index as integer = 0 to rank - 1
		dim as FBARRAYDIM ptr bound = @sym->var_.array.dimtb[index]
		dim as string upper_bound, bound_kind = "fixed"
		if( bound->upper = FB_ARRAYDIM_UNKNOWN ) then
			bound_kind = "unknown"
		else
			upper_bound = fbSemanticModelNumber(bound->upper)
		end if
		fbSemanticModelAppendDetail(prefix + fbSemanticModelNumber(index) + TABCHAR + _
			bound_kind + TABCHAR + fbSemanticModelNumber(bound->lower) + TABCHAR + upper_bound)
	next
end sub

private sub hExportProcedure(byval sym as FBSYMBOL ptr, byval symbolid as longint, _
	byval variables_live as integer)
	dim as string proc_kind, operator_code
	if( symbGetIsFuncPtr(sym) ) then
		proc_kind = "procedure-pointer"
	elseif( symbIsConstructor(sym) ) then
		proc_kind = "constructor"
	elseif( symbIsDestructor1(sym) or symbIsDestructor0(sym) ) then
		proc_kind = "destructor"
	elseif( symbIsOperator(sym) ) then
		proc_kind = "operator"
		operator_code = fbSemanticModelOperatorCode(symbGetProcOpOvl(sym))
	elseif( symbIsProperty(sym) ) then
		proc_kind = iif(symbGetType(sym) = FB_DATATYPE_VOID, "property-set", "property-get")
	else
		proc_kind = iif(symbGetType(sym) = FB_DATATYPE_VOID, "sub", "function")
	end if
	dim as longint overriddenid = 0
	dim as integer vtableindex = 0
	if( sym->proc.ext <> NULL ) then
		overriddenid = fbSemanticModelSymbolId(sym->proc.ext->overridden)
		vtableindex = sym->proc.ext->vtableindex
	end if
	dim as integer param_count = sym->proc.params
	if( symbIsMethod(sym) ) then param_count -= 1
	fbSemanticModelAppendDetail("F" + TABCHAR + fbSemanticModelNumber(symbolid) + TABCHAR + _
		proc_kind + TABCHAR + hCallingConvention(sym->proc.mode) + TABCHAR + _
		fbSemanticModelNumber(param_count) + TABCHAR + fbSemanticModelNumber(sym->proc.optparams) + _
		TABCHAR + fbSemanticModelNumber(abs(symbIsReturnByRef(sym))) + TABCHAR + _
		fbSemanticModelNumber(sym->proc.realdtype) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(sym->proc.realsubtype)) + TABCHAR + _
		fbSemanticModelNumber(sym->proc.returnMethod) + TABCHAR + operator_code + TABCHAR + _
		fbSemanticModelNumber(overriddenid) + TABCHAR + fbSemanticModelNumber(vtableindex))
	fbSemanticModelExportRelation("symbol", symbolid, sym->proc.ovl.next, "overload-next")
	if( sym->proc.ext <> NULL ) then
		hSymbolNumber(symbolid, "procedure-status", sym->proc.ext->stats)
		hSymbolNumber(symbolid, "startup-priority", sym->proc.ext->priority)
		hSymbolNumber(symbolid, "declaration-statement", sym->proc.ext->stmtnum)
		hSymbolNumber(symbolid, "return-used", abs((sym->proc.ext->stats and FB_PROCSTATS_RETURNUSED) <> 0))
		hSymbolNumber(symbolid, "result-assignment-used", abs((sym->proc.ext->stats and FB_PROCSTATS_ASSIGNUSED) <> 0))
		hSymbolNumber(symbolid, "gosub-used", abs((sym->proc.ext->stats and FB_PROCSTATS_GOSUBUSED) <> 0))
		fbSemanticModelExportRelation("symbol", symbolid, sym->proc.ext->overridden, "overrides")
		if( variables_live ) then
			fbSemanticModelExportRelation("symbol", symbolid, sym->proc.ext->res, "result-variable")
		end if
	end if
	dim as FBSYMBOL ptr param = symbGetProcHeadParam(sym)
	dim as integer ordinal = 0
	if( symbIsMethod(sym) ) then ordinal = -1
	while( param <> NULL )
		dim as longint paramid = fbSemanticModelSymbolId(param)
		dim as longint variableid = fbSemanticModelParameterVariable(param, variables_live)
		fbSemanticModelAppendDetail("G" + TABCHAR + fbSemanticModelNumber(symbolid) + _
			TABCHAR + fbSemanticModelNumber(paramid) + TABCHAR + fbSemanticModelNumber(ordinal) + _
			TABCHAR + hParamMode(param->param.mode) + TABCHAR + _
			fbSemanticModelNumber(abs(symbParamIsOptional(param))) + TABCHAR + _
			fbSemanticModelNumber(param->param.bydescdimensions) + TABCHAR + _
			fbSemanticModelNumber(variableid) + TABCHAR + _
			fbSemanticModelNumber(abs(ordinal < 0)))
		'' A procedure-pointer signature copied from a method retains THIS's
		'' raw attributes, but passes it as an explicit parameter at ordinal 0.
		ordinal += 1
		param = param->next
	wend
end sub

private function hWideTokenText(byval text as wstring ptr) as string
	if( text = NULL ) then return ""
	'' Preprocessor text is retained as compiler wide units, not evaluated as
	'' a string literal. Eight hex digits preserve each unit on either host ABI.
	dim as integer units = len(*text)
	if( units > SEMANTIC_MAX_LITERAL_UNITS ) then
		fbSemanticModelFail( )
		return ""
	end if
	dim as string result = space(units * 8)
	if( len(result) <> units * 8 ) then
		fbSemanticModelFail( )
		return ""
	end if
	for index as integer = 0 to units - 1
		mid(result, index * 8 + 1, 8) = hex(culng((*text)[index]), 8)
	next
	return result
end function

private sub hExportDefine(byval sym as FBSYMBOL ptr, byval symbolid as longint)
	dim as string prefix = "K" + TABCHAR + "symbol" + TABCHAR + fbSemanticModelNumber(symbolid) + TABCHAR
	fbSemanticModelAppendDetail(prefix + "macro-argument-count" + TABCHAR + fbSemanticModelNumber(sym->def.params))
	fbSemanticModelAppendDetail(prefix + "macro-flags" + TABCHAR + fbSemanticModelNumber(sym->def.flags))
	fbSemanticModelAppendDetail(prefix + "macro-argless" + TABCHAR + fbSemanticModelNumber(abs(sym->def.isargless <> FALSE)))
	'' Text defines initialize dprocz, while parameterized macros initialize
	'' both callback fields. Do not read mprocw from a reused text-define node.
	dim as integer is_macro = (sym->typ and FB_DATATYPE_INVALID) <> 0
	dim as integer is_callback = (sym->def.dprocz <> NULL)
	if( is_macro ) then is_callback or= (sym->def.mprocw <> NULL)
	if( is_callback ) then
		fbSemanticModelAppendDetail(prefix + "macro-kind" + TABCHAR + "callback")
		exit sub
	end if
	fbSemanticModelAppendDetail(prefix + "macro-kind" + TABCHAR + iif(is_macro, "tokens", "text"))
	dim as string token_prefix = "Z" + TABCHAR + fbSemanticModelNumber(symbolid) + TABCHAR
	dim as FB_DEFPARAM ptr param = sym->def.paramhead
	while( param <> NULL )
		dim as string param_name
		if( param->name <> NULL ) then param_name = *param->name
		fbSemanticModelAppendDetail(token_prefix + fbSemanticModelNumber(param->num) + _
			TABCHAR + "parameter" + TABCHAR + fbSemanticModelEscape(param_name))
		param = param->next
	wend
	if( is_macro = FALSE ) then
		dim as string value, token_kind = "text"
		if( sym->typ = FB_DATATYPE_WCHAR ) then
			token_kind = "wide-text"
			value = hWideTokenText(sym->def.textw)
		elseif( sym->def.text <> NULL ) then
			value = *sym->def.text
		end if
		fbSemanticModelAppendDetail(token_prefix + "0" + TABCHAR + token_kind + TABCHAR + fbSemanticModelEscape(value))
		exit sub
	end if
	dim as FB_DEFTOK ptr token = sym->def.tokhead
	dim as integer ordinal = 0
	while( (token <> NULL) and fbSemanticModelFullEnabled( ) )
		dim as string value, token_kind
		select case token->type
		case FB_DEFTOK_TYPE_PARAM
			token_kind = "parameter-reference"
			value = fbSemanticModelNumber(token->paramnum)
		case FB_DEFTOK_TYPE_PARAMSTR
			token_kind = "stringify-reference"
			value = fbSemanticModelNumber(token->paramnum)
		case FB_DEFTOK_TYPE_TEX
			token_kind = "text"
			if( token->text <> NULL ) then value = *token->text
		case FB_DEFTOK_TYPE_TEXW
			token_kind = "wide-text"
			value = hWideTokenText(token->textw)
		case else
			fbSemanticModelFail( )
			exit sub
		end select
		fbSemanticModelAppendDetail(token_prefix + fbSemanticModelNumber(ordinal) + TABCHAR + _
			token_kind + TABCHAR + fbSemanticModelEscape(value))
		ordinal += 1
		token = token->next
	wend
end sub

sub fbSemanticModelExportSymbolDetails(byval sym as FBSYMBOL ptr, byval variables_live as integer)
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	dim as longint symbolid = fbSemanticModelSymbolId(sym)
	if( symbolid = 0 ) then exit sub
	fbSemanticModelExportType("symbol", symbolid, sym->typ, sym->subtype, sym->lgt)
	dim as longint ownerid = 0, namespaceid = 0
	if( sym->symtb <> NULL ) then ownerid = fbSemanticModelSymbolId(sym->symtb->owner)
	if( sym->hash.tb <> NULL ) then namespaceid = fbSemanticModelSymbolId(sym->hash.tb->owner)
	dim as string type_name = hTypeName(sym), alias_name, symbol_name
	symbol_name = fbSemanticModelDeclarationName(sym)
	if( sym->id.alias <> NULL ) then alias_name = *sym->id.alias
	fbSemanticModelAppendDetail("T" + TABCHAR + fbSemanticModelNumber(symbolid) + TABCHAR + _
		fbSemanticModelEscape(symbol_name) + TABCHAR + hSymbolClass(sym) + TABCHAR + fbSemanticModelTypeKind(sym) + TABCHAR + _
		fbSemanticModelEscape(type_name) + TABCHAR + fbSemanticModelNumber(sym->typ) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(sym->subtype)) + TABCHAR + _
		fbSemanticModelNumber(ownerid) + TABCHAR + fbSemanticModelNumber(namespaceid) + TABCHAR + _
		fbSemanticModelNumber(sym->attrib) + TABCHAR + fbSemanticModelNumber(sym->pattrib) + TABCHAR + _
		fbSemanticModelNumber(sym->stats) + TABCHAR + fbSemanticModelNumber(sym->lgt) + TABCHAR + _
		fbSemanticModelNumber(sym->ofs) + TABCHAR + fbSemanticModelEscape(alias_name) + TABCHAR + _
		hVisibility(sym) + TABCHAR + hStorage(sym) + TABCHAR + fbSemanticModelSymbolOrigin(sym))

	select case sym->class
	case FB_SYMBCLASS_VAR, FB_SYMBCLASS_FIELD
		hExportArray(sym, symbolid)
		hSymbolNumber(symbolid, "declaration-statement", sym->var_.stmtnum)
		if( sym->attrib and FB_SYMBATTRIB_LITERAL ) then
			if( (symbGetType(sym) = FB_DATATYPE_CHAR) or (symbGetType(sym) = FB_DATATYPE_WCHAR) ) then
				dim as FBVALUE value
				value.s = sym
				fbSemanticModelExportValue("symbol", symbolid, sym->typ, @value)
			end if
		else
			fbSemanticModelExportInitializer(sym, sym->var_.initree, "initializer")
		end if
		fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + _
			fbSemanticModelNumber(symbolid) + TABCHAR + "alignment" + TABCHAR + _
			fbSemanticModelNumber(sym->var_.align))
		if( symbIsField(sym) ) then
			fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + _
				fbSemanticModelNumber(symbolid) + TABCHAR + "bit-position" + TABCHAR + _
				fbSemanticModelNumber(sym->var_.bitpos))
			fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + _
				fbSemanticModelNumber(symbolid) + TABCHAR + "bit-width" + TABCHAR + _
				fbSemanticModelNumber(sym->var_.bits))
		end if
		fbSemanticModelExportVariable(sym)
	case FB_SYMBCLASS_CONST
		fbSemanticModelExportValue("symbol", symbolid, sym->typ, @sym->val.value)
		hSymbolNumber(symbolid, "literal-suffix", abs(sym->val.hassuffix <> FALSE))
	case FB_SYMBCLASS_DEFINE
		hExportDefine(sym, symbolid)
	case FB_SYMBCLASS_PROC
		hExportProcedure(sym, symbolid, variables_live)
	case FB_SYMBCLASS_PARAM
		fbSemanticModelExportInitializer(sym, sym->param.optexpr, "default-initializer")
		hSymbolNumber(symbolid, "argument-register", sym->param.regnum)
		fbSemanticModelExportRelation("symbol", symbolid, sym->param.bydescrealsubtype, "parameter-descriptor-type")
	case FB_SYMBCLASS_STRUCT, FB_SYMBCLASS_ENUM, FB_SYMBCLASS_SCOPE
		dim as longint baseid = 0, unpadded = 0
		dim as integer alignment = 0, options = 0, retdtype = 0, abstract_count = 0
		dim as integer elements = 0, start_line = 0, end_line = 0
		if( symbIsStruct(sym) ) then
			hSymbolNumber(symbolid, "natural-alignment", sym->udt.natalign)
			hSymbolNumber(symbolid, "packing-alignment", sym->udt.align)
			'' The return classifier initializes retin2regs only after layout
			'' is finalized. Earlier observations must not read a pool slot.
			hSymbolNumber(symbolid, "layout-finalized", abs(sym->udt.retdtype <> FB_DATATYPE_INVALID))
			if( sym->udt.retdtype <> FB_DATATYPE_INVALID ) then
				hSymbolNumber(symbolid, "aggregate-register-return", sym->udt.retin2regs)
			end if
			'' udt.base is the compiler's hidden base FIELD, whose subtype is
			'' the declared base type. Export that type, not the storage field.
			if( sym->udt.base <> NULL ) then baseid = fbSemanticModelSymbolId(sym->udt.base->subtype)
			alignment = sym->udt.natalign
			if( sym->udt.align > 0 ) then alignment = sym->udt.align
			unpadded = sym->udt.unpadlgt
			options = sym->udt.options
			retdtype = sym->udt.retdtype
			if( sym->udt.ext <> NULL ) then abstract_count = sym->udt.ext->abstractcount
		elseif( symbIsEnum(sym) ) then
			elements = sym->enum_.elements
		else
			start_line = sym->scp.dbg.iniline
			end_line = sym->scp.dbg.endline
		end if
		fbSemanticModelAppendDetail("U" + TABCHAR + fbSemanticModelNumber(symbolid) + _
			TABCHAR + hSymbolClass(sym) + TABCHAR + fbSemanticModelNumber(baseid) + _
			TABCHAR + fbSemanticModelNumber(alignment) + TABCHAR + fbSemanticModelNumber(unpadded) + _
			TABCHAR + fbSemanticModelNumber(options) + TABCHAR + fbSemanticModelNumber(retdtype) + _
			TABCHAR + fbSemanticModelNumber(abstract_count) + TABCHAR + fbSemanticModelNumber(elements) + _
			TABCHAR + fbSemanticModelNumber(start_line) + TABCHAR + fbSemanticModelNumber(end_line))
		if( symbIsStruct(sym) andalso symbGetUDTIsAnon(sym) ) then
			'' anonparent is initialized only for anonymous aggregate members.
			fbSemanticModelExportRelation("symbol", symbolid, sym->udt.anonparent, "anonymous-parent")
		end if
		if( symbIsStruct(sym) andalso (sym->udt.ext <> NULL) ) then
			hSymbolNumber(symbolid, "virtual-table-elements", sym->udt.ext->vtableelements)
			fbSemanticModelExportRelation("symbol", symbolid, sym->udt.ext->vtable, "virtual-table")
			fbSemanticModelExportRelation("symbol", symbolid, sym->udt.ext->rtti, "runtime-type-info")
			fbSemanticModelExportRelation("symbol", symbolid, sym->udt.ext->defctor, "default-constructor")
			fbSemanticModelExportRelation("symbol", symbolid, sym->udt.ext->copyctor, "copy-constructor")
			fbSemanticModelExportRelation("symbol", symbolid, sym->udt.ext->copyctorconst, "const-copy-constructor")
			fbSemanticModelExportRelation("symbol", symbolid, sym->udt.ext->dtor1, "destructor")
			fbSemanticModelExportRelation("symbol", symbolid, sym->udt.ext->dtor0, "deleting-destructor")
			fbSemanticModelExportRelation("symbol", symbolid, sym->udt.ext->copyletop, "copy-assignment")
			fbSemanticModelExportRelation("symbol", symbolid, sym->udt.ext->copyletopconst, "const-copy-assignment")
		end if
	case FB_SYMBCLASS_NSIMPORT
		fbSemanticModelExportRelation("symbol", symbolid, sym->nsimp.imp_ns, "imports")
	case FB_SYMBCLASS_LABEL
		hSymbolNumber(symbolid, "declared-label", abs(sym->lbl.declared <> FALSE))
		if( sym->lbl.declared ) then
			hSymbolNumber(symbolid, "declaration-statement", sym->lbl.stmtnum)
			hSymbolNumber(symbolid, "gosub-label", abs(sym->lbl.gosub <> FALSE))
			fbSemanticModelExportRelation("symbol", symbolid, sym->lbl.parent, "label-scope")
		end if
	case FB_SYMBCLASS_KEYWORD
		hSymbolNumber(symbolid, "keyword-token", sym->key.id)
		hSymbolNumber(symbolid, "keyword-class", sym->key.tkclass)
	end select
end sub

sub fbSemanticModelExportSymbolReplacement(byval previous as FBSYMBOL ptr, byval canonical as FBSYMBOL ptr)
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	fbSemanticModelExportSymbolDetails(previous)
	fbSemanticModelExportRelation("symbol", fbSemanticModelSymbolId(previous), canonical, "canonical-symbol")
end sub

sub fbSemanticModelExportProcPtrReplacement(byval previous as FBSYMBOL ptr, byval canonical as FBSYMBOL ptr)
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( (previous = NULL) or (canonical = NULL) or (previous = canonical) ) then exit sub
	if( (previous->class <> FB_SYMBCLASS_PROC) or (canonical->class <> FB_SYMBCLASS_PROC) ) then
		fbSemanticModelFail( )
		exit sub
	end if
	if( previous->proc.params <> canonical->proc.params ) then
		fbSemanticModelFail( )
		exit sub
	end if

	'' Parameter declarations can already have source bindings when an equal
	'' procptr signature is reused. The unused header never enters a live
	'' symbol table, and its ABI metadata has not been finalized. Preserve
	'' those bindings through the selected header and corresponding formals.
	fbSemanticModelExportRelation("symbol", fbSemanticModelSymbolId(previous), canonical, "canonical-symbol")
	dim as FBSYMBOL ptr previous_param = symbGetProcHeadParam(previous)
	dim as FBSYMBOL ptr canonical_param = symbGetProcHeadParam(canonical)
	while( (previous_param <> NULL) and (canonical_param <> NULL) )
		fbSemanticModelExportRelation("symbol", fbSemanticModelSymbolId(previous_param), canonical_param, "canonical-symbol")
		previous_param = symbGetParamNext(previous_param)
		canonical_param = symbGetParamNext(canonical_param)
	wend
	if( (previous_param <> NULL) or (canonical_param <> NULL) ) then fbSemanticModelFail( )
end sub

'' -------------------------------------------------------------------------
'' Bounded inventory of nested symbol tables
'' -------------------------------------------------------------------------

sub fbSemanticModelExportSymbols(byval head as FBSYMBOL ptr)
	if( (fbSemanticModelFullEnabled( ) = FALSE) or (head = NULL) ) then exit sub
	'' Each stack item resumes a parent table after its child. Visit children
	'' immediately so thousands of sibling prototypes do not consume one stack
	'' slot apiece. The bound applies to nesting, including generated types.
	const MAX_PENDING_TABLES = 4096
	dim as FBSYMBOL ptr ptr stack = callocate(MAX_PENDING_TABLES, sizeof(FBSYMBOL ptr))
	if( stack = NULL ) then
		fbSemanticModelFail( )
		exit sub
	end if
	dim as integer count = 1
	dim as ulongint visit = fbSemanticModelNextVisit( )
	stack[0] = head
	while( (count > 0) and fbSemanticModelFullEnabled( ) )
		count -= 1
		dim as FBSYMBOL ptr sym = stack[count]
		while( (sym <> NULL) and fbSemanticModelFullEnabled( ) )
			if( fbSemanticModelVisitSymbol(sym, visit) ) then
				'' Global and preprocessor namespaces are mock owners outside
				'' the ordinary tables. They still need a metadata snapshot.
				if( sym->hash.tb <> NULL ) then
					dim as FBSYMBOL ptr ns = sym->hash.tb->owner
					if( ns <> NULL ) then
						if( symbIsNamespace(ns) andalso (ns->id.name = NULL) ) then
							if( fbSemanticModelVisitSymbol(ns, visit) ) then fbSemanticModelExportSymbolDetails(ns)
						end if
					end if
				end if
				'' A nested type can contain a method whose locals were already
				'' flushed. Only the explicit ExportProc snapshot can certify that
				'' one procedure's parameter variables are still alive.
				fbSemanticModelExportSymbolDetails(sym)
				dim as FBSYMBOL ptr child = NULL
				select case sym->class
				case FB_SYMBCLASS_STRUCT: child = symbGetUDTSymbTbHead(sym)
				case FB_SYMBCLASS_ENUM: child = symbGetEnumSymbTbHead(sym)
				case FB_SYMBCLASS_NAMESPACE: child = symbGetNamespaceTbHead(sym)
				case FB_SYMBCLASS_SCOPE: child = symbGetScopeSymbTb(sym).head
				case FB_SYMBCLASS_PROC: child = symbGetProcHeadParam(sym)
				end select
				if( child <> NULL ) then
					if( count >= MAX_PENDING_TABLES ) then
						fbSemanticModelFail( )
						exit while
					end if
					stack[count] = sym->next
					count += 1
					sym = child
					continue while
				end if
			end if
			sym = sym->next
		wend
	wend
	deallocate(stack)
end sub

'' end of tooling/semantic-symbols.bas
