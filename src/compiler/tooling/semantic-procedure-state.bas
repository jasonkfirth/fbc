'' Project: FreeBASIC compiler - procedure runtime state
'' File: tooling/semantic-procedure-state.bas
'' Purpose: Retain hidden error-handler and GOSUB relationships before teardown.
'' Responsibilities: Actual procedure/storage/target IDs and bounded action receipts.
'' This file intentionally does NOT contain: runtime state changes or flow inference.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-procedure-state.bi"
#include once "tooling/semantic-constructs.bi"
#include once "tooling/semantic-source.bi"

'' The compiler processes procedures serially. Snapshots contain existing
'' symbol IDs only; no native extension, symbol or AST pointer is retained.
private const PROCEDURE_STATE_LIMIT = 1000000
private dim shared as longint state_count

sub fbSemanticModelResetProcedureState( )
	state_count = 0
end sub

private function hSymbol( byval sym as FBSYMBOL ptr, byval kind as FB_SYMBCLASS ) as longint
	if( sym = NULL ) then return 0
	if( sym->class <> kind ) then
		fbSemanticModelFailAt("procedure state has another native symbol class")
		return 0
	end if
	return fbSemanticModelSymbolId(sym)
end function

private sub hProperty( byref key as const string, byref value as const string )
	fbSemanticModelAppendDetail("K" + TABCHAR + "module" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelModuleIdentity( )) + TABCHAR + key + TABCHAR + fbSemanticModelEscape(value))
end sub

sub fbSemanticModelProcedureState( byval proc as FBSYMBOL ptr, byref kind as const string, _
	byref mode as const string, byval temporary as FBSYMBOL ptr, byval target as FBSYMBOL ptr )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( proc = NULL ) then fbSemanticModelFailAt("procedure state has no owner"): exit sub
	if( proc->class <> FB_SYMBCLASS_PROC ) then fbSemanticModelFailAt("procedure state owner is not a procedure"): exit sub
	if( proc->proc.ext = NULL ) then fbSemanticModelFailAt("procedure state has no initialized extension"): exit sub
	select case kind
	case "begin", "handler-allocated", "handler-save", "module-name-save", "function-name-save", _
		"before-error-exit", "after-error-exit", "gosub-allocated", "gosub-direct", "gosub-computed", "gosub-return", "gosub-exit"
	case else
		fbSemanticModelFailAt("unknown procedure state action")
		exit sub
	end select
	if( mode <> "none" and mode <> "call-ret" and mode <> "setjmp" ) then
		fbSemanticModelFailAt("unknown procedure state implementation")
		exit sub
	end if
	if( state_count >= PROCEDURE_STATE_LIMIT ) then fbSemanticModelFailAt("procedure state limit"): exit sub
	state_count += 1
	'' hSymbol validates only the relevant common class. Native union data is
	'' read here only after the procedure extension's initialized-state guard.
	dim as string value = fbSemanticModelNumber(hSymbol(proc, FB_SYMBCLASS_PROC)) + TABCHAR + kind + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelCurrentStatement( )) + TABCHAR + fbSemanticModelNumber(fbSemanticModelCurrentContext( )) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelCurrentSource( )) + TABCHAR + _
		fbSemanticModelNumber(hSymbol(proc->proc.ext->err.lasthnd, FB_SYMBCLASS_VAR)) + TABCHAR + _
		fbSemanticModelNumber(hSymbol(proc->proc.ext->err.lastmod, FB_SYMBCLASS_VAR)) + TABCHAR + _
		fbSemanticModelNumber(hSymbol(proc->proc.ext->err.lastfun, FB_SYMBCLASS_VAR)) + TABCHAR + _
		fbSemanticModelNumber(hSymbol(proc->proc.ext->err.hndsaved, FB_SYMBCLASS_VAR)) + TABCHAR + _
		fbSemanticModelNumber(hSymbol(proc->proc.ext->gosub.ctx, FB_SYMBCLASS_VAR)) + TABCHAR + mode + TABCHAR + _
		fbSemanticModelNumber(hSymbol(temporary, FB_SYMBCLASS_VAR)) + TABCHAR + fbSemanticModelNumber(hSymbol(target, FB_SYMBCLASS_LABEL))
	hProperty("native-procedure-state:" + fbSemanticModelNumber(state_count), value)
end sub

sub fbSemanticModelFinishProcedureState( byval commit as integer )
	if( commit and fbSemanticModelFullEnabled( ) ) then
		hProperty("native-procedure-state-count", fbSemanticModelNumber(state_count))
	end if
	fbSemanticModelResetProcedureState( )
end sub

'' end of tooling/semantic-procedure-state.bas
