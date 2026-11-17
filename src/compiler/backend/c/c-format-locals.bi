/'
	Project: FreeBASIC C backend
	File: backend/c/c-format-locals.bi
	Purpose: Retain literal format contracts through private pointer bindings.
	Responsibilities: Check complete-body storage uses, track direct stores within
	a basic block and retain closed IIF format choices beside the evaluated pointer.
	This file intentionally does NOT contain general constant propagation,
	control-flow joins, source type changes or native diagnostic suppression.
'/

type CFORMATLOCAL
	sym as FBSYMBOL ptr
	identifier as zstring ptr
	literal as FBSYMBOL ptr
	plural as FBSYMBOL ptr
	translated as integer
	translation_only as integer
	choices(0 to 3) as FBSYMBOL ptr
	choice_count as integer
	iif_owner as ASTNODE ptr
end type

'' The current procedure owns the entries and copied C identifiers. Literal
'' symbols belong to the output's literal pool. Both identities must match
'' because AST loading can recycle temporary symbol slots during emission.
dim shared as TLIST c_format_locals
dim shared as integer c_format_locals_ready, c_format_local_value_emitted

private sub hCFormatLocalsClear()
	if( c_format_locals_ready = FALSE ) then exit sub
	do
		var entry = cptr(CFORMATLOCAL ptr, listGetHead(@c_format_locals))
		if( entry = NULL ) then exit do
		ZstrFree(entry->identifier)
		listDelNode(@c_format_locals, entry)
	loop
end sub

private sub hCFormatLocalsFinish()
	if( c_format_locals_ready = FALSE ) then exit sub
	hCFormatLocalsClear()
	listEnd(@c_format_locals)
	c_format_locals_ready = FALSE
end sub

private sub hCFormatLocalsInit()
	hCFormatLocalsFinish()
	listInit(@c_format_locals, 16, sizeof(CFORMATLOCAL), LIST_FLAGS_CLEAR)
	c_format_locals_ready = TRUE
	c_format_local_value_emitted = FALSE
end sub

private function hCFormatLocalFind(byval sym as FBSYMBOL ptr) as CFORMATLOCAL ptr
	if( sym = NULL or c_format_locals_ready = FALSE ) then return NULL
	var entry = cptr(CFORMATLOCAL ptr, listGetHead(@c_format_locals))
	while( entry <> NULL )
		if( entry->sym = sym andalso *entry->identifier = hGetCId(sym) ) then return entry
		entry = listGetNext(entry)
	wend
	return NULL
end function

private function hCFormatLocalParameter(byval param as FBSYMBOL ptr) as integer
	if( param = NULL orelse param->class <> FB_SYMBCLASS_PARAM ) then return FALSE
	if( symbGetParamMode(param) <> FB_PARAMMODE_BYVAL ) then return FALSE
	var proc = symbGetParent(param)
	if( symbGetProcMode(proc) <> FB_FUNCMODE_CDECL ) then return FALSE
	if( symbGetIsParsed(proc) or symbGetIsFuncPtr(proc) ) then return FALSE
	var identifier = symbGetMangledName(proc)
	if( identifier = NULL ) then return FALSE
	dim as integer index
	select case *identifier
	case "printf", "vprintf"
		index = 0
	case "fprintf", "sprintf", "vfprintf", "vsprintf"
		index = 1
	case "snprintf", "vsnprintf", "strftime"
		index = 2
	case else
		return FALSE
	end select
	var expected = symbGetProcHeadParam(proc)
	for i as integer = 1 to index
		if( expected = NULL ) then return FALSE
		expected = symbGetNext(expected)
	next
	if( expected <> param ) then return FALSE
	var dtype = symbGetFullType(param)
	if( typeGetPtrCnt(dtype) <> 1 ) then return FALSE
	select case as const typeGetDtOnly(dtype)
	case FB_DATATYPE_CHAR, FB_DATATYPE_BYTE, FB_DATATYPE_UBYTE
		return TRUE
	end select
	return FALSE
end function

private function hCFormatLocalCandidate(byval sym as FBSYMBOL ptr) as integer
	if( sym = NULL orelse symbIsVar(sym) = FALSE ) then return FALSE
	if( symbIsLocal(sym) = FALSE or symbIsParamVar(sym) or symbIsRef(sym) ) then return FALSE
	if( (sym->attrib and (FB_SYMBATTRIB_STATIC or FB_SYMBATTRIB_SHARED or _
	    FB_SYMBATTRIB_PUBLIC or FB_SYMBATTRIB_COMMON or FB_SYMBATTRIB_EXTERN or _
	    FB_SYMBATTRIB_IMPORT or FB_SYMBATTRIB_FUNCRESULT or FB_SYMBATTRIB_DESCRIPTOR)) <> 0 ) then return FALSE
	if( symbIsExport(sym) or symbIsThreadLocal(sym) or symbIsThreadLocalC(sym) ) then return FALSE
	if( symbGetArrayDimensions(sym) <> 0 or symbGetTypeIniTree(sym) <> NULL ) then return FALSE
	var dtype = symbGetFullType(sym)
	if( typeGetPtrCnt(dtype) <> 1 ) then return FALSE
	select case as const typeGetDtOnly(dtype)
	case FB_DATATYPE_CHAR, FB_DATATYPE_BYTE, FB_DATATYPE_UBYTE
		return TRUE
	end select
	return FALSE
end function

private function hCFormatAstLiteral(byval n as ASTNODE ptr) as FBSYMBOL ptr
	while( n <> NULL andalso n->class = AST_NODECLASS_CONV )
		if( typeIsPtr(n->dtype) = FALSE ) then return NULL
		n = n->l
	wend
	if( n = NULL ) then return NULL
	if( n->class = AST_NODECLASS_ADDROF ) then
		n = n->l
		if( n = NULL orelse n->class <> AST_NODECLASS_VAR ) then return NULL
		if( n->var_.ofs <> 0 ) then return NULL
	elseif( n->class = AST_NODECLASS_OFFSET ) then
		if( n->ofs.ofs <> 0 ) then return NULL
	else
		return NULL
	end if
	if( n->sym = NULL orelse symbGetIsLiteral(n->sym) = FALSE ) then return NULL
	if( symbGetType(n->sym) <> FB_DATATYPE_CHAR ) then return NULL
	return n->sym
end function

private function hCFormatAstChoices(byval n as ASTNODE ptr, _
	byref first as FBSYMBOL ptr, byref second as FBSYMBOL ptr) as integer
	first = hCFormatAstLiteral(n)
	second = NULL
	if( first <> NULL ) then return TRUE
	while( n <> NULL andalso n->class = AST_NODECLASS_CONV )
		if( typeIsPtr(n->dtype) = FALSE ) then return FALSE
		n = n->l
	wend
	if( n = NULL orelse n->class <> AST_NODECLASS_CALL ) then return FALSE
	if( n->l <> NULL ) then return FALSE
	dim as integer plural
	var index = hCFormatArgument(n->sym, plural)
	if( index < 0 ) then return FALSE
	var param = symbGetProcHeadParam(n->sym)
	for i as integer = 1 to index
		param = symbGetNext(param)
	next
	var arg = n->r
	while( arg <> NULL )
		if( arg->sym = param ) then first = hCFormatAstLiteral(arg->l)
		if( plural andalso arg->sym = symbGetNext(param) ) then second = hCFormatAstLiteral(arg->l)
		arg = arg->r
	wend
	return first <> NULL andalso (plural = FALSE orelse second <> NULL)
end function

private sub hCFormatIifCollect(byval n as ASTNODE ptr)
	if( n->class <> AST_NODECLASS_IIF orelse hCFormatLocalCandidate(n->sym) = FALSE ) then exit sub
	if( symbIsTemp(n->sym) = FALSE orelse hCFormatLocalFind(n->sym) <> NULL ) then exit sub
	if( n->l = NULL orelse n->l->class <> AST_NODECLASS_VAR ) then exit sub
	if( n->l->sym <> n->sym or n->l->var_.ofs <> 0 ) then exit sub
	if( n->r = NULL orelse n->r->r = NULL ) then exit sub
	var branches = n->r->r
	if( branches->class <> AST_NODECLASS_LINK ) then exit sub
	dim as FBSYMBOL ptr choices(0 to 3)
	for i as integer = 0 to 1
		var branch = iif(i = 0, branches->l, branches->r)
		if( branch = NULL orelse branch->class <> AST_NODECLASS_ASSIGN ) then exit sub
		if( branch->l = NULL orelse branch->l->class <> AST_NODECLASS_VAR ) then exit sub
		if( branch->l->sym <> n->sym or branch->l->var_.ofs <> 0 ) then exit sub
		if( hCFormatAstChoices(branch->r, choices(i * 2), choices(i * 2 + 1)) = FALSE ) then exit sub
	next
	var entry = cptr(CFORMATLOCAL ptr, listNewNode(@c_format_locals))
	entry->sym = n->sym
	var identifier = hGetCId(n->sym)
	entry->identifier = ZstrDup(strptr(identifier))
	entry->iif_owner = n
	'' An IIF has two branches. Each branch contributes one literal, or the
	'' two original format arguments of a native plural translation call.
	for i as integer = 0 to 3
		if( choices(i) <> NULL ) then
			entry->choices(entry->choice_count) = choices(i)
			entry->choice_count += 1
		end if
	next
end sub

private function hCFormatIifOnlyUses(byval n as ASTNODE ptr, byval owner as ASTNODE ptr) as integer
	if( n = NULL ) then return TRUE
	if( n->sym = owner->sym ) then
		if( n <> owner and n <> owner->l and n->class <> AST_NODECLASS_DECL ) then
			var branches = owner->r->r
			if( n <> branches->l->l and n <> branches->r->l ) then return FALSE
		end if
	end if
	if( n->class = AST_NODECLASS_CALL ) then
		if( n->call.tmpres = owner->sym ) then return FALSE
		var item = n->call.strtail
		while( item <> NULL )
			if( item->sym = owner->sym or hCReadOnlyLocalContains(item->srctree, owner->sym) ) then return FALSE
			item = item->prev
		wend
		if( hCReadOnlyLocalContains(n->call.profbegin, owner->sym) ) then return FALSE
		if( hCReadOnlyLocalContains(n->call.profend, owner->sym) ) then return FALSE
	end if
	return hCFormatIifOnlyUses(n->l, owner) andalso hCFormatIifOnlyUses(n->r, owner)
end function

private sub hCFormatLocalCollect(byval n as ASTNODE ptr)
	if( n = NULL ) then exit sub
	if( n->class = AST_NODECLASS_ARG andalso hCFormatLocalParameter(n->sym) ) then
		var value = n->l
		while( value <> NULL andalso value->class = AST_NODECLASS_CONV )
			if( typeIsPtr(value->dtype) = FALSE ) then exit while
			value = value->l
		wend
		if( value <> NULL ) then hCFormatIifCollect(value)
		if( c_local_refs_complete andalso value <> NULL andalso value->class = AST_NODECLASS_VAR andalso value->var_.ofs = 0 ) then
			if( hCFormatLocalCandidate(value->sym) andalso hCFormatLocalFind(value->sym) = NULL ) then
				var entry = cptr(CFORMATLOCAL ptr, listNewNode(@c_format_locals))
				entry->sym = value->sym
				var identifier = hGetCId(value->sym)
				entry->identifier = ZstrDup(strptr(identifier))
			end if
		end if
	end if
	hCFormatLocalCollect(n->l)
	hCFormatLocalCollect(n->r)
end sub

private function hCFormatCopySource(byval n as ASTNODE ptr) as FBSYMBOL ptr
	while( n <> NULL andalso n->class = AST_NODECLASS_CONV )
		if( typeIsPtr(n->dtype) = FALSE ) then return NULL
		n = n->l
	wend
	if( n = NULL orelse n->class <> AST_NODECLASS_VAR ) then return NULL
	if( n->var_.ofs <> 0 orelse hCFormatLocalCandidate(n->sym) = FALSE ) then return NULL
	return n->sym
end function

private function hCFormatCopyOnlyUses(byval n as ASTNODE ptr, _
	byval source as FBSYMBOL ptr, byval destination as FBSYMBOL ptr) as integer
	if( hCReadOnlyLocalCheck(n, source) ) then return TRUE
	'' The destination has already passed complete-body validation. Copying
	'' into that binding exposes neither the source slot nor a writable alias.
	if( n->class = AST_NODECLASS_ASSIGN andalso n->l <> NULL ) then
		if( n->l->class = AST_NODECLASS_VAR andalso n->l->sym = destination andalso n->l->var_.ofs = 0 ) then
			return hCFormatCopySource(n->r) = source
		end if
	elseif( n->class = AST_NODECLASS_LINK ) then
		return hCFormatCopyOnlyUses(n->l, source, destination) andalso _
			hCFormatCopyOnlyUses(n->r, source, destination)
	end if
	return FALSE
end function

private sub hCFormatCopyCollect(byval n as ASTNODE ptr, byval first as ASTNODE ptr)
	if( n = NULL ) then exit sub
	if( n->class = AST_NODECLASS_ASSIGN andalso n->l <> NULL andalso n->l->class = AST_NODECLASS_VAR ) then
		var destination = hCFormatLocalFind(n->l->sym)
		var source = hCFormatCopySource(n->r)
		if( destination <> NULL andalso destination->choice_count = 0 andalso n->l->var_.ofs = 0 andalso _
		    source <> NULL andalso hCFormatLocalFind(source) = NULL ) then
			var statement = first
			while( statement <> NULL )
				if( hCFormatCopyOnlyUses(statement, source, destination->sym) = FALSE ) then exit while
				statement = statement->next
			wend
			if( statement = NULL ) then
				var entry = cptr(CFORMATLOCAL ptr, listNewNode(@c_format_locals))
				entry->sym = source
				var identifier = hGetCId(source)
				entry->identifier = ZstrDup(strptr(identifier))
				entry->translation_only = TRUE
			end if
		end if
	end if
	hCFormatCopyCollect(n->l, first)
	hCFormatCopyCollect(n->r, first)
end sub

private sub hCFormatLocalsBegin(byval first as ASTNODE ptr)
	hCFormatLocalsClear()
	if( c_format_locals_ready = FALSE or c_abi_body_complete = FALSE ) then exit sub
	'' Implicit IIF slots have their own complete-use proof in debug builds.
	'' Ordinary source bindings still use the scalar storage-analysis gate.
	var n = first
	while( n <> NULL )
		hCFormatLocalCollect(n)
		n = n->next
	wend
	var entry = cptr(CFORMATLOCAL ptr, listGetHead(@c_format_locals))
	while( entry <> NULL )
		var following = cptr(CFORMATLOCAL ptr, listGetNext(entry))
		n = first
		while( n <> NULL )
			'' Reuse the complete-body pointer check. A BYREF escape, address
			'' exposure, pointee write or opaque operation rejects the binding.
			var valid = FALSE
			if( entry->iif_owner <> NULL ) then
				valid = hCFormatIifOnlyUses(n, entry->iif_owner)
			else
				valid = hCReadOnlyLocalCheck(n, entry->sym)
			end if
			if( valid = FALSE ) then
				ZstrFree(entry->identifier)
				listDelNode(@c_format_locals, entry)
				exit while
			end if
			n = n->next
		wend
		if( n = NULL ) then
			if( entry->choice_count <> 0 ) then hCReadOnlyLocalRequire(entry->sym)
			entry->iif_owner = NULL
		end if
		entry = following
	wend
	if( c_local_refs_complete andalso listGetHead(@c_format_locals) <> NULL ) then
		n = first
		while( n <> NULL )
			hCFormatCopyCollect(n, first)
			n = n->next
		wend
	end if
end sub

private sub hCFormatLocalsInvalidate(byval translations_only as integer)
	if( c_format_locals_ready = FALSE ) then exit sub
	var entry = cptr(CFORMATLOCAL ptr, listGetHead(@c_format_locals))
	while( entry <> NULL )
		if( translations_only = FALSE or entry->translated ) then
			entry->literal = NULL
			entry->plural = NULL
			entry->translated = FALSE
		end if
		entry = listGetNext(entry)
	wend
	var value = cptr(EXPRNODE ptr, listGetHead(@ctx.exprnodes))
	while( value <> NULL )
		value->format_literal = NULL
		value->format_plural = NULL
		value = listGetNext(value)
	wend
end sub

private function hCFormatLocalLiteral(byval value as EXPRNODE ptr, _
	byref plural as FBSYMBOL ptr, byref translated as integer) as FBSYMBOL ptr
	plural = NULL
	translated = FALSE
	var literal = hCLiteralAddressSymbol(value)
	if( literal <> NULL ) then return literal
	while( value <> NULL andalso value->class = EXPRCLASS_CAST )
		if( typeIsPtr(value->dtype) = FALSE ) then return NULL
		value = value->l
	wend
	if( value = NULL ) then return NULL
	if( value->format_literal <> NULL ) then
		plural = value->format_plural
		translated = TRUE
		return value->format_literal
	end if
	if( value->class <> EXPRCLASS_SYM ) then return NULL
	var entry = hCFormatLocalFind(value->sym)
	if( entry = NULL ) then return NULL
	plural = entry->plural
	translated = entry->translated
	return entry->literal
end function

private sub hCFormatLocalStore(byval vr as IRVREG ptr, byval value as EXPRNODE ptr)
	if( vr->typ = IR_VREGTYPE_REG ) then exit sub
	if( vr->typ <> IR_VREGTYPE_VAR or vr->ofs <> 0 or vr->vidx <> NULL ) then
		hCFormatLocalsInvalidate()
		exit sub
	end if
	var entry = hCFormatLocalFind(vr->sym)
	if( entry = NULL ) then exit sub
	dim as FBSYMBOL ptr plural
	dim as integer translated
	var literal = hCFormatLocalLiteral(value, plural, translated)
	'' Complete-body validation admits only the two assignments owned by
	'' this IIF. Its result contract survives the labels used for lowering.
	if( entry->choice_count <> 0 ) then exit sub
	if( entry->translation_only and translated = FALSE ) then literal = NULL
	entry->literal = literal
	entry->plural = plural
	entry->translated = translated
end sub

private function hCFormatLocalValue(byval value as EXPRNODE ptr) as EXPRNODE ptr
	'' Direct literal expressions already retain native format checking.
	if( hCLiteralAddressSymbol(value) <> NULL ) then return value
	var binding = value
	while( binding <> NULL andalso binding->class = EXPRCLASS_CAST )
		if( typeIsPtr(binding->dtype) = FALSE ) then return value
		binding = binding->l
	wend
	if( binding = NULL ) then return value
	'' The evaluated translation helper already carries format_arg. A saved
	'' binding needs a new wrapper because its C identifier loses that contract.
	if( binding->format_literal <> NULL ) then return value
	dim as FBSYMBOL ptr plural
	dim as integer translated
	var literal = hCFormatLocalLiteral(value, plural, translated)
	dim as CFORMATLOCAL ptr choices
	if( literal = NULL ) then
		if( binding = NULL orelse binding->class <> EXPRCLASS_SYM ) then return value
		choices = hCFormatLocalFind(binding->sym)
		if( choices = NULL orelse choices->choice_count = 0 ) then return value
	end if
	if( c_format_local_value_emitted = FALSE ) then
		var section = sectionGosub(0)
		var first = ctx.sections(0).bufferlen
		'' Backend C identifiers are disjoint from encoded source names. The
		'' assembler-local label also keeps explicit foreign aliases distinct.
		hWriteLine("static inline const char* fb_c_format_value(const char* value, const char* format) " + _
			"__asm__("".Lfb_c_format_value"") __attribute__((format_arg(2)));")
		hWriteLine("static inline const char* fb_c_format_value(const char* value, const char* format)")
		hWriteLine("{ (void)format; return value; }")
		hCLeafNativeSpan("fb_c_format_value", TRUE, first, ctx.sections(0).bufferlen)
		sectionReturn(section)
		c_format_local_value_emitted = TRUE
	end if
	'' Pass the evaluated binding. Literal addresses describe its proven
	'' format contract without repeating a source expression or call.
	hCLeafNativeReference("fb_c_format_value")
	var evaluated = exprFlush(value)
	dim as string format
	if( choices <> NULL ) then
		'' The private IIF slot has exactly its two branch stores. Metadata
		'' reads cannot repeat the source condition or a translation call.
		format = hCLiteralStorageName(choices->choices(choices->choice_count - 1))
		for i as integer = choices->choice_count - 2 to 0 step -1
			var source = hCLiteralStorageName(choices->choices(i))
			format = "((const char*)" + evaluated + " == " + source + " ? " + source + " : " + format + ")"
		next
	else
		format = hCLiteralStorageName(literal)
		if( plural <> NULL ) then
			format = "(" + evaluated + " ? " + format + " : " + hCLiteralStorageName(plural) + ")"
		end if
	end if
	var text = "fb_c_format_value((const char*)" + evaluated + ", " + format + ")"
	return exprNewTEXT(typeAddrOf(typeSetIsConst(FB_DATATYPE_CHAR)), NULL, text)
end function

/' end of backend/c/c-format-locals.bi '/
