'' Project: FreeBASIC semantic sidecar tests
'' File: effect-guards.bas
'' Purpose: Exercise runtime contract selection and production export guards.
'' Responsibilities: Independent cloneability, unknown aliases, indirect calls and reset.
'' This file intentionally does NOT execute the selected runtime procedures.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-effects.bi"
#include once "runtime/rtl.bi"

dim shared as SYMBCTX symb
dim shared as FBSYMBOL selected_symbol
dim shared as integer observing = TRUE, failures, identity_queries
dim shared as string captured

function fbSemanticModelFullEnabled( ) as integer
	return observing
end function
function fbSemanticModelNumber( byval value as longint ) as string
	return ltrim(str(value))
end function
function fbSemanticModelEscape( byref value as const string ) as string
	return value
end function
sub fbSemanticModelAppendDetail( byref value as string )
	captured += value + chr(10)
end sub
sub fbSemanticModelFailAt( byref reason as const string )
	failures += 1
end sub
function fbSemanticModelModuleIdentity( ) as longint
	return 1
end function
function fbSemanticModelSymbolId( byval sym as FBSYMBOL ptr ) as longint
	identity_queries += 1
	if sym = @symb.globnspc then return 1
	if sym = @selected_symbol then return 2
	end 10
end function

dim as FB_RTL_PROCDEF definition
selected_symbol.class = FB_SYMBCLASS_PROC
definition.name = @FB_RTL_ABS
if rtlSemanticEffects(FB_RTL_DOMAIN_MATH, @definition) <> FB_RTL_EFFECT_COMPUTATION then end 1
if definition.options <> FB_RTL_OPT_NONE then end 2
definition.name = @"rnd"
if (rtlSemanticEffects(FB_RTL_DOMAIN_MATH, @definition) and FB_RTL_EFFECT_RUNTIME_WRITE) = 0 then end 3
definition.name = @"allocate"
if rtlSemanticEffects(FB_RTL_DOMAIN_UNKNOWN, @definition) <> FB_RTL_EFFECT_UNKNOWN then end 4
if rtlSemanticEffects(FB_RTL_DOMAIN_MEMORY, @definition) <> FB_RTL_EFFECT_ALLOCATE then end 5
definition.name = @FB_RTL_NULLPTRCHK
definition.options = FB_RTL_OPT_CANBECLONED
if (rtlSemanticEffects(FB_RTL_DOMAIN_MEMORY, @definition) and FB_RTL_EFFECT_ERROR_HANDLING) = 0 then end 6
if definition.options <> FB_RTL_OPT_CANBECLONED then end 7

'' Unregistered lookalike data must not become a runtime contract.
selected_symbol.proc.rtl.semantic_domain = FB_RTL_DOMAIN_MATH
selected_symbol.proc.rtl.semantic_effects = FB_RTL_EFFECT_COMPUTATION
fbSemanticModelEffectDeclaration(@selected_symbol, 2)
if instr(captured, "unknown-callee" + chr(9) + "unknown" + chr(9) + "1") = 0 then end 8
selected_symbol.stats = FB_SYMBSTATS_RTL
fbSemanticModelEffectDeclaration(@selected_symbol, 2)
if instr(captured, "runtime-registration" + chr(9) + "math" + chr(9) + "8192") = 0 then end 9
fbSemanticModelEffectCall(@selected_symbol, TRUE, 1, 1, 1)
if instr(captured, "indirect-target" + chr(9) + "unknown" + chr(9) + "1") = 0 then end 11
selected_symbol.stats or= FB_SYMBSTATS_FUNCPTR
fbSemanticModelEffectDeclaration(@selected_symbol, 2)
fbSemanticModelEffectOperation("assembly", 0, "unknown-memory-registers-control", 1, 2, 1)
fbSemanticModelFinishEffects(TRUE)
if failures or instr(captured, "unknown-memory-registers-control" + chr(9) + "unknown-aliases") = 0 then end 12
if instr(captured, "effect-counts:1" + chr(9) + "3" + chr(9) + "1" + chr(9) + "1") = 0 then end 13

captured = ""
identity_queries = 0
fbSemanticModelEffectDeclaration(NULL, 0)
fbSemanticModelEffectCall(NULL, FALSE, 0, 0, 0)
fbSemanticModelEffectOperation("other", 0, "", 0, 0, 0)
if failures <> 3 or identity_queries or len(captured) then end 14
observing = FALSE
fbSemanticModelEffectDeclaration(NULL, 0)
fbSemanticModelEffectCall(NULL, FALSE, 0, 0, 0)
fbSemanticModelEffectOperation("other", 0, "", 0, 0, 0)
fbSemanticModelFinishEffects(TRUE)
if failures <> 3 or identity_queries or len(captured) then end 15
print "EFFECT_GUARDS_PASS"

'' end of effect-guards.bas
