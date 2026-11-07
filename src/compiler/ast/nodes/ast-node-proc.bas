'' Project: FreeBASIC compiler - typed syntax tree nodes
'' -----------------------------------------
''
'' File: ast/nodes/ast-node-proc.bas
''
'' Purpose:
''
''     Build and lower AST proc body nodes.
''
'' Responsibilities:
''
''     - check operand types and build the node family described below
''     - lower nodes through the common IR and runtime call interfaces
''
'' This file intentionally does NOT contain:
''
''     - token scanning or physical register allocation
''

''
'' AST proc body nodes
'' l = head node; r = tail node
''
'' note: an implicit scope block isn't created, because the
''       implicit main() function (inside scope blocks only
''       non-decl statements are allowed)
''


#include once "core/fb.bi"
#include once "core/fbint.bi"
#include once "ast/nodes/c-procedure-reachability.bi"
#include once "support/containers/list.bi"
#include once "support/containers/stack.bi"
#include once "lexer/lex.bi"
#include once "parser/parser.bi"
#include once "backend/ir.bi"
#include once "runtime/rtl.bi"
#include once "ast/ast.bi"
#include once "tooling/semantic-startup.bi"
#include once "tooling/semantic-procedure-state.bi"
#include once "tooling/semantic-object-operations.bi"
#include once "ast/nodes/source-unused-warnings.bi"
#include once "ast/nodes/source-null-warnings.bi"

declare function irhlcErrorOriginalProcObserved( byval proc as FBSYMBOL ptr ) as integer
declare function irhlcErrorOriginalStatementNeeded( byval n as ASTNODE ptr ) as integer
declare sub irhlcErrorSetCleanupBoundary( byval proc as FBSYMBOL ptr, byval label as FBSYMBOL ptr )
declare function irhlcErrorNeedsCleanupBoundary( byval body as ASTNODE ptr ) as integer

declare sub fbSemanticModelExportProc(byval proc as FBSYMBOL ptr, _
	byval astproc as ASTNODE ptr, byval emitted as integer)

type FB_GLOBINSTANCE
	sym             as FBSYMBOL_ ptr            '' for symbol
	initree         as ASTNODE ptr              '' can't store in sym, or emit will use it
	call_dtor       as integer
	semantic_identity as longint                '' copied observation identity, never a native address
end type

declare function hModLevelIsEmpty( byval p as ASTNODE ptr ) as integer
declare sub hLoadProcResult( byval proc as FBSYMBOL ptr )
declare function hDeclVarsForProcParams( byval proc as FBSYMBOL ptr ) as integer
declare function hInitVptr _
	( _
		byval parent as FBSYMBOL ptr, _
		byval proc as FBSYMBOL ptr _
	) as ASTNODE ptr
declare sub hCallCtors( byval n as ASTNODE ptr, byval sym as FBSYMBOL ptr )
declare sub hCallDtors( byval proc as FBSYMBOL ptr )
declare sub hCallDeleteDtor( byval proc as FBSYMBOL ptr )
declare sub hGenStaticInstancesDtors( byval proc as FBSYMBOL ptr )
declare sub hDiscardStaticInstancesDtors( byval proc as FBSYMBOL ptr )
declare sub hGenGlobalInstancesCtor( )

/'
    Procedure reachability state is kept separately from symbol status bits.
    Semantic exports expose those bits, so backend analysis must not alter them.
'/
type CPROCRECORD
	name as zstring ptr
	flags as integer
	node as ASTNODE ptr
	next_pending as CPROCRECORD ptr
end type

private dim shared as TLIST cproc_records
private dim shared as THASH cproc_names
private dim shared as integer cproc_ready

private sub hCProcFinish( )
	if( cproc_ready = FALSE ) then exit sub
	hashEnd( @cproc_names )
	dim as CPROCRECORD ptr entry = listGetHead( @cproc_records )
	while( entry <> NULL )
		ZstrFree( entry->name )
		entry = listGetNext( entry )
	wend
	listEnd( @cproc_records )
	cproc_ready = FALSE
end sub

private function hCProcGet( byval proc as FBSYMBOL ptr, byval create as integer ) as CPROCRECORD ptr
	if( proc = NULL ) then return NULL
	if( cproc_ready = FALSE ) then
		if( create = FALSE ) then return NULL
		'' Match the other procedure pools; records grow as they are needed.
		listInit( @cproc_records, 32, sizeof( CPROCRECORD ), LIST_FLAGS_CLEAR )
		'' 1024 chained buckets limit collisions without limiting procedure count.
		hashInit( @cproc_names, 1024, FALSE )
		cproc_ready = TRUE
	end if
	'' Convert through the native pointer width before widening the key.
	dim as string key = hex( culngint( cuint( proc ) ) )
	dim as CPROCRECORD ptr entry = hashLookup( @cproc_names, strptr( key ) )
	if( entry = NULL andalso create ) then
		entry = listNewNode( @cproc_records )
		entry->name = ZstrDup( strptr( key ) )
		hashAdd( @cproc_names, entry->name, entry, hashHash( entry->name ) )
	end if
	return entry
end function

sub astCProcRecordAddress( byval proc as FBSYMBOL ptr )
	dim as CPROCRECORD ptr entry = hCProcGet( proc, TRUE )
	if( entry <> NULL ) then entry->flags or= CPROC_ADDRESS_TAKEN
end sub

''::::
sub astProcListInit( )

	hCProcFinish( )
	ast.proc.head = NULL
	ast.proc.tail = NULL
	ast.proc.curr = NULL

	''
	listInit( @ast.globinst.list, 32, len( FB_GLOBINSTANCE ), LIST_FLAGS_NOCLEAR )
	ast.globinst.ctorcnt = 0
	ast.globinst.dtorcnt = 0

end sub

''::::
sub astProcListEnd( )

	hSourceWarningsClear()
	hCProcFinish( )
	ast.globinst.dtorcnt = 0
	ast.globinst.ctorcnt = 0
	listEnd( @ast.globinst.list )

	''
	ast.proc.head = NULL
	ast.proc.tail = NULL
	ast.proc.curr = NULL

end sub

private function hNewProcNode( ) as ASTNODE ptr
	dim as ASTNODE ptr n = any

	n = astNewNode( AST_NODECLASS_PROC, FB_DATATYPE_INVALID, NULL )

	'' add to list
	if( ast.proc.tail <> NULL ) then
		ast.proc.tail->next = n
	else
		ast.proc.head = n
	end if

	n->prev = ast.proc.tail
	n->next = NULL
	ast.proc.tail = n

	function = n
end function

private sub hDelProcNode( byval n as ASTNODE ptr )
	n->l = NULL
	n->r = NULL

	'' remove from list
	if( n->prev <> NULL ) then
		n->prev->next = n->next
	else
		ast.proc.head = n->next
	end If

	if( n->next <> NULL ) then
		n->next->prev = n->prev
	else
		ast.proc.tail = n->prev
	end If

	astDelNode( n )
end sub

private function hMarkCLabelReference( byval label as FBSYMBOL ptr ) as integer
	if( label = NULL ) then return FALSE
	if( symbIsLabel( label ) = FALSE ) then return FALSE
	if( symbGetIsAccessed( label ) ) then return FALSE
	symbSetIsAccessed( label )
	function = TRUE
end function

private function hMarkCLabelReferences _
	( _
		byval n as ASTNODE ptr, _
		byval branches as integer _
	) as integer
	if( n = NULL ) then return FALSE
	dim as integer changed = FALSE

	'' Label definitions are not references. Record direct branches, address
	'' expressions and assembler operands before consuming any AST nodes.
	'' A branch can add a target only when its statement is reachable.
	select case( n->class )
	case AST_NODECLASS_BOP, AST_NODECLASS_UOP, AST_NODECLASS_BRANCH
		if( branches ) then changed or= hMarkCLabelReference( n->op.ex )
	case AST_NODECLASS_JMPTB
		if( branches ) then
			for i as integer = 0 to n->jmptb.labelcount - 1
				changed or= hMarkCLabelReference( n->jmptb.labels[i] )
			next
			changed or= hMarkCLabelReference( n->jmptb.deflabel )
		end if
	case AST_NODECLASS_ASM
		var token = n->asm.tokhead
		while( token <> NULL )
			if( token->type = AST_ASMTOK_SYMB ) then
				changed or= hMarkCLabelReference( token->sym )
			end if
			token = token->next
		wend
	case AST_NODECLASS_LABEL
		'' The definition alone cannot make code following END reachable.
	case else
		changed or= hMarkCLabelReference( n->sym )
	end select

	changed or= hMarkCLabelReferences( n->l, branches )
	changed or= hMarkCLabelReferences( n->r, branches )
	function = changed
end function

private sub hResetCLabelReferences( byval n as ASTNODE ptr )
	if( n = NULL ) then exit sub
	if( n->class = AST_NODECLASS_LABEL ) then
		n->sym->stats and= not FB_SYMBSTATS_ACCESSED
	end if
	hResetCLabelReferences( n->l )
	hResetCLabelReferences( n->r )
end sub

function astCStatementEndsFlow( byval n as ASTNODE ptr ) as integer
	function = FALSE
	select case( n->class )
	case AST_NODECLASS_CALL
		if( n->l = NULL ) then
			if( symbGetProcIsNoReturn( n->sym ) ) then
				return TRUE
			elseif( symbGetIsRTL( n->sym ) ) then
				function = (*symbGetMangledName( n->sym ) = FB_RTL_END)
			end if
		end if
	case AST_NODECLASS_BRANCH
		select case( n->op.op )
		case AST_OP_JMP, AST_OP_JUMPPTR, AST_OP_RET
			function = TRUE
		end select
	case AST_NODECLASS_JMPTB
		function = TRUE
	case AST_NODECLASS_LOAD
		function = n->lod.isres
	end select
end function

private sub hCheckCLabelReferences _
	( _
		byval n as ASTNODE ptr, _
		byref fallthrough as integer, _
		byref changed as integer _
	)
	if( n = NULL ) then exit sub
	if( n->class = AST_NODECLASS_LINK ) then
		hCheckCLabelReferences( n->l, fallthrough, changed )
		hCheckCLabelReferences( n->r, fallthrough, changed )
		exit sub
	end if
	if( n->class = AST_NODECLASS_LABEL ) then
		if( symbGetIsAccessed( n->sym ) ) then fallthrough = TRUE
	end if
	if( fallthrough = FALSE ) then exit sub
	changed or= hMarkCLabelReferences( n, TRUE )
	if( astCStatementEndsFlow( n ) ) then fallthrough = FALSE
end sub

private sub hCheckCReturn _
	( _
		byval n as ASTNODE ptr, _
		byref fallthrough as integer, _
		byref returns as integer _
	)
	if( n = NULL ) then exit sub
	if( n->class = AST_NODECLASS_LINK ) then
		hCheckCReturn( n->l, fallthrough, returns )
		hCheckCReturn( n->r, fallthrough, returns )
		exit sub
	end if
	if( n->class = AST_NODECLASS_LABEL ) then
		if( symbGetIsAccessed( n->sym ) ) then fallthrough = TRUE
	end if
	if( fallthrough = FALSE ) then exit sub
	if( n->class = AST_NODECLASS_LOAD ) then
		if( n->lod.isres ) then returns = TRUE
	elseif( n->class = AST_NODECLASS_BRANCH ) then
		if( n->op.op = AST_OP_RET ) then returns = TRUE
	end if
	if( astCStatementEndsFlow( n ) ) then fallthrough = FALSE
end sub

function astProcMayReturn( byval proc as ASTNODE ptr ) as integer
	assert( proc->class = AST_NODECLASS_PROC )

	'' Address-taken and assembler targets may be entered indirectly. Keep
	'' them as roots, then follow branches from reachable statements. Repeat
	'' for backwards edges, without allowing dead branch cycles to introduce
	'' their own entries. The growing set is bounded by this procedure's labels.
	var n = proc->l
	while( n <> NULL )
		hResetCLabelReferences( n )
		n = n->next
	wend
	n = proc->l
	while( n <> NULL )
		hMarkCLabelReferences( n, FALSE )
		n = n->next
	wend
	dim as integer changed, live
	do
		changed = FALSE
		live = TRUE
		n = proc->l
		while( n <> NULL )
			hCheckCLabelReferences( n, live, changed )
			n = n->next
		wend
	loop while( changed )
	dim as integer fallthrough = TRUE, returns = FALSE
	n = proc->l
	while( n <> NULL )
		hCheckCReturn( n, fallthrough, returns )
		n = n->next
	wend
	function = returns or fallthrough
end function

private sub hFlushCStatement( byval n as ASTNODE ptr, byref fallthrough as integer )
	if( n = NULL ) then exit sub

	'' A statement LINK has no observed result at the procedure boundary.
	'' Visit both sides separately so labels and temporary destruction keep
	'' their order even when an earlier statement terminates the application.
	if( n->class = AST_NODECLASS_LINK ) then
		hFlushCStatement( n->l, fallthrough )
		hFlushCStatement( n->r, fallthrough )
		astDelNode( n )
		exit sub
	end if

	if( irhlcErrorOriginalStatementNeeded(n) = FALSE ) then
		select case n->class
		case AST_NODECLASS_DECL
			'' Preserve lexical storage, but not an unreachable initializer.
			astDelTree(n->l)
			n->l = NULL
		case AST_NODECLASS_SCOPEBEGIN, AST_NODECLASS_SCOPEEND, _
		     AST_NODECLASS_DBG, AST_NODECLASS_LIT, AST_NODECLASS_NOP
			'' Balanced scopes and source annotations still belong to the owner.
		case else
			if( n->class = AST_NODECLASS_JMPTB ) then
				deallocate n->jmptb.values
				deallocate n->jmptb.labels
			end if
			astDelTree(n)
			exit sub
		end select
	end if

	select case( n->class )
	case AST_NODECLASS_LABEL
		if( symbGetIsAccessed( n->sym ) ) then
			fallthrough = TRUE
		elseif( n->sym <> astGetProc( )->block.initlabel ) then
			'' Unreferenced AST labels need no C basic block. Keep the procedure
			'' entry: an indirect jump may still need its address while lowering.
			astDelNode( n )
			exit sub
		end if
	case AST_NODECLASS_DECL
		'' Keep the lexical declaration: a later branch may still refer to
		'' this variable. Its default initialization is executable code.
		if( fallthrough = FALSE ) then
			astDelTree( n->l )
			n->l = NULL
		end if
	case AST_NODECLASS_SCOPEBEGIN, AST_NODECLASS_SCOPEEND, _
	     AST_NODECLASS_DBG, AST_NODECLASS_LIT, AST_NODECLASS_NOP
		'' Preserve balanced scopes and source annotations for the live code.
	case else
		if( fallthrough = FALSE ) then
			astDelTree( n )
			exit sub
		end if
	end select

	var terminates = astCStatementEndsFlow( n )
	astLoad( n )
	astDelNode( n )
	if( terminates ) then fallthrough = FALSE
end sub

''::::
private sub hProcFlush _
	( _
		byval p as ASTNODE ptr, _
		byval doemit as integer _
	)

	dim as ASTNODE ptr n = any, nxt = any
	dim as FBSYMBOL ptr sym = any

	sym = p->sym

	if( doemit ) then
		'' emit the static dtor wrappers
		hGenStaticInstancesDtors( sym )
	end if

	''
	ast.proc.curr = p
	ast.currblock = p
	ast.doemit = doemit

	''
	parser.scope = iif( p->block.proc.ismain, FB_MAINSCOPE, FB_MAINSCOPE+1 )
	parser.currproc = sym
	parser.currblock = sym

	''
	symbNestBegin( sym, FALSE )

	'' Preserve the parsed procedure before any backend rewrites its storage
	'' or control flow. C header emission can prune unobserved initializers;
	'' doing this afterwards makes source analysis depend on backend liveness.
	'' The exporter owns one snapshot per procedure, including error-owner
	'' callbacks which may request it again while preparing emission.
	if( (env.clopt.backend = FB_BACKEND_GCC) or (env.clopt.backend = FB_BACKEND_CLANG) ) then
		if( symbGetIsParsed(sym) and (errGetCount( ) = 0) ) then
			fbSemanticModelExportProc(sym, p, doemit)
		end if
	end if

	'' emit header
	if( ast.doemit ) then
		symbSetProcIsEmitted( sym )

		irEmitPROCBEGIN( sym, p->block.initlabel )

		'' allocate the non-static local variables on stack
		astScopeAllocLocals(symbGetProcSymbTbHead(sym))
	end if

	'' Other backends keep their allocated stack offsets in the observation.
	'' C was already captured before its header rewrites. Repeated requests
	'' are harmless because the exporter owns the first snapshot.
	if( symbGetIsParsed(sym) and (errGetCount( ) = 0) ) then
		fbSemanticModelExportProc(sym, p, doemit)
	end if

	dim as integer c_fallthrough = doemit and (not symbIsNaked( sym )) and _
		((env.clopt.backend = FB_BACKEND_GCC) or (env.clopt.backend = FB_BACKEND_CLANG))

	'' flush nodes
	dim as integer fallthrough = TRUE
	n = p->l
	do while( n <> NULL )
		nxt = n->next
		if( c_fallthrough ) then
			'' Only a referenced label can resume execution after fb_End().
			'' This also removes the dead implicit main epilogue and result load.
			hFlushCStatement( n, fallthrough )
		else
			astLoad( n )
			astDelNode( n )
		end if
		n = nxt
	loop

	'' Loading a discarded AST can mark its generated wrappers as accessed.
	'' Clear those observations after loading, before C prototypes are emitted.
	if( doemit = FALSE ) then hDiscardStaticInstancesDtors( sym )

	'' emit footer
	if( ast.doemit ) then
		irEmitPROCEND( sym, p->block.initlabel, p->block.exitlabel )

		'' Emit static local variables
		irProcAllocStaticVars( symbGetProcSymbTbHead( sym ) )
	end if

	'' del symbols from hash and symbol tb's
	symbDelSymbolTb( @sym->proc.symtb, FALSE )

	''
	symbNestEnd( FALSE )

	hDelProcNode( p )

	ast.doemit = TRUE

end sub

/'
    C procedure reachability

    A private routine can be marked accessed by a call inside another private
    routine that is never emitted. Keep address-taken procedures and any
    unclassified use, then follow direct calls from emitted roots. Inline
    assembly can name symbols outside the AST, so it disables this analysis
    for the entire translation unit.

    Use a bounded work stack for both inspection and call marking. Large
    initializers can exceed the old recursion limit without containing assembly.
'/
private sub hCProcQueue _
	( _
		byval entry as CPROCRECORD ptr, _
		byref first as CPROCRECORD ptr, _
		byref last as CPROCRECORD ptr _
	)
	if( (entry->flags and CPROC_REACHABLE) <> 0 ) then exit sub
	entry->flags or= CPROC_REACHABLE
	if( entry->node = NULL ) then exit sub

	entry->next_pending = NULL
	if( last <> NULL ) then
		last->next_pending = entry
	else
		first = entry
	end if
	last = entry
end sub

private function hCProcWalkPush _
	( byval work as TSTACK ptr, byval n as ASTNODE ptr, byref pending as integer ) as integer

	if( n = NULL ) then return TRUE
	'' The pool's quarter-size growth stays below 2 MiB on 64-bit hosts.
	'' A wider pending frontier keeps the conservative emission path.
	if( pending >= 65536 ) then return FALSE
	var slot = cptr(ASTNODE ptr ptr, stackPush(work))
	if( slot = NULL ) then return FALSE
	*slot = n
	pending += 1
	return TRUE
end function

private function hCProcWalk _
	( _
		byval n as ASTNODE ptr, _
		byval flag as integer, _
		byval work as TSTACK ptr, _
		byref first as CPROCRECORD ptr, _
		byref last as CPROCRECORD ptr _
	) as integer

	dim as integer pending = 0, visited = 0
	if( hCProcWalkPush(work, n, pending) = FALSE ) then return FALSE
	while( pending > 0 )
		n = *cptr(ASTNODE ptr ptr, stackGetTOS(work))
		stackPop(work)
		pending -= 1
		'' Bound work as well as storage. This also stops a malformed child
		'' graph or string-copyback list from keeping the compiler here.
		visited += 1
		if( visited > 1048576 ) then return FALSE
		if( n->class = AST_NODECLASS_ASM ) then return FALSE
		if( hCProcWalkPush(work, n->r, pending) = FALSE ) then return FALSE
		if( hCProcWalkPush(work, n->l, pending) = FALSE ) then return FALSE
		if( n->class = AST_NODECLASS_CALL ) then
			if( flag <> 0 andalso n->sym <> NULL andalso _
			    symbGetClass(n->sym) = FB_SYMBCLASS_PROC ) then
				var entry = hCProcGet(n->sym, TRUE)
				if( entry = NULL ) then return FALSE
				if( flag = CPROC_REACHABLE ) then
					hCProcQueue(entry, first, last)
				else
					entry->flags or= flag
				end if
			end if
			'' Profiling and copy-back expressions belong to the call, not
			'' its ordinary children. Their visitation order does not change
			'' the fixed-point set of reachable procedures.
			dim as AST_TMPSTRLIST_ITEM ptr item = n->call.strtail
			while( item <> NULL )
				visited += 1
				if( visited > 1048576 ) then return FALSE
				if( hCProcWalkPush(work, item->srctree, pending) = FALSE ) then return FALSE
				item = item->prev
			wend
			if( hCProcWalkPush(work, n->call.profend, pending) = FALSE ) then return FALSE
			if( hCProcWalkPush(work, n->call.profbegin, pending) = FALSE ) then return FALSE
		end if
	wend
	return TRUE
end function


private function hCProcMarkReachable( ) as integer
	dim as ASTNODE ptr procnode = ast.proc.head
	dim as ASTNODE ptr stmt
	dim as FBSYMBOL ptr sym
	dim as CPROCRECORD ptr entry
	dim as CPROCRECORD ptr pending_first = NULL, pending_last = NULL
	dim as TSTACK work
	if( stackNew(@work, 32, sizeof(ASTNODE ptr), FALSE) = FALSE ) then
		stackFree(@work)
		return FALSE
	end if

	'' Assembly may reference a private symbol without an AST call or address.
	while( procnode <> NULL )
		if( symbIsNaked( procnode->sym ) ) then goto conservative
		stmt = procnode->l
		while( stmt <> NULL )
			if( hCProcWalk(stmt, 0, @work, pending_first, pending_last) = FALSE ) then goto conservative
			stmt = stmt->next
		wend
		procnode = procnode->next
	wend

	'' Ordinary private procedures and methods can be reached through calls or
	'' saved addresses. Operators, static locals, module bodies and generated
	'' entry points keep their existing emission rules.
	procnode = ast.proc.head
	while( procnode <> NULL )
		sym = procnode->sym
		if( symbGetIsParsed( sym ) ) then
			entry = hCProcGet( sym, TRUE )
			entry->flags and= (CPROC_ADDRESS_TAKEN or CPROC_STATIC_ROOT)
			entry->node = procnode
			entry->next_pending = NULL
			var allowed_pattrs = FB_PROCATTRIB_OVERLOADED or FB_PROCATTRIB_METHOD or _
			                     FB_PROCATTRIB_CONSTRUCTOR or FB_PROCATTRIB_RETURNBYREF or _
			                     FB_PROCATTRIB_NOTHISCONSTNESS or FB_PROCATTRIB_DESTRUCTOR1
			'' Division overloads are explicit calls. Unlike assignment and
			'' allocation operators, they are not synthesized during lowering.
			if( symbIsOperator( sym ) andalso sym->proc.ext <> NULL andalso _
			    symbGetProcOpOvl( sym ) = AST_OP_DIV ) then
				allowed_pattrs or= FB_PROCATTRIB_OPERATOR
			end if
			if( symbIsPrivate( sym ) andalso _
			    ((sym->pattrib and not allowed_pattrs) = 0) andalso _
			    (symbGetIsModLevelProc( sym ) = FALSE) andalso _
			    (symbGetIsGlobalCtor( sym ) = FALSE) andalso _
			    (symbGetIsGlobalDtor( sym ) = FALSE) andalso _
			    (procnode->block.proc.ismain = FALSE) ) then
				entry->flags or= CPROC_CANDIDATE
			end if
		end if
		procnode = procnode->next
	wend

	'' First distinguish calls in retained ASTs from accesses made elsewhere,
	'' such as a global initializer. Unclassified accesses remain roots.
	procnode = ast.proc.head
	while( procnode <> NULL )
		stmt = procnode->l
		while( stmt <> NULL )
			if( hCProcWalk(stmt, CPROC_DIRECT_CALL, @work, pending_first, pending_last) = FALSE ) then goto conservative
			stmt = stmt->next
		wend
		procnode = procnode->next
	wend

	procnode = ast.proc.head
	while( procnode <> NULL )
		sym = procnode->sym
		if( symbGetIsParsed( sym ) ) then
			entry = hCProcGet( sym, FALSE )
			if( (not symbIsPrivate( sym )) or procnode->block.proc.ismain ) then
				hCProcQueue( entry, pending_first, pending_last )
			elseif( symbGetIsAccessed( sym ) ) then
				if( ((entry->flags and CPROC_CANDIDATE) = 0) or _
				    ((entry->flags and CPROC_DIRECT_CALL) = 0) or _
				    ((entry->flags and (CPROC_ADDRESS_TAKEN or CPROC_STATIC_ROOT)) <> 0) ) then
					hCProcQueue( entry, pending_first, pending_last )
				end if
			end if
		end if
		procnode = procnode->next
	wend

	'' Each reachable body enters the queue once, even if many callers share it.
	while( pending_first <> NULL )
		entry = pending_first
		pending_first = entry->next_pending
		if( pending_first = NULL ) then pending_last = NULL
		stmt = entry->node->l
		while( stmt <> NULL )
			if( hCProcWalk(stmt, CPROC_REACHABLE, @work, pending_first, pending_last) = FALSE ) then goto conservative
			stmt = stmt->next
		wend
	wend
	stackFree(@work)
	return TRUE

conservative:
	stackFree(@work)
	return FALSE
end function

/'
    Static-local cleanup wrappers are generated while procedure bodies are
    flushed. Mark their destructors before that starts, otherwise an earlier
    private destructor body can be discarded before its wrapper calls it.
'/
private sub hMarkStaticInstanceDtors( )
	dim as ASTNODE ptr procnode = ast.proc.head
	dim as CPROCRECORD ptr entry, pending_first = NULL, pending_last = NULL

	'' Index procedures owning static cleanup lists. A destructor can itself
	'' own a static with a destructor, so their dependencies need a work queue.
	while( procnode <> NULL )
		dim as FBSYMBOL ptr proc = procnode->sym
		if( symbGetIsParsed( proc ) andalso proc->proc.ext <> NULL ) then
			if( proc->proc.ext->statdtor <> NULL ) then
				entry = hCProcGet( proc, TRUE )
				entry->node = procnode
				entry->next_pending = NULL
				entry->flags and= not CPROC_STATIC_QUEUED
			end if
		end if
		procnode = procnode->next
	wend

	procnode = ast.proc.head
	while( procnode <> NULL )
		dim as FBSYMBOL ptr proc = procnode->sym
		if( symbGetIsParsed( proc ) andalso proc->proc.ext <> NULL ) then
			if( proc->proc.ext->statdtor <> NULL ) then
				if( (not symbIsPrivate( proc )) or symbGetIsAccessed( proc ) or _
				    procnode->block.proc.ismain ) then
					entry = hCProcGet( proc, FALSE )
					entry->flags or= CPROC_STATIC_QUEUED
					if( pending_last = NULL ) then
						pending_first = entry
					else
						pending_last->next_pending = entry
					end if
					pending_last = entry
				end if
			end if
		end if
		procnode = procnode->next
	wend

	while( pending_first <> NULL )
		entry = pending_first
		pending_first = entry->next_pending
		if( pending_first = NULL ) then pending_last = NULL
		dim as TLIST ptr dtorlist = entry->node->sym->proc.ext->statdtor
		dim as FB_DTORWRAPPER ptr wrapper = listGetHead( dtorlist )
		while( wrapper <> NULL )
			dim as FBSYMBOL ptr dtor = symbGetCompDtor1( symbGetSubtype( wrapper->sym ) )
			if( dtor <> NULL ) then
				symbSetIsAccessed( dtor )
				'' A dead direct caller does not replace this late wrapper root.
				dim as CPROCRECORD ptr dependency = hCProcGet( dtor, TRUE )
				dependency->flags or= CPROC_STATIC_ROOT
				if( dependency <> NULL andalso dependency->node <> NULL ) then
					if( (dependency->flags and CPROC_STATIC_QUEUED) = 0 ) then
						dependency->flags or= CPROC_STATIC_QUEUED
						dependency->next_pending = NULL
						if( pending_last = NULL ) then
							pending_first = dependency
						else
							pending_last->next_pending = dependency
						end if
						pending_last = dependency
					end if
				end if
			end if
			wrapper = listGetNext( wrapper )
		wend
	wend
end sub

private function hShouldEmitProc( byval n as ASTNODE ptr, byval c_reachability as integer ) as integer
	dim as FBSYMBOL ptr sym = n->sym
	if( symbGetIsParsed( sym ) = FALSE ) then return FALSE
	if( symbIsPrivate( sym ) = FALSE ) then return TRUE
	if( symbGetIsAccessed( sym ) = FALSE ) then return FALSE
	if( symbGetIsModLevelProc( sym ) ) then return (hModLevelIsEmpty( n ) = FALSE)
	if( c_reachability ) then
		dim as CPROCRECORD ptr entry = hCProcGet( sym, FALSE )
		if( entry <> NULL ) then
			if( ((entry->flags and CPROC_CANDIDATE) <> 0) andalso _
			    ((entry->flags and CPROC_REACHABLE) = 0) ) then return FALSE
		end if
	end if
	return TRUE
end function

''::::
private sub hProcFlushAll _
	( _
		_
	)

	dim as ASTNODE ptr n = any
	dim as integer doemit = any
	dim as FBSYMBOL ptr sym = any

	'' procs should be sorted by include file

	if( (env.clopt.backend = FB_BACKEND_GCC) or (env.clopt.backend = FB_BACKEND_CLANG) ) then
		'' C prototypes can expose a non-returning callee to earlier callers.
		'' Settle these contracts before flushing any body, so its dead tail
		'' is removed using the same knowledge as the eventual C declaration.
		dim as integer changed = TRUE
		while( changed )
			changed = FALSE
			n = ast.proc.head
			while( n <> NULL )
				sym = n->sym
				if( symbGetIsParsed( sym ) and (not symbIsNaked( sym )) and _
				    (not symbGetProcIsNoReturn( sym )) ) then
					if( astProcMayReturn( n ) = FALSE ) then
						symbSetProcIsNoReturn( sym )
						changed = TRUE
					end if
				end if
				n = n->next
			wend
		wend
	end if

	'' gen the global ctors/dtors, if any (done before emitting main()
	'' because the ctors/dtors could be private and not called yet)
	hGenGlobalInstancesCtor( )
	hMarkStaticInstanceDtors( )

	dim as integer c_reachability = FALSE
	if( (env.clopt.backend = FB_BACKEND_GCC) or (env.clopt.backend = FB_BACKEND_CLANG) ) then
		c_reachability = hCProcMarkReachable( )
	end if


	do
		n = ast.proc.head
		if( n = NULL ) then
			exit do
		end if

		sym = n->sym

		doemit = hShouldEmitProc( n, c_reachability )

		hProcFlush( n, doemit )
	loop

end sub

private function astUpdate( byval n as ASTNODE ptr ) as ASTNODE ptr
	if( n = NULL ) then
		return NULL
	end if

	'' RTL function and the result was discarded? do not allocate a result
	if( astIsCALL( n ) ) then
		if( n->call.isrtl ) then
			if( astGetFullType( n ) <> FB_DATATYPE_VOID ) then
				astSetType( n, FB_DATATYPE_VOID, NULL )
			end if
		end if
	end if

	'' Note: Any updating here that can create temp vars with dtors (such
	'' as astTypeIniUpdate()) should also be done in astBuildBranch() (for
	'' the condition expression) and astNewIIF() (for the true/false
	'' expressions) since those cases require dtor calls for temp vars to
	'' be in specific locations, not just generically at the end of the
	'' statement (which is what the astDtorListFlush() below will do).

	'' Turn TYPEINI trees into real assignments
	'' Note: This can allocate temporary variables, so it must be done
	'' while in the proper scope context!
	n = astTypeIniUpdate( n )

	'' Bitfield assignment/access updating
	n = astUpdateBitfields( n )

	'' Tree optimizations
	n = astOptimizeTree( n )

	'' Assignment optimizations
	n = astOptAssignment( n )

	'' Update string '+' BOPs to 'fb_StrConcat' CALLs
	n = astUpdStrConcat( n )

	'' Destroy temporaries if needed
	if( ast.flushdtorlist ) then
		n = astNewLINK( n, astDtorListFlush( ), AST_LINK_RETURN_NONE )
	end if

	function = n
end function

''
'' Add a statement to the current procedure.
''
'' This will also
''  * update & optimize the statement (astUpdate())
''  * add clean up code behind the statement for currently-registered temporary
''    variables (astDtorListFlush())
''
'' When calling astAdd(), care must be taken not to destroy any temporaries
'' too early. After parsing an expression (e.g. with cExpression()), the first
'' astAdd() will flush the AST dtor list and add the code for destroying the
'' temp vars used in that expression. Thus, the expression itself *must* be
'' added in this first astAdd(). Otherwise, if the expression is added during a
'' secondary astAdd(), the temp var clean up would occur in front of the
'' expression, which is wrong (it must be emitted behind the expression).
''
'' If there is a statement (or multiple ones) which need to be added in front of
'' the one using the expression, they must all be linked together using
'' astNewLINK(), such that there is only one astAdd, to avoid the dtor list
'' being flushed too early.
''
'' Sometimes it's not obvious where the astAdd's are because some astAdd's
'' are "hidden" inside other functions such as astScopeBegin().
''
'' astAddUnscoped() does not have this problem because it unsets
'' ast.flushdtorlist.
''
function astAdd( byval n as ASTNODE ptr ) as ASTNODE ptr
	if( source_warning_proc <> NULL andalso _
	    env.clopt.warninglevel <= FB_WARNINGMSGS_DEFAULT_LEVEL ) then
		if( hSourceNullUnknownEffects(n) = FALSE ) then hWarnSourceNullAccess(n)
	end if
	n = astUpdate( n )
	if( n = NULL ) then
		return NULL
	end if

	'' Link the tree into the procedure's statement list
	if( ast.proc.curr->r <> NULL ) then
		ast.proc.curr->r->next = n
	else
		ast.proc.curr->l = n
	end if
	n->prev = ast.proc.curr->r
	n->next = NULL
	ast.proc.curr->r = n

	function = n
end function

'' -eassert can enable this validation independently of debug code generation.
private function hNodeIsFromCurrentProc( byval n as ASTNODE ptr ) as integer
	dim as ASTNODE ptr i = any
	function = FALSE

	i = ast.proc.curr->l
	while( i )

		if( i = n ) then
			function = TRUE
			exit while
		end if

		i = i->next
	wend
end function

function astAddAfter _
	( _
		byval n as ASTNODE ptr, _
		byval ref as ASTNODE ptr _
	) as ASTNODE ptr

	n = astUpdate( n )
	if( n = NULL ) then
		return NULL
	end if

	if( ref ) then
		assert( hNodeIsFromCurrentProc( ref ) )

		'' Insert behind this reference node in the current procedure
		n->prev = ref
		n->next = ref->next
		if( ref->next ) then
			if( ref->next->prev ) then
				ref->next->prev = n
			end if
		else
			assert( ast.proc.curr->r = ref )
			ast.proc.curr->r = n
		end if
		ref->next = n
	else
		'' Insert at the top of the current procedure
		n->prev = NULL
		n->next = ast.proc.curr->l
		if( ast.proc.curr->l ) then
			ast.proc.curr->l->prev = n
		else
			assert( ast.proc.curr->r = NULL )
			ast.proc.curr->r = n
		end if
		ast.proc.curr->l = n
	end if

	function = n
end function

''::::
sub astAddUnscoped _
	( _
		byval n as ASTNODE ptr _
	)

	dim as ASTNODE ptr last = any

	if( n = NULL ) then
		exit sub
	end if

	last = ast.proc.curr->block.proc.decl_last
	if( last = NULL ) then
		last = ast.proc.curr->l
	end if

	ast.flushdtorlist = FALSE

	if( last = NULL ) then
		n = astAdd( n )
	else
		n = astAddAfter( n, last )
	end if

	ast.flushdtorlist = TRUE

	ast.proc.curr->block.proc.decl_last = n

end sub

function astFindFirstCode( byval proc as ASTNODE ptr ) as ASTNODE ptr
	dim as ASTNODE ptr i = any

	assert( proc->class = AST_NODECLASS_PROC )

	i = proc->l
	while( i )
		'' Skip over nodes that don't represent executable code
		select case( i->class )
		case AST_NODECLASS_NOP, AST_NODECLASS_LABEL, _
		     AST_NODECLASS_DECL, AST_NODECLASS_LIT, _
		     AST_NODECLASS_DATASTMT, AST_NODECLASS_DBG _

		case else
			exit while
		end select

		i = i->next
	wend

	function = i
end function

sub astProcBegin( byval sym as FBSYMBOL ptr, byval ismain as integer )
	dim as ASTNODE ptr n = any
	dim as integer enable_implicit_code = any

	n = hNewProcNode( )

	symbSymbTbInit( sym->proc.symtb, sym )

	symbProcAllocExt( sym )

	'' File name where the procedure body was found
	sym->proc.ext->dbg.incfile = env.inf.incfile

	ast.proc.curr = n
	ast.currblock = n

	parser.scope = iif( ismain, FB_MAINSCOPE, FB_MAINSCOPE+1 )
	parser.currproc = sym
	parser.currblock = sym

	symbNestBegin( sym, FALSE )

	'' add init and exit labels (see the note in the top,
	'' procs don't create an implicit scope block)
	n->block.initlabel = symbAddLabel( NULL )
	n->block.exitlabel = symbAddLabel( NULL, FB_SYMBOPT_NONE )

	n->sym = sym
	n->block.proc.ismain = ismain
	n->block.parent = NULL
	n->block.inistmt = parser.stmt.cnt
	n->block.breaklist.head = NULL
	n->block.breaklist.tail = NULL
	n->block.proc.decl_last = NULL
	hSourceWarningsBegin(sym)

	irProcBegin( sym )

	enable_implicit_code = not symbIsNaked( sym )

	' Don't allocate anything for a naked function, because they will be allowed
	' at ebp-N, which won't exist, no result is needed either
	if( enable_implicit_code ) then
		'' alloc parameters
		hDeclVarsForProcParams( sym )
		if( ismain = FALSE ) then
			var param = symbGetProcHeadParam(sym)
			while( param <> NULL )
				if( symbGetName(param) <> NULL and symbIsInstanceParam(param) = FALSE ) then
					astWarnDeclareVariable(param->param.var)
				end if
				param = param->next
			wend
		end if

		'' alloc result local var
		if( symbGetType( sym ) <> FB_DATATYPE_VOID ) then
			symbAddProcResultVar( sym )
		end if
	end if

	'' local error handler
	with sym->proc.ext->err
		.lasthnd = NULL
		.lastmod = NULL
		.lastfun = NULL
		.hndsaved = NULL
	end with

	sym->proc.ext->stmtnum = parser.stmt.cnt
	fbSemanticModelProcedureState(sym, "begin")

	'' main()?
	if( symbGetIsMainProc( sym ) ) then
		dim as FBSYMBOL ptr argc = any, argv = any

		assert( symbGetProcParams( sym ) = 2 )
		argc = symbGetProcHeadParam( sym )
		argv = symbGetProcTailParam( sym )

		'' fb_Init( argc, argv )
		'' (plus some other calls depending on -exx etc.)
		env.main.initnode = rtlInitApp( _
			astNewVAR( symbGetParamVar( argc ) ), _
			astNewVAR( symbGetParamVar( argv ) ) )

	'' Destructor?
	elseif( (symbIsDestructor0( sym ) or symbIsDestructor1( sym )) and enable_implicit_code ) then
		''
		'' If the UDT has a vptr, reset it at the top of destructors,
		'' such that the vptr always matches the type of object that
		'' we're destructing, as in C++.
		''
		'' For example:
		''    type A extends object
		''    type B extends A
		''
		'' When destroying a B object, the body of B.destructor() runs
		'' before the body of A.destructor() (B.destructor() actually
		'' calls A.destructor() at its end before returning). This means
		'' the B part of the object is destroyed before the A part.
		''
		'' Thus, it's not safe to allow virtual calls from inside
		'' A.destructor() to any of B's methods, and this is prevented
		'' by resetting the vptr at the top of each destructor.
		''
		astAdd( hInitVptr( symbGetNamespace( sym ), sym ) )
	end if

	'' Label at beginning of lexical block, used by debug stabs output
	astAdd( astNewLABEL( n->block.initlabel ) )
end sub

'':::::
private function hCheckErrHnd _
	( _
		byval head_node as ASTNODE ptr, _
		byval sym as FBSYMBOL ptr _
	) as ASTNODE ptr

	'' error check? add to head (must be done only when closing the proc body
	'' or constructor's field would be initialized and break ctor chaining)
	if( env.clopt.errlocation ) then
		var save_owner = sym
		if( (env.clopt.backend = FB_BACKEND_GCC or env.clopt.backend = FB_BACKEND_CLANG) andalso _
		    (symbIsNaked(sym) = FALSE) andalso (env.clopt.debuginfo = FALSE) ) then
			'' A body that cannot return never restores its caller's location.
			'' Keep both runtime updates, but do not create unread save slots.
			'' The same label/assembler-aware proof governs C flow emission.
			if( astProcMayReturn(astGetProc()) = FALSE ) then save_owner = NULL
		end if
		head_node = astAddAfter( rtlErrorSetModName( save_owner, _
		                         astNewCONSTstr( @env.inf.name ) ), head_node )

		head_node = astAddAfter( rtlErrorSetFuncName( save_owner, _
		                         astNewCONSTstr( symbGetName( sym ) ) ), head_node )
	end if

	fbSemanticModelProcedureState(sym, "before-error-exit")
	with sym->proc.ext->err
		if( .lastfun <> NULL ) then
			astAdd( rtlErrorSetFuncName( NULL, astNewVAR( .lastfun ) ) )
			.lastfun = NULL
		end if

		if( .lastmod <> NULL ) then
			astAdd( rtlErrorSetModName( NULL, astNewVAR( .lastmod ) ) )
			.lastmod = NULL
		end if

		if( .lasthnd <> NULL ) then
			'' ON LOCAL ERROR may be bypassed by an early exit or branch.
			'' Initialize at procedure entry, then restore only if one of
			'' those statements actually saved the previous handler.
			symbSetIsDeclared( .lasthnd )
			symbSetIsDeclared( .hndsaved )
			head_node = astAddAfter( astNewDECL( .lasthnd, FALSE ), head_node )
			head_node = astAddAfter( astNewDECL( .hndsaved, FALSE ), head_node )
			head_node = astAddAfter( astNewASSIGN( astNewVAR( .lasthnd ), _
				astNewCONSTi( NULL, typeAddrOf( FB_DATATYPE_VOID ) ) ), head_node )
			head_node = astAddAfter( astNewASSIGN( astNewVAR( .hndsaved ), _
				astNewCONSTi( FALSE ) ), head_node )
			dim as FBSYMBOL ptr skiprestore = symbAddLabel( NULL )
			astAdd( astNewBOP( AST_OP_EQ, astNewVAR( .hndsaved ), _
				astNewCONSTi( FALSE ), skiprestore, AST_OPOPT_NONE ) )
			rtlErrorSetHandler( astNewVAR( .lasthnd ), FALSE )
			astAdd( astNewLABEL( skiprestore ) )
			.lasthnd = NULL
			.hndsaved = NULL
		end if
	end with

	if( sym->proc.ext->stats and FB_PROCSTATS_NATIVEERROROWNER ) then
		rtlErrorNativeOwnerLeave()
	end if

	fbSemanticModelProcedureState(sym, "after-error-exit")

	function = head_node

end function

private function hMaybeCallResultCtor _
	( _
		byval head_node as ASTNODE ptr, _
		byval sym as FBSYMBOL ptr _
	) as ASTNODE ptr

	dim as FBSYMBOL ptr res = any, defctor = any

	'' Not returning BYVAL, or BYVAL but not an UDT?
	if( symbIsReturnByRef( sym ) or (symbGetType( sym ) <> FB_DATATYPE_STRUCT) ) then
		return head_node
	end if

	'' Add result ctor call to the top of the function,
	''    a) if FUNCTION= (and/or EXIT FUNCTION) was used,
	''    b) or if neither FUNCTION= nor RETURN was used,
	'' but not if RETURN was used, because that already calls the copy
	'' ctor at every RETURN.
	''
	'' This way the result will be constructed properly,
	'' even if nothing was explicitly returned.

	'' only RETURN used?
	if( (not symbGetProcStatAssignUsed( sym )) and symbGetProcStatReturnUsed( sym ) ) then
		return head_node
	end if

	'' UDT with default ctor? (if there is none, nothing needs to be done)
	defctor = symbGetCompDefCtor( symbGetSubtype( sym ) )
	if( defctor = NULL ) then
		'' No default ctor, but others? Must show an error, because we
		'' cannot leave the result unconstructed. It would be nicer to
		'' detect & show this error at the top of the function already,
		'' but that's not possible because of RETURN which doesn't
		'' require a defctor...
		if( symbHasCtor( sym ) ) then
			errReport( FB_ERRMSG_RESULTHASNODEFCTOR )
		end if
		return head_node
	end if

	res = symbGetProcResult( sym )
	if( res = NULL ) then
		return head_node
	end if

	dim as longint semantic_operation = fbSemanticModelObjectOperationBegin("result-initialization", symbGetSubtype(sym), res, defctor)
	dim as ASTNODE ptr semantic_tree = astAddAfter( astBuildCtorCall( symbGetSubtype( sym ), _
	                                          astBuildProcResultVar( sym, res ) ), head_node )
	fbSemanticModelObjectOperationEnd(semantic_operation, semantic_tree, iif(semantic_tree = NULL, "empty", "built"))
	function = semantic_tree
end function

'':::::
private function hCallProfiler _
	( _
		byval head_node as ASTNODE ptr, _
		byval sym as FBSYMBOL ptr _
	) as ASTNODE ptr

	select case fbGetOption( FB_COMPOPT_PROFILE )
	case FB_PROFILE_OPT_GMON
		select case fbGetOption( FB_COMPOPT_BACKEND )
		case FB_BACKEND_GCC, FB_BACKEND_CLANG
			'' _mcount call is inserted by the C compiler
			exit select
		case else
			'' On all ports except dos _mcount() is just a normal call
			if( env.clopt.target <> FB_COMPTARGET_DOS ) then
				head_node = astAddAfter( rtlProfileCall_mcount(), head_node )
			end if
		end select
	case FB_PROFILE_OPT_CALLS
		if( env.opt.procprofile ) then
			dim as FBSYMBOL ptr procinfosym = any

			'' dim as const any ptr <PROCINFOCTX>
			procinfosym = symbAddImplicitVar( typeAddrOf( FB_DATATYPE_VOID ), NULL, FB_SYMBOPT_UNSCOPE )
			symbSetIsDeclared( procinfosym )
			head_node = astAddAfter( astNewDECL( procinfosym, FALSE ), head_node )

			'' <PROCINFOCTX> = fb_ProfileBeginProc( <PROCNAME> )
			head_node = astAddAfter( astNewAssign( astNewVar( procinfosym ), rtlProfileBeginProc( sym ) ), head_node )

			'' fb_ProfileEndProc( <PROCINFOCTX> )
			astAdd( rtlProfileEndProc( procinfosym ) )
		end if
	end select

	function = head_node

end function

function astProcEnd( byval callrtexit as integer ) as integer
	static as integer rec_cnt = 0
	dim as integer res = any, do_flush = any, enable_implicit_code = any
	dim as FBSYMBOL ptr sym = any
	dim as ASTNODE ptr n = any

	n = ast.proc.curr
	rec_cnt += 1
	sym = n->sym
	n->block.endstmt = parser.stmt.cnt
	hSourceWarningsEnd(n)

	'' No implicit code should be added for naked functions -- i.e. no stack
	'' frame setup, no function result, no error checking, no profiling.
	'' (Any calls would "trash" the stack)
	'' No need to worry about any "explicit" code though (any statements),
	'' including local variables and possibly resulting destructor calls;
	'' they are "the coders fault", not ours.
	enable_implicit_code = not symbIsNaked( sym )

	if( errGetCount( ) = 0 ) then
		'' Constructor?
		if( symbIsConstructor( sym ) and enable_implicit_code ) then
			'' No constructor initialization code yet? (constructor chaining)
			if( symbGetIsCtorInited( sym ) = FALSE ) then
				symbSetIsCtorInited( sym )
				'' Add constructor initialization code
				hCallCtors( n, sym )
			end if
		end if

		if( enable_implicit_code andalso (env.clopt.backend = FB_BACKEND_GCC or env.clopt.backend = FB_BACKEND_CLANG) ) then
			'' Only a selected handler needs a transfer destination. An extra
			'' label would otherwise retain an empty module initializer/finalizer.
			if( irhlcErrorNeedsCleanupBoundary(n->l) ) then
				var cleanup_label = symbAddLabel(NULL)
				astAdd(astNewLABEL(cleanup_label))
				irhlcErrorSetCleanupBoundary(sym, cleanup_label)
			end if
		end if
		'' Retire abandoned C callees before the native owner's local variables.
		'' Keep its handler registered until the existing final exit hook.
		if( enable_implicit_code andalso (sym->proc.ext->stats and FB_PROCSTATS_NATIVEERROROWNER) ) then
			astAdd( rtlErrorNativeOwnerCleanup() )
		end if
		astScopeDestroyVars(symbGetProcSymbTb(sym).tail, sym)
	end if

	''
	astAdd( astNewLABEL( n->block.exitlabel ) )

	'' Check for any undefined labels (labels can be forward references)
	res = (symbCheckLabels(symbGetProcSymbTbHead(parser.currproc)) = 0)

	if( res ) then
		'' Complete Destructor? (D1)
		if( symbIsDestructor1( sym ) and enable_implicit_code ) then
			'' Call destructors, behind the exit label, so they'll
			'' always be called, even with early returns.
			hCallDtors( sym )
		end if

		'' Deleting Destructor? (D0)
		if( symbIsDestructor0( sym ) and enable_implicit_code ) then
			hCallDeleteDtor( sym )
		end if

		'' update proc's breaks list, adding calls to destructors when needed
		if( n->block.breaklist.head <> NULL ) then
			res = astScopeUpdBreakList( n )
		end if

		'' gosub used?
		astGosubAddExit( sym )

		dim as ASTNODE ptr head_node = n->l

		if( enable_implicit_code ) then
			head_node = hCallProfiler( head_node, sym )
			head_node = hCheckErrHnd( head_node, sym )
		end if

		'' if main(), END 0 must be called because it's not safe to return to crt if
		'' an ON ERROR module-level handler was called while inside some proc
		if( callrtexit ) then
			if( n->block.proc.ismain ) then
				rtlExitApp( NULL )
			end if
		end if

		if( enable_implicit_code ) then
			'' if it's a function, load result
			if( symbGetType( sym ) <> FB_DATATYPE_VOID ) then
				head_node = hMaybeCallResultCtor( head_node, sym )
				hLoadProcResult( sym )
			end if
		end if
	end if

	''
	irProcEnd( sym )

	do_flush = FALSE
	if( res and (errGetCount( ) = 0) ) then
		symbSetIsParsed( sym )

		'' not into a recursion?
		if( rec_cnt = 1 ) then
			if( n->block.proc.ismain = FALSE ) then
				''
				'' Assembly/LLVM can emit public procedures immediately.
				''
				'' Emitting of private ones is delayed until hProcFlushAll(),
				'' so that they're only emitted if actually used.
				''
				'' C bodies wait for translation-unit non-returning contracts.
				'' LLVM still delays signatures containing forward references,
				'' because they cannot be emitted with incomplete types.
				''
				if( (env.clopt.backend <> FB_BACKEND_GCC) and _
				    (env.clopt.backend <> FB_BACKEND_CLANG) and _
				    (not symbIsPrivate( sym )) and _
				    ((not symbProcHasFwdRefInSignature( sym )) or _
				     (env.clopt.backend = FB_BACKEND_GAS)) ) then
					do_flush = TRUE

				'' remove from hash tb only
				else
					symbDelSymbolTb( @sym->proc.symtb, TRUE )
				end if

			'' main? flush all remaining, it's the latest
			else
				do_flush = TRUE
			end if
		end if

	'' errors.. remove everything from hash and symbol tb
	else
		symbDelSymbolTb( @sym->proc.symtb, FALSE )
	end if

	''
	symbNestEnd( FALSE )

	if( env.clopt.vectorize >= FB_VECTORIZE_NORMAL ) then
		'' only gas + SSE will expect AST_OP_HADD or AST_OP_SWZREP
		if( env.clopt.backend = FB_BACKEND_GAS ) then
			astProcVectorize( n->l )
		end if
	end if

	''
	if( do_flush ) then
		if( n->block.proc.ismain = FALSE ) then
			hProcFlush( n, TRUE )
		else
			hProcFlushAll( )
		end if
	end if

	'' back to main (or NULL if main was emitted already)
	ast.proc.curr = ast.proc.head
	ast.currblock = ast.proc.head

	parser.scope = FB_MAINSCOPE
	parser.currproc = env.main.proc
	parser.currblock = env.main.proc

	''
	rec_cnt -= 1

	function = res

end function

private function hDeclVarsForProcParams( byval proc as FBSYMBOL ptr ) as integer
	dim as integer i = any
	dim as FBSYMBOL ptr p = any

	function = FALSE

	'' Allocate the hidden result parameter for stack-returned values.
	if( symbProcReturnsOnStack( proc ) ) then
		symbAddVarForProcResultParam( proc )
	end if

	'' Param vars must be declared in the order they should be allocated
	'' on stack, i.e. depending on the calling convention
	'' (see also irProcAllocArg())
	i = 1
	p = symbGetProcLastParam( proc )
	do while( p <> NULL )
		if( p->param.mode <> FB_PARAMMODE_VARARG ) then
			p->param.var = symbAddVarForParam( p )
			if( p->param.var = NULL ) then
				errReportParam( proc, i, NULL, FB_ERRMSG_DUPDEFINITION )
				exit function
			end if
		end if

		p = symbGetProcPrevParam( proc, p )
		i += 1
	loop

	function = TRUE
end function

private sub hLoadProcResult( byval proc as FBSYMBOL ptr )
	dim as FBSYMBOL ptr s = any
	dim as ASTNODE ptr n = any

	s = symbGetProcResult( proc )

	if( symbProcReturnsOnStack( proc ) andalso _
	    typeIsInteger128Scalar( symbGetFullType( proc ) ) ) then
		'' The function body wrote the result through its hidden destination.
		'' No scalar return registers are used by this assembly ABI.
		exit sub
	end if

	'' if result is a string, a temp descriptor is needed, as the current one (on stack)
	'' will be trashed when the function returns (also, the string returned will be
	'' set as temp, so any assignment or when passed as parameter to another proc
	'' will deallocate this string)
	if( (typeIsDynamicString( symbGetType( proc ) )) and (not symbIsReturnByRef( proc )) ) then
		n = rtlStrAllocTempResult( astNewVAR( s ) )

		select case env.clopt.backend
		case FB_BACKEND_GCC, FB_BACKEND_CLANG, FB_BACKEND_LLVM, FB_BACKEND_GAS64
			n = astNewLOAD( n, symbGetFullType( proc ), TRUE )
		case FB_BACKEND_GAS
			select case fbGetOption( FB_COMPOPT_PROFILE )
			case FB_PROFILE_OPT_CALLS
				n = astNewLOAD( n, symbGetFullType( proc ), TRUE )
			end select
		end select
	else
		'' Use the real type, in case it's BYREF return or a UDT result
		n = astNewLOAD( astNewVAR( s, 0, symbGetProcRealType( proc ), _
		                symbGetProcRealSubtype( proc ) ), _
		                symbGetProcRealType( proc ), TRUE )
	end if

	astAdd( n )
end sub

private function hModLevelIsEmpty( byval p as ASTNODE ptr ) as integer
	dim as ASTNODE ptr n = any

	'' an empty module-level proc will have just the
	'' initial and final labels as nodes and nothing else
	'' (note: when debugging it will be emitted even if empty)

	n = p->l
	if( n = NULL ) then
		return TRUE
	end if
	if( n->class <> AST_NODECLASS_LABEL ) then
		return FALSE
	end if

	n = n->next
	if( n = NULL ) then
		return TRUE
	end if
	if( n->class <> AST_NODECLASS_LABEL ) then
		return FALSE
	end if

	n = n->next
	if( n = NULL ) then
		return TRUE
	end if

	return FALSE
end function

private function hCallCtorList _
	( _
		byval is_ctor as integer, _
		byval this_ as FBSYMBOL ptr, _
		byval fld as FBSYMBOL ptr _
	) as ASTNODE ptr

	dim as FBSYMBOL ptr cnt = any, label = any, iter = any, subtype = any
	dim as ASTNODE ptr fldexpr = any, tree = any
	dim as integer dtype = any
	dim as longint elements = any

	'' instance? (this function is also used by the static dtor wrapper)
	if( fld <> NULL ) then
		dtype = symbGetType( fld )
		subtype = symbGetSubtype( fld )
		elements = symbGetArrayElements( fld )
	else
		dtype = symbGetType( this_ )
		subtype = symbGetSubtype( this_ )
		elements = symbGetArrayElements( this_ )
	end if

	assert( elements > 0 )
	cnt = symbAddTempVar( FB_DATATYPE_INTEGER )
	label = symbAddLabel( NULL )
	iter = symbAddTempVar( typeAddrOf( dtype ), subtype )

	'' Instance?
	if( fld <> NULL ) then
		if( is_ctor ) then
			'' iter = @this.field(0)
			fldexpr = astBuildVarField( this_, fld )
		else
			'' iter = @this.field(elements-1)
			fldexpr = astBuildVarField( this_, fld, (elements - 1) * symbGetSizeOf( fld ) )
		end if
	else
		if( is_ctor ) then
			'' iter = @symbol(0)
			fldexpr = astBuildVarField( this_, NULL, 0 )
		else
			'' iter = @symbol(0) + (elements - 1)
			fldexpr = astBuildVarField( this_, NULL, (elements - 1) * symbGetSizeOf( subtype ) )
		end if
	end if
	tree = astBuildVarAssign( iter, astNewADDROF( fldexpr ), AST_OPOPT_ISINI )

	'' for cnt = 0 to symbGetArrayElements( fld )-1
	tree = astBuildForBegin( tree, cnt, label, 0 )

	if( is_ctor ) then
		'' ctor( *iter )
		tree = astNewLINK( tree, astBuildCtorCall( subtype, astBuildVarDeref( iter ) ), AST_LINK_RETURN_NONE )
	else
		'' dtor( *iter )
		tree = astNewLINK( tree, astBuildDtorCall( subtype, astBuildVarDeref( iter ) ), AST_LINK_RETURN_NONE )
	end if

	'' iter += 1
	tree = astNewLINK( tree, astBuildVarInc( iter, iif( is_ctor, 1, -1 ) ), AST_LINK_RETURN_NONE )

	'' next
	tree = astBuildForEnd( tree, cnt, label, astNewCONSTi( elements ) )

	function = tree
end function

private function hCallFieldCtor _
	( _
		byval this_ as FBSYMBOL ptr, _
		byval fld as FBSYMBOL ptr _
	) as ASTNODE ptr

	assert( symbIsDescriptor( fld ) = FALSE )  '' should have had an initree
	assert( symbIsRef( fld ) = FALSE )  '' reference? ditto

	'' Fake dynamic array field that didn't have an initree - nothing to do
	if( symbIsDynamic( fld ) ) then
		assert( symbGetTypeIniTree( fld ) = NULL )
		fbSemanticModelObjectOperationChoice("field-init", "fake-dynamic-skip")
		exit function
	end if

	'' Do not initialize?
	if( symbGetDontInit( fld ) ) then
		fbSemanticModelObjectOperationChoice("field-init", "no-initialize")
		exit function
	end if

	'' has a default ctor too?
	if( symbHasDefCtor( fld ) ) then
		'' not an array?
		if( symbGetArrayDimensions( fld ) = 0 ) then
			fbSemanticModelObjectOperationChoice("field-init", "default-constructor")
			'' ctor( this.field )
			function = astBuildCtorCall( symbGetSubtype( fld ), astBuildVarField( this_, fld ) )
		'' array..
		else
			fbSemanticModelObjectOperationChoice("field-init", "default-constructor-array")
			function = hCallCtorList( TRUE, this_, fld )
		end if
		exit function
	end if

	'' bitfield?
	if( symbFieldIsBitfield( fld ) ) then
		fbSemanticModelObjectOperationChoice("field-init", "zero-bitfield")
		if( fld->var_.bitpos = 0 ) then
			'' Start with a defined storage unit before later bitfield writes
			'' read and preserve the other fields in that unit.
			function = astNewMEM( AST_OP_MEMFILL, _
			                      astBuildVarField( this_, NULL, symbGetOfs( fld ) ), _
			                      astNewCONSTi( typeGetSize( symbGetFullType( fld ) ) ) )
		else
			function = astNewASSIGN( astBuildVarField( this_, fld ), _
			                         astNewCONSTi( 0, FB_DATATYPE_UINT ), _
			                         AST_OPOPT_ISINI )
		end if
	else
		fbSemanticModelObjectOperationChoice("field-init", "zero-storage")
		function = astNewMEM( AST_OP_MEMFILL, _
		                      astBuildVarField( this_, fld ), _
		                      astNewCONSTi( symbGetRealSize( fld ) ), _
		                      0, iif( (symbGetType( fld ) = FB_DATATYPE_FIXSTR), 32, 0 ) )
	end if
end function

private function hClearUnionFields _
	( _
		byval this_ as FBSYMBOL ptr, _
		byval base_fld as FBSYMBOL ptr, _
		byval pfinalfield as FBSYMBOL ptr ptr _
	) as ASTNODE ptr

	dim as FBSYMBOL ptr fld = any
	dim as longint bytes = any, lgt = any, base_ofs = any

	'' merge all union fields
	fld = base_fld
	bytes = 0
	base_ofs = symbGetOfs( base_fld )

	do
		fbSemanticModelObjectOperationSymbolChoice("union-member", fld)
		lgt = symbGetRealSize( fld ) + (symbGetOfs( fld ) - base_ofs)
		if( lgt > bytes ) then
			bytes = lgt
		end if

		'' Visit following union fields (but not any methods/static member vars/etc.)
		fld = fld->next
	loop while( fld andalso symbIsField( fld ) andalso symbGetIsUnionField( fld ) )

	*pfinalfield = fld
	fbSemanticModelObjectOperationNumberChoice("union-bytes", bytes)

	'' clear all them at once
	function = astNewMEM( AST_OP_MEMFILL, _
	                      astBuildVarField( this_, base_fld ), _
	                      astNewCONSTi( bytes ) )
end function

private function hInitDynamicArrayField _
	( _
		byval this_ as FBSYMBOL ptr, _
		byval fld as FBSYMBOL ptr _
	) as ASTNODE ptr

	dim as ASTNODE ptr exprTB(0 to FB_MAXARRAYDIMS-1, 0 to 1)
	dim as ASTNODE ptr boundstypeini = any, n = any
	dim as integer dimensions = any

	'' Duplicate the expressions and its temp vars into the current scope
	boundstypeini = astTypeIniClone( symbGetTypeIniTree( fld ) )

	'' Fill the exprTB() with the bounds expressions from the TYPEINI
	dimensions = 0
	assert( astIsTYPEINI( boundstypeini ) )
	n = boundstypeini->l
	do
		assert( dimensions < FB_MAXARRAYDIMS )

		'' lbound
		assert( n->class = AST_NODECLASS_TYPEINI_ASSIGN )
		exprTB(dimensions,0) = n->l
		n->l = NULL  '' so we can astDelTree() the TYPEINI without free'ing the bounds expressions

		'' ubound
		n = n->r
		assert( n->class = AST_NODECLASS_TYPEINI_ASSIGN )
		exprTB(dimensions,1) = n->l
		n->l = NULL  '' ditto

		n = n->r
		dimensions += 1
	loop while( n )

	assert( dimensions = symbGetArrayDimensions( fld ) )

	'' Delete the TYPEINI (but not the bounds expressions), no longer needed
	astDelTree( boundstypeini )

	'' Build the REDIM CALL with the bounds expressions
	function = rtlArrayRedim( astNewNIDXARRAY( astBuildVarField( this_, fld ) ), dimensions, exprTB(), FALSE, (not symbGetDontInit( fld )) )
end function

private function hCallFieldCtors _
	( _
		byval parent as FBSYMBOL ptr, _
		byval proc as FBSYMBOL ptr _
	) as ASTNODE ptr

	dim as FBSYMBOL ptr fld = any, this_ = any
	dim as ASTNODE ptr tree = any, semantic_tree = NULL
	dim as longint semantic_operation

	tree = NULL
	this_ = symbGetParamVar( symbGetProcHeadParam( proc ) )

	'' For all real fields, excluding...
	''  - other members like procedures,
	''  - the base field if any; it's initialized separately already
	''  - fake dynamic array fields
	fld = symbGetCompSymbTb( parent ).head
	while( fld )

		if( symbIsField( fld ) ) then
			if( fld <> parent->udt.base ) then
				'' part of an union?
				if( symbGetIsUnionField( fld ) ) then
					semantic_operation = fbSemanticModelObjectOperationBegin("union-initialization", parent, fld)
					semantic_tree = hClearUnionFields( this_, fld, @fld )
					fbSemanticModelObjectOperationEnd(semantic_operation, semantic_tree, "built")
					tree = astNewLINK( tree, semantic_tree, AST_LINK_RETURN_NONE )
					'' hClearUnionFields() already skipped to the next field behind the union
					continue while
				else
					'' not initialized?
					if( symbGetTypeIniTree( fld ) = NULL ) then
						semantic_operation = fbSemanticModelObjectOperationBegin("field-initialization", parent, fld, NULL, 0)
						semantic_tree = hCallFieldCtor( this_, fld )
					elseif( symbIsDynamic( fld ) ) then
						semantic_operation = fbSemanticModelObjectOperationBegin("field-initialization", parent, fld, NULL, 1)
						semantic_tree = hInitDynamicArrayField( this_, fld )
					else
						'' Note: flushing the field's TYPEINI against the whole "THIS" instance,
						'' not against "THIS.thefield", because the TYPEINI contains absolute offsets.
						semantic_operation = fbSemanticModelObjectOperationBegin("field-initialization", parent, fld, NULL, 2)
						semantic_tree = astTypeIniFlush( astBuildVarField( this_ ), _
						                   astTypeIniClone( symbGetTypeIniTree( fld ) ), FALSE, AST_OPOPT_ISINI )
					end if
					fbSemanticModelObjectOperationEnd(semantic_operation, semantic_tree, iif(semantic_tree = NULL, "empty", "built"))
					tree = astNewLINK( tree, semantic_tree, AST_LINK_RETURN_NONE )
				end if
			else
				semantic_operation = fbSemanticModelObjectOperationBegin("field-exclusion", parent, fld, NULL, 1)
				fbSemanticModelObjectOperationEnd(semantic_operation, NULL, "skipped")
			end if
		else
			semantic_operation = fbSemanticModelObjectOperationBegin("field-exclusion", parent, fld, NULL, 0)
			fbSemanticModelObjectOperationEnd(semantic_operation, NULL, "skipped")
		end if

		fld = fld->next
	wend

	function = tree
end function

private function hCallBaseCtor _
	( _
		byval parent as FBSYMBOL ptr, _
		byval proc as FBSYMBOL ptr _
	) as ASTNODE ptr

	dim as ASTNODE ptr initree = any
	dim as FBSYMBOL ptr base_ = any, this_ = any, subtype = any, defctor = any

	base_ = parent->udt.base
	dim as longint semantic_operation = fbSemanticModelObjectOperationBegin("base-initialization", parent, base_)

	'' No base UDT? Then there's nothing to do.
	if( base_ = NULL ) then
		fbSemanticModelObjectOperationChoice("base-init", "no-base")
		fbSemanticModelObjectOperationEnd(semantic_operation, NULL, "empty")
		exit function
	end if

	this_ = symbGetParamVar( symbGetProcHeadParam( proc ) )

	'' Do we have a BASE() ctorcall/initializer?
	initree = proc->proc.ext->base_initree
	if( initree ) then
		proc->proc.ext->base_initree = NULL
		fbSemanticModelObjectOperationChoice("base-init", "explicit")
		dim as ASTNODE ptr semantic_tree = astTypeIniFlush( astBuildVarField( this_ ), initree, FALSE, AST_OPOPT_ISINI )
		fbSemanticModelObjectOperationEnd(semantic_operation, semantic_tree, iif(semantic_tree = NULL, "empty", "built"))
		return semantic_tree
	end if

	subtype = symbGetSubtype( base_ )
	defctor = symbGetCompDefCtor( subtype )

	'' Otherwise, try to call a default ctor, if any
	if( defctor ) then
		fbSemanticModelObjectOperationChoice("base-init", "default-constructor")
		'' Check access here, because (unlike fields) it's not done
		'' during the TYPE compound parsing
		'' Check visibility of the default construtor
		if( symbCheckAccess( defctor ) = FALSE ) then
			errReport( FB_ERRMSG_NOACCESSTOBASEDEFCTOR )
		end if
	'' No default ctor, but others? Then BASE() should have been used,
	'' since a ctor must be called, but we cannot do it automatically.
	elseif( symbGetCompCtorHead( subtype ) ) then
		fbSemanticModelObjectOperationChoice("base-init", "missing-required-constructor")
		errReport( FB_ERRMSG_NOBASEINIT )
	else
		fbSemanticModelObjectOperationChoice("base-init", "plain-storage")
	end if

	dim as ASTNODE ptr semantic_tree = hCallFieldCtor( this_, base_ )
	fbSemanticModelObjectOperationEnd(semantic_operation, semantic_tree, iif(semantic_tree = NULL, "empty", "built"))
	function = semantic_tree
end function

private function hInitVptr _
	( _
		byval parent as FBSYMBOL ptr, _
		byval proc as FBSYMBOL ptr _
	) as ASTNODE ptr

	dim as longint semantic_operation = fbSemanticModelObjectOperationBegin("vptr-initialization", parent)

	'' Only if there is a vptr
	if( symbGetHasRTTI( parent ) = FALSE ) then
		fbSemanticModelObjectOperationEnd(semantic_operation, NULL, "empty")
		exit function
	end if

	'' The vtable must be present/known for this
	assert( parent->udt.ext->vtable )
	fbSemanticModelObjectOperationSymbolChoice("vptr-table", parent->udt.ext->vtable)

	var this_ = symbGetParamVar( symbGetProcHeadParam( proc ) )

	'' this.vptr = cast( any ptr, (cast(byte ptr, @vtable) + sizeof(void *) * 2) )
	'' assuming that everything with a vptr extends fb_Object
	dim as ASTNODE ptr semantic_tree = astNewASSIGN( _
		astBuildVarField( this_, symbUdtGetFirstField( symb.rtti.fb_object ) ), _
		                  astNewCONV( typeAddrOf( FB_DATATYPE_VOID ), NULL, _
		                  astNewADDROF( astNewVAR( parent->udt.ext->vtable, env.pointersize * 2 ) ) ), _
		AST_OPOPT_ISINI )
	fbSemanticModelObjectOperationEnd(semantic_operation, semantic_tree, "built")
	function = semantic_tree

end function

private sub hCallCtors( byval n as ASTNODE ptr, byval sym as FBSYMBOL ptr )
	dim as ASTNODE ptr tree = any
	dim as FBSYMBOL ptr parent = any

	parent = symbGetNamespace( sym )
	dim as longint semantic_operation = fbSemanticModelObjectOperationBegin("constructor-initialization", parent)

	'' 1. base ctor (will set vtable ptr according to base)
	tree = hCallBaseCtor( parent, sym )

	'' 2. fields (each field will be constructed, or initialized, or cleared unless =ANY was used on it)
	tree = astNewLINK( tree, hCallFieldCtors( parent, sym ), AST_LINK_RETURN_NONE )

	'' 3. vtable ptr (to point to this class's vtable instead of the base's)
	tree = astNewLINK( tree, hInitVptr( parent, sym ), AST_LINK_RETURN_NONE )

	'' Find the first statement that is executable code,
	'' and insert the constructor calls above it.
	n = astFindFirstCode( n )
	if( n ) then
		n = n->prev
	end if
	tree = astAddAfter( tree, n )
	fbSemanticModelObjectOperationEnd(semantic_operation, tree, iif(tree = NULL, "empty", "built"))
end sub

private function hBuildFieldDtor _
	( _
		byval this_ as FBSYMBOL ptr, _
		byval fld as FBSYMBOL ptr _
	) as ASTNODE ptr

	'' Keep one field-destruction builder for ordinary epilogues and private
	'' cleanup of an unfinished object. This builds a tree without appending
	'' it to the source procedure; the caller owns its placement and lifetime.
	dim as ASTNODE ptr tree = NULL
	assert( symbIsDescriptor( fld ) = FALSE )
	assert( symbIsRef( fld ) = FALSE )

	if( symbIsDynamic( fld ) ) then
		tree = rtlArrayErase( astNewNIDXARRAY( astBuildVarField( this_, fld ) ), TRUE, FALSE )
	elseif( symbGetArrayDimensions( fld ) = 0 ) then
		if( typeIsDynamicString( symbGetType( fld ) ) ) then
			tree = rtlStrDelete( astBuildVarField( this_, fld ) )
		elseif( symbHasDtor( fld ) ) then
			tree = astBuildDtorCall( symbGetSubtype( fld ), astBuildVarField( this_, fld ) )
		end if
	else
		if( typeIsDynamicString( symbGetType( fld ) ) ) then
			tree = rtlArrayErase( astNewNIDXARRAY( astBuildVarField( this_, fld ) ), FALSE, FALSE )
		elseif( symbHasDtor( fld ) ) then
			tree = hCallCtorList( FALSE, this_, fld )
		end if
	end if
	return tree
end function

function astBuildVarCleanupTree( byval resource as FBSYMBOL ptr ) as ASTNODE ptr
	'' Normalize a standalone variable retirement through the same statement
	'' preparation as ordinary astAdd, without appending it to the source body.
	return astUpdate(astBuildVarDtorCall(resource, FALSE))
end function

function astBuildFieldDtorTree _
	( _
		byval this_ as FBSYMBOL ptr, _
		byval fld as FBSYMBOL ptr _
	) as ASTNODE ptr

	'' astUpdate gives standalone calls VOID type and lowers descriptor
	'' initializers. It performs the same statement preparation as astAdd,
	'' without appending this cleanup to the already flushed source body.
	return astUpdate(hBuildFieldDtor(this_, fld))
end function

private sub hCallFieldDtor _
	( _
		byval this_ as FBSYMBOL ptr, _
		byval fld as FBSYMBOL ptr _
	)

	dim as longint semantic_operation = fbSemanticModelObjectOperationBegin("field-destruction", symbGetSubtype(this_), fld)
	dim as ASTNODE ptr semantic_tree = astAdd(hBuildFieldDtor(this_, fld))
	fbSemanticModelObjectOperationEnd(semantic_operation, semantic_tree, iif(semantic_tree = NULL, "empty", "built"))
end sub

private sub hCallFieldDtors _
	( _
		byval parent as FBSYMBOL ptr, _
		byval proc as FBSYMBOL ptr _
	)

	dim as FBSYMBOL ptr fld = any, this_ = any

	this_ = symbGetParamVar( symbGetProcHeadParam( proc ) )

	'' For each field (in inverse order)
	''  - excluding non-field members (e.g. methods)
	''  - excluding the base field, that will be destructed separately
	''  - dynamic array descriptor fields: hCallFieldDtor() frees dynamic
	''    array fields by calling ERASE on the fake dynamic array field,
	''    for which astNewARG() will automagically pass the descriptor.
	''  - excluding references (nothing to clean-up)
	fld = symbGetCompSymbTb( parent ).tail
	while( fld )

		if( symbIsField( fld ) ) then
			if( (not symbIsDescriptor( fld )) and (fld <> parent->udt.base) and (not symbIsRef( fld )) ) then
				hCallFieldDtor( this_, fld )
			end if
		end if

		fld = fld->prev
	wend

end sub

private sub hCallDeleteDtor _
	( _
		byval proc as FBSYMBOL ptr _
	)

	dim as FBSYMBOL ptr parent = any, dtor1 = any, this_ = any
	dim as ASTNODE ptr thisptr = any, tree = any

	parent = symbGetNamespace( proc )
	dim as longint semantic_operation = fbSemanticModelObjectOperationBegin("deleting-destruction", parent)
	dtor1 = symbGetCompDtor1( parent )
	if( dtor1 = NULL ) then
		fbSemanticModelObjectOperationEnd(semantic_operation, NULL, "empty")
		exit sub
	end if
	this_ = symbGetParamVar( symbGetProcHeadParam( proc ) )
	thisptr = astNewADDROF( astBuildVarField( this_ ) )
	tree = astBuildDeleteOp( AST_OP_DEL, thisptr, NULL )
	tree = astAdd( tree )
	fbSemanticModelObjectOperationEnd(semantic_operation, tree, iif(tree = NULL, "empty", "built"))

end sub

private sub hCallBaseDtor _
	( _
		byval parent as FBSYMBOL ptr, _
		byval proc as FBSYMBOL ptr _
	)

	dim as FBSYMBOL ptr base_ = any, dtor = any, this_ = any

	base_ = parent->udt.base
	dim as longint semantic_operation = fbSemanticModelObjectOperationBegin("base-destruction", parent, base_)

	'' No base UDT? Then there's nothing to do.
	if( base_ = NULL ) then
		fbSemanticModelObjectOperationEnd(semantic_operation, NULL, "empty")
		exit sub
	end if

	'' Call its dtor, if there is any.
	''
	'' Note: As in C++, the base class' destructor implementation is called
	'' from this derived class destructor, and no vtable lookup is done
	'' for this (it would just result in infinite recursion anyways).
	''
	'' Just like derived classes are not responsible for initializing their
	'' base class, they shouldn't be made responsible for cleaning it up.

	dtor = symbGetCompDtor1( symbGetSubtype( base_ ) )
	if( dtor = NULL ) then
		fbSemanticModelObjectOperationEnd(semantic_operation, NULL, "empty")
		exit sub
	end if

	'' Check access here, because (unlike fields) it's not done
	'' during the TYPE compound parsing
	'' Check visibility of the default destructor
	if( symbCheckAccess( dtor ) = FALSE ) then
		errReport( FB_ERRMSG_NOACCESSTOBASEDTOR )
	end if

	'' The only exception is if the base class' destructor is ABSTRACT,
	'' then there is no implementation to call.
	if( symbIsAbstract( dtor ) ) then
		fbSemanticModelObjectOperationEnd(semantic_operation, NULL, "empty")
		exit sub
	end if

	this_ = symbGetParamVar( symbGetProcHeadParam( proc ) )
	dim as ASTNODE ptr semantic_tree = astAdd( astBuildDtorCall( symbGetSubtype( base_ ), _
	        astBuildVarField( this_, base_ ), TRUE ) )
	fbSemanticModelObjectOperationEnd(semantic_operation, semantic_tree, iif(semantic_tree = NULL, "empty", "built"))
end sub

private sub hCallDtors( byval proc as FBSYMBOL ptr )
	dim as FBSYMBOL ptr parent = any

	parent = symbGetNamespace( proc )
	dim as longint semantic_operation = fbSemanticModelObjectOperationBegin("complete-destruction", parent)

	'' 1st) fields dtors
	hCallFieldDtors( parent, proc )

	'' 2nd) base dtor
	hCallBaseDtor( parent, proc )
	fbSemanticModelObjectOperationEnd(semantic_operation, NULL, "complete")

end sub

private sub hCallStaticCtor _
	( _
		byval sym as FBSYMBOL ptr, _
		byval initree as ASTNODE ptr _
	)

	astAdd( astTypeIniFlush( sym, astTypeIniClone( initree ), FALSE, AST_OPOPT_ISINI ) )
	astTypeIniDelete( initree )

end sub

private sub hCallStaticDtor( byval sym as FBSYMBOL ptr )
	assert( symbIsReturnByRef( sym ) = FALSE )

	'' dynamic?
	if( symbIsDynamic( sym ) ) then
		astAdd( rtlArrayErase( astNewNIDXARRAY( astBuildVarField( sym, NULL, 0 ) ), TRUE, FALSE ) )
	else
		'' not an array?
		if( symbGetArrayDimensions( sym ) = 0 ) then
			'' dtor( var )
			astAdd( astBuildDtorCall( symbGetSubtype( sym ), astBuildVarField( sym, NULL, 0 ) ) )
		else
			astAdd( hCallCtorList( FALSE, sym, NULL ) )
		end if
	end if
end sub

private sub hDiscardStaticInstancesDtors( byval proc as FBSYMBOL ptr )
	if( proc->proc.ext = NULL ) then exit sub
	var dtorlist = proc->proc.ext->statdtor
	if( dtorlist = NULL ) then exit sub

	'' Each wrapper has a unique compiler-generated name and is referenced
	'' only from its owning procedure. If that procedure is discarded, its
	'' wrapper has no C definition or remaining C reference.
	if( (env.clopt.backend = FB_BACKEND_GCC) or (env.clopt.backend = FB_BACKEND_CLANG) ) then
		var wrapper = cptr( FB_DTORWRAPPER ptr, listGetHead( dtorlist ) )
		while( wrapper <> NULL )
			wrapper->proc->stats and= not FB_SYMBSTATS_ACCESSED
			wrapper = listGetNext( wrapper )
		wend
	end if

	listEnd( dtorlist )
	deallocate( dtorlist )
	proc->proc.ext->statdtor = NULL
end sub

''::::
'' Generate wrappers only for procedures that will be emitted.
''::::
private sub hGenStaticInstancesDtors( byval proc as FBSYMBOL ptr )
	dim as TLIST ptr dtorlist = any
	dim as FB_DTORWRAPPER ptr wrap = any
	dim as ASTNODE ptr n = any

	dtorlist = proc->proc.ext->statdtor

	if( dtorlist = NULL ) then
		exit sub
	end if

	'' for each node..
	wrap = listGetHead( dtorlist )
	do while( wrap <> NULL )
		astProcBegin( wrap->proc, FALSE )
		n = ast.proc.curr

		'' call the dtor
		hCallStaticDtor( wrap->sym )

		astProcEnd( FALSE )

		'' must be flushed before the proc that has the static vars, because
		'' they will be removed from hash and symbols table right-after that
		'' proc is flushed
		fbSemanticModelStaticCleanupBody(wrap->sym, proc, wrap->proc)
		hProcFlush( n, TRUE )

		wrap = listGetNext( wrap )
	loop

	'' destroy list
	listEnd( dtorlist )
	deallocate( proc->proc.ext->statdtor )
	proc->proc.ext->statdtor = NULL
end sub

'':::::
function astProcAddStaticInstance _
	( _
		byval sym as FBSYMBOL ptr _
	) as FBSYMBOL ptr

	dim as TLIST ptr dtorlist = any
	dim as FB_DTORWRAPPER ptr wrap = any
	dim as FBSYMBOL ptr proc = any

	assert( symbIsReturnByRef( sym ) = FALSE )

	dtorlist = parser.currproc->proc.ext->statdtor

	'' create a new list
	if( dtorlist = NULL ) then
		dtorlist = xcallocate( len( TLIST ) )
		parser.currproc->proc.ext->statdtor = dtorlist

		listInit( dtorlist, 16, len( FB_DTORWRAPPER ), LIST_FLAGS_NOCLEAR )
	end if

	''
	wrap = listNewNode( dtorlist )

	proc = symbAddProc( symbPreAddProc( NULL ), symbUniqueLabel( ), NULL, FB_DATATYPE_VOID, NULL, _
	                    FB_SYMBATTRIB_PRIVATE, FB_PROCATTRIB_NONE, FB_FUNCMODE_CDECL, FB_SYMBOPT_DECLARING )

	wrap->proc = proc
	wrap->sym = sym

	'' can't be undefined
	symbSetCantUndef( sym )

	function = proc

end function

'':::::
sub astProcAddGlobalInstance _
	( _
		byval sym as FBSYMBOL ptr, _
		byval initree as ASTNODE ptr, _
		byval call_dtor as integer _
	)

	dim as FB_GLOBINSTANCE ptr wrap = any

	''
	wrap = listNewNode( @ast.globinst.list )

	wrap->sym = sym
	wrap->initree = initree
	wrap->call_dtor = call_dtor
	wrap->semantic_identity = fbSemanticModelGlobalInstance(sym, initree <> NULL, call_dtor)

	'' can't be undefined
	symbSetCantUndef( sym )

	if( initree <> NULL ) then
		ast.globinst.ctorcnt += 1
	end if

	if( call_dtor ) then
		ast.globinst.dtorcnt += 1
	end if

end sub

private sub hGlobCtorBegin( byval is_ctor as integer )
	dim as FBSYMBOL ptr proc = any

	'' sub ctorname|dtorname cdecl( ) constructor|destructor
	proc = symbAddProc( symbPreAddProc( NULL ), symbUniqueLabel( ), _
	                    iif( is_ctor, cptr( const zstring ptr, @FB_GLOBCTORNAME ), _
	                         cptr( const zstring ptr, @FB_GLOBDTORNAME ) ), _
	                    FB_DATATYPE_VOID, NULL, FB_SYMBATTRIB_PRIVATE, FB_PROCATTRIB_NONE, _
	                    FB_FUNCMODE_CDECL, FB_SYMBOPT_DECLARING )

	if( is_ctor ) then
		symbAddGlobalCtor( proc )
	else
		symbAddGlobalDtor( proc )
	end if
	symbSetIsAccessed( proc )
	symbSetIsParsed( proc )

	fbSemanticModelStartupWrapper(proc, iif(is_ctor, "constructor", "destructor"))
	astProcBegin( proc, FALSE )
end sub

private sub hGenGlobalInstancesCtor( )
	dim as FB_GLOBINSTANCE ptr inst = any
	dim as integer semantic_ordinal

	'' any global instance with ctors?
	if( ast.globinst.ctorcnt > 0 ) then
		'' sub ctor cdecl( ) constructor
		hGlobCtorBegin( TRUE )
		semantic_ordinal = 0

		'' for each node..
		inst = listGetHead( @ast.globinst.list )
		while( inst )
			'' has ctor/initializer?
			if( inst->initree <> NULL ) then
				hCallStaticCtor( inst->sym, inst->initree )
				inst->initree = NULL
				semantic_ordinal += 1
				fbSemanticModelGlobalInstanceAction(inst->semantic_identity, astGetProc( )->sym, "initialize", semantic_ordinal)
			end if

			inst = listGetNext( inst )
		wend

		'' end sub
		astProcEnd( FALSE )
	end if

	'' any global instance with dtors?
	if( ast.globinst.dtorcnt > 0 ) then
		'' sub dtor cdecl( ) destructor
		hGlobCtorBegin( FALSE )
		semantic_ordinal = 0

		'' for each node (in inverse order)..
		inst = listGetTail( @ast.globinst.list )
		while( inst )
			'' need to call dtor?
			if( inst->call_dtor ) then
				hCallStaticDtor( inst->sym )
				semantic_ordinal += 1
				fbSemanticModelGlobalInstanceAction(inst->semantic_identity, astGetProc( )->sym, "cleanup", semantic_ordinal)
			end if

			inst = listGetPrev( inst )
		wend

		'' end sub
		astProcEnd( FALSE )
	end if

	'' list will be deleted by astProcListEnd( )
end sub

'' end of ast/nodes/ast-node-proc.bas
