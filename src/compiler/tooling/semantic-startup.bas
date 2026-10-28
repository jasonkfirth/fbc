'' Project: FreeBASIC compiler - startup and instance lifecycle observations
'' File: tooling/semantic-startup.bas
'' Purpose: Preserve actual startup/list order and generated instance relationships.
'' Responsibilities: Copied identities, guard semantics, phases and bounded closure.
'' This file intentionally does NOT contain: registration, sorting or destructor generation.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-startup.bi"

'' The compiler processes modules serially. Counters identify observations
'' within that module; all native pointers are consumed in the immediate call.
private const STARTUP_LIMIT = 1000000
private const STARTUP_TEXT_BYTES = 65536
private dim shared as longint registration_count, ctor_count, dtor_count, priority_count
private dim shared as longint global_count, static_count, guard_count, wrapper_count, action_count, static_body_count, emission_count
private dim shared as longint declaration_count

sub fbSemanticModelResetStartup( )
	registration_count = 0
	ctor_count = 0
	dtor_count = 0
	priority_count = 0
	global_count = 0
	static_count = 0
	guard_count = 0
	wrapper_count = 0
	action_count = 0
	static_body_count = 0
	emission_count = 0
	declaration_count = 0
end sub

private function hNext( byref count as longint ) as longint
	if( count >= STARTUP_LIMIT ) then
		fbSemanticModelFailAt("startup observation exceeds its bound")
		return 0
	end if
	count += 1
	return count
end function

private function hSymbol( byval sym as FBSYMBOL ptr, byval kind as FB_SYMBCLASS, byval nullable as integer = FALSE ) as longint
	if( (sym = NULL) and nullable ) then return 0
	if( (sym = NULL) orElse (sym->class <> kind) ) then
		fbSemanticModelFailAt("startup observation has another native symbol class")
		return 0
	end if
	return fbSemanticModelSymbolId(sym)
end function

private sub hProperty( byref key as const string, byref value as const string )
	fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(@symbGetGlobalNamespc( ))) + TABCHAR + _
		key + TABCHAR + fbSemanticModelEscape(value))
end sub

private function hKind( byref kind as const string ) as integer
	if( (kind = "constructor") or (kind = "destructor") ) then return TRUE
	fbSemanticModelFailAt("startup observation has another registration kind")
	return FALSE
end function

'' -------------------------------------------------------------------------
'' Append-time registrations and accepted source priorities
'' -------------------------------------------------------------------------

sub fbSemanticModelStartupRegistration( byval proc as FBSYMBOL ptr, byref kind as const string )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( hKind(kind) = FALSE ) then exit sub
	dim as longint proc_id = hSymbol(proc, FB_SYMBCLASS_PROC)
	if( proc_id = 0 ) then exit sub
	dim as longint ordinal
	if( kind = "constructor" ) then
		ordinal = hNext(ctor_count)
	else
		ordinal = hNext(dtor_count)
	end if
	dim as longint identity = hNext(registration_count)
	if( (identity = 0) or (ordinal = 0) ) then exit sub
	'' Source priorities are assigned after list append in parser-proc.bas.
	'' Do not misrepresent that earlier zero as the final requested priority.
	hProperty("startup-registration:" + fbSemanticModelNumber(identity), fbSemanticModelNumber(proc_id) + _
		TABCHAR + kind + TABCHAR + fbSemanticModelNumber(ordinal))
end sub

sub fbSemanticModelStartupPriority( byval proc as FBSYMBOL ptr, byval priority as integer )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( (priority < 0) or (priority > 65535) ) then
		fbSemanticModelFailAt("startup priority is outside the native range")
		exit sub
	end if
	dim as longint proc_id = hSymbol(proc, FB_SYMBCLASS_PROC)
	if( proc_id = 0 ) then exit sub
	dim as longint identity = hNext(priority_count)
	if( identity = 0 ) then exit sub
	hProperty("startup-priority-input:" + fbSemanticModelNumber(identity), _
		fbSemanticModelNumber(proc_id) + TABCHAR + fbSemanticModelNumber(priority))
end sub

'' -------------------------------------------------------------------------
'' Global instances, local static guards and generated wrapper bodies
'' -------------------------------------------------------------------------

sub fbSemanticModelDeclarationOrder( byval sym as longint, byval owner as longint, byval groupid as longint, _
	byval ordinal as integer, byval statement as longint, byval sourceid as longint )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( (sym <= 0) or (owner <= 0) or (groupid <= 0) or (ordinal <= 0) or (statement <= 0) or (sourceid <= 0) ) then
		fbSemanticModelFailAt("declaration order has an unavailable native input")
		exit sub
	end if
	dim as longint identity = hNext(declaration_count)
	if( identity = 0 ) then exit sub
	hProperty("accepted-declarator-order:" + fbSemanticModelNumber(identity), fbSemanticModelNumber(sym) + _
		TABCHAR + fbSemanticModelNumber(owner) + TABCHAR + fbSemanticModelNumber(groupid) + _
		TABCHAR + fbSemanticModelNumber(ordinal) + TABCHAR + fbSemanticModelNumber(statement) + TABCHAR + fbSemanticModelNumber(sourceid))
end sub

function fbSemanticModelGlobalInstance( byval sym as FBSYMBOL ptr, byval initialized as integer, _
	byval cleanup as integer ) as longint
	if( fbSemanticModelFullEnabled( ) = FALSE ) then return 0
	dim as longint symbol_id = hSymbol(sym, FB_SYMBCLASS_VAR)
	if( symbol_id = 0 ) then return 0
	dim as longint identity = hNext(global_count)
	if( identity = 0 ) then return 0
	hProperty("global-instance:" + fbSemanticModelNumber(identity), fbSemanticModelNumber(symbol_id) + _
		TABCHAR + fbSemanticModelNumber(abs(initialized <> FALSE)) + TABCHAR + fbSemanticModelNumber(abs(cleanup <> FALSE)))
	return identity
end function

function fbSemanticModelStaticInstance( byval sym as FBSYMBOL ptr, byval parent as FBSYMBOL ptr, _
	byval wrapper as FBSYMBOL ptr, byval initialized as integer, byval cleanup as integer, byval registered as integer, _
	byref purpose as const string ) as longint
	if( fbSemanticModelFullEnabled( ) = FALSE ) then return 0
	if( (purpose <> "initialization") and (purpose <> "array-allocation") ) then
		fbSemanticModelFailAt("static instance observation has another native path")
		return 0
	end if
	dim as longint symbol_id = hSymbol(sym, FB_SYMBCLASS_VAR), parent_id = hSymbol(parent, FB_SYMBCLASS_PROC)
	dim as longint wrapper_id = hSymbol(wrapper, FB_SYMBCLASS_PROC, TRUE)
	if( (symbol_id = 0) or (parent_id = 0) ) then return 0
	dim as longint identity = hNext(static_count)
	if( identity = 0 ) then return 0
	hProperty("static-instance:" + fbSemanticModelNumber(identity), fbSemanticModelNumber(symbol_id) + _
		TABCHAR + fbSemanticModelNumber(parent_id) + TABCHAR + fbSemanticModelNumber(wrapper_id) + _
		TABCHAR + fbSemanticModelNumber(abs(initialized <> FALSE)) + TABCHAR + fbSemanticModelNumber(abs(cleanup <> FALSE)) + _
		TABCHAR + fbSemanticModelNumber(abs(registered <> FALSE)) + TABCHAR + purpose)
	return identity
end function

sub fbSemanticModelStaticGuard( byval identity as longint, byval flag as FBSYMBOL ptr, byval exit_label as FBSYMBOL ptr )
	if( (identity = 0) or (fbSemanticModelFullEnabled( ) = FALSE) ) then exit sub
	if( (identity < 0) or (identity > static_count) ) then
		fbSemanticModelFailAt("static guard has no observed native path")
		exit sub
	end if
	dim as longint flag_id = hSymbol(flag, FB_SYMBCLASS_VAR), label_id = hSymbol(exit_label, FB_SYMBCLASS_LABEL)
	if( (flag_id = 0) or (label_id = 0) ) then exit sub
	dim as longint ordinal = hNext(guard_count)
	if( ordinal = 0 ) then exit sub
	'' The actual guard branches around the path when nonzero, writes one
	'' before initialization, and flushes temporary cleanup inside that path.
	hProperty("static-instance-guard:" + fbSemanticModelNumber(ordinal), fbSemanticModelNumber(identity) + _
		TABCHAR + fbSemanticModelNumber(flag_id) + TABCHAR + fbSemanticModelNumber(label_id) + TABCHAR + "zero" + TABCHAR + "before-actions")
end sub

sub fbSemanticModelStartupWrapper( byval proc as FBSYMBOL ptr, byref kind as const string )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( hKind(kind) = FALSE ) then exit sub
	dim as longint proc_id = hSymbol(proc, FB_SYMBCLASS_PROC)
	if( proc_id = 0 ) then exit sub
	dim as longint identity = hNext(wrapper_count)
	if( identity = 0 ) then exit sub
	hProperty("startup-instance-wrapper:" + fbSemanticModelNumber(identity), fbSemanticModelNumber(proc_id) + TABCHAR + kind)
end sub

sub fbSemanticModelGlobalInstanceAction( byval identity as longint, byval wrapper as FBSYMBOL ptr, _
	byref kind as const string, byval ordinal as integer )
	if( (identity = 0) or (fbSemanticModelFullEnabled( ) = FALSE) ) then exit sub
	if( (identity < 0) or (identity > global_count) or (ordinal <= 0) or (ordinal > STARTUP_LIMIT) or _
		((kind <> "initialize") and (kind <> "cleanup")) ) then
		fbSemanticModelFailAt("global instance action has another identity or kind")
		exit sub
	end if
	dim as longint wrapper_id = hSymbol(wrapper, FB_SYMBCLASS_PROC)
	if( wrapper_id = 0 ) then exit sub
	dim as longint action = hNext(action_count)
	if( action = 0 ) then exit sub
	hProperty("global-instance-action:" + fbSemanticModelNumber(action), fbSemanticModelNumber(identity) + TABCHAR + _
		fbSemanticModelNumber(wrapper_id) + TABCHAR + kind + TABCHAR + fbSemanticModelNumber(ordinal))
end sub

sub fbSemanticModelStaticCleanupBody( byval sym as FBSYMBOL ptr, byval parent as FBSYMBOL ptr, byval wrapper as FBSYMBOL ptr )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	dim as longint symbol_id = hSymbol(sym, FB_SYMBCLASS_VAR), parent_id = hSymbol(parent, FB_SYMBCLASS_PROC)
	dim as longint wrapper_id = hSymbol(wrapper, FB_SYMBCLASS_PROC)
	if( (symbol_id = 0) or (parent_id = 0) or (wrapper_id = 0) ) then exit sub
	dim as longint identity = hNext(static_body_count)
	if( identity = 0 ) then exit sub
	hProperty("static-cleanup-body:" + fbSemanticModelNumber(identity), _
		fbSemanticModelNumber(symbol_id) + TABCHAR + fbSemanticModelNumber(parent_id) + TABCHAR + fbSemanticModelNumber(wrapper_id))
end sub

'' -------------------------------------------------------------------------
'' Selected backend registration spellings and module closure
'' -------------------------------------------------------------------------

sub fbSemanticModelStartupEmission( byval proc as FBSYMBOL ptr, byref kind as const string, _
	byref stage as const string, byref role as const string, byref selected_priority as const string, _
	byref placement as const string, byref spelling as const string )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( hKind(kind) = FALSE ) then exit sub
	dim as integer valid = TRUE
	select case stage
	case "c"
		valid = (role = "definition-attribute") or (role = "declaration-attribute") or (role = "array-entry")
	case "llvm"
		valid = (role = "array-entry")
	case "gas"
		valid = (role = "section-request")
	case "gas64"
		valid = (role = "section-entry")
	case else
		valid = FALSE
	end select
	if( valid = FALSE ) then
		fbSemanticModelFailAt("startup emission has another backend stage or role")
		exit sub
	end if
	dim as longint proc_id = hSymbol(proc, FB_SYMBCLASS_PROC)
	if( proc_id = 0 ) then exit sub
	if( (len(spelling) = 0) or (len(placement) = 0) or (len(spelling) > STARTUP_TEXT_BYTES) or (len(placement) > STARTUP_TEXT_BYTES) or _
		(len(selected_priority) > 20) or (proc->proc.ext = NULL) ) then
		fbSemanticModelFailAt("startup emission is unavailable or exceeds its bound")
		exit sub
	end if
	dim as longint identity = hNext(emission_count)
	if( identity = 0 ) then exit sub
	hProperty("startup-emission:" + fbSemanticModelNumber(identity), fbSemanticModelNumber(proc_id) + _
		TABCHAR + kind + TABCHAR + stage + TABCHAR + role + TABCHAR + fbSemanticModelNumber(symbGetProcPriority(proc)) + _
		TABCHAR + selected_priority + TABCHAR + placement + TABCHAR + spelling)
end sub

sub fbSemanticModelFinishStartup( byval commit as integer )
	if( commit and fbSemanticModelFullEnabled( ) ) then
		hProperty("startup-lifecycle-counts:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )), _
			fbSemanticModelNumber(registration_count) + TABCHAR + fbSemanticModelNumber(priority_count) + TABCHAR + _
			fbSemanticModelNumber(global_count) + TABCHAR + fbSemanticModelNumber(static_count) + TABCHAR + _
			fbSemanticModelNumber(guard_count) + TABCHAR + fbSemanticModelNumber(wrapper_count) + TABCHAR + _
			fbSemanticModelNumber(action_count) + TABCHAR + fbSemanticModelNumber(static_body_count) + TABCHAR + fbSemanticModelNumber(emission_count))
		hProperty("accepted-declarator-count:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )), fbSemanticModelNumber(declaration_count))
	end if
	fbSemanticModelResetStartup( )
end sub

'' end of tooling/semantic-startup.bas
