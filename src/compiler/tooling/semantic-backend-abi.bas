'' Project: FreeBASIC compiler - selected backend ABI signatures
'' File: tooling/semantic-backend-abi.bas
'' Purpose: Preserve actual selected backend signature components without guessing.
'' Responsibilities: Signature identities, exact spellings, component order and closure.
'' This file intentionally does NOT contain: formatting, type lowering or native pointers.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-backend-abi.bi"
#include once "support/containers/hash.bi"

private type SEMANTIC_BACKEND_SIGNATURE
	key as string
	identity as longint
	stage as string
	components as longint
	closed as integer
	next_signature as SEMANTIC_BACKEND_SIGNATURE ptr
end type

'' Signature formatting can recurse through function-pointer types. Explicit
'' identities keep nested signatures independent without retaining native
'' pointers or changing either backend's formatting order. The hash borrows
'' immutable entry keys and must end before the entries are released.
private const BACKEND_ABI_LIMIT = 1000000
private const BACKEND_ABI_BUCKETS = 4096
'' Match the selected-name spelling bound. Oversized facts fail publication;
'' they are never truncated into an apparently complete ABI signature.
private const BACKEND_ABI_TYPE_BYTES = 65536
private dim shared as THASH backend_signatures
private dim shared as integer backend_signatures_ready
private dim shared as SEMANTIC_BACKEND_SIGNATURE ptr backend_signature_head, backend_signature_tail
private dim shared as longint backend_signature_count, backend_component_count

sub fbSemanticModelResetBackendAbi( )
	if( backend_signatures_ready ) then hashEnd(@backend_signatures)
	backend_signatures_ready = FALSE
	dim as SEMANTIC_BACKEND_SIGNATURE ptr item = backend_signature_head
	while( item <> NULL )
		dim as SEMANTIC_BACKEND_SIGNATURE ptr following = item->next_signature
		delete item
		item = following
	wend
	backend_signature_head = NULL
	backend_signature_tail = NULL
	backend_signature_count = 0
	backend_component_count = 0
end sub

private sub hProperty( byref key as const string, byref value as const string )
	fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(@symbGetGlobalNamespc( ))) + TABCHAR + _
		key + TABCHAR + fbSemanticModelEscape(value))
end sub

private function hSignature( byval identity as longint ) as SEMANTIC_BACKEND_SIGNATURE ptr
	if( backend_signatures_ready = FALSE ) then return NULL
	dim as string key = fbSemanticModelNumber(identity)
	return hashLookup(@backend_signatures, strptr(key))
end function

function fbSemanticModelBackendAbiBegin( byval proc as FBSYMBOL ptr, byref stage as const string, _
	byref role as const string, byref result_type as const string, byref convention as const string ) as longint
	if( fbSemanticModelFullEnabled( ) = FALSE ) then return 0
	if( (proc = NULL) orElse (proc->class <> FB_SYMBCLASS_PROC) ) then
		fbSemanticModelFailAt("backend ABI signature has another procedure class")
		return 0
	end if
	if( (stage <> "c") and (stage <> "llvm") ) then
		fbSemanticModelFailAt("unknown selected backend ABI stage")
		return 0
	end if
	if( (role <> "declaration") and (role <> "definition") and (role <> "pointer-type") ) then
		fbSemanticModelFailAt("unknown selected backend ABI role")
		return 0
	end if
	if( (backend_signature_count >= BACKEND_ABI_LIMIT) or (len(result_type) = 0) or _
		(len(result_type) > BACKEND_ABI_TYPE_BYTES) or (len(convention) > BACKEND_ABI_TYPE_BYTES) ) then
		fbSemanticModelFailAt("selected backend ABI signature exceeds its bound")
		return 0
	end if
	dim as SEMANTIC_BACKEND_SIGNATURE ptr item = new SEMANTIC_BACKEND_SIGNATURE
	if( item = NULL ) then
		fbSemanticModelFailAt("selected backend ABI signature allocation failed")
		return 0
	end if
	backend_signature_count += 1
	item->identity = backend_signature_count
	item->stage = stage
	item->key = fbSemanticModelNumber(item->identity)
	item->components = 0
	item->closed = FALSE
	item->next_signature = NULL
	if( backend_signature_tail = NULL ) then
		backend_signature_head = item
	else
		backend_signature_tail->next_signature = item
	end if
	backend_signature_tail = item
	if( backend_signatures_ready = FALSE ) then
		hashInit(@backend_signatures, BACKEND_ABI_BUCKETS, FALSE)
		backend_signatures_ready = TRUE
	end if
	if( hashAdd(@backend_signatures, strptr(item->key), item, hashHash(strptr(item->key))) = NULL ) then
		fbSemanticModelFailAt("selected backend ABI index allocation failed")
		return 0
	end if
	hProperty("emitted-abi-signature:" + item->key, fbSemanticModelNumber(fbSemanticModelSymbolId(proc)) + _
		TABCHAR + stage + TABCHAR + role + TABCHAR + result_type + TABCHAR + convention)
	return item->identity
end function

sub fbSemanticModelBackendAbiParameter( byval signature as longint, byval param as FBSYMBOL ptr, _
	byval part as integer, byval parts as integer, byref kind as const string, byref spelling as const string, _
	byval dtype as integer, byval subtype as FBSYMBOL ptr, byref attributes as const string )
	if( (signature = 0) or (fbSemanticModelFullEnabled( ) = FALSE) ) then exit sub
	dim as SEMANTIC_BACKEND_SIGNATURE ptr item = hSignature(signature)
	if( (item = NULL) orElse item->closed ) then
		fbSemanticModelFailAt("selected ABI component has no open signature")
		exit sub
	end if
	if( param <> NULL ) then
		if( param->class <> FB_SYMBCLASS_PARAM ) then
			fbSemanticModelFailAt("selected ABI component has another formal class")
			exit sub
		end if
	end if
	dim as integer valid = TRUE
	select case kind
	case "variadic"
		valid = (param <> NULL) and (part = 0) and (parts = 0) and (spelling = "...") and _
			(dtype = FB_DATATYPE_INVALID) and (len(attributes) = 0)
	case "hidden-result"
		valid = (param = NULL) and (part = 0) and (parts = 1) and (item->components = 0) and (dtype <> FB_DATATYPE_INVALID)
	case "i386-int128-low", "i386-int128-high"
		valid = (item->stage = "llvm") and (param <> NULL) and (parts = 2) and _
			(part = abs(kind = "i386-int128-high")) and (spelling = "i64") and _
			(dtype = FB_DATATYPE_ULONGINT) and (subtype = NULL) and (len(attributes) = 0)
	case "value", "libc-adjusted", "memory-byval", "va-list-decay"
		valid = (param <> NULL) and (part = 0) and (parts = 1) and (dtype <> FB_DATATYPE_INVALID)
		if( kind = "libc-adjusted" ) then valid and= (item->stage = "c")
		if( (kind = "memory-byval") or (kind = "va-list-decay") ) then valid and= (item->stage = "llvm")
		if( kind <> "memory-byval" ) then valid and= (len(attributes) = 0)
	case "argv", "libc-callback"
		valid = (item->stage = "c") and (param <> NULL) and (part = 0) and (parts = 1) and _
			(dtype = FB_DATATYPE_INVALID) and (len(attributes) = 0)
		if( kind = "argv" ) then valid and= (spelling = "char**")
	case else
		valid = FALSE
	end select
	if( valid = FALSE ) then
		fbSemanticModelFailAt("selected backend ABI component contradicts its kind")
		exit sub
	end if
	if( (backend_component_count >= BACKEND_ABI_LIMIT) or (part < 0) or (parts < 0) or _
		(len(spelling) = 0) or (len(spelling) > BACKEND_ABI_TYPE_BYTES) or (len(attributes) > BACKEND_ABI_TYPE_BYTES) ) then
		fbSemanticModelFailAt("selected backend ABI component exceeds its bound")
		exit sub
	end if
	dim as string raw_dtype, raw_subtype
	if( dtype <> FB_DATATYPE_INVALID ) then
		raw_dtype = fbSemanticModelTypeNumber(culng(dtype))
		raw_subtype = fbSemanticModelNumber(fbSemanticModelSymbolId(subtype))
	end if
	backend_component_count += 1
	hProperty("emitted-abi-parameter:" + fbSemanticModelNumber(backend_component_count), item->key + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(param)) + TABCHAR + fbSemanticModelNumber(item->components) + _
		TABCHAR + fbSemanticModelNumber(part) + TABCHAR + fbSemanticModelNumber(parts) + TABCHAR + kind + _
		TABCHAR + spelling + TABCHAR + raw_dtype + TABCHAR + raw_subtype + TABCHAR + attributes)
	item->components += 1
end sub

sub fbSemanticModelBackendAbiEnd( byval signature as longint, byref header as const string )
	if( (signature = 0) or (fbSemanticModelFullEnabled( ) = FALSE) ) then exit sub
	dim as SEMANTIC_BACKEND_SIGNATURE ptr item = hSignature(signature)
	if( (item = NULL) orElse item->closed ) then
		fbSemanticModelFailAt("selected ABI signature closes another observation")
		exit sub
	end if
	if( (len(header) = 0) or (len(header) > BACKEND_ABI_TYPE_BYTES) ) then
		fbSemanticModelFailAt("selected ABI header exceeds its bound")
		exit sub
	end if
	hProperty("emitted-abi-end:" + item->key, fbSemanticModelNumber(item->components) + TABCHAR + header)
	item->closed = TRUE
end sub

sub fbSemanticModelFinishBackendAbi( byval commit as integer )
	if( commit and fbSemanticModelFullEnabled( ) and _
		((env.clopt.backend = FB_BACKEND_GCC) or (env.clopt.backend = FB_BACKEND_CLANG) or _
		(env.clopt.backend = FB_BACKEND_LLVM)) ) then
		dim as SEMANTIC_BACKEND_SIGNATURE ptr item = backend_signature_head
		while( item <> NULL )
			if( item->closed = FALSE ) then fbSemanticModelFailAt("selected backend ABI signature remains open")
			item = item->next_signature
		wend
		hProperty("emitted-abi-counts:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )), _
			fbSemanticModelNumber(backend_signature_count) + TABCHAR + fbSemanticModelNumber(backend_component_count))
	end if
	fbSemanticModelResetBackendAbi( )
end sub

'' end of tooling/semantic-backend-abi.bas
