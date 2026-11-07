'' Project: FreeBASIC C error-owner lowering
'' File: backend/c/error-owner-code.bi
'' Purpose: Outline callable handlers while keeping their owner storage live.
'' Responsibilities: Follow handler control flow, retain heap-owned values across
''     GOSUB returns, and return retry/next to the failing C activation.
'' This file intentionally does NOT implement general object/partial-construction unwinding.

declare function astBuildFieldDtorTree(byval this_ as FBSYMBOL ptr, byval fld as FBSYMBOL ptr) as ASTNODE ptr
declare sub hCErrEmitPartialFields( byval resource as FBSYMBOL ptr )
declare sub hCErrObjectCall( byval index as integer )
declare sub hCErrEmitFixedArray( byval resource as FBSYMBOL ptr )
declare function astBuildVarCleanupTree( byval resource as FBSYMBOL ptr ) as ASTNODE ptr

'' -------------------------------------------------------------------------
'' Handler control flow
'' -------------------------------------------------------------------------

sub irhlcErrorSetCleanupBoundary( byval proc as FBSYMBOL ptr, byval label as FBSYMBOL ptr )
	var entry = cptr( C_ERROR_BOUNDARY ptr, callocate( sizeof( C_ERROR_BOUNDARY ) ) )
	if( entry = NULL ) then errReportEx( FB_ERRMSG_INTERNAL, "C cleanup boundary allocation" ): exit sub
	entry->proc = proc
	entry->label = label
	entry->next = errorboundaries
	errorboundaries = entry
end sub

private function hCErrTakeCleanupBoundary( byval proc as FBSYMBOL ptr ) as FBSYMBOL ptr
	var link = @errorboundaries
	while( *link )
		var entry = *link
		if( entry->proc = proc ) then
			var label = entry->label
			*link = entry->next
			deallocate entry
			return label
		end if
		link = @entry->next
	wend
	return NULL
end function

private sub hCErrDestroyBoundaries( )
	hCErrDropRecoveries(errorrecoveries)
	while( errorboundaries )
		var entry = errorboundaries
		errorboundaries = entry->next
		deallocate entry
	wend
end sub

private function hCErrName( byval n as ASTNODE ptr ) as string
	if( n = NULL ) then return ""
	if( n->class <> AST_NODECLASS_CALL or n->sym = NULL ) then return ""
	return *symbGetMangledName( n->sym )
end function

private function hCErrLabel( byval n as ASTNODE ptr ) as FBSYMBOL ptr
	if( n = NULL ) then return NULL
	if( n->class = AST_NODECLASS_VAR andalso n->sym <> NULL andalso symbIsLabel( n->sym ) ) then return n->sym
	var found = hCErrLabel( n->l )
	if( found ) then return found
	return hCErrLabel( n->r )
end function

private function hCErrTreeHasHandler( byval n as ASTNODE ptr ) as integer
	if( n = NULL ) then return FALSE
	if( n->class = AST_NODECLASS_CALL andalso n->sym <> NULL ) then
		if( *symbGetMangledName(n->sym) = "fb_ErrorSetHandler" ) then
			if( hCErrLabel( n ) <> NULL ) then return TRUE
		end if
	end if
	return hCErrTreeHasHandler( n->l ) orelse hCErrTreeHasHandler( n->r )
end function

function irhlcErrorNeedsCleanupBoundary( byval body as ASTNODE ptr ) as integer
	while( body )
		if( hCErrTreeHasHandler( body ) ) then return TRUE
		body = body->next
	wend
	return FALSE
end function

private sub hCErrAppend( byval n as ASTNODE ptr )
	if( n = NULL ) then exit sub
	if( n->class = AST_NODECLASS_LINK ) then
		hCErrAppend( n->l )
		hCErrAppend( n->r )
		exit sub
	end if
	if( errorplan->count = errorplan->capacity ) then
		var capacity = errorplan->capacity + 64
		var storage = reallocate( errorplan->statements, capacity * sizeof( ASTNODE ptr ) )
		if( storage = NULL ) then errReportEx( FB_ERRMSG_INTERNAL, "C error statement allocation" ): exit sub
		errorplan->statements = storage
		errorplan->capacity = capacity
	end if
	errorplan->statements[errorplan->count] = n
	errorplan->count += 1
end sub

private function hCErrLabelIndex( byval label as FBSYMBOL ptr ) as integer
	for i as integer = 0 to errorplan->count - 1
		var n = errorplan->statements[i]
		if( n->class = AST_NODECLASS_LABEL and n->sym = label ) then return i
	next
	return -1
end function

private function hCErrResume( byval n as ASTNODE ptr ) as integer
	if( n->class <> AST_NODECLASS_BRANCH ) then return 0
	if( n->op.op <> AST_OP_JUMPPTR ) then return 0
	select case hCErrName( n->l )
	case "fb_ErrorResume": return 1
	case "fb_ErrorResumeNext": return 2
	end select
	return 0
end function

private function hCErrMarkContinuations _
	( _
		byval n as ASTNODE ptr, _
		byval h as C_ERROR_HANDLER ptr, _
		byref changed as integer _
	) as integer
	if( n = NULL ) then return TRUE
	if( n->class = AST_NODECLASS_VAR andalso n->sym <> NULL andalso symbIsLabel( n->sym ) ) then
		var target = hCErrLabelIndex( n->sym )
		if( target < 0 ) then return FALSE
		if( h->live[target] = 0 ) then h->live[target] = 1: changed = TRUE
	end if
	return hCErrMarkContinuations( n->l, h, changed ) andalso hCErrMarkContinuations( n->r, h, changed )
end function

private function hCErrMark( byval h as C_ERROR_HANDLER ptr ) as integer
	var start = hCErrLabelIndex( h->label )
	if( start < 0 ) then return FALSE
	h->live[start] = 1
	dim as integer changed
	do
		changed = FALSE
		for i as integer = 0 to errorplan->count - 1
			if( h->live[i] <> 1 ) then continue for
			h->live[i] = 2
			var n = errorplan->statements[i]
			'' Stop at the owner's final boundaries. Proven cleanup can transfer
			'' before destruction; otherwise source cleanup remains in the clone
			'' until it returns, retaining the active error for a possible RESUME.
			if( n->class = AST_NODECLASS_LABEL andalso (n->sym = errorplan->cleanup_label or n->sym = errorplan->exit_label) ) then continue for
			if( errorplan->recovery_ids <> NULL andalso errorplan->recovery_ids[i] <> 0 ) then continue for
			dim as integer falls = TRUE, target = -1
			select case n->class
			case AST_NODECLASS_BRANCH
				if( hCErrResume( n ) ) then
					falls = FALSE
				elseif( n->op.op = AST_OP_JUMPPTR andalso _
				        (hCErrName( n->l ) = "fb_ErrorThrowAt" or hCErrName( n->l ) = "fb_ErrorThrowEx") ) then
					'' Checked operations such as GOSUB RETURN can fail. Their
					'' retry/next labels belong to this clone, not its owner.
					if( hCErrMarkContinuations( n->l, h, changed ) = FALSE ) then return FALSE
					falls = FALSE
				elseif( n->op.op = AST_OP_CALL andalso n->op.ex <> NULL ) then
					'' Direct C GOSUB has both a callee edge and a return site.
					'' Its local RET returns through this clone's dispatcher.
					target = hCErrLabelIndex(n->op.ex)
					if( target < 0 ) then return FALSE
				elseif( n->op.op = AST_OP_RET andalso n->op.ex = NULL ) then
					falls = FALSE
				elseif( n->op.op = AST_OP_JMP and n->op.ex <> NULL ) then
					target = hCErrLabelIndex( n->op.ex )
					if( target < 0 ) then return FALSE
					falls = FALSE
				else
					return FALSE
				end if
			case AST_NODECLASS_BOP, AST_NODECLASS_UOP
				if( n->op.ex ) then
					target = hCErrLabelIndex( n->op.ex )
					if( target < 0 ) then return FALSE
				end if
			case AST_NODECLASS_CALL
				if( hProcDoesNotReturn( n->sym ) ) then falls = FALSE
			case AST_NODECLASS_JMPTB, AST_NODECLASS_ASM, AST_NODECLASS_LOAD
				return FALSE
			end select
			if( target >= 0 ) then
				if( h->live[target] = 0 ) then h->live[target] = 1: changed = TRUE
			end if
			if( falls ) then
				if( i + 1 = errorplan->count ) then return FALSE
				if( h->live[i+1] = 0 ) then h->live[i+1] = 1: changed = TRUE
			end if
		next
	loop while( changed )
	return TRUE
end function

'' Project: FreeBASIC outlined ERROR owner flow
'' File: owner-flow.bi
'' Purpose: Keep original source entries distinct from outlined handler entries.
'' Responsibilities: Trace typed edges and provide an ordered emission mask.
'' This file intentionally does NOT infer destinations of opaque native text.

private function hCErrOriginalTarget(byval label as FBSYMBOL ptr, byref changed as integer) as integer
	if( label = NULL ) then return TRUE
	var target = hCErrLabelIndex(label)
	if( target < 0 ) then return FALSE
	if( errorplan->original_live[target] = 0 ) then
		errorplan->original_live[target] = 1
		changed = TRUE
	end if
	return TRUE
end function

private function hCErrOriginalAddresses(byval n as ASTNODE ptr, byref changed as integer) as integer
	if( n = NULL ) then return TRUE
	if( n->class = AST_NODECLASS_CALL andalso n->sym <> NULL andalso symbGetIsRTL(n->sym) ) then
		'' The emitted setter receives a callback identity, not this source
		'' label address. Other escaped label addresses remain ordinary roots.
		if( hCErrName(n) = "fb_ErrorSetHandler" ) then return TRUE
	end if
	if( n->class = AST_NODECLASS_VAR andalso n->sym <> NULL andalso symbIsLabel(n->sym) ) then
		if( hCErrOriginalTarget(n->sym, changed) = FALSE ) then return FALSE
	end if
	return hCErrOriginalAddresses(n->l, changed) andalso hCErrOriginalAddresses(n->r, changed)
end function

private sub hCErrMarkOriginal()
	if( errorplan->handlercount = 0 or errorplan->count = 0 ) then exit sub
	'' Raw assembly may expose an entry independently of typed source edges.
	for i as integer = 0 to errorplan->count - 1
		if( errorplan->statements[i]->class = AST_NODECLASS_ASM ) then exit sub
	next
	errorplan->original_live = callocate(errorplan->count)
	if( errorplan->original_live = NULL ) then exit sub
	errorplan->original_live[0] = 1
	if( errorplan->recovery_ids <> NULL ) then
		for i as integer = 0 to errorplan->count - 1
			if( errorplan->recovery_ids[i] <> 0 ) then errorplan->original_live[i] = 1
		next
	end if
	dim as integer changed, valid = TRUE
	'' The owner checkpoint can transfer directly to final destruction.
	if( errorplan->can_transfer ) then valid = hCErrOriginalTarget(errorplan->cleanup_label, changed)
	do
		changed = FALSE
		for i as integer = 0 to errorplan->count - 1
			if( valid = FALSE ) then exit for
			if( errorplan->original_live[i] <> 1 ) then continue for
			errorplan->original_live[i] = 2
			var n = errorplan->statements[i]
			valid = hCErrOriginalAddresses(n, changed)
			select case n->class
			case AST_NODECLASS_BRANCH
				select case n->op.op
				case AST_OP_JMP, AST_OP_CALL, AST_OP_RET
					valid and= hCErrOriginalTarget(n->op.ex, changed)
				case AST_OP_JUMPPTR
					'' Known ERROR calls carry their retry/next address operands.
					'' Source RESUME is a known terminal handoff. Only an unknown
					'' indirect transfer requires conservative original-body retention.
					dim as integer known = FALSE
					if( n->l <> NULL andalso n->l->class = AST_NODECLASS_CALL ) then
						known = hCResumeIsThrow(n->l->sym) or hCResumeIsCheck(n->l->sym) or (hCErrResume(n) <> 0)
					end if
					valid and= known
				case else
					valid = FALSE
				end select
			case AST_NODECLASS_BOP, AST_NODECLASS_UOP
				valid and= hCErrOriginalTarget(n->op.ex, changed)
			case AST_NODECLASS_JMPTB
				for j as integer = 0 to n->jmptb.labelcount - 1
					valid and= hCErrOriginalTarget(n->jmptb.labels[j], changed)
				next
				valid and= hCErrOriginalTarget(n->jmptb.deflabel, changed)
			end select
			if( astCStatementEndsFlow(n) = FALSE andalso i + 1 < errorplan->count ) then
				if( errorplan->original_live[i + 1] = 0 ) then
					errorplan->original_live[i + 1] = 1
					changed = TRUE
				end if
			end if
		next
	loop while( changed and valid )
	if( valid = FALSE ) then
		deallocate errorplan->original_live
		errorplan->original_live = NULL
	end if
end sub

function irhlcErrorOriginalStatementNeeded(byval n as ASTNODE ptr) as integer
	if( errorplan = NULL or errorhelper ) then return TRUE
	if( errorplan->original_live = NULL ) then return TRUE
	var index = errorplan->original_cursor
	if( index >= errorplan->count orelse errorplan->statements[index] <> n ) then
		errReportEx(FB_ERRMSG_INTERNAL, "C owner statement order")
		return TRUE
	end if
	errorplan->original_cursor += 1
	if( errorplan->original_live[index] = 0 ) then return FALSE
	if( n->class = AST_NODECLASS_LABEL andalso errorplan->original_labels <> NULL ) then
		return errorplan->original_labels[index] <> 0
	end if
	return TRUE
end function

'' end of owner-flow.bi

#include once "backend/c/ordinary-recovery.bi"

'' -------------------------------------------------------------------------
'' Capture identities and clone ownership
'' -------------------------------------------------------------------------

private function hCErrPhysical( byval sym as FBSYMBOL ptr ) as FBSYMBOL ptr
	if( sym = NULL ) then return NULL
	if( symbIsVar( sym ) = FALSE ) then return NULL
	if( symbIsDynamic( sym ) andalso symbIsParamVar( sym ) = FALSE ) then sym = sym->var_.array.desc
	if( sym = NULL ) then return NULL
	if( symbIsLocal( sym ) = FALSE ) then return NULL
	return sym
end function

private function hCErrCapture( byval sym as FBSYMBOL ptr ) as integer
	if( errorplan = NULL ) then return -1
	sym = hCErrPhysical( sym )
	if( sym = NULL ) then return -1
	for i as integer = 0 to errorplan->capturecount - 1
		if( errorplan->captures[i] = sym ) then return i
	next
	return -1
end function

#include once "backend/c/c-error-emission-uses.bi"

private function hCErrScan( byval n as ASTNODE ptr ) as integer
	if( n = NULL ) then return TRUE
	if( n->class = AST_NODECLASS_CALL ) then
		if( n->call.tmpres <> NULL or n->call.strtail <> NULL ) then return FALSE
	end if
	if( n->class = AST_NODECLASS_ASM or n->class = AST_NODECLASS_JMPTB ) then return FALSE
	var sym = hCErrPhysical( n->sym )
	if( sym ) then
		if( symbIsStatic( sym ) ) then return FALSE
		if( hCErrCapture( sym ) < 0 ) then
			if( errorplan->capturecount = errorplan->capturecapacity ) then
				errorplan->capturecapacity += 32
				var storage = reallocate( errorplan->captures, errorplan->capturecapacity * sizeof( FBSYMBOL ptr ) )
				if( storage = NULL ) then return FALSE
				errorplan->captures = storage
			end if
			errorplan->captures[errorplan->capturecount] = sym
			errorplan->capturecount += 1
		end if
	end if
	return hCErrScan( n->l ) andalso hCErrScan( n->r )
end function

private function hCErrClone( byval n as ASTNODE ptr ) as ASTNODE ptr
	if( n = NULL ) then return NULL
	if( n->class = AST_NODECLASS_LIT ) then return astNewLIT( n->lit.text )
	var c = astNewNode( n->class, n->dtype, n->subtype )
	astCopy( c, n )
	c->prev = NULL
	c->next = NULL
	c->l = hCErrClone( n->l )
	c->r = hCErrClone( n->r )
	c->semantic_context = 0
	c->semantic_source = 0
	c->semantic_statement = 0
	c->semantic_expressions = 0
	c->semantic_expression = 0
	c->semantic_operands = 0
	c->semantic_binding = 0
	select case n->class
	case AST_NODECLASS_CALL
		astCloneCALL( n, c )
	case AST_NODECLASS_TYPEINI
		ast.typeinicount += 1
		'' The declaration's scope is owned by its source tree, not the clone.
		c->typeini.scp = NULL
	case AST_NODECLASS_FIELD
		if( symbFieldIsBitfield( c->sym ) ) then ast.bitfieldcount += 1
	end select
	if( n->class = AST_NODECLASS_VAR andalso n->sym <> NULL ) then
		if( symbIsVar( n->sym ) and symbIsTemp( n->sym ) ) then astDtorListAddRef( n->sym )
	end if
	return c
end function

private function hCErrType( byval sym as FBSYMBOL ptr ) as string
	if( sym->stats and FB_SYMBSTATS_ARGV ) then return "char**"
	dim as integer dtype
	dim as FBSYMBOL ptr subtype
	symbGetRealType( sym, dtype, subtype )
	'' A borrowed reference must match the C local declaration, including
	'' its proved read-only pointee. Pointer-to-pointer slots cannot convert.
	dtype = hCReadOnlyLocalType(sym, dtype)
	if( symbIsParamVar(sym) andalso typeIsPtr(dtype) ) then
		var variant = hCReadOnlyFind(errorplan->owner)
		var param = symbGetProcLastParam(errorplan->owner)
		while( param <> NULL )
			if( symbGetParamVar(param) = sym ) then
				if( hCReadOnlyParam(variant, param) ) then dtype = typeAddrOf(typeSetIsConst(typeDeref(dtype)))
				exit while
			end if
			param = symbGetProcPrevParam(errorplan->owner, param)
		wend
	end if
	return hEmitType( typeUnsetIsConst( dtype ), subtype, TRUE )
end function

#include once "backend/c/c-error-capture-layout.bi"

private function hCErrStruct( ) as string
	return "fb_error_captures_" + str( errorplan->serial )
end function

private function hCErrDiscover( byval n as ASTNODE ptr ) as integer
	if( n = NULL ) then return TRUE
	if( hCErrName( n ) = "fb_ErrorSetHandler" ) then
		var label = hCErrLabel( n )
		if( label ) then
			dim as integer found = FALSE
			for i as integer = 0 to errorplan->handlercount - 1
				if( errorplan->handlers[i].label = label ) then found = TRUE
			next
			if( found = FALSE ) then
				var h = @errorplan->handlers[errorplan->handlercount]
				h->label = label
				h->live = callocate( errorplan->count )
				errorplan->handlercount += 1
				if( h->live = NULL ) then return FALSE
				if( hCErrMark( h ) = FALSE ) then return FALSE
				for i as integer = 0 to errorplan->count - 1
					if( h->live[i] ) then
						if( hCErrScan( errorplan->statements[i] ) = FALSE ) then return FALSE
					end if
				next
			end if
		end if
	end if
	return hCErrDiscover( n->l ) andalso hCErrDiscover( n->r )
end function

'' -------------------------------------------------------------------------
'' Owner storage and handler preparation
'' -------------------------------------------------------------------------

private function hCErrOwnerCaptures( byval sym as FBSYMBOL ptr ) as integer
	while( sym )
		select case symbGetClass( sym )
		case FB_SYMBCLASS_SCOPE
			if( hCErrOwnerCaptures( symbGetScopeSymbTbHead( sym ) ) = FALSE ) then return FALSE
		case FB_SYMBCLASS_VAR
			var physical = hCErrPhysical( sym )
			if( physical <> NULL andalso symbIsStatic( physical ) = FALSE ) then
				'' Returning callbacks need all owner/GOSUB values. A cleanup-only
				'' worker needs just owned resources and their array descriptors.
				'' Keep other locals in their original C scopes and storage types.
				dim as integer needs_capture = errorplan->heap
				if( needs_capture = FALSE andalso symbGetProcStatGosub(errorplan->owner) ) then
					'' Cleanup can abandon the GOSUB stack while the worker lives.
					'' A borrowed reference is enough without an ERROR checkpoint.
					needs_capture = (physical = hCErrPhysical(symbGetProcGosubSym(errorplan->owner)))
				end if
				if( needs_capture = FALSE ) then
					for i as integer = 0 to errorplan->resourcecount - 1
						var resource = errorplan->resources[i]
						if( hCErrPhysical(resource) = physical ) then needs_capture = TRUE: exit for
						if( symbGetArrayDimensions(resource) <> 0 andalso resource->var_.array.desc = physical ) then needs_capture = TRUE: exit for
					next
				end if
				if( needs_capture = FALSE ) then sym = symbGetNext(sym): continue while
				if( hCErrCapture( physical ) < 0 ) then
					if( errorplan->capturecount = errorplan->capturecapacity ) then
						errorplan->capturecapacity += 32
						var storage = reallocate( errorplan->captures, errorplan->capturecapacity * sizeof( FBSYMBOL ptr ) )
						if( storage = NULL ) then return FALSE
						errorplan->captures = storage
					end if
					errorplan->captures[errorplan->capturecount] = physical
					errorplan->capturecount += 1
				end if
			end if
		end select
		sym = symbGetNext( sym )
	wend
	return TRUE
end function

private function hCErrPlainObjectFields( byval udt as FBSYMBOL ptr, byval depth as integer = 0 ) as integer
	if( udt = NULL or depth > 64 ) then return FALSE
	var fld = symbUdtGetFirstField(udt)
	while( fld <> NULL )
		if( symbIsDynamic(fld) or typeIsDynamicString(symbGetType(fld)) ) then return FALSE
		if( typeHasDtor(symbGetType(fld), symbGetSubtype(fld)) ) then return FALSE
		if( typeGetDtAndPtrOnly(symbGetFullType(fld)) = FB_DATATYPE_STRUCT ) then
			if( hCErrPlainObjectFields(symbGetSubtype(fld), depth + 1) = FALSE ) then return FALSE
		end if
		fld = symbUdtGetNextField(fld)
	wend
	return TRUE
end function

private function hCErrZeroSafeObjectFields( byval udt as FBSYMBOL ptr, byval depth as integer = 0, byval allow_user_dtor as integer = FALSE ) as integer
	if( udt = NULL or depth > 64 ) then return FALSE
	if( udt->udt.base <> NULL or symbGetUDTIsUnion(udt) or symbGetUDTHasAnonUnion(udt) ) then return FALSE
	var dtor = symbGetCompDtor1(udt)
	if( dtor <> NULL ) then
		if( symbIsVirtual(dtor) ) then return FALSE
		if( allow_user_dtor = FALSE andalso (symbGetAttrib(dtor) and FB_SYMBATTRIB_INTERNAL) = 0 ) then return FALSE
	end if
	'' Generated destruction visits only zero-safe owning fields. A user
	'' default constructor may stop after establishing any subset of them;
	'' the caller zeroes all storage before registration. Only the root
	'' may have a user destructor, whose partial cleanup uses this field
	'' contract without calling its body. Nested user destructors stay
	'' excluded. Descriptors use their fake dynamic fields below.
	var fld = symbUdtGetFirstField(udt)
	while( fld <> NULL )
		if( symbIsDescriptor(fld) = FALSE ) then
			if( symbIsRef(fld) or symbGetDontInit(fld) ) then return FALSE
			if( symbIsDynamic(fld) ) then
				'' Element constructors can fail before an array publishes its
				'' completed extent. Such arrays still need progress tracking.
				if( typeHasCtor(symbGetType(fld), symbGetSubtype(fld)) or _
				    typeHasDtor(symbGetType(fld), symbGetSubtype(fld)) ) then return FALSE
			elseif( typeGetDtAndPtrOnly(symbGetFullType(fld)) = FB_DATATYPE_STRUCT ) then
				if( hCErrZeroSafeObjectFields(symbGetSubtype(fld), depth + 1) = FALSE ) then return FALSE
			end if
		end if
		fld = symbUdtGetNextField(fld)
	wend
	return TRUE
end function

private function hCErrUserObject( byval resource as FBSYMBOL ptr ) as integer
	if( symbGetType(resource) <> FB_DATATYPE_STRUCT or symbGetArrayDimensions(resource) <> 0 ) then return FALSE
	'' Resource collection already checked the complete recognized constructor
	'' shape and field contract. Every such user destructor needs state
	'' guarding, including plain records with no managed fields to retire.
	var dtor = symbGetCompDtor1(symbGetSubtype(resource))
	if( dtor = NULL ) then return FALSE
	return (symbGetAttrib(dtor) and FB_SYMBATTRIB_INTERNAL) = 0
end function

private function hCErrCallObject( byval call_node as ASTNODE ptr ) as FBSYMBOL ptr
	if( call_node = NULL orelse call_node->class <> AST_NODECLASS_CALL orelse call_node->sym = NULL ) then return NULL
	var instance_param = symbGetProcHeadParam(call_node->sym)
	var arg = call_node->r
	while( arg <> NULL )
		if( arg->sym = instance_param ) then
			var target = arg->l
			while( target <> NULL andalso (target->class = AST_NODECLASS_CONV or target->class = AST_NODECLASS_ADDROF) )
				target = target->l
			wend
			if( target <> NULL andalso target->class = AST_NODECLASS_VAR andalso target->var_.ofs = 0 ) then
				'' A first field can share its parent's address. The member call
				'' must still belong to the complete resource's actual type.
				if( symbGetNamespace(call_node->sym) = symbGetSubtype(target->sym) ) then return target->sym
			end if
			return NULL
		end if
		arg = arg->r
	wend
	return NULL
end function

private function hCErrConstructionCallCount _
	( _
		byval n as ASTNODE ptr, _
		byval sym as FBSYMBOL ptr, _
		byref qualified as integer _
	) as integer
	if( n = NULL ) then return 0
	if( n->class = AST_NODECLASS_CALL andalso n->sym <> NULL ) then
		if( symbIsConstructor(n->sym) andalso hCErrCallObject(n) = sym ) then
			var udt = symbGetSubtype(sym)
			dim as integer selected = FALSE
			if( n->call.args = 1 andalso n->sym = symbGetCompDefCtor(udt) ) then
				selected = TRUE
			elseif( n->call.args = 2 andalso udt->udt.ext <> NULL ) then
				'' Generated copies use the existing zero-safe field contract.
				'' Their two arguments are THIS and the source record. A user
				'' overload has a separate construction contract and stays guarded.
				if( symbGetAttrib(n->sym) and FB_SYMBATTRIB_INTERNAL ) then
					selected = (n->sym = udt->udt.ext->copyctor or n->sym = udt->udt.ext->copyctorconst)
				end if
			end if
			qualified and= selected
			return 1
		end if
	end if
	return hCErrConstructionCallCount(n->l, sym, qualified) + hCErrConstructionCallCount(n->r, sym, qualified)
end function

private function hCErrConstructedObject( byval sym as FBSYMBOL ptr ) as integer
	if( symbGetType(sym) <> FB_DATATYPE_STRUCT or symbGetArrayDimensions(sym) <> 0 ) then return FALSE
	if( symbGetDontInit(sym) or symbIsRef(sym) ) then return FALSE
	var dtor = symbGetCompDtor1(symbGetSubtype(sym))
	if( dtor = NULL orelse symbIsVirtual(dtor) ) then return FALSE
	if( hCErrPlainObjectFields(symbGetSubtype(sym)) = FALSE andalso _
	    hCErrZeroSafeObjectFields(symbGetSubtype(sym), 0, TRUE) = FALSE ) then return FALSE
	'' The parser may flush construction separately from DECL. Require
	'' exactly one selected complete-object call, not one selected call
	'' among additional unknown constructors. Storage is already cleared
	'' before registration; the existing call hooks mark completion.
	dim as integer calls = 0, qualified = TRUE
	for i as integer = 0 to errorplan->count - 1
		calls += hCErrConstructionCallCount(errorplan->statements[i], sym, qualified)
	next
	return qualified andalso (calls = 1)
end function

private function hCErrGeneratedArrayFields( byval udt as FBSYMBOL ptr, byval depth as integer = 0 ) as integer
	if( udt = NULL or depth > 64 ) then return FALSE
	var ctor = symbGetCompCtorHead(udt)
	while( ctor <> NULL )
		if( (symbGetAttrib(ctor) and FB_SYMBATTRIB_INTERNAL) = 0 ) then return FALSE
		ctor = symbGetProcOvlNext(ctor)
	wend
	var fld = symbUdtGetFirstField(udt)
	while( fld <> NULL )
		if( symbIsDescriptor(fld) = FALSE andalso typeGetDtAndPtrOnly(symbGetFullType(fld)) = FB_DATATYPE_STRUCT ) then
			if( hCErrGeneratedArrayFields(symbGetSubtype(fld), depth + 1) = FALSE ) then return FALSE
		end if
		fld = symbUdtGetNextField(fld)
	wend
	return TRUE
end function

private function hCErrFixedObjectArray( byval sym as FBSYMBOL ptr ) as integer
	if( symbGetType(sym) <> FB_DATATYPE_STRUCT or symbGetArrayDimensions(sym) <= 0 ) then return FALSE
	if( symbIsDynamic(sym) or symbIsRef(sym) or symbGetDontInit(sym) ) then return FALSE
	if( sym->var_.array.desc = NULL ) then return FALSE
	'' Both the fixed payload and descriptor are zeroed before registration.
	'' Generated element destruction can retire every zero-safe slot even if
	'' a field initializer stopped construction. User element lifetimes need
	'' a completed-element contract and are deliberately excluded here.
	if( hCErrZeroSafeObjectFields(symbGetSubtype(sym)) = FALSE ) then return FALSE
	if( symbGetCompDtor1(symbGetSubtype(sym)) = NULL ) then return FALSE
	return hCErrGeneratedArrayFields(symbGetSubtype(sym))
end function

private function hCErrObjectIndex( byval sym as FBSYMBOL ptr ) as integer
	if( errorplan = NULL or sym = NULL ) then return -1
	for i as integer = 0 to errorplan->resourcecount - 1
		if( errorplan->resources[i] = sym andalso symbGetType(sym) = FB_DATATYPE_STRUCT andalso symbGetArrayDimensions(sym) = 0 ) then return i
	next
	return -1
end function

function irhlcErrorUDTCallBegin( byval call_node as ASTNODE ptr ) as integer
	if( errorplan = NULL or call_node = NULL ) then return -1
	if( symbIsConstructor(call_node->sym) = FALSE and symbIsDestructor1(call_node->sym) = FALSE ) then return -1
	var index = hCErrObjectIndex(hCErrCallObject(call_node))
	if( index < 0 ) then return -1
	if( symbIsDestructor1(call_node->sym) ) then
		if( hCErrUserObject(errorplan->resources[index]) ) then
			'' If ordinary user destruction is abandoned, its managed fields
			'' still need retirement, but its body must never be called again.
			hWriteLine("if (fb_error_capture->resource_" + str(index) + " == fb_error_resource_live) {")
			hWriteLine("fb_error_capture->resource_" + str(index) + " = fb_error_resource_destroying;")
			return index
		end if
		hWriteLine("fb_error_capture->resource_" + str(index) + " = fb_error_resource_unseen;")
		return -1
	end if
	hWriteLine("fb_error_capture->resource_" + str(index) + " = fb_error_resource_initializing;")
	return index
end function

sub irhlcErrorUDTCallComplete( byval index as integer, byval is_dtor as integer )
	if( index < 0 ) then exit sub
	if( is_dtor ) then
		hWriteLine("fb_error_capture->resource_" + str(index) + " = fb_error_resource_unseen;")
		hWriteLine("} else if (fb_error_capture->resource_" + str(index) + " == fb_error_resource_initializing || fb_error_capture->resource_" + str(index) + " == fb_error_resource_destroying) {")
		hCErrObjectCall(index)
		hWriteLine("}")
	else
		hWriteLine("fb_error_capture->resource_" + str(index) + " = fb_error_resource_live;")
	end if
end sub

private function hCErrResources( byval sym as FBSYMBOL ptr ) as integer
	while( sym )
		select case symbGetClass( sym )
		case FB_SYMBCLASS_SCOPE
			if( hCErrResources( symbGetScopeSymbTbHead( sym ) ) = FALSE ) then return FALSE
		case FB_SYMBCLASS_VAR
			if( symbIsLocal( sym ) andalso symbIsStatic( sym ) = FALSE andalso symbIsParamVar( sym ) = FALSE ) then
				'' Variable ownership also covers strings and dynamic arrays;
				'' symbHasDtor alone describes only the UDT destructor case.
				if( symbGetVarHasDtor( sym ) ) then
					if( symbGetType(sym) = FB_DATATYPE_STRING or (symbGetArrayDimensions(sym) <> 0 andalso _
					    typeHasDtor(symbGetType(sym), symbGetSubtype(sym)) = FALSE) or hCErrConstructedObject(sym) or hCErrFixedObjectArray(sym) ) then
						if( errorplan->resourcecount = errorplan->resourcecapacity ) then
							errorplan->resourcecapacity += 16
							var storage = reallocate( errorplan->resources, errorplan->resourcecapacity * sizeof( FBSYMBOL ptr ) )
							if( storage = NULL ) then return FALSE
							errorplan->resources = storage
						end if
						errorplan->resources[errorplan->resourcecount] = sym
						errorplan->resourcecount += 1
					else
						'' Keep a barrier for unqualified resource kinds. A private
						'' one-way transfer must not silently abandon their storage.
						errorplan->unsupported_cleanup = TRUE
					end if
				end if
			end if
		end select
		sym = symbGetNext( sym )
	wend
	return TRUE
end function

private function hCErrKeepAPI( byval api as FBSYMBOL ptr ) as integer
	if( api = NULL ) then
		errReportEx(FB_ERRMSG_INTERNAL, "C error owner runtime binding")
		return FALSE
	end if
	'' These calls are written as C text, without AST CALL nodes. Keep their
	'' real declarations through the ordinary generated-dependency graph.
	symbSetIsAccessed(api)
	var identifier = hGetCId(api)
	hCLeafNativeReference(identifier)
	return TRUE
end function

private sub hCErrPrototype( byval proc as FBSYMBOL ptr, byval has_address_reference as integer = TRUE )
	var identifier = hGetCId(proc)
	'' Generated callback addresses are written directly as C, not AST operands.
	symbSetIsAccessed(proc)
	if( has_address_reference ) then
		hCLeafNativeReference(identifier)
	else
		'' A forward declaration owns a removable span, not a caller edge.
		'' The shared object helper already has explicit generated calls.
		hCLeafGetName(identifier)
	end if
	var prototype = hEmitProcHeader(proc, EMITPROC_ISPROTO) + ";"
	var first = ctx.sections(0).bufferlen
	hWriteLine(prototype)
	hCLeafAuxPrototype(proc, first)
	'' The late prototype walk also sees generated callback symbols.
	'' Register this declaration so it does not emit a second copy.
	hCPrototypeRemember(identifier, prototype)
end sub

private function hCErrPrepare( byval proc as FBSYMBOL ptr ) as integer
	if( symbIsNaked( proc ) ) then return FALSE
	errorplan = callocate( sizeof( C_ERROR_PLAN ) )
	if( errorplan = NULL ) then return FALSE
	errorplan->owner = proc
	errorplan->exit_label = astGetProc()->block.exitlabel
	errorplan->cleanup_label = hCErrTakeCleanupBoundary( proc )
	errorplan->recoveries = hCErrTakeRecoveries(proc)
	var n = astGetProc( )->l
	while( n )
		hCErrAppend( n )
		n = n->next
	wend
	if( errGetCount( ) ) then hCErrDestroy( ): return FALSE
	errorplan->handlers = callocate( errorplan->count * sizeof( C_ERROR_HANDLER ) )
	if( errorplan->handlers = NULL ) then hCErrDestroy( ): return FALSE
	for i as integer = 0 to errorplan->count - 1
		if( hCErrDiscover( errorplan->statements[i] ) = FALSE ) then hCErrDestroy( ): return FALSE
	next
	if( hCErrResources( symbGetProcSymbTbHead( proc ) ) = FALSE ) then hCErrDestroy( ): return FALSE
	if( errorplan->handlercount = 0 ) then
		'' The root activation cannot be abandoned by an outer BASIC owner.
		'' In particular, do not introduce TLS entry before main's fb_Init.
		if( astGetProc( )->block.proc.ismain ) then hCErrDestroy( ): return FALSE
		if( errorplan->resourcecount = 0 and errorplan->unsupported_cleanup = FALSE and symbGetProcStatGosub( proc ) = FALSE and hCResumeNeedsCaptures() = FALSE ) then hCErrDestroy( ): return FALSE
	end if
	'' A RESUME-only callback never transfers to the owner's final cleanup.
	'' Its checkpoint still owns handler metadata, but must not introduce a
	'' synthetic return edge into a genuinely non-returning source procedure.
	var cleanup_index = hCErrLabelIndex(errorplan->cleanup_label)
	if( cleanup_index >= 0 ) then
		for i as integer = 0 to errorplan->handlercount - 1
			errorplan->can_transfer or= (errorplan->handlers[i].live[cleanup_index] <> 0)
		next
	end if
	'' Heap values survive actual ERROR checkpoints. Local GOSUB dispatch
	'' uses return identifiers and does not revisit a setjmp activation.
	'' Keep the existing storage for assembly that can observe hidden frames.
	errorplan->heap = (errorplan->handlercount > 0 or hCResumeNeedsCaptures() or _
		(symbGetProcStatGosub(proc) andalso ctx.computedgotoopaque))
	if( hCErrOwnerCaptures( symbGetProcSymbTbHead( proc ) ) = FALSE ) then hCErrDestroy( ): return FALSE
	'' Preserve the observed source procedure before replacing codegen roots.
	fbSemanticModelExportProc( proc, astGetProc( ), TRUE )
	errorplan->observed = TRUE
	hCErrMarkOriginal()
	if( hCErrRecoveryEntries() = FALSE ) then hCErrDestroy(): return FALSE
	hCErrMarkOriginalLabels()
	errorserial += 1
	errorplan->serial = errorserial
	'' These callbacks have no source body for a debugger to retain. Their
	'' emitting owner supplies the address edge, including in debug builds.
	for i as integer = 0 to errorplan->handlercount - 1
		var h = @errorplan->handlers[i]
		var cb = symbPreAddProc( NULL )
		symbAddProcParam( cb, NULL, typeAddrOf( FB_DATATYPE_VOID ), NULL, 0, FB_PARAMMODE_BYVAL, 0, 0 )
		var cbname = "fb_error_callback_" + str( errorserial ) + "_" + str( i )
		h->callback = symbAddProc( cb, strptr(cbname), strptr(cbname), FB_DATATYPE_LONG, NULL, FB_SYMBATTRIB_PRIVATE or FB_SYMBATTRIB_INTERNAL, FB_PROCATTRIB_NONE, FB_FUNCMODE_CDECL, FB_SYMBOPT_NONE )
		if( h->callback = NULL ) then
			errReportEx( FB_ERRMSG_INTERNAL, "C error callback symbol" )
			hCErrDestroy( )
			return FALSE
		end if
	next
	for i as integer = 0 to errorplan->handlercount - 1
		var h = @errorplan->handlers[i]
		for j as integer = 0 to errorplan->count - 1
			n = errorplan->statements[j]
			if( h->live[j] or n->class = AST_NODECLASS_SCOPEBEGIN or n->class = AST_NODECLASS_SCOPEEND ) then
				var clone = hCErrClone( n )
				if( h->clones = NULL ) then
					h->clones = clone
				else
					h->clonetail->next = clone
				end if
				h->clonetail = clone
			end if
		next
	next
	var cleanup = symbPreAddProc( NULL )
	symbAddProcParam( cleanup, NULL, typeAddrOf( FB_DATATYPE_VOID ), NULL, 0, FB_PARAMMODE_BYVAL, 0, 0 )
	var cleanupname = "fb_error_cleanup_" + str( errorserial )
	errorplan->cleanup_callback = symbAddProc( cleanup, strptr(cleanupname), strptr(cleanupname), FB_DATATYPE_VOID, NULL, FB_SYMBATTRIB_PRIVATE or FB_SYMBATTRIB_INTERNAL, FB_PROCATTRIB_NONE, FB_FUNCMODE_CDECL, FB_SYMBOPT_NONE )
	if( errorplan->cleanup_callback = NULL ) then errReportEx( FB_ERRMSG_INTERNAL, "C cleanup callback symbol" ): hCErrDestroy( ): return FALSE
	if( errorplan->unsupported_cleanup ) then symbSetProcIsNoReturn(errorplan->cleanup_callback)
	for i as integer = 0 to 7
		dim as FBSYMBOL ptr api
		select case i
		case 0: api = rtlProcLookup( @FB_RTL_ERRORFRAMEENTER, FB_RTL_IDX_ERRORFRAMEENTER )
		case 1: api = rtlProcLookup( @FB_RTL_ERRORFRAMEBUFFER, FB_RTL_IDX_ERRORFRAMEBUFFER )
		case 2: api = rtlProcLookup( @FB_RTL_ERRORFRAMESETHANDLER, FB_RTL_IDX_ERRORFRAMESETHANDLER )
		case 3: api = rtlProcLookup( @FB_RTL_ERRORFRAMELEAVE, FB_RTL_IDX_ERRORFRAMELEAVE )
		case 4: api = rtlProcLookup( @FB_RTL_END, FB_RTL_IDX_END )
		case 5: api = rtlProcLookup( @FB_RTL_ERRORFRAMEREADY, FB_RTL_IDX_ERRORFRAMEREADY )
		case 6: api = rtlProcLookup( @FB_RTL_ERRORFRAMESETTARGET, FB_RTL_IDX_ERRORFRAMESETTARGET )
		case else: api = rtlProcLookup( @FB_RTL_ERRORFRAMEGETTARGET, FB_RTL_IDX_ERRORFRAMEGETTARGET )
		end select
		if( hCErrKeepAPI(api) = FALSE ) then hCErrDestroy(): return FALSE
	next
	for i as integer = 0 to errorplan->resourcecount - 1
		var resource = errorplan->resources[i]
		dim as FBSYMBOL ptr api
		if( symbGetType(resource) = FB_DATATYPE_STRUCT andalso symbGetArrayDimensions(resource) = 0 ) then
			api = symbGetCompDtor1(symbGetSubtype(resource))
		elseif( hCErrFixedObjectArray(resource) ) then
			api = rtlProcLookup(@FB_RTL_ARRAYDESTRUCTOBJ, FB_RTL_IDX_ARRAYDESTRUCTOBJ)
		elseif( symbGetArrayDimensions(resource) = 0 ) then
			api = rtlProcLookup(@FB_RTL_STRDELETE, FB_RTL_IDX_STRDELETE)
		elseif( typeIsDynamicString(symbGetType(resource)) ) then
			if( symbIsDynamic(resource) ) then
				api = rtlProcLookup(@FB_RTL_ARRAYERASESTR, FB_RTL_IDX_ARRAYERASESTR)
			else
				api = rtlProcLookup(@FB_RTL_ARRAYDESTRUCTSTR, FB_RTL_IDX_ARRAYDESTRUCTSTR)
			end if
		else
			api = rtlProcLookup(@FB_RTL_ARRAYERASE, FB_RTL_IDX_ARRAYERASE)
		end if
		if( hCErrKeepAPI(api) = FALSE ) then hCErrDestroy(): return FALSE
	next
	if( errorplan->handlercount ) then
		var jump = rtlSetJmp( astNewCONSTi( 0, typeAddrOf( FB_DATATYPE_VOID ) ) )
		if( jump = NULL ) then errReportEx( FB_ERRMSG_INTERNAL, "C owner checkpoint binding" ): hCErrDestroy( ): return FALSE
		errorplan->checkpoint_api = jump->sym
		if( hCErrKeepAPI(jump->sym) = FALSE ) then astDelTree(jump): hCErrDestroy(): return FALSE
		astDelTree( jump )
		if( errorplan->cleanup_label = NULL ) then errReportEx( FB_ERRMSG_INTERNAL, "C owner cleanup boundary" ): hCErrDestroy( ): return FALSE
		if( errorplan->can_transfer ) then symbSetIsAccessed(errorplan->cleanup_label)
	end if
	var saved = sectionGosub( 0 )
	if( errorabiwritten = FALSE ) then
		hWriteLine( "struct FB_ERROR_FRAME { struct FB_ERROR_FRAME *previous; void *context; void (*cleanup)(void *); void *owner; };" )
		hWriteLine( "enum { fb_error_frame_entered = 1, fb_error_checkpoint_ready = 2, fb_error_action_transfer = 3, fb_error_target_cleanup = 1 };" )
		hWriteLine("enum { fb_error_resource_unseen = 0, fb_error_resource_initializing = 1, fb_error_resource_live = 2, fb_error_resource_destroying = 3 };")
		errorabiwritten = TRUE
	end if
	for i as integer = 0 to errorplan->capturecount - 1
		var capture_type = hCErrType( errorplan->captures[i] )
	next
	hWriteLine( "struct " + hCErrStruct( ) + " {" )
	hWriteLine( "struct FB_ERROR_FRAME frame;" )
	hWriteLine( hEmitType( FB_DATATYPE_UINT, NULL ) + " active;" )
	hWriteLine( "void *previous;" )
	for i as integer = 0 to errorplan->resourcecount - 1
		if( symbGetType(errorplan->resources[i]) = FB_DATATYPE_STRUCT andalso symbGetArrayDimensions(errorplan->resources[i]) = 0 ) then
			hWriteLine(hEmitType(FB_DATATYPE_UINT, NULL) + " resource_" + str(i) + ";")
		end if
	next
	hCErrCaptureFields()
	hWriteLine( "};" )
	hCErrPrototype(errorplan->cleanup_callback)
	for i as integer = 0 to errorplan->handlercount - 1
		hCErrPrototype(errorplan->handlers[i].callback)
	next
	sectionReturn( saved )
	return TRUE
end function

function irhlcErrorOriginalProcObserved( byval proc as FBSYMBOL ptr ) as integer
	if( errorplan = NULL ) then return FALSE
	return errorplan->owner = proc and errorplan->observed
end function

private function hCErrSymbolText( byval sym as FBSYMBOL ptr ) as string
	var index = hCErrCapture( sym )
	if( index < 0 ) then return ""
	if( errorplan->heap ) then return "fb_error_capture->value_" + str(index)
	if( errorhelper ) then return "(*fb_error_capture->reference_" + str(index) + ")"
	if( symbIsParamVar( sym ) ) then return hGetCId( sym )
	return "fb_error_local_" + str(index)
end function

private function hCErrCallHandler( byval expr as EXPRNODE ptr ) as integer
	if( errorplan = NULL or expr = NULL ) then return -1
	if( expr->class = EXPRCLASS_SYM ) then
		for i as integer = 0 to errorplan->handlercount - 1
			'' Keep the frontend's original label expression intact. Replacing
            '' it with a codegen callback changes the deferred source-call ABI.
            if( errorplan->handlers[i].label = expr->sym ) then return i
			if( errorplan->handlers[i].callback = expr->sym ) then return i
		next
	end if
	var found = hCErrCallHandler( expr->l )
	if( found >= 0 ) then return found
	return hCErrCallHandler( expr->r )
end function

'' -------------------------------------------------------------------------
'' Activation emission and retirement
'' -------------------------------------------------------------------------

private sub hCErrOwnerLocals( )
	if( errorplan = NULL ) then exit sub
	if( errorplan->heap ) then
		'' Stable before any checkpoint; mutable values live outside the
		'' automatic storage that longjmp can make indeterminate.
		hWriteLine( "struct " + hCErrStruct( ) + " *const fb_error_capture = (struct " + hCErrStruct( ) + " *)__builtin_calloc(1u, sizeof(struct " + hCErrStruct( ) + "));" )
		hWriteLine( "if (fb_error_capture == 0) fb_End(" + str(FB_RTERROR_OUTOFMEM) + ");" )
		for i as integer = 0 to errorplan->capturecount - 1
			var sym = errorplan->captures[i]
			if( symbIsParamVar( sym ) ) then hWriteLine( hCErrSymbolText( sym ) + " = " + hGetCId( sym ) + ";" )
		next
	else
		'' Cleanup runs while the worker's C activation is still alive. It
		'' does not need an owner checkpoint or a heap allocation.
		for i as integer = 0 to errorplan->capturecount - 1
			var sym = errorplan->captures[i]
			if( symbIsParamVar( sym ) = FALSE ) then hEmitVarDecl( FALSE, sym, NULL )
		next
		hWriteLine( "struct " + hCErrStruct( ) + " fb_error_storage = { .active = 0 };" )
		hWriteLine( "struct " + hCErrStruct( ) + " *const fb_error_capture = &fb_error_storage;" )
		for i as integer = 0 to errorplan->capturecount - 1
			hWriteLine( "fb_error_capture->reference_" + str(i) + " = &" + hCErrSymbolText(errorplan->captures[i]) + ";" )
		next
		'' Inactive scalar string descriptors must also be safe to retire if
		'' an error occurs before their original source declaration executes.
		for i as integer = 0 to errorplan->resourcecount - 1
			var spelling = hCErrSymbolText( errorplan->resources[i] )
			hWriteLine( "__builtin_memset(&" + spelling + ", 0, sizeof(" + spelling + "));" )
			var resource = errorplan->resources[i]
			if( symbGetArrayDimensions(resource) <> 0 andalso symbIsDynamic(resource) = FALSE ) then
				'' A fixed array has separate physical elements and descriptor.
				'' An early error must not read either before source initialization.
				var descriptor = resource->var_.array.desc
				assert(descriptor <> NULL)
				var descriptor_text = hCErrSymbolText(descriptor)
				hWriteLine("__builtin_memset(&" + descriptor_text + ", 0, sizeof(" + descriptor_text + "));")
			end if
		next
		if( symbGetProcStatGosub(errorplan->owner) ) then
			'' Registration precedes source initializers. An early retirement
			'' must see an empty GOSUB stack, even before its first source use.
			var context_text = hCErrSymbolText(symbGetProcGosubSym(errorplan->owner))
			assert(len(context_text) > 0)
			hWriteLine(context_text + " = 0;")
		end if
	end if
	if( astGetProc()->block.proc.ismain = FALSE ) then hCErrRegisterFrame()
end sub

private sub hCErrRecordOwnerPrologue()
	if( errorplan = NULL or errorhelper ) then exit sub
	assert(ctx.section = 3)
	var body = @ctx.sections(ctx.section)
	if( body->old ) then exit sub
	'' No source statement has been emitted yet. Keep the complete generated
	'' prologue, including native parameter conversions used by captures.
	errorplan->owner_prologue_section = ctx.section
	errorplan->owner_prologue_bytes = body->bufferlen
end sub

private sub hCErrMoveOwnerPrologue()
	if( errorplan = NULL or errorhelper ) then exit sub
	var bytes = errorplan->owner_prologue_bytes
	if( bytes = 0 ) then exit sub
	var source_section = errorplan->owner_prologue_section
	if( source_section <> ctx.section + 1 orelse source_section >= MAX_SECTIONS ) then
		errReportEx(FB_ERRMSG_INTERNAL, "C owner prologue section")
		exit sub
	end if
	var body = @ctx.sections(source_section)
	var header = @ctx.sections(ctx.section)
	if( body->old orelse header->old orelse bytes > body->bufferlen ) then
		errReportEx(FB_ERRMSG_INTERNAL, "C owner prologue boundary")
		exit sub
	end if
	if( ctx.procentrysection = source_section andalso ctx.procentrybytes > 0 ) then
		if( ctx.procentryoffset < bytes ) then
			errReportEx(FB_ERRMSG_INTERNAL, "C owner prologue entry boundary")
			exit sub
		end if
	end if
	'' The GOSUB header already contains all ordinary declarations and its
	'' dispatch-target slot. Move activation setup before the dispatch labels,
	'' so RETURN never crosses capture initialization or repeats allocation.
	var previous_bytes = header->bufferlen
	sectionAppend(header, body->buffer, bytes)
	if( header->bufferlen < previous_bytes orelse header->bufferlen - previous_bytes <> bytes ) then
		errReportEx(FB_ERRMSG_INTERNAL, "C owner prologue insertion")
		exit sub
	end if
	memmove(body->buffer, body->buffer + bytes, body->bufferlen - bytes)
	body->bufferlen -= bytes
	if( ctx.procentrysection = source_section andalso ctx.procentrybytes > 0 ) then ctx.procentryoffset -= bytes
	errorplan->owner_prologue_bytes = 0
end sub

private sub hCErrRegisterFrame()
	'' Registration protects allocations even before ON ERROR executes. The
	'' checkpoint is established separately at the first handler selection.
	hWriteLine( "{ int32 fb_error_status = fb_ErrorFrameEnter(&fb_error_capture->frame, fb_error_capture, &" + hGetCId(errorplan->cleanup_callback) + ");" )
	hWriteLine( "if (fb_error_status != 0) fb_End(fb_error_status); }" )
	hWriteLine( "fb_error_capture->active = fb_error_frame_entered;" )
end sub

private sub hCErrObserveSelection( byval index as integer, byval level as integer, byref call_text as const string )
	var api = rtlProcLookup(@FB_RTL_ERRORFRAMESETHANDLER, FB_RTL_IDX_ERRORFRAMESETHANDLER)
	assert(api <> NULL)
	assert(symbGetProcParams(api) = 3)
	'' The source setter expands to frame preparation and this real C binding.
	'' Report the selected three-argument ABI with the original source context.
	var identity = fbSemanticModelCallAbiBegin(api, "c", FALSE, TRUE, FB_DATATYPE_LONG, NULL, "", "", level, 0)
	if( identity = 0 ) then exit sub
	dim as string components(0 to 2)
	components(0) = "&fb_error_capture->frame"
	components(1) = "&" + hGetCId(errorplan->handlers[index].callback)
	components(2) = "&fb_error_capture->previous"
	var param = symbGetProcHeadParam(api)
	for component as integer = 0 to 2
		assert(param <> NULL)
		var selected_dtype = symbGetType(param)
		var selected_subtype = symbGetSubtype(param)
		dim as integer input_dtype = selected_dtype
		dim as FBSYMBOL ptr input_subtype = selected_subtype
		if( component = 1 ) then
			input_dtype = typeAddrOf(FB_DATATYPE_FUNCTION)
			input_subtype = errorplan->handlers[index].callback
		end if
		fbSemanticModelCallAbiArgument(identity, param, 0, 1, "value", input_dtype, input_subtype, _
			selected_dtype, selected_subtype, "", "", components(component))
		param = param->next
	next
	assert(param = NULL)
	fbSemanticModelCallAbiEnd(identity, call_text)
end sub

private sub hCErrSelect( byval index as integer, byval vr as IRVREG ptr, byval level as integer )
	hWriteLine( "{" )
	hWriteLine( "int32 fb_error_status;" )
	if( errorhelper = FALSE ) then
		hWriteLine( "if (fb_error_capture->active == fb_error_frame_entered) {" )
		hWriteLine( "void *const fb_error_buffer = fb_ErrorFrameBuffer(&fb_error_capture->frame);" )
		hWriteLine( "if (fb_error_buffer == 0) fb_End(" + str(FB_RTERROR_OUTOFMEM) + ");" )
		var checkpoint_call = hGetCId( errorplan->checkpoint_api ) + "(fb_error_buffer"
		if( symbGetProcParams(errorplan->checkpoint_api) = 2 ) then
			'' The runtime retires abandoned C activations and their resources
			'' before restoring this checkpoint. Do not ask the Win64 CRT to
			'' unwind them again using a compiler frame-pointer approximation.
			'' Match the existing C statement-checkpoint convention instead.
			checkpoint_call += ", (void*)0"
		end if
		checkpoint_call += ")"
		hWriteLine( "if (" + checkpoint_call + " != 0) {" )
		if( errorplan->can_transfer ) then
			hWriteLine( "if (fb_ErrorFrameGetTarget(&fb_error_capture->frame) == fb_error_target_cleanup) goto " + hGetCId(errorplan->cleanup_label) + ";" )
		end if
		var recovery = errorplan->recoveries
		while( recovery <> NULL )
			if( recovery->target_id > 0 ) then
				hWriteLine("if (fb_ErrorFrameGetTarget(&fb_error_capture->frame) == " + str(recovery->target_id) + ") goto " + hGetCId(recovery->label) + ";")
			end if
			recovery = recovery->next
		wend
		if( errorplan->exit_target_id > 0 ) then
			hWriteLine("if (fb_ErrorFrameGetTarget(&fb_error_capture->frame) == " + str(errorplan->exit_target_id) + ") goto " + hGetCId(errorplan->exit_label) + ";")
		end if
		hWriteLine( "fb_End(" + str(FB_RTERROR_ILLEGALRESUME) + "); }" )
		hWriteLine( "fb_error_status = fb_ErrorFrameReady(&fb_error_capture->frame);" )
		hWriteLine( "if (fb_error_status != 0) fb_End(fb_error_status);" )
		hWriteLine( "fb_error_capture->active = fb_error_checkpoint_ready;" )
		hWriteLine( "}" )
	end if
	var selected_call = "fb_ErrorFrameSetHandler(&fb_error_capture->frame, &" + hGetCId(errorplan->handlers[index].callback) + ", &fb_error_capture->previous)"
	hWriteLine("fb_error_status = " + selected_call + ";")
	hCErrObserveSelection(index, level, selected_call)
	hWriteLine( "if (fb_error_status != 0) fb_End(fb_error_status);" )
	hWriteLine( "}" )
	if( vr <> NULL ) then exprSTORE( vr, exprNewTEXT( vr->dtype, vr->subtype, "fb_error_capture->previous" ), TRUE )
end sub

private sub hCErrLeave( )
	if( errorplan = NULL or errorhelper ) then exit sub
	'' Normal source cleanup has already run. Retire the registration before
	'' releasing its heap storage, including exits before registration.
	hWriteLine( "if (fb_error_capture->active) {" )
	hWriteLine( "int32 fb_error_status;" )
	'' ERROR can abandon a nested source scope without returning through its
	'' destructor call. Retire still-owned user objects while the owner frame
	'' is live, so a destructor error can still select the proper handler.
	for i as integer = errorplan->resourcecount - 1 to 0 step -1
		if( hCErrUserObject(errorplan->resources[i]) ) then
			hWriteLine("if (fb_error_capture->resource_" + str(i) + " != fb_error_resource_unseen) {")
			hCErrObjectCall(i)
			hWriteLine("}")
		elseif( hCErrFixedObjectArray(errorplan->resources[i]) ) then
			'' A local handler may leave an inner scope before its ordinary
			'' array cleanup. Generated field retirement is idempotent and
			'' invokes no user body, including after a normal scope exit.
			hCErrEmitFixedArray(errorplan->resources[i])
		end if
	next
	hWriteLine( "fb_error_status = fb_ErrorFrameLeave(&fb_error_capture->frame);" )
	hWriteLine( "if (fb_error_status != 0) fb_End(fb_error_status);" )
	hWriteLine( "fb_error_capture->active = 0;" )
	hWriteLine( "}" )
	if( errorplan->heap ) then hWriteLine( "__builtin_free(fb_error_capture);" )
end sub

'' -------------------------------------------------------------------------
'' Callback bodies and compiler cleanup
'' -------------------------------------------------------------------------

private sub hCErrEmitPartialFields( byval resource as FBSYMBOL ptr )
	'' Reuse the AST builder so field order, array descriptors and generated
	'' child destruction stay identical to an ordinary destructor epilogue.
	var fld = symbGetCompSymbTb(symbGetSubtype(resource)).tail
	while( fld <> NULL )
		if( symbIsField(fld) andalso symbIsDescriptor(fld) = FALSE andalso symbIsRef(fld) = FALSE ) then
			var previous = errorplan->owner->proc.symtb.tail
			var tree = astBuildFieldDtorTree(resource, fld)
			'' Array descriptors and fixed child-array loops can introduce
			'' temporaries after source local allocation. They belong to this
			'' helper, not the owner's capture storage or ordinary scope.
			dim as FBSYMBOL ptr temporary
			if( previous <> NULL ) then
				temporary = previous->next
			else
				temporary = errorplan->owner->proc.symtb.head
			end if
			var saved = sectionGosub(iif(errorhelper, errordeclsection, 2))
			while( temporary <> NULL )
				if( symbIsVar(temporary) ) then hEmitVarDecl(FALSE, temporary, NULL)
				temporary = temporary->next
			wend
			sectionReturn(saved)
			if( tree <> NULL ) then
				astLoad(tree)
				astDelNode(tree)
			end if
		end if
		fld = fld->prev
	wend
end sub

private sub hCErrEmitFixedArray( byval resource as FBSYMBOL ptr )
	'' Use the ordinary AST array builder. It supplies the generic descriptor
	'' view and typed-to-erased callback adapter used by normal source cleanup.
	var previous = errorplan->owner->proc.symtb.tail
	var tree = astBuildVarCleanupTree(resource)
	dim as FBSYMBOL ptr temporary
	if( previous <> NULL ) then
		temporary = previous->next
	else
		temporary = errorplan->owner->proc.symtb.head
	end if
	var saved = sectionGosub(iif(errorhelper, errordeclsection, 2))
	while( temporary <> NULL )
		if( symbIsVar(temporary) ) then hEmitVarDecl(FALSE, temporary, NULL)
		temporary = temporary->next
	wend
	sectionReturn(saved)
	if( tree <> NULL ) then
		astLoad(tree)
		astDelNode(tree)
	end if
end sub

private sub hCErrObjectCall( byval index as integer )
	if( errorplan->object_callback = NULL ) then
		'' Declarations in a terminating owner may never need retirement.
		'' Make the symbol at the first real call, rather than retaining an
		'' unused body through a prototype or the module's opaque-code guard.
		var helper = symbPreAddProc(NULL)
		symbAddProcParam(helper, NULL, typeAddrOf(FB_DATATYPE_VOID), NULL, 0, FB_PARAMMODE_BYVAL, 0, 0)
		symbAddProcParam(helper, NULL, FB_DATATYPE_INTEGER, NULL, 0, FB_PARAMMODE_BYVAL, 0, 0)
		var identifier = "fb_error_object_" + str(errorplan->serial)
		errorplan->object_callback = symbAddProc(helper, strptr(identifier), strptr(identifier), FB_DATATYPE_VOID, NULL, FB_SYMBATTRIB_PRIVATE or FB_SYMBATTRIB_INTERNAL, FB_PROCATTRIB_NONE, FB_FUNCMODE_CDECL, FB_SYMBOPT_NONE)
		if( errorplan->object_callback = NULL ) then
			errReportEx(FB_ERRMSG_INTERNAL, "C object retirement symbol")
			exit sub
		end if
	end if
	hCLeafReference(errorplan->object_callback)
	hWriteLine(hGetCId(errorplan->object_callback) + "(fb_error_capture, " + str(index) + ");")
end sub

private sub hCErrEmitObjectHelper( )
	if( errorplan->object_callback = NULL ) then exit sub
	var saved = sectionGosub(0)
	hCErrPrototype(errorplan->object_callback, FALSE)
	sectionReturn(saved)
	hCLeafBegin(errorplan->object_callback)
	irhlEmitProcBegin()
	sectionBegin()
	hWriteLine("static void " + hGetCId(errorplan->object_callback) + "(void *fb_error_opaque, " + hEmitType(FB_DATATYPE_INTEGER, NULL) + " fb_error_resource)")
	hWriteLine("{")
	sectionIndent()
	hWriteLine("struct " + hCErrStruct() + " *fb_error_capture = (struct " + hCErrStruct() + " *)fb_error_opaque;")
	errordeclsection = ctx.section
	sectionBegin()
	hWriteLine("switch (fb_error_resource) {")
	for i as integer = 0 to errorplan->resourcecount - 1
		var resource = errorplan->resources[i]
		if( hCErrUserObject(resource) ) then
			hWriteLine("case " + str(i) + ": {")
			hWriteLine("if (fb_error_capture->resource_" + str(i) + " == fb_error_resource_initializing || fb_error_capture->resource_" + str(i) + " == fb_error_resource_destroying) {")
			hWriteLine("fb_error_capture->resource_" + str(i) + " = fb_error_resource_unseen;")
			hCErrEmitPartialFields(resource)
			hWriteLine("} else if (fb_error_capture->resource_" + str(i) + " == fb_error_resource_live) {")
			'' A later ordinary error must retire fields without repeating the
			'' user body. During abandoned-frame cleanup the existing runtime
			'' guard still rejects a second error while unwinding.
			hWriteLine("fb_error_capture->resource_" + str(i) + " = fb_error_resource_destroying;")
			var dtor = symbGetCompDtor1(symbGetSubtype(resource))
			hCLeafReference(dtor)
			hWriteLine(hGetCId(dtor) + "(&" + hCErrSymbolText(resource) + ");")
			hWriteLine("fb_error_capture->resource_" + str(i) + " = fb_error_resource_unseen;")
			hWriteLine("}")
			hWriteLine("return; }")
		end if
	next
	hWriteLine("default: fb_End(" + str(FB_RTERROR_ILLEGALFUNCTIONCALL) + ");")
	hWriteLine("}")
	do
		var cached = cptr(EXPRCACHENODE ptr, listGetHead(@ctx.exprcache))
		if( cached = NULL ) then exit do
		exprFreeTree(cached->expr)
		listDelNode(@ctx.exprcache, cached)
	loop
	assert(listGetHead(@ctx.exprnodes) = NULL)
	sectionEnd()
	sectionUnindent()
	hWriteLine("}")
	sectionEnd()
	hCLeafEnd()
	irhlEmitProcEnd()
end sub

private sub hCErrEmitHelpers( )
	if( errorplan = NULL ) then exit sub
	errorhelper = TRUE
	for i as integer = 0 to errorplan->handlercount - 1
		var h = @errorplan->handlers[i]
		'' A callback is a private graph node, not an output-wide root.
		'' Keep its body and callees only through a live owning procedure.
		hCLeafBegin(h->callback)
		irhlEmitProcBegin( )
		sectionBegin( )
		ctx.computedgotoaddress = FALSE
		ctx.computedgotoentry = FALSE
		ctx.computedgotodispatch = FALSE
		ctx.computedgotoopaque = FALSE
		hCResumeBegin(h->clones)
		ctx.procentrylabel = h->label
		ctx.procentrybytes = 0
		hWriteLine( "static int32 " + hGetCId(h->callback) + "(void *fb_error_opaque)" )
		hWriteLine( "{" )
		hCResumeDeclarationStart()
		sectionIndent( )
		var context_first = ctx.sections(ctx.section).bufferlen
		hWriteLine( "struct " + hCErrStruct( ) + " *fb_error_capture = (struct " + hCErrStruct( ) + " *)fb_error_opaque;" )
		var context_following = ctx.sections(ctx.section).bufferlen
		'' The child buffers executable statements while exprSTORE appends
		'' their temporary declarations to this function's header section.
		errordeclsection = ctx.section
		hCGosubBegin(errorplan->owner, h->clones)
		sectionBegin( )
		hWriteLine( "goto " + hGetCId(h->label) + ";" )
		while( h->clones <> NULL )
			var n = h->clones
			h->clones = n->next
			var action = hCErrResume( n )
			var recovery_target = iif(n->class = AST_NODECLASS_LABEL, hCErrRecoveryTarget(n->sym), 0)
			if( n->class = AST_NODECLASS_LABEL andalso n->sym = errorplan->cleanup_label ) then
				'' This hidden boundary falls through inside the callback. Its
			'' named entry belongs only to the original owner checkpoint.
				hWriteLine( "if (fb_ErrorFrameSetTarget(&fb_error_capture->frame, fb_error_target_cleanup) != 0) fb_End(" + str(FB_RTERROR_ILLEGALRESUME) + ");" )
				hWriteLine( "return fb_error_action_transfer;" )
				astDelTree( n )
			elseif( recovery_target > 0 ) then
				'' A final exit also has source RETURN branches inside this clone.
				'' Hidden pre-cleanup recovery boundaries are fallthrough-only.
				if( n->sym = errorplan->exit_label ) then hWriteLine(hGetCId(n->sym) + ":;")
				'' This private boundary has no incoming callback edge. Its
				'' named entry belongs only to the original owner's checkpoint.
				hWriteLine("if (fb_ErrorFrameSetTarget(&fb_error_capture->frame, " + str(recovery_target) + ") != 0) fb_End(" + str(FB_RTERROR_ILLEGALRESUME) + ");")
				hWriteLine("return fb_error_action_transfer;")
				astDelTree(n)
			elseif( action ) then
				hWriteLine( "return " + str(action) + ";" )
				astDelTree( n )
			elseif( n->class = AST_NODECLASS_LABEL andalso symbGetIsAccessed(n->sym) = FALSE ) then
				'' Apply the ordinary C flusher's unused-label rule to clones too.
				astDelNode(n)
			else
				astLoad( n )
				astDelNode( n )
			end if
		wend
		'' A discarded function result can leave an expression cached. Each
		'' helper starts its own vreg numbering, so no cache may cross it.
		do
			var cached = cptr(EXPRCACHENODE ptr, listGetHead( @ctx.exprcache ))
			if( cached = NULL ) then exit do
			exprFreeTree( cached->expr )
			listDelNode( @ctx.exprcache, cached )
		loop
		assert( listGetHead( @ctx.exprnodes ) = NULL )
		hCGosubEnd()
		hCErrTrimUnusedContext(context_first, context_following)
		hCResumeCloseBody()
		sectionEnd( )
		sectionUnindent( )
		hWriteLine( "}" )
		hCResumeEnd()
		sectionEnd( )
		hCLeafEnd()
		irhlEmitProcEnd( )
	next
	hCLeafBegin(errorplan->cleanup_callback)
	irhlEmitProcBegin( )
	sectionBegin( )
	var cleanup_prefix = "static void "
	if( errorplan->unsupported_cleanup ) then cleanup_prefix = "static void __attribute__((noreturn)) "
	var unused_context = errorplan->unsupported_cleanup
	if( unused_context ) then
		'' This barrier cannot inspect unsupported objects. Its ABI still takes
		'' a context; C23 permits the unused slot to have no local name.
		hWriteLine("#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L", TRUE)
		hWriteLine(cleanup_prefix + hGetCId(errorplan->cleanup_callback) + "(void *)")
		hWriteLine("#else", TRUE)
		hWriteLine(cleanup_prefix + hGetCId(errorplan->cleanup_callback) + "(void *fb_error_opaque)")
		hWriteLine("#endif", TRUE)
	else
		hWriteLine(cleanup_prefix + hGetCId(errorplan->cleanup_callback) + "(void *fb_error_opaque)")
	end if
	hWriteLine( "{" )
	sectionIndent( )
	if( unused_context ) then
		'' Before C23 the ABI slot needs a name, even though this nonreturning
		'' barrier cannot inspect its context or retire unsupported resources.
		hWriteLine("#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L", TRUE)
		hWriteLine("(void)fb_error_opaque;")
		hWriteLine("#endif", TRUE)
	else
		hWriteLine("struct " + hCErrStruct() + " *fb_error_capture = (struct " + hCErrStruct() + " *)fb_error_opaque;")
	end if
	errordeclsection = ctx.section
	sectionBegin( )
	if( errorplan->unsupported_cleanup ) then
		'' This is an explicit private capability boundary, not a successful
		'' cleanup claim for UDTs, arrays or partially constructed objects.
		hWriteLine( "fb_End(" + str(FB_RTERROR_ILLEGALFUNCTIONCALL) + ");" )
	else
		for i as integer = errorplan->resourcecount - 1 to 0 step -1
			var resource = errorplan->resources[i]
			if( symbGetType(resource) = FB_DATATYPE_STRUCT andalso symbGetArrayDimensions(resource) = 0 ) then
				'' Clear ownership before user code runs. Normal scope exits do
				'' the same, so an already-retired object is never destroyed twice.
				if( hCErrUserObject(resource) ) then
					hCErrObjectCall(i)
				else
				var owned_state = " == fb_error_resource_live"
				if( hCErrZeroSafeObjectFields(symbGetSubtype(resource)) ) then
					'' Every field was zeroed before registration. An interrupted
					'' generated constructor can retire its initialized subset.
					owned_state = " != fb_error_resource_unseen"
				end if
				hWriteLine("if (fb_error_capture->resource_" + str(i) + owned_state + ") {")
				hWriteLine("fb_error_capture->resource_" + str(i) + " = fb_error_resource_unseen;")
				var dtor = symbGetCompDtor1(symbGetSubtype(resource))
				'' This call is emitted as C text; retain the destructor edge too.
				hCLeafReference(dtor)
				hWriteLine(hGetCId(dtor) + "(&" + hCErrSymbolText(resource) + ");")
				hWriteLine("}")
				end if
			elseif( hCErrFixedObjectArray(resource) ) then
				hCErrEmitFixedArray(resource)
			elseif( symbGetArrayDimensions(resource) = 0 ) then
				hWriteLine("fb_StrDelete(&" + hCErrSymbolText(resource) + ");")
			else
				var descriptor = resource->var_.array.desc
				assert(descriptor <> NULL)
				dim as string api
				if( typeIsDynamicString(symbGetType(resource)) ) then
					api = iif(symbIsDynamic(resource), "fb_ArrayStrErase", "fb_ArrayDestructStr")
				else
					api = "fb_ArrayErase"
				end if
				dim as FBSYMBOL ptr api_symbol
				select case api
				case "fb_ArrayStrErase": api_symbol = rtlProcLookup(@FB_RTL_ARRAYERASESTR, FB_RTL_IDX_ARRAYERASESTR)
				case "fb_ArrayDestructStr": api_symbol = rtlProcLookup(@FB_RTL_ARRAYDESTRUCTSTR, FB_RTL_IDX_ARRAYDESTRUCTSTR)
				case else: api_symbol = rtlProcLookup(@FB_RTL_ARRAYERASE, FB_RTL_IDX_ARRAYERASE)
				end select
				dim as integer parameter_dtype
				dim as FBSYMBOL ptr parameter_subtype
				symbGetRealParamDtype(symbGetProcHeadParam(api_symbol), parameter_dtype, parameter_subtype)
				'' Fixed-rank and element-typed descriptors share the runtime prefix.
				'' Use the same declared generic view as an ordinary RTL array call.
				hWriteLine(hGetCId(api_symbol) + "((" + hEmitType(parameter_dtype, parameter_subtype) + ")&" + hCErrSymbolText(descriptor) + ");")
			end if
		next
		if( symbGetProcStatGosub( errorplan->owner ) ) then
			hWriteLine( "fb_GosubExit(&" + hCErrSymbolText(symbGetProcGosubSym(errorplan->owner)) + ");" )
		end if
	end if
	'' The unsupported-resource path ends the program. A free after that
	'' nonreturning call cannot retire storage and must not be emitted.
	if( errorplan->heap andalso (errorplan->unsupported_cleanup = FALSE) ) then hWriteLine("__builtin_free(fb_error_capture);")
	do
		var cached = cptr(EXPRCACHENODE ptr, listGetHead(@ctx.exprcache))
		if( cached = NULL ) then exit do
		exprFreeTree(cached->expr)
		listDelNode(@ctx.exprcache, cached)
	loop
	assert(listGetHead(@ctx.exprnodes) = NULL)
	sectionEnd( )
	sectionUnindent( )
	hWriteLine( "}" )
	sectionEnd( )
	hCLeafEnd()
	irhlEmitProcEnd( )
	hCErrEmitObjectHelper()
	errorhelper = FALSE
	ctx.procentrylabel = NULL
end sub

private sub hCErrDestroy( )
	if( errorplan = NULL ) then exit sub
	for i as integer = 0 to errorplan->handlercount - 1
		var h = @errorplan->handlers[i]
		while( h->clones <> NULL )
			var n = h->clones
			h->clones = n->next
			astDelTree( n )
		wend
		deallocate h->live
	next
	hCErrDropRecoveries(errorplan->recoveries)
	deallocate errorplan->recovery_ids
	deallocate errorplan->original_labels
	deallocate errorplan->original_live
	deallocate errorplan->handlers
	deallocate errorplan->statements
	deallocate errorplan->captures
	deallocate errorplan->resources
	deallocate errorplan
	errorplan = NULL
end sub

'' end of backend/c/error-owner-code.bi
