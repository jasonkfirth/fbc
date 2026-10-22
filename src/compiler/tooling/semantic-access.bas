'' Project: FreeBASIC compiler - semantic source accesses
'' File: tooling/semantic-access.bas
'' Purpose: Preserve parser-selected access roles independently of AST emission.
'' Responsibilities: Own binding-indexed roles and export surviving node links.
'' This file intentionally does NOT contain: invented writes by BYREF callees.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-access.bi"
#include once "crt/mem.bi"
#include once "tooling/semantic-extra-facts.bi"

'' Project: FreeBASIC semantic source access observations
'' File: macro-access-observer.bi
'' Purpose: Preserve access roles for exact generated variable occurrences.
'' Responsibilities: Bounded MR identities, final parser roles and closed counts.
'' This file intentionally does NOT change AST, access flags or alias effects.

const MACRO_ACCESS_LIMIT as longint = 1000000
dim shared as ubyte ptr macro_access_roles
dim shared as longint ptr macro_access_symbols
dim shared as longint macro_access_capacity, macro_access_last, macro_access_count

private sub hMacroAccessReset( )
	deallocate(macro_access_roles)
	deallocate(macro_access_symbols)
	macro_access_roles = NULL
	macro_access_symbols = NULL
	macro_access_capacity = 0
	macro_access_last = 0
	macro_access_count = 0
end sub

'' Like the existing physical binding counter, this tag is sampled immediately
'' around one parser-selected variable/member export, before its operands.
'' A changed tag belongs to that occurrence, never a later nested expression.
function fbSemanticModelMacroAccessBinding( ) as longint
	if( fbSemanticExtraFactsEnabled(FB_SEMEXTRA_MACRO_ACCESS) = FALSE ) then return 0
	return -macro_access_last
end function

sub fbSemanticModelCaptureMacroAccess(byval reference as longint, byval symbolid as longint)
	if( fbSemanticExtraFactsEnabled(FB_SEMEXTRA_MACRO_ACCESS) = FALSE ) then exit sub
	if( (fbSemanticModelFullEnabled( ) = FALSE) or _
		(fbSemanticModelMacroProvenanceEnabled( ) = FALSE) or lex.ctx->semantic_probe ) then exit sub
	if( (reference < 1) or (reference >= MACRO_ACCESS_LIMIT) or (symbolid < 1) ) then
		fbSemanticModelFailAt("invalid macro source access identity")
		exit sub
	end if
	if( reference >= macro_access_capacity ) then
		dim as longint capacity = iif(macro_access_capacity = 0, 128, macro_access_capacity)
		do while( reference >= capacity )
			capacity *= 2
			if( capacity > MACRO_ACCESS_LIMIT ) then capacity = MACRO_ACCESS_LIMIT
		loop
		dim as ubyte ptr roles = reallocate(macro_access_roles, capacity)
		if( roles = NULL ) then fbSemanticModelFailAt("macro access role allocation failed"): exit sub
		macro_access_roles = roles
		dim as longint ptr symbols = reallocate(macro_access_symbols, capacity * sizeof(longint))
		if( symbols = NULL ) then fbSemanticModelFailAt("macro access symbol allocation failed"): exit sub
		macro_access_symbols = symbols
		memset(macro_access_roles + macro_access_capacity, 0, capacity - macro_access_capacity)
		memset(macro_access_symbols + macro_access_capacity, 0, (capacity - macro_access_capacity) * sizeof(longint))
		macro_access_capacity = capacity
	end if
	if( macro_access_roles[reference] <> 0 ) then
		fbSemanticModelFailAt("macro source access was captured twice")
		exit sub
	end if
	macro_access_symbols[reference] = symbolid
	macro_access_roles[reference] = 1
	macro_access_count += 1
	if( reference > macro_access_last ) then macro_access_last = reference
end sub

private sub hMacroAccessSet(byval binding as longint, byref role as const string)
	if( fbSemanticExtraFactsEnabled(FB_SEMEXTRA_MACRO_ACCESS) = FALSE ) then exit sub
	if( binding >= 0 ) then exit sub
	dim as longint reference = -binding
	if( reference >= macro_access_capacity ) then exit sub
	if( macro_access_roles[reference] = 0 ) then exit sub
	select case role
	case "read": macro_access_roles[reference] = 1
	case "write": macro_access_roles[reference] = 2
	case "read-write": macro_access_roles[reference] = 3
	case "address": macro_access_roles[reference] = 4
	case "byref": macro_access_roles[reference] = 5
	case "callee": macro_access_roles[reference] = 6
	end select
end sub

private sub hMacroAccessExport( )
	if( fbSemanticExtraFactsEnabled(FB_SEMEXTRA_MACRO_ACCESS) = FALSE ) then exit sub
	if( (fbSemanticModelFullEnabled( ) = FALSE) or _
		(fbSemanticModelMacroProvenanceEnabled( ) = FALSE) ) then exit sub
	for reference as longint = 1 to macro_access_last
		dim as string role
		select case macro_access_roles[reference]
		case 1: role = "read"
		case 2: role = "write"
		case 3: role = "read-write"
		case 4: role = "address"
		case 5: role = "byref"
		case 6: role = "callee"
		case else: continue for
		end select
		fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + _
			fbSemanticModelNumber(macro_access_symbols[reference]) + TABCHAR + _
			"macro-source-access:" + fbSemanticModelNumber(reference) + TABCHAR + role)
	next
	fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(@symbGetGlobalNamespc( ))) + TABCHAR + _
		"macro-source-access-count:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )) + TABCHAR + _
		fbSemanticModelNumber(macro_access_count))
end sub

'' end of macro-access-observer.bi


'' Binding identities are contiguous across one invocation. Roles are staged
'' with their module; AST copies retain the binding identity without owning
'' this checked array. Classification never changes compiler symbol flags.
const SEMANTIC_ACCESS_MAX_BINDINGS = 1000000
dim shared as ubyte ptr access_roles
dim shared as integer access_capacity
dim shared as longint access_base, access_last
'' Counts surviving CALL-to-MR associations, including cloned call nodes.
'' Reset with the module; MR identities themselves belong to the exporter.
dim shared as longint macro_call_count
'' Direct writes are separate from final ACC roles. Runtime lowering may use
'' BYREF and generated tokens may have no binding, but neither is a negative
'' observation about a parser-selected assignment to a parameter variable.
dim shared as ubyte ptr direct_writes
dim shared as integer direct_capacity

sub fbSemanticModelResetAccess( )
	hMacroAccessReset( )
	macro_call_count = 0
	deallocate(access_roles)
	deallocate(direct_writes)
	direct_writes = NULL
	direct_capacity = 0
	access_roles = NULL
	access_capacity = 0
	access_base = fbSemanticModelBindingCount( )
	access_last = access_base
end sub

sub fbSemanticModelCaptureAccess(byval binding as longint)
	if( binding <= access_base ) then exit sub
	dim as longint index = binding - access_base - 1
	if( index >= SEMANTIC_ACCESS_MAX_BINDINGS ) then
		fbSemanticModelFailAt("semantic-access.bas:39")
		exit sub
	end if
	if( index >= access_capacity ) then
		dim as integer capacity = iif(access_capacity = 0, 128, access_capacity)
		do while( index >= capacity )
			capacity *= 2
			if( capacity > SEMANTIC_ACCESS_MAX_BINDINGS ) then capacity = SEMANTIC_ACCESS_MAX_BINDINGS
		loop
		dim as ubyte ptr storage = reallocate(access_roles, capacity)
		if( storage = NULL ) then
			fbSemanticModelFailAt("semantic-access.bas:50")
			exit sub
		end if
		memset(storage + access_capacity, 0, capacity - access_capacity)
		access_roles = storage
		access_capacity = capacity
	end if
	access_roles[index] = 1
	if( binding > access_last ) then access_last = binding
end sub

private sub hDirectSourceWrite(byval node as ASTNODE ptr, byref role as const string)
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( (role <> "write") and (role <> "read-write") ) then exit sub
	dim as integer depth = 0
	while( node <> NULL )
		select case node->class
		case AST_NODECLASS_CONV, AST_NODECLASS_ADDROF, AST_NODECLASS_NIDXARRAY
			node = node->l
		case AST_NODECLASS_DEREF
			'' String/BYREF parameter variables use an implicit dereference.
			'' It is still the written variable when the inner VAR's declared
			'' type equals this lvalue type. Explicit *pointer writes have a
			'' different declared pointer type, so must stop here instead.
			if( node->l = NULL ) then exit sub
			if( node->l->class <> AST_NODECLASS_VAR ) then exit sub
			if( node->l->sym = NULL ) then exit sub
			if( node->l->sym->typ <> node->dtype ) then exit sub
			if( (node->l->sym->attrib and (FB_SYMBATTRIB_PARAMVARBYVAL or _
				FB_SYMBATTRIB_PARAMVARBYREF)) = 0 ) then exit sub
			node = node->l
		case AST_NODECLASS_VAR
			if( node->sym = NULL ) then exit sub
			dim as longint identity = fbSemanticModelSymbolId(node->sym)
			if( identity > 0 ) then
				if( identity >= SEMANTIC_ACCESS_MAX_BINDINGS ) then fbSemanticModelFailAt("semantic-access.bas:85"): exit sub
				if( identity >= direct_capacity ) then
					dim as integer capacity = iif(direct_capacity = 0, 128, direct_capacity)
					do while( identity >= capacity )
						capacity *= 2
						if( capacity > SEMANTIC_ACCESS_MAX_BINDINGS ) then capacity = SEMANTIC_ACCESS_MAX_BINDINGS
					loop
					dim as ubyte ptr storage = reallocate(direct_writes, capacity)
					if( storage = NULL ) then fbSemanticModelFailAt("semantic-access.bas:93"): exit sub
					memset(storage + direct_capacity, 0, capacity - direct_capacity)
					direct_writes = storage
					direct_capacity = capacity
				end if
				direct_writes[identity] = 1
			end if
			exit sub
		case else
			'' A field, index or dereference writes other storage, not the
			'' containing variable or its pointer value.
			exit sub
		end select
		depth += 1
		if( depth > 64 ) then exit sub
	wend
end sub

function fbSemanticModelDirectSourceWrite(byval identity as longint) as integer
	if( (identity < 1) or (identity >= direct_capacity) ) then return 0
	return direct_writes[identity]
end function

sub fbSemanticModelSetAccess(byval node as ASTNODE ptr, byref role as const string)
	if( (fbSemanticModelEnabled( ) = FALSE) or (node = NULL) ) then exit sub
	if( node->semantic_binding < 0 ) then
		hMacroAccessSet(node->semantic_binding, role)
		exit sub
	end if
	hDirectSourceWrite(node, role)
	'' Transparent address/type wrappers can surround a written lvalue.
	'' Stop at its actual occurrence, so assigning a field does not mark
	'' the containing pointer or the index expression as written too.
	dim as integer depth = 0
	while( node->semantic_binding = 0 )
		select case node->class
		case AST_NODECLASS_CONV, AST_NODECLASS_ADDROF, AST_NODECLASS_NIDXARRAY
			node = node->l
		case else
			exit sub
		end select
		depth += 1
		if( (node = NULL) or (depth > 64) ) then exit sub
	wend
	if( node->semantic_binding < 0 ) then
		hMacroAccessSet(node->semantic_binding, role)
		exit sub
	end if
	dim as longint index = node->semantic_binding - access_base - 1
	if( (index < 0) or (index >= access_capacity) ) then exit sub
	if( access_roles[index] = 0 ) then exit sub
	select case role
	case "read": access_roles[index] = 1
	case "write": access_roles[index] = 2
	case "read-write": access_roles[index] = 3
	case "address": access_roles[index] = 4
	case "byref": access_roles[index] = 5
	case "callee": access_roles[index] = 6
	end select
end sub

sub fbSemanticModelExportAccess( )
	hMacroAccessExport( )
	fbSemanticModelExportMacroCallCount(macro_call_count)
	for binding as longint = access_base + 1 to access_last
		dim as string role
		select case access_roles[binding - access_base - 1]
		case 1: role = "read"
		case 2: role = "write"
		case 3: role = "read-write"
		case 4: role = "address"
		case 5: role = "byref"
		case 6: role = "callee"
		case else: continue for
		end select
		fbSemanticModelAppendProvenance("ACC" + TABCHAR + fbSemanticModelNumber(binding) + TABCHAR + role)
	next
end sub

sub fbSemanticModelExportBindingLink(byval node as ASTNODE ptr, byval identity as longint)
	if( node->semantic_binding = 0 ) then exit sub
	'' Generated variable tags use their independent source-access stream.
	'' They are not selected procedure references, including after AST copies.
	if( node->semantic_binding < 0 ) then
		dim as longint reference = -node->semantic_binding
		if( reference < macro_access_capacity ) then
			if( macro_access_roles[reference] <> 0 ) then exit sub
		end if
	end if
	if( node->semantic_binding < 0 ) then
		'' Negative tags are exact macro references captured after overload
		'' selection. Never export them as B ordinals or editable locations.
		if( (node->class <> AST_NODECLASS_CALL) or _
			(fbSemanticModelFullEnabled( ) = FALSE) or _
			(fbSemanticModelMacroProvenanceEnabled( ) = FALSE) or _
			(macro_call_count >= SEMANTIC_ACCESS_MAX_BINDINGS) ) then
			fbSemanticModelFailAt("invalid macro source-call association")
			exit sub
		end if
		fbSemanticModelAppendDetail("K" + TABCHAR + "node" + TABCHAR + fbSemanticModelNumber(identity) + _
			TABCHAR + "source-call-macro-reference" + TABCHAR + fbSemanticModelNumber(-node->semantic_binding))
		macro_call_count += 1
		exit sub
	end if
	fbSemanticModelAppendDetail("H" + TABCHAR + "node" + TABCHAR + fbSemanticModelNumber(identity) + _
		TABCHAR + "binding" + TABCHAR + fbSemanticModelNumber(node->semantic_binding) + TABCHAR + "source-binding" + TABCHAR + "0")
end sub

'' end of tooling/semantic-access.bas
