'' Project: FreeBASIC compiler - native object model observations
'' File: tooling/semantic-object-model.bas
'' Purpose: Preserve selected virtual slots and RTTI payloads without resolving again.
'' Responsibilities: Copied identities, initialization phases, bounds and closure.
'' This file intentionally does NOT contain: dispatch, layout or runtime type inference.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-object-model.bi"

'' -------------------------------------------------------------------------
'' Module-owned counters and readonly native initializer inspection
'' -------------------------------------------------------------------------

private const OBJECT_MODEL_LIMIT = 1000000
'' Native object tables reserve two prefix slots before any method pointers.
private const VTABLE_PREFIX = 2
'' A compiler-built fb_RTTI has stdlib-table, type-name and base-RTTI pointers.
private const RTTI_FIELDS = 3
'' astNewADDROF can retain a bounded chain of pointer-only conversions.
private const ADDRESS_CONVERSIONS = 16
private dim shared as longint snapshot_count, table_count, slot_count, member_count, rtti_count

sub fbSemanticModelResetObjectModel( )
	snapshot_count = 0
	table_count = 0
	slot_count = 0
	member_count = 0
	rtti_count = 0
end sub

private sub hProperty( byval owner as longint, byref key as const string, byref value as const string )
	fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + fbSemanticModelNumber(owner) + _
		TABCHAR + key + TABCHAR + fbSemanticModelEscape(value))
end sub

private function hGlobal( ) as longint
	return fbSemanticModelSymbolId(@symbGetGlobalNamespc( ))
end function

private function hBase( byval udt as FBSYMBOL ptr ) as FBSYMBOL ptr
	if( udt->udt.base = NULL ) then return NULL
	return udt->udt.base->subtype
end function

private function hFirst( byval tree as ASTNODE ptr, byval owner as FBSYMBOL ptr ) as ASTNODE ptr
	if( (tree = NULL) orElse (tree->class <> AST_NODECLASS_TYPEINI) ) then return NULL
	dim as ASTNODE ptr first = tree->l
	if( (first = NULL) orElse (first->class <> AST_NODECLASS_TYPEINI_SCOPEINI) ) then return NULL
	if( first->sym <> owner ) then return NULL
	return first->r
end function

private function hClosed( byval item as ASTNODE ptr, byval owner as FBSYMBOL ptr ) as integer
	if( item = NULL ) then return FALSE
	return (item->class = AST_NODECLASS_TYPEINI_SCOPEEND) and (item->sym = owner) and (item->r = NULL)
end function

private function hValue( byval node as ASTNODE ptr, byref target as FBSYMBOL ptr ) as integer
	target = NULL
	for depth as integer = 0 to ADDRESS_CONVERSIONS
		if( node = NULL ) then return FALSE
		select case node->class
		case AST_NODECLASS_CONST
			return typeIsPtr(node->dtype) and (node->val.i = 0)
		case AST_NODECLASS_OFFSET
			if( (node->sym = NULL) or (node->ofs.ofs <> 0) ) then return FALSE
			target = node->sym
			return TRUE
		case AST_NODECLASS_ADDROF
			if( node->op.op <> AST_OP_ADDROF ) then return FALSE
			if( (node->l = NULL) orElse (node->l->class <> AST_NODECLASS_VAR) ) then return FALSE
			if( node->l->var_.ofs <> 0 ) then return FALSE
			target = node->l->sym
			return target <> NULL
		case AST_NODECLASS_CONV
			if( (node->l = NULL) orElse (typeIsPtr(node->l->dtype) = FALSE) ) then return FALSE
			if( typeIsPtr(node->dtype) = FALSE ) then return FALSE
			node = node->l
		case else
			return FALSE
		end select
	next
	return FALSE
end function

'' -------------------------------------------------------------------------
'' UDT snapshot companions
'' -------------------------------------------------------------------------

sub fbSemanticModelObjectSnapshot( byval udt as FBSYMBOL ptr, byval identity as longint )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( (udt = NULL) orElse (udt->class <> FB_SYMBCLASS_STRUCT) ) then
		fbSemanticModelFailAt("object model snapshot has another type class")
		exit sub
	end if
	dim as FBSYMBOL ptr table = NULL, rtti = NULL
	dim as integer elements = 0, abstracts = 0, table_unused = FALSE, rtti_unused = FALSE
	if( udt->udt.ext <> NULL ) then
		table = udt->udt.ext->vtable
		rtti = udt->udt.ext->rtti
		elements = udt->udt.ext->vtableelements
		abstracts = udt->udt.ext->abstractcount
	end if
	if( table <> NULL ) then table_unused = symbGetIsUnusedVtable(table)
	if( rtti <> NULL ) then rtti_unused = symbGetIsUnusedVtable(rtti)
	dim as string state = "plain"
	if( symbGetHasRTTI(udt) ) then
		state = "pending"
		if( (table <> NULL) and (rtti <> NULL) ) then state = "owned"
		if( udt = symb.rtti.fb_object ) then
			state = iif(rtti = NULL, "runtime-pending", "runtime")
		end if
	end if
	'' Resolve identities before numbering. Exporting another native symbol
	'' may recursively export an independent UDT snapshot first.
	dim as longint base_id = fbSemanticModelSymbolId(hBase(udt))
	dim as longint table_id = fbSemanticModelSymbolId(table), rtti_id = fbSemanticModelSymbolId(rtti)
	if( (snapshot_count >= OBJECT_MODEL_LIMIT) or (elements < 0) or (elements > OBJECT_MODEL_LIMIT) or _
		(abstracts < 0) or (abstracts > OBJECT_MODEL_LIMIT) ) then
		fbSemanticModelFailAt("object model snapshot exceeds its bound")
		exit sub
	end if
	snapshot_count += 1
	hProperty(identity, "object-model-snapshot:" + fbSemanticModelNumber(snapshot_count), state + TABCHAR + _
		fbSemanticModelNumber(base_id) + TABCHAR + fbSemanticModelNumber(table_id) + TABCHAR + _
		fbSemanticModelNumber(rtti_id) + TABCHAR + fbSemanticModelNumber(elements) + TABCHAR + _
		fbSemanticModelNumber(abstracts) + TABCHAR + fbSemanticModelNumber(abs(table_unused <> FALSE)) + _
		TABCHAR + fbSemanticModelNumber(abs(rtti_unused <> FALSE)) + TABCHAR + fbSemanticModelNumber(abs(symbGetHasRTTI(udt))))
end sub

'' -------------------------------------------------------------------------
'' Completed native virtual table and declared member-slot associations
'' -------------------------------------------------------------------------

sub fbSemanticModelObjectVtable( byval udt as FBSYMBOL ptr, byval table as FBSYMBOL ptr, _
	byval base_elements as integer, byval copied_elements as integer, byval tree as ASTNODE ptr )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( (udt = NULL) orElse (udt->class <> FB_SYMBCLASS_STRUCT) ) then
		fbSemanticModelFailAt("virtual table observation has another owner class")
		exit sub
	end if
	if( (udt->udt.ext = NULL) orElse (udt->udt.ext->vtable <> table) orElse (table = NULL) ) then
		fbSemanticModelFailAt("virtual table observation lacks native ownership")
		exit sub
	end if
	dim as integer elements = udt->udt.ext->vtableelements
	if( (table_count >= OBJECT_MODEL_LIMIT) or (elements < VTABLE_PREFIX) or (elements > OBJECT_MODEL_LIMIT) or _
		(base_elements < 0) or (base_elements > elements) or (copied_elements < 0) or (copied_elements > elements - VTABLE_PREFIX) ) then
		fbSemanticModelFailAt("virtual table observation exceeds its bound")
		exit sub
	end if
	dim as FBSYMBOL ptr base_type = hBase(udt), base_table = NULL
	if( (base_type <> NULL) andalso (base_type->udt.ext <> NULL) ) then base_table = base_type->udt.ext->vtable
	dim as longint owner_id = fbSemanticModelSymbolId(udt), table_id = fbSemanticModelSymbolId(table)
	dim as longint rtti_id = fbSemanticModelSymbolId(udt->udt.ext->rtti)
	dim as longint base_id = fbSemanticModelSymbolId(base_type), base_table_id = fbSemanticModelSymbolId(base_table)
	table_count += 1
	dim as longint build = table_count
	hProperty(hGlobal( ), "virtual-table-build:" + fbSemanticModelNumber(build), _
		fbSemanticModelNumber(owner_id) + TABCHAR + fbSemanticModelNumber(table_id) + TABCHAR + _
		fbSemanticModelNumber(rtti_id) + TABCHAR + fbSemanticModelNumber(base_id) + TABCHAR + _
		fbSemanticModelNumber(base_table_id) + TABCHAR + fbSemanticModelNumber(elements) + TABCHAR + _
		fbSemanticModelNumber(base_elements) + TABCHAR + fbSemanticModelNumber(copied_elements) + _
		TABCHAR + fbSemanticModelNumber(abs(symbGetIsUnusedVtable(table))))
	dim as ASTNODE ptr item = hFirst(tree, table)
	for index as integer = 0 to elements - 1
		if( (item = NULL) orElse (item->class <> AST_NODECLASS_TYPEINI_ASSIGN) ) then
			fbSemanticModelFailAt("virtual table initializer has an incomplete element sequence")
			exit sub
		end if
		dim as FBSYMBOL ptr target
		if( hValue(item->l, target) = FALSE ) then
			fbSemanticModelFailAt("virtual table initializer has an unsupported pointer value")
			exit sub
		end if
		dim as longint target_id = fbSemanticModelSymbolId(target)
		if( slot_count >= OBJECT_MODEL_LIMIT ) then
			fbSemanticModelFailAt("virtual table slots exceed their bound")
			exit sub
		end if
		slot_count += 1
		hProperty(hGlobal( ), "virtual-table-slot:" + fbSemanticModelNumber(build) + ":" + fbSemanticModelNumber(index), _
			iif(target = NULL, "null", "address") + TABCHAR + fbSemanticModelNumber(target_id) + TABCHAR + _
			fbSemanticModelNumber(item->typeini.ofs) + TABCHAR + fbSemanticModelNumber(item->typeini.bytes))
		item = item->r
	next
	if( hClosed(item, table) = FALSE ) then
		fbSemanticModelFailAt("virtual table initializer has another closure")
		exit sub
	end if
	'' These are already assigned native slot indices. Reading declarations
	'' separately from actual slot contents preserves abstract redeclarations
	'' that deliberately do not replace an inherited concrete pointer.
	dim as integer own_members = 0, visited = 0
	dim as FBSYMBOL ptr member = symbGetCompSymbTb(udt).head
	while( member <> NULL )
		visited += 1
		if( visited > OBJECT_MODEL_LIMIT ) then
			fbSemanticModelFailAt("virtual member scan exceeds its bound")
			exit sub
		end if
		if( symbIsProc(member) ) then
			dim as integer index = symbProcGetVtableIndex(member)
			if( index > 0 ) then
				dim as longint member_id = fbSemanticModelSymbolId(member)
				dim as longint overridden_id = fbSemanticModelSymbolId(symbProcGetOverridden(member))
				if( member_count >= OBJECT_MODEL_LIMIT ) then
					fbSemanticModelFailAt("virtual members exceed their bound")
					exit sub
				end if
				member_count += 1
				own_members += 1
				hProperty(hGlobal( ), "virtual-table-member:" + fbSemanticModelNumber(build) + ":" + fbSemanticModelNumber(own_members), _
					fbSemanticModelNumber(index) + TABCHAR + fbSemanticModelNumber(member_id) + TABCHAR + _
					fbSemanticModelNumber(abs(symbIsAbstract(member))) + TABCHAR + fbSemanticModelNumber(overridden_id))
			end if
		end if
		member = member->next
	wend
	hProperty(hGlobal( ), "virtual-table-end:" + fbSemanticModelNumber(build), _
		fbSemanticModelNumber(elements) + TABCHAR + fbSemanticModelNumber(own_members))
end sub

'' -------------------------------------------------------------------------
'' Native RTTI payload initialization and deferred-state receipts
'' -------------------------------------------------------------------------

sub fbSemanticModelObjectRtti( byval udt as FBSYMBOL ptr, byval rtti as FBSYMBOL ptr, _
	byref state as const string, byval tree as ASTNODE ptr )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( (udt = NULL) orElse (udt->class <> FB_SYMBCLASS_STRUCT) ) then
		fbSemanticModelFailAt("RTTI observation has another owner class")
		exit sub
	end if
	if( (udt->udt.ext = NULL) orElse (udt->udt.ext->rtti <> rtti) orElse (rtti = NULL) ) then
		fbSemanticModelFailAt("RTTI observation lacks native ownership")
		exit sub
	end if
	dim as FBSYMBOL ptr base_type = hBase(udt), targets(0 to RTTI_FIELDS - 1)
	dim as string offsets(0 to RTTI_FIELDS - 1), stdlib_state = "unavailable"
	dim as integer values = 0
	if( state = "deferred" ) then
		if( (tree = NULL) orElse (tree->class <> AST_NODECLASS_NOP) ) then
			fbSemanticModelFailAt("deferred RTTI has another native placeholder")
			exit sub
		end if
	elseif( (state = "initialized") or (state = "activated") ) then
		dim as ASTNODE ptr item = hFirst(tree, rtti)
		for index as integer = 0 to RTTI_FIELDS - 1
			if( (item = NULL) orElse (item->class <> AST_NODECLASS_TYPEINI_ASSIGN) ) then
				fbSemanticModelFailAt("RTTI initializer has another field sequence")
				exit sub
			end if
			if( hValue(item->l, targets(index)) = FALSE ) then
				fbSemanticModelFailAt("RTTI initializer has another pointer value")
				exit sub
			end if
			offsets(index) = fbSemanticModelNumber(item->typeini.ofs)
			values += 1
			item = item->r
		next
		if( (hClosed(item, rtti) = FALSE) or (targets(0) <> NULL) ) then
			fbSemanticModelFailAt("RTTI initializer has another closure or stdlib table")
			exit sub
		end if
		stdlib_state = "null"
	else
		fbSemanticModelFailAt("RTTI observation has an unknown initialization state")
		exit sub
	end if
	dim as longint owner_id = fbSemanticModelSymbolId(udt), rtti_id = fbSemanticModelSymbolId(rtti)
	dim as longint base_id = fbSemanticModelSymbolId(base_type)
	dim as longint name_id = fbSemanticModelSymbolId(targets(1)), base_rtti_id = fbSemanticModelSymbolId(targets(2))
	if( rtti_count >= OBJECT_MODEL_LIMIT ) then
		fbSemanticModelFailAt("RTTI observations exceed their bound")
		exit sub
	end if
	rtti_count += 1
	hProperty(hGlobal( ), "rtti-initialization:" + fbSemanticModelNumber(rtti_count), _
		fbSemanticModelNumber(owner_id) + TABCHAR + fbSemanticModelNumber(rtti_id) + TABCHAR + state + _
		TABCHAR + fbSemanticModelNumber(base_id) + TABCHAR + fbSemanticModelNumber(base_rtti_id) + _
		TABCHAR + fbSemanticModelNumber(name_id) + TABCHAR + stdlib_state + TABCHAR + fbSemanticModelNumber(values) + _
		TABCHAR + offsets(0) + TABCHAR + offsets(1) + TABCHAR + offsets(2))
end sub

sub fbSemanticModelFinishObjectModel( byval commit as integer )
	if( commit and fbSemanticModelFullEnabled( ) ) then
		hProperty(hGlobal( ), "object-model-counts:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )), _
			fbSemanticModelNumber(snapshot_count) + TABCHAR + fbSemanticModelNumber(table_count) + TABCHAR + _
			fbSemanticModelNumber(slot_count) + TABCHAR + fbSemanticModelNumber(member_count) + TABCHAR + fbSemanticModelNumber(rtti_count))
	end if
	fbSemanticModelResetObjectModel( )
end sub

'' end of tooling/semantic-object-model.bas
