'' Project: FreeBASIC compiler - observed IR dispatch
'' File: backend/ir-observe.bas
'' Purpose: Preserve ordinary backend dispatch while observing semantic transfers.
'' Responsibilities: Delegate each callback once and preserve absent callbacks.
'' This file intentionally does NOT contain: CFG construction or backend policy.

#include once "core/fb.bi"
#include once "core/fbint.bi"
#include once "backend/ir.bi"
#include once "tooling/semantic-ir-flow.bi"
#include once "tooling/semantic-hooks.bi"

'' irInit runs before the module transaction opens. Requested sidecars install
'' decorators here; the observer callbacks separately require an active full
'' module. Ordinary compilation retains the original vtable without wrappers.
'' Nested dispatch from a backend implementation remains inside that atomic
'' IR operation, rather than appearing as another source-level operation.
private dim shared as IR_VTBL observed_backend
private dim shared as integer observer_installed, dispatch_depth

private sub hObservedEmitProcBegin( byval proc as FBSYMBOL ptr, byval initlabel as FBSYMBOL ptr )
	if( dispatch_depth = 0 ) then
		fbSemanticModelIRFlowBegin(proc, initlabel)
	end if
	dispatch_depth += 1
	observed_backend.emitProcBegin(proc, initlabel)
	dispatch_depth -= 1
end sub

private sub hObservedEmitProcEnd( byval proc as FBSYMBOL ptr, byval initlabel as FBSYMBOL ptr, byval exitlabel as FBSYMBOL ptr )
	dispatch_depth += 1
	observed_backend.emitProcEnd(proc, initlabel, exitlabel)
	dispatch_depth -= 1
	if( dispatch_depth = 0 ) then fbSemanticModelIRFlowEnd(proc, exitlabel)
end sub

private sub hObservedEmitLabel( byval label as FBSYMBOL ptr )
	if( dispatch_depth = 0 ) then
		fbSemanticModelIRFlowAction("label", 0, label, NULL, "normal")
	end if
	dispatch_depth += 1
	observed_backend.emitLabel(label)
	dispatch_depth -= 1
end sub

private sub hObservedEmitLabelNF( byval l as FBSYMBOL ptr )
	if( dispatch_depth = 0 ) then
		fbSemanticModelIRFlowAction("label", 0, l, NULL, "no-flush")
	end if
	dispatch_depth += 1
	observed_backend.emitLabelNF(l)
	dispatch_depth -= 1
end sub

private sub hObservedEmitBop( byval op as integer, byval v1 as IRVREG ptr, byval v2 as IRVREG ptr, byval vr as IRVREG ptr, byval label as FBSYMBOL ptr, byval options as IR_EMITOPT )
	if( dispatch_depth = 0 ) then
		if( label <> NULL ) then
			fbSemanticModelIRFlowAction("conditional", op, label)
		else
			fbSemanticModelIRFlowAction("operation", op, NULL, NULL, "binary")
		end if
	end if
	dispatch_depth += 1
	observed_backend.emitBop(op, v1, v2, vr, label, options)
	dispatch_depth -= 1
end sub

private sub hObservedEmitUop( byval op as integer, byval v1 as IRVREG ptr, byval vr as IRVREG ptr )
	if( dispatch_depth = 0 ) then
		fbSemanticModelIRFlowAction("operation", op, NULL, NULL, "unary")
	end if
	dispatch_depth += 1
	observed_backend.emitUop(op, v1, vr)
	dispatch_depth -= 1
end sub

private sub hObservedEmitConvert( byval v1 as IRVREG ptr, byval v2 as IRVREG ptr )
	if( dispatch_depth = 0 ) then
		fbSemanticModelIRFlowAction("operation", 0, NULL, NULL, "convert")
	end if
	dispatch_depth += 1
	observed_backend.emitConvert(v1, v2)
	dispatch_depth -= 1
end sub

private sub hObservedEmitStore( byval v1 as IRVREG ptr, byval v2 as IRVREG ptr )
	if( dispatch_depth = 0 ) then
		fbSemanticModelIRFlowAction("operation", 0, NULL, NULL, "store")
	end if
	dispatch_depth += 1
	observed_backend.emitStore(v1, v2)
	dispatch_depth -= 1
end sub

private sub hObservedEmitLoad( byval v1 as IRVREG ptr )
	if( dispatch_depth = 0 ) then
		fbSemanticModelIRFlowAction("operation", 0, NULL, NULL, "load")
	end if
	dispatch_depth += 1
	observed_backend.emitLoad(v1)
	dispatch_depth -= 1
end sub

private sub hObservedEmitLoadRes( byval v1 as IRVREG ptr, byval vr as IRVREG ptr )
	if( dispatch_depth = 0 ) then
		select case env.clopt.backend
		case FB_BACKEND_GCC, FB_BACKEND_CLANG, FB_BACKEND_LLVM
			fbSemanticModelIRFlowAction("return-value")
		case else
			fbSemanticModelIRFlowAction("operation", 0, NULL, NULL, "result-carrier")
		end select
	end if
	dispatch_depth += 1
	observed_backend.emitLoadRes(v1, vr)
	dispatch_depth -= 1
end sub

private sub hObservedEmitReturn( byval bytestopop as integer )
	if( dispatch_depth = 0 ) then
		fbSemanticModelIRFlowAction("subroutine-return", 0, NULL, NULL, "unknown-return-address", bytestopop)
	end if
	dispatch_depth += 1
	observed_backend.emitReturn(bytestopop)
	dispatch_depth -= 1
end sub

private sub hObservedEmitCall( byval proc as FBSYMBOL ptr, byval bytestopop as integer, byval vr as IRVREG ptr, byval level as integer )
	if( dispatch_depth = 0 ) then
		fbSemanticModelIRFlowAction("call", 0, NULL, proc, "direct", bytestopop)
	end if
	dispatch_depth += 1
	observed_backend.emitCall(proc, bytestopop, vr, level)
	dispatch_depth -= 1
end sub

private sub hObservedEmitCallPtr( byval proc as FBSYMBOL ptr, byval v1 as IRVREG ptr, byval vr as IRVREG ptr, byval bytestopop as integer, byval level as integer )
	if( dispatch_depth = 0 ) then
		fbSemanticModelIRFlowAction("call", 1, NULL, proc, "indirect", bytestopop)
	end if
	dispatch_depth += 1
	observed_backend.emitCallPtr(proc, v1, vr, bytestopop, level)
	dispatch_depth -= 1
end sub

private sub hObservedEmitJumpPtr( byval v1 as IRVREG ptr )
	if( dispatch_depth = 0 ) then
		fbSemanticModelIRFlowAction("indirect")
	end if
	dispatch_depth += 1
	observed_backend.emitJumpPtr(v1)
	dispatch_depth -= 1
end sub

private sub hObservedEmitBranch( byval op as integer, byval label as FBSYMBOL ptr )
	if( dispatch_depth = 0 ) then
		select case op
		case AST_OP_JMP: fbSemanticModelIRFlowAction("jump", op, label)
		case AST_OP_CALL: fbSemanticModelIRFlowAction("gosub", op, label)
		case else: fbSemanticModelIRFlowAction("conditional", op, label)
		end select
	end if
	dispatch_depth += 1
	observed_backend.emitBranch(op, label)
	dispatch_depth -= 1
end sub

private sub hObservedEmitJmpTb( byval v1 as IRVREG ptr, byval tbsym as FBSYMBOL ptr, byval values as ulongint ptr, byval labels as FBSYMBOL ptr ptr, byval labelcount as integer, byval deflabel as FBSYMBOL ptr, byval bias as ulongint, byval span as ulongint )
	if( dispatch_depth = 0 ) then
		fbSemanticModelIRFlowTable(values, labels, labelcount, deflabel, bias, span)
	end if
	dispatch_depth += 1
	observed_backend.emitJmpTb(v1, tbsym, values, labels, labelcount, deflabel, bias, span)
	dispatch_depth -= 1
end sub

private sub hObservedEmitAsmLine( byval asmtokenhead as ASTASMTOK ptr )
	if( dispatch_depth = 0 ) then
		fbSemanticModelIRFlowAction("assembly", 0, NULL, NULL, "unknown-memory-registers-control")
	end if
	dispatch_depth += 1
	observed_backend.emitAsmLine(asmtokenhead)
	dispatch_depth -= 1
end sub

private sub hObservedEmitMem( byval op as integer, byval v1 as IRVREG ptr, byval v2 as IRVREG ptr, byval bytes as longint, byval fillchar as integer )
	if( dispatch_depth = 0 ) then
		fbSemanticModelIRFlowAction("operation", op, NULL, NULL, "memory")
	end if
	dispatch_depth += 1
	observed_backend.emitMem(op, v1, v2, bytes, fillchar)
	dispatch_depth -= 1
end sub

private sub hObservedEmitStack( byval op as integer, byval v1 as IRVREG ptr, byval v2 as IRVREG ptr )
	if( dispatch_depth = 0 ) then
		fbSemanticModelIRFlowAction("operation", op, NULL, NULL, "stack")
	end if
	dispatch_depth += 1
	observed_backend.emitStack(op, v1, v2)
	dispatch_depth -= 1
end sub

private sub hObservedEmitAddr( byval op as integer, byval v1 as IRVREG ptr, byval vr as IRVREG ptr )
	if( dispatch_depth = 0 ) then
		fbSemanticModelIRFlowAction("operation", op, NULL, NULL, "address")
	end if
	dispatch_depth += 1
	observed_backend.emitAddr(op, v1, vr)
	dispatch_depth -= 1
end sub

private function hObservedAllocVrVar( byval dtype as integer, byval subtype as FBSYMBOL ptr, byval symbol as FBSYMBOL ptr, byval ofs as longint ) as IRVREG ptr
	if( dispatch_depth = 0 ) then
		fbSemanticModelIRFlowAddress(symbol)
	end if
	dispatch_depth += 1
	dim as IRVREG ptr result = observed_backend.allocVrVar(dtype, subtype, symbol, ofs)
	dispatch_depth -= 1
	return result
end function

private function hObservedAllocVrOfs( byval dtype as integer, byval subtype as FBSYMBOL ptr, byval symbol as FBSYMBOL ptr, byval ofs as longint ) as IRVREG ptr
	if( dispatch_depth = 0 ) then
		fbSemanticModelIRFlowAddress(symbol)
	end if
	dispatch_depth += 1
	dim as IRVREG ptr result = observed_backend.allocVrOfs(dtype, subtype, symbol, ofs)
	dispatch_depth -= 1
	return result
end function

private function hObservedAllocVrIdx( byval dtype as integer, byval subtype as FBSYMBOL ptr, byval symbol as FBSYMBOL ptr, byval ofs as longint, byval mult as integer, byval vidx as IRVREG ptr ) as IRVREG ptr
	if( dispatch_depth = 0 ) then
		fbSemanticModelIRFlowAddress(symbol)
	end if
	dispatch_depth += 1
	dim as IRVREG ptr result = observed_backend.allocVrIdx(dtype, subtype, symbol, ofs, mult, vidx)
	dispatch_depth -= 1
	return result
end function

sub irObserveInit( )
	observer_installed = FALSE
	dispatch_depth = 0
	if( fbSemanticModelEnabled( ) = FALSE ) then exit sub
	observed_backend = ir.vtbl
	if( observed_backend.emitProcBegin <> NULL ) then ir.vtbl.emitProcBegin = @hObservedEmitProcBegin
	if( observed_backend.emitProcEnd <> NULL ) then ir.vtbl.emitProcEnd = @hObservedEmitProcEnd
	if( observed_backend.emitLabel <> NULL ) then ir.vtbl.emitLabel = @hObservedEmitLabel
	if( observed_backend.emitLabelNF <> NULL ) then ir.vtbl.emitLabelNF = @hObservedEmitLabelNF
	if( observed_backend.emitBop <> NULL ) then ir.vtbl.emitBop = @hObservedEmitBop
	if( observed_backend.emitUop <> NULL ) then ir.vtbl.emitUop = @hObservedEmitUop
	if( observed_backend.emitConvert <> NULL ) then ir.vtbl.emitConvert = @hObservedEmitConvert
	if( observed_backend.emitStore <> NULL ) then ir.vtbl.emitStore = @hObservedEmitStore
	if( observed_backend.emitLoad <> NULL ) then ir.vtbl.emitLoad = @hObservedEmitLoad
	if( observed_backend.emitLoadRes <> NULL ) then ir.vtbl.emitLoadRes = @hObservedEmitLoadRes
	if( observed_backend.emitReturn <> NULL ) then ir.vtbl.emitReturn = @hObservedEmitReturn
	if( observed_backend.emitCall <> NULL ) then ir.vtbl.emitCall = @hObservedEmitCall
	if( observed_backend.emitCallPtr <> NULL ) then ir.vtbl.emitCallPtr = @hObservedEmitCallPtr
	if( observed_backend.emitJumpPtr <> NULL ) then ir.vtbl.emitJumpPtr = @hObservedEmitJumpPtr
	if( observed_backend.emitBranch <> NULL ) then ir.vtbl.emitBranch = @hObservedEmitBranch
	if( observed_backend.emitJmpTb <> NULL ) then ir.vtbl.emitJmpTb = @hObservedEmitJmpTb
	if( observed_backend.emitAsmLine <> NULL ) then ir.vtbl.emitAsmLine = @hObservedEmitAsmLine
	if( observed_backend.emitMem <> NULL ) then ir.vtbl.emitMem = @hObservedEmitMem
	if( observed_backend.emitStack <> NULL ) then ir.vtbl.emitStack = @hObservedEmitStack
	if( observed_backend.emitAddr <> NULL ) then ir.vtbl.emitAddr = @hObservedEmitAddr
	if( observed_backend.allocVrVar <> NULL ) then ir.vtbl.allocVrVar = @hObservedAllocVrVar
	if( observed_backend.allocVrOfs <> NULL ) then ir.vtbl.allocVrOfs = @hObservedAllocVrOfs
	if( observed_backend.allocVrIdx <> NULL ) then ir.vtbl.allocVrIdx = @hObservedAllocVrIdx
	observer_installed = TRUE
end sub

sub irObserveEnd( )
	if( observer_installed ) then ir.vtbl = observed_backend
	observer_installed = FALSE
	dispatch_depth = 0
end sub

'' end of backend/ir-observe.bas
