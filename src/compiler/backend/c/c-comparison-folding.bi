/'
 Project: FreeBASIC C backend
 File: backend/c/c-comparison-folding.bi
 Purpose: Apply proven comparison facts before local storage analysis.
 Responsibilities: Share expression, branch and label rewriting between integer
 range and object-address proofs, retaining BASIC boolean conventions.
 This file intentionally does NOT contain: Value proofs, storage ownership,
 source access flag changes or native diagnostic suppression.
'/

'' -------------------------------------------------------------------------
'' Tree rewriting before local storage analysis
'' -------------------------------------------------------------------------

private sub hCComparisonRememberLabel(byval labels as THASH ptr, byval sym as FBSYMBOL ptr)
	if( sym = NULL orelse symbIsLabel(sym) = FALSE ) then exit sub
	var key = str(culngint(sym))
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
		var key = str(culngint(n->sym))
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

private function hCComparisonFold(byval n as ASTNODE ptr, byval removed as THASH ptr, byref removed_branches as integer) as integer
	if( n = NULL ) then return FALSE
	var changed = hCComparisonFold(n->l, removed, removed_branches)
	changed or= hCComparisonFold(n->r, removed, removed_branches)
	if( n->class <> AST_NODECLASS_BOP orelse n->vector <> 0 ) then return changed
	if( astOpIsRelational(n->op.op) = FALSE ) then return changed
	dim as integer truth
	if( hCRangeTruth(n, truth) = FALSE ) then
		if( hCAddressTruth(n, truth) = FALSE ) then return changed
	end if
	astDelTree(n->l)
	astDelTree(n->r)
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
	return TRUE
end function

private function hCComparisonsBegin(byval first as ASTNODE ptr) as integer
	if( c_local_refs_complete = FALSE ) then return FALSE
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
