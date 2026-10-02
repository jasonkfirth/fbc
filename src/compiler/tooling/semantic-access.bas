'' Project: FreeBASIC compiler - semantic source accesses
'' File: tooling/semantic-access.bas
'' Purpose: Preserve parser-selected access roles independently of AST emission.
'' Responsibilities: Own binding-indexed roles and export surviving node links.
'' This file intentionally does NOT contain: invented writes by BYREF callees.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-access.bi"
#include once "crt/mem.bi"

'' Binding identities are contiguous across one invocation. Roles are staged
'' with their module; AST copies retain the binding identity without owning
'' this checked array. Classification never changes compiler symbol flags.
const SEMANTIC_ACCESS_MAX_BINDINGS = 1000000
dim shared as ubyte ptr access_roles
dim shared as integer access_capacity
dim shared as longint access_base, access_last

sub fbSemanticModelResetAccess( )
	deallocate(access_roles)
	access_roles = NULL
	access_capacity = 0
	access_base = fbSemanticModelBindingCount( )
	access_last = access_base
end sub

sub fbSemanticModelCaptureAccess(byval binding as longint)
	if( binding <= access_base ) then exit sub
	dim as longint index = binding - access_base - 1
	if( index >= SEMANTIC_ACCESS_MAX_BINDINGS ) then
		fbSemanticModelFail( )
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
			fbSemanticModelFail( )
			exit sub
		end if
		memset(storage + access_capacity, 0, capacity - access_capacity)
		access_roles = storage
		access_capacity = capacity
	end if
	access_roles[index] = 1
	if( binding > access_last ) then access_last = binding
end sub

sub fbSemanticModelSetAccess(byval node as ASTNODE ptr, byref role as const string)
	if( (fbSemanticModelEnabled( ) = FALSE) or (node = NULL) ) then exit sub
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
	fbSemanticModelAppendDetail("H" + TABCHAR + "node" + TABCHAR + fbSemanticModelNumber(identity) + _
		TABCHAR + "binding" + TABCHAR + fbSemanticModelNumber(node->semantic_binding) + TABCHAR + "source-binding" + TABCHAR + "0")
end sub

'' end of tooling/semantic-access.bas
