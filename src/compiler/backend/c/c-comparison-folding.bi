/'
 Project: FreeBASIC C backend
 File: backend/c/c-comparison-folding.bi
 Purpose: Apply proven comparison facts before local storage analysis.
 Responsibilities: Share expression, branch and label rewriting between integer
 range, object-address and repeatable-operand proofs, retaining BASIC booleans.
 This file intentionally does NOT contain: Value proofs, storage ownership,
 source access flag changes or native diagnostic suppression.
'/

#include once "backend/c/c-self-comparisons.bi"

'' -------------------------------------------------------------------------
'' Tree rewriting before local storage analysis
'' -------------------------------------------------------------------------

private sub hCComparisonRememberLabel(byval labels as THASH ptr, byval sym as FBSYMBOL ptr)
	if( sym = NULL orelse symbIsLabel(sym) = FALSE ) then exit sub
	var key = str(culngint(cuint(sym)))
	if( hashLookup(labels, strptr(key)) = NULL ) then
		hashAdd(labels, ZstrDup(strptr(key)), sym, hashHash(strptr(key)))
	end if
end sub

private sub hCComparisonLabelReferences(byval n as ASTNODE ptr, byval labels as THASH ptr)
	if( n = NULL ) then exit sub
	if( n->class <> AST_NODECLASS_LABEL ) then hCComparisonRememberLabel(labels, n->sym)
	select case as const n->class
	case AST_NODECLASS_BOP, AST_NODECLASS_UOP, AST_NODECLASS_BRANCH
		hCComparisonRememberLabel(labels, n->op.ex)
	case AST_NODECLASS_JMPTB
		hCComparisonRememberLabel(labels, n->jmptb.deflabel)
		for i as integer = 0 to n->jmptb.labelcount - 1
			hCComparisonRememberLabel(labels, n->jmptb.labels[i])
		next
	case AST_NODECLASS_CALL
		'' Match the loader's profiling and string-copyback subtrees, which
		'' are not ordinary L/R children. Real label addresses must survive.
		hCComparisonLabelReferences(n->call.profbegin, labels)
		hCComparisonLabelReferences(n->call.profend, labels)
		var item = n->call.strtail
		while( item <> NULL )
			hCComparisonLabelReferences(item->srctree, labels)
			item = item->prev
		wend
	end select
	hCComparisonLabelReferences(n->l, labels)
	hCComparisonLabelReferences(n->r, labels)
end sub

private sub hCComparisonPruneLabel(byval n as ASTNODE ptr, byval removed as THASH ptr, byval live as THASH ptr)
	if( n = NULL ) then exit sub
	if( n->class = AST_NODECLASS_LABEL andalso n->l = NULL andalso n->r = NULL ) then
		var key = str(culngint(cuint(n->sym)))
		if( hashLookup(removed, strptr(key)) <> NULL andalso hashLookup(live, strptr(key)) = NULL ) then
			'' Only retire labels whose branch this pass removed. Keep the
			'' symbol's source-access facts intact for semantic observations.
			n->class = AST_NODECLASS_NOP
			n->dtype = FB_DATATYPE_VOID
			n->sym = NULL
		end if
	end if
	hCComparisonPruneLabel(n->l, removed, live)
	hCComparisonPruneLabel(n->r, removed, live)
end sub

private function hCComparisonIifArms(byval n as ASTNODE ptr) as integer
	if( hCRangeType(n->dtype) = FALSE ) then return FALSE
	if( n->r = NULL orelse n->r->class <> AST_NODECLASS_LINK ) then return FALSE
	var condition = n->r->l
	var choices = n->r->r
	if( condition = NULL or choices = NULL ) then return FALSE
	if( choices->class <> AST_NODECLASS_LINK ) then return FALSE
	'' These are the scalar assignments created by astNewIIF. Keep managed
	'' result initialization and branch-local destructor wrappers unchanged.
	for arm as integer = 0 to 1
		var assignment = iif(arm = 0, choices->l, choices->r)
		if( assignment = NULL orelse assignment->class <> AST_NODECLASS_ASSIGN ) then return FALSE
		if( assignment->l = NULL orelse assignment->l->class <> AST_NODECLASS_VAR ) then return FALSE
		if( assignment->l->sym <> n->sym ) then return FALSE
	next
	return TRUE
end function

private function hCComparisonFoldIif(byval n as ASTNODE ptr) as integer
	if( hCComparisonIifArms(n) = FALSE ) then return FALSE
	var condition = n->r->l
	var choices = n->r->r
	var decision = condition
	while( decision->class = AST_NODECLASS_LINK )
		if( decision->link.ret <> AST_LINK_RETURN_NONE ) then return FALSE
		if( decision->l = NULL orelse decision->l->class <> AST_NODECLASS_MACRO ) then return FALSE
		if( decision->l->op.op <> AST_OP_C_EVAL or decision->r = NULL ) then return FALSE
		decision = decision->r
	wend
	dim as integer take_false = FALSE
	if( decision->class = AST_NODECLASS_BRANCH ) then
		if( decision->op.op <> AST_OP_JMP or decision->op.ex <> n->iif.falselabel ) then return FALSE
		take_false = TRUE
	elseif( decision->class <> AST_NODECLASS_NOP ) then
		return FALSE
	end if
	var selected = iif(take_false, choices->r, choices->l)
	var discarded = iif(take_false, choices->l, choices->r)
	'' The IIF-owned target will not be emitted after selecting its arm.
	decision->class = AST_NODECLASS_NOP
	decision->op.ex = NULL
	astDelTree(discarded)
	astDelNode(choices)
	astDelNode(n->r)
	var value = n->l
	n->class = AST_NODECLASS_LINK
	n->sym = NULL
	n->l = astNewLINK(condition, selected, AST_LINK_RETURN_NONE)
	n->r = value
	n->link.ret = AST_LINK_RETURN_RIGHT
	return TRUE
end function

private function hCComparisonBitwiseValue(byval n as ASTNODE ptr, byref value as integer) as integer
	if( n->op.op <> AST_OP_AND and n->op.op <> AST_OP_OR ) then return FALSE
	if( hCRangeType(n->dtype) = FALSE or n->op.ex <> NULL ) then return FALSE
	'' In-place operators still own a write. Only fold ordinary value results,
	'' and retain both operands because BASIC AND/OR do not short-circuit.
	if( (n->op.options and AST_OPOPT_ALLOCRES) = 0 ) then return FALSE
	for operand as integer = 0 to 1
		dim as CINTEGERRANGE interval
		var child = iif(operand = 0, n->l, n->r)
		if( hCRangeValue(child, interval, TRUE) = FALSE ) then continue for
		if( hCRangeConvert(interval, n->dtype) = FALSE ) then continue for
		if( hCRangeCompare(interval.lower, interval.upper) <> 0 ) then continue for
		if( n->op.op = AST_OP_AND ) then
			if( interval.lower.magnitude = 0 ) then
				value = 0
				return TRUE
			end if
		elseif( typeIsSigned(n->dtype) ) then
			if( interval.lower.negative and interval.lower.magnitude = 1 ) then
				value = -1
				return TRUE
			end if
		elseif( interval.lower.magnitude = hCRangeMask(typeGetBits(n->dtype)) ) then
			value = -1
			return TRUE
		end if
	next
	return FALSE
end function

private function hCComparisonFold(byval n as ASTNODE ptr, byval removed as THASH ptr, byref removed_branches as integer) as integer
	if( n = NULL ) then return FALSE
	if( n->class = AST_NODECLASS_IIF andalso hCComparisonIifArms(n) = FALSE ) then
		'' The IIF loader owns both arms and its private false label. Leave
		'' its condition intact unless the entire scalar selection can fold.
		var changed = hCComparisonFold(n->l, removed, removed_branches)
		if( n->r <> NULL ) then changed or= hCComparisonFold(n->r->r, removed, removed_branches)
		return changed
	end if
	var changed = hCComparisonFold(n->l, removed, removed_branches)
	changed or= hCComparisonFold(n->r, removed, removed_branches)
	if( n->class = AST_NODECLASS_IIF ) then return hCComparisonFoldIif(n) or changed
	if( n->class <> AST_NODECLASS_BOP orelse n->vector <> 0 ) then return changed
	dim as integer truth, retain_operands = FALSE
	if( astOpIsRelational(n->op.op) = FALSE ) then
		'' Propagate facts exposed by this pass. Existing constant bitwise
		'' expressions retain the ordinary optimizer's branch/label handling.
		if( changed = FALSE ) then return changed
		if( hCComparisonBitwiseValue(n, truth) = FALSE ) then return changed
		retain_operands = TRUE
	elseif( env.clopt.debuginfo ) then
		'' Debug storage keeps its original reads even for plain variables.
		if( hCRangeTruth(n, truth, TRUE) = FALSE ) then return changed
		retain_operands = TRUE
	elseif( hCRangeTruth(n, truth) = FALSE ) then
		if( hCRangeTruth(n, truth, TRUE) ) then
			retain_operands = TRUE
		elseif( hCAddressTruth(n, truth) = FALSE ) then
			if( hCSelfTruth(n, truth) = FALSE ) then return changed
		end if
	end if
	dim as ASTNODE ptr evaluation = NULL
	if( retain_operands ) then
		'' Only this C-backend pass creates the evaluation macro. The loader
		'' retains both reads before the fixed value or unconditional branch.
		evaluation = astNewMACRO(AST_OP_C_EVAL, n->l, n->r, FB_DATATYPE_VOID, NULL)
	else
		astDelTree(n->l)
		astDelTree(n->r)
	end if
	n->l = NULL
	n->r = NULL
	n->sym = NULL
	if( n->op.ex <> NULL ) then
		n->dtype = FB_DATATYPE_INVALID
		n->subtype = NULL
		if( truth ) then
			n->class = AST_NODECLASS_BRANCH
			n->op.op = AST_OP_JMP
			n->op.options = AST_OPOPT_ALLOCRES
		else
			hCComparisonRememberLabel(removed, n->op.ex)
			removed_branches += 1
			n->class = AST_NODECLASS_NOP
		end if
	else
		'' astNewCONSTi normalizes BOOLEAN separately from BASIC integer -1.
		var value = astNewCONSTi(truth, n->dtype, n->subtype)
		n->class = AST_NODECLASS_CONST
		n->val = value->val
		astDelNode(value)
	end if
	if( evaluation <> NULL ) then
		'' Keep this statement's list links and source position. Its folded
		'' child is no longer a conditional edge, so ordinary label pruning
		'' can retire only labels that have no remaining references.
		var folded = astNewNode(n->class, n->dtype, n->subtype)
		if( n->class = AST_NODECLASS_CONST ) then
			folded->val = n->val
		else
			folded->op = n->op
		end if
		n->class = AST_NODECLASS_LINK
		n->l = evaluation
		n->r = folded
		n->link.ret = AST_LINK_RETURN_RIGHT
		if( folded->class <> AST_NODECLASS_CONST ) then
			n->link.ret = AST_LINK_RETURN_NONE
			n->dtype = FB_DATATYPE_VOID
		end if
	end if
	return TRUE
end function

private function hCComparisonsBegin(byval first as ASTNODE ptr) as integer
	if( c_local_refs_complete = FALSE ) then
		'' Debug range folding retains every operand. Opaque assembly still
		'' prevents branch/label rewriting because it can name hidden targets.
		if( env.clopt.debuginfo = FALSE or ctx.computedgotoopaque ) then return FALSE
	end if
	dim as integer changed = FALSE
	dim as integer removed_branches = 0
	dim as THASH removed
	hashInit(@removed, 32, TRUE)
	var n = first
	while( n <> NULL )
		changed or= hCComparisonFold(n, @removed, removed_branches)
		n = n->next
	wend
	if( removed_branches <> 0 ) then
		dim as THASH live
		hashInit(@live, 32, TRUE)
		n = first
		while( n <> NULL )
			hCComparisonLabelReferences(n, @live)
			n = n->next
		wend
		n = first
		while( n <> NULL )
			hCComparisonPruneLabel(n, @removed, @live)
			n = n->next
		wend
		hashEnd(@live)
	end if
	hashEnd(@removed)
	return changed
end function

/' end of backend/c/c-comparison-folding.bi '/
