/'
	Project: FreeBASIC source diagnostics
	File: ast/nodes/source-unused-warnings.bi
	Purpose: Report named source values which have no surviving read or escape.
	Responsibilities: Own declaration locations until procedure completion, inspect
	reads independently of the selected backend, and exclude compiler temporaries.
	This file intentionally does NOT remove stores or change procedure signatures.
'/

#include once "support/allocate.bi"
#include once "support/strings/dstr.bi"

type SOURCEWARNINGVAR
	sym as FBSYMBOL ptr
	filename as string
	linenum as integer
	read_value as integer
	next_var as SOURCEWARNINGVAR ptr
end type

type SOURCEWARNINGPROC
	proc as FBSYMBOL ptr
	index as THASH
	vars as SOURCEWARNINGVAR ptr
	previous as SOURCEWARNINGPROC ptr
	opaque as integer
end type

'' Parsing can temporarily enter a generated procedure. Each frame owns its
'' location strings, hash keys and records; symbols are borrowed only until END.
dim shared source_warning_proc as SOURCEWARNINGPROC ptr

private sub hSourceWarningsBegin(byval proc as FBSYMBOL ptr)
	var frame = cptr(SOURCEWARNINGPROC ptr, xcallocate(len(SOURCEWARNINGPROC)))
	frame->proc = proc
	frame->previous = source_warning_proc
	source_warning_proc = frame
end sub

private function hSourceWarningVar(byval sym as FBSYMBOL ptr) as SOURCEWARNINGVAR ptr
	if( source_warning_proc = NULL orelse source_warning_proc->index.list = NULL ) then return NULL
	var key = str(culngint(cuint(sym)))
	return hashLookup(@source_warning_proc->index, strptr(key))
end function

sub astWarnDeclareVariable(byval sym as FBSYMBOL ptr)
	if( source_warning_proc = NULL ) then exit sub
	if( symbGetIsImplicit(source_warning_proc->proc) and _
	    symbGetIsMainProc(source_warning_proc->proc) = FALSE ) then exit sub
	if( env.clopt.warninglevel <> FB_WARNINGMSGS_LOWEST_LEVEL ) then exit sub
	if( sym = NULL orelse symbIsVar(sym) = FALSE ) then exit sub
	if( symbIsTemp(sym) or symbGetIsTemporary(sym) or symbGetIsImplicit(sym) ) then exit sub
	if( symbIsLocal(sym) = FALSE and symbIsParamVar(sym) = FALSE ) then exit sub
	if( (sym->attrib and (FB_SYMBATTRIB_SHARED or FB_SYMBATTRIB_COMMON or _
	    FB_SYMBATTRIB_PUBLIC or FB_SYMBATTRIB_EXTERN or FB_SYMBATTRIB_FUNCRESULT or _
	    FB_SYMBATTRIB_DESCRIPTOR)) <> 0 ) then exit sub
	if( symbGetName(sym) = NULL ) then exit sub
	if( hSourceWarningVar(sym) <> NULL ) then exit sub
	'' Owned objects and strings can be observed by implicit lifetime code.
	'' Parameters of any type are safe to inspect: their declaration is explicit.
	if( symbIsParamVar(sym) = FALSE ) then
		if( symbGetArrayDimensions(sym) <> 0 or symbIsRef(sym) ) then exit sub
		if( symbHasCtor(sym) or symbHasDtor(sym) ) then exit sub
		select case as const typeGetClass(symbGetFullType(sym))
		case FB_DATACLASS_INTEGER, FB_DATACLASS_FPOINT
		case else
			exit sub
		end select
		select case as const symbGetType(sym)
		case FB_DATATYPE_CHAR, FB_DATATYPE_WCHAR, FB_DATATYPE_XMMWORD
			exit sub
		end select
	end if
	if( source_warning_proc->index.list = NULL ) then hashInit(@source_warning_proc->index, 64, TRUE)
	var entry = cptr(SOURCEWARNINGVAR ptr, xcallocate(len(SOURCEWARNINGVAR)))
	entry->sym = sym
	entry->filename = env.inf.name
	entry->linenum = lexLineNum()
	entry->read_value = FALSE
	entry->next_var = source_warning_proc->vars
	source_warning_proc->vars = entry
	var key = str(culngint(cuint(sym)))
	hashAdd(@source_warning_proc->index, ZstrDup(strptr(key)), entry, hashHash(strptr(key)))
end sub

private sub hSourceWarningsRead(byval n as ASTNODE ptr)
	if( n = NULL ) then exit sub
	select case as const n->class
	case AST_NODECLASS_ASM
		'' Assembly can access locals through textual offsets or saved addresses.
		source_warning_proc->opaque = TRUE
		return
	case AST_NODECLASS_DECL
		'' Default initialization does not read the declared variable.
		return
	case AST_NODECLASS_ASSIGN
		if( n->l <> NULL andalso n->l->class = AST_NODECLASS_VAR ) then
			var sym = n->l->sym
			'' BYREF bindings designate caller storage; an output-only use is
			'' still a use of the parameter, unlike an unread BYVAL assignment.
			if( sym <> NULL andalso symbIsParamVarBydescOrByref(sym) ) then
				var entry = hSourceWarningVar(sym)
				if( entry <> NULL ) then entry->read_value = TRUE
			end if
			hSourceWarningsRead(n->r)
			return
		end if
	case AST_NODECLASS_VAR
		var entry = hSourceWarningVar(n->sym)
		if( entry <> NULL ) then entry->read_value = TRUE
	end select
	'' Taking an address or passing BYREF is conservatively an observation.
	hSourceWarningsRead(n->l)
	hSourceWarningsRead(n->r)
end sub

private sub hSourceWarningsEnd(byval body as ASTNODE ptr)
	var frame = source_warning_proc
	assert(frame <> NULL andalso frame->proc = body->sym)
	var report = (errGetCount() = 0)
	if( frame->vars <> NULL and report ) then
		var statement = body->l
		while( statement <> NULL )
			hSourceWarningsRead(statement)
			statement = statement->next
		wend
	end if
	var entry = frame->vars
	while( entry <> NULL )
		var next_var = entry->next_var
		if( report and (frame->opaque = FALSE) and (entry->read_value = FALSE) ) then
			var warning = FB_WARNINGMSG_UNUSEDVARIABLE
			if( symbIsParamVar(entry->sym) ) then warning = FB_WARNINGMSG_UNUSEDPARAMETER
			errReportWarnAt(warning, symbGetName(entry->sym), strptr(entry->filename), entry->linenum)
		end if
		entry->filename = ""
		deallocate(entry)
		entry = next_var
	wend
	if( frame->index.list <> NULL ) then hashEnd(@frame->index)
	source_warning_proc = frame->previous
	deallocate(frame)
end sub

private sub hSourceWarningsClear()
	'' Parser recovery can abandon a body before astProcEnd(). Release only
	'' owned metadata here; the symbol tables have their own teardown order.
	while( source_warning_proc <> NULL )
		var frame = source_warning_proc
		var entry = frame->vars
		while( entry <> NULL )
			var next_var = entry->next_var
			entry->filename = ""
			deallocate(entry)
			entry = next_var
		wend
		if( frame->index.list <> NULL ) then hashEnd(@frame->index)
		source_warning_proc = frame->previous
		deallocate(frame)
	wend
end sub

'' end of ast/nodes/source-unused-warnings.bi
