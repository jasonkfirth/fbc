'' Project: FreeBASIC compiler semantic observations
'' File: tooling/semantic-iif-compare.bi
'' Purpose: Compare mutually exclusive IIf arm structures before lowering.
'' Responsibilities: Selected types, bindings, node payloads and bounded traversal.
'' This file intentionally does NOT eliminate effects or authorize AST rewrites.
#ifndef __SEMANTIC_IIF_COMPARE_BI__
#define __SEMANTIC_IIF_COMPARE_BI__

'' This predicate differs from astIsTreeEqual(), which serves optimizations and
'' deliberately rejects CALLs. Mutually exclusive arms may contain the same
'' effectful call. Reporting that structure does not allow dropping condition
'' evaluation, ABI conversions, temporary lifetimes or a selected call.
enum FB_SEMANTIC_ARM_COMPARISON
	FB_SEMANTIC_ARMS_UNKNOWN = -1
	FB_SEMANTIC_ARMS_DIFFERENT = 0
	FB_SEMANTIC_ARMS_SAME = 1
end enum

'' A source expression can be deeply nested. Bound optional observation work
'' independently of the parser stack and leave valid compilation unaffected.
const FB_SEMANTIC_ARM_DEPTH_LIMIT = 128
const FB_SEMANTIC_ARM_WORK_LIMIT = 100000

private function hSemanticIifArmCompare _
	( byval lhs as ASTNODE ptr, byval rhs as ASTNODE ptr, _
	  byref work_left as integer, byval depth as integer = 0 ) as integer
	work_left -= 1
	if( (work_left < 0) or (depth > FB_SEMANTIC_ARM_DEPTH_LIMIT) ) then return FB_SEMANTIC_ARMS_UNKNOWN
	if( (lhs = NULL) or (rhs = NULL) ) then
		if( lhs = rhs ) then return FB_SEMANTIC_ARMS_SAME
		return FB_SEMANTIC_ARMS_DIFFERENT
	end if
	if( (lhs->class <> rhs->class) or (astGetFullType(lhs) <> astGetFullType(rhs)) or _
	    (lhs->subtype <> rhs->subtype) ) then return FB_SEMANTIC_ARMS_DIFFERENT

	select case as const lhs->class
	case AST_NODECLASS_VAR
		if( (lhs->sym <> rhs->sym) or (lhs->var_.ofs <> rhs->var_.ofs) ) then return FB_SEMANTIC_ARMS_DIFFERENT
	case AST_NODECLASS_FIELD
		if( lhs->sym <> rhs->sym ) then return FB_SEMANTIC_ARMS_DIFFERENT
	case AST_NODECLASS_CONST
		'' FBVALUE is a union. Its integer view preserves all floating bits,
		'' including signed zero and NaN payloads, unlike numeric equality.
		if( lhs->val.i <> rhs->val.i ) then return FB_SEMANTIC_ARMS_DIFFERENT
	case AST_NODECLASS_DEREF
		if( lhs->ptr.ofs <> rhs->ptr.ofs ) then return FB_SEMANTIC_ARMS_DIFFERENT
	case AST_NODECLASS_IDX
		if( (lhs->sym <> rhs->sym) or (lhs->idx.ofs <> rhs->idx.ofs) or _
		    (lhs->idx.mult <> rhs->idx.mult) ) then return FB_SEMANTIC_ARMS_DIFFERENT
	case AST_NODECLASS_BOP
		if( (lhs->sym <> rhs->sym) or (lhs->op.op <> rhs->op.op) or _
		    (lhs->op.options <> rhs->op.options) or (lhs->op.ex <> rhs->op.ex) ) then return FB_SEMANTIC_ARMS_DIFFERENT
	case AST_NODECLASS_UOP
		if( (lhs->op.op <> rhs->op.op) or (lhs->op.options <> rhs->op.options) ) then return FB_SEMANTIC_ARMS_DIFFERENT
	case AST_NODECLASS_ADDROF
		if( (lhs->sym <> rhs->sym) or (lhs->op.op <> rhs->op.op) ) then return FB_SEMANTIC_ARMS_DIFFERENT
	case AST_NODECLASS_OFFSET
		if( (lhs->sym <> rhs->sym) or (lhs->ofs.ofs <> rhs->ofs.ofs) ) then return FB_SEMANTIC_ARMS_DIFFERENT
	case AST_NODECLASS_CONV
		if( (lhs->cast.doconv <> rhs->cast.doconv) or _
		    (lhs->cast.do_convfd2fs <> rhs->cast.do_convfd2fs) or _
		    (lhs->cast.convconst <> rhs->cast.convconst) ) then return FB_SEMANTIC_ARMS_DIFFERENT
	case AST_NODECLASS_CALL
		if( (lhs->sym <> rhs->sym) or (lhs->call.semantic_target <> rhs->call.semantic_target) or _
		    (lhs->call.isrtl <> rhs->call.isrtl) or (lhs->call.args <> rhs->call.args) ) then return FB_SEMANTIC_ARMS_DIFFERENT
		'' Fixed-string copyback and generated UDT result storage carry effects
		'' outside the ordinary child tree. Do not invent an equivalence for them.
		if( (lhs->call.strtail <> NULL) or (rhs->call.strtail <> NULL) or _
		    (lhs->call.tmpres <> NULL) or (rhs->call.tmpres <> NULL) or _
		    (lhs->call.profbegin <> NULL) or (rhs->call.profbegin <> NULL) ) then return FB_SEMANTIC_ARMS_UNKNOWN
	case AST_NODECLASS_ARG
		if( (lhs->sym <> rhs->sym) or (lhs->arg.mode <> rhs->arg.mode) or _
		    (lhs->arg.lgt <> rhs->arg.lgt) or _
		    (lhs->arg.semantic_defaulted <> rhs->arg.semantic_defaulted) ) then return FB_SEMANTIC_ARMS_DIFFERENT
	case else
		return FB_SEMANTIC_ARMS_UNKNOWN
	end select
	if( lhs->vector <> rhs->vector ) then return FB_SEMANTIC_ARMS_DIFFERENT
	dim as integer left_result = hSemanticIifArmCompare(lhs->l, rhs->l, work_left, depth + 1)
	if( left_result <> FB_SEMANTIC_ARMS_SAME ) then return left_result
	return hSemanticIifArmCompare(lhs->r, rhs->r, work_left, depth + 1)
end function

#endif
'' end of tooling/semantic-iif-compare.bi
