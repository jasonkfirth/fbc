'' Project: FreeBASIC compiler - aggregate layout observations
'' File: tooling/semantic-layout.bas
'' Purpose: Retain source requests separately from actual native layout decisions.
'' Responsibilities: Bounded observation sequences, initialized snapshots and closure.
'' This file intentionally does NOT contain: layout computation or inferred alignment.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-layout.bi"
#include once "tooling/semantic-constructs.bi"
#include once "tooling/semantic-source.bi"
#include once "support/containers/hash.bi"

private enum SEMANTIC_LAYOUT_FAMILY
	SLF_PACKING
	SLF_BEGIN
	SLF_FIELD
	SLF_INSERT
	SLF_PROMOTE
	SLF_END
	SLF_SNAPSHOT
	SLF_COUNT
end enum

private type SEMANTIC_LAYOUT_SUBJECT
	key as string
	identity as longint
	ordinal as longint
	created as integer
	finalized as integer
	counts(0 to SLF_COUNT - 1) as longint
	next_subject as SEMANTIC_LAYOUT_SUBJECT ptr
end type

'' Module compilation is serial. The hash borrows each entry's immutable key;
'' end the hash before deleting its entries. No native pointer escapes a hook.
'' These bounds match the reader; the ordinary staging bound applies as well.
private const LAYOUT_LIMIT = 1000000
private const LAYOUT_BUCKETS = 4096
private dim shared as THASH layout_subjects
private dim shared as integer layout_hash_ready
private dim shared as SEMANTIC_LAYOUT_SUBJECT ptr layout_head, layout_tail
private dim shared as longint layout_subject_count
private dim shared as longint layout_counts(0 to SLF_COUNT - 1)
private dim shared as longint active_insert, active_parent, promotion_start

sub fbSemanticModelResetLayouts( )
	if( layout_hash_ready ) then hashEnd(@layout_subjects)
	layout_hash_ready = FALSE
	dim as SEMANTIC_LAYOUT_SUBJECT ptr item = layout_head
	while( item <> NULL )
		dim as SEMANTIC_LAYOUT_SUBJECT ptr following = item->next_subject
		delete item
		item = following
	wend
	layout_head = NULL
	layout_tail = NULL
	layout_subject_count = 0
	for family as integer = 0 to SLF_COUNT - 1
		layout_counts(family) = 0
	next
	active_insert = 0
	active_parent = 0
	promotion_start = 0
end sub

private sub hProperty( byref key as const string, byref value as const string )
	fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(@symbGetGlobalNamespc( ))) + TABCHAR + _
		key + TABCHAR + fbSemanticModelEscape(value))
end sub

private function hContext( ) as string
	return fbSemanticModelNumber(fbSemanticModelCurrentStatement( )) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelCurrentSource( )) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelCurrentCompound( ))
end function

private function hSubject( byval sym as FBSYMBOL ptr ) as SEMANTIC_LAYOUT_SUBJECT ptr
	if( (sym = NULL) orElse (sym->class <> FB_SYMBCLASS_STRUCT) ) then
		fbSemanticModelFailAt("aggregate layout observation has another symbol class")
		return NULL
	end if
	dim as longint identity = fbSemanticModelSymbolId(sym)
	if( identity = 0 ) then return NULL
	dim as string key = fbSemanticModelNumber(identity)
	if( layout_hash_ready ) then
		dim as SEMANTIC_LAYOUT_SUBJECT ptr existing = hashLookup(@layout_subjects, strptr(key))
		if( existing <> NULL ) then
			existing->finalized = abs(sym->udt.retdtype <> FB_DATATYPE_INVALID)
			return existing
		end if
	end if
	if( layout_subject_count >= LAYOUT_LIMIT ) then
		fbSemanticModelFailAt("aggregate layout subject limit exceeded")
		return NULL
	end if
	dim as SEMANTIC_LAYOUT_SUBJECT ptr item = new SEMANTIC_LAYOUT_SUBJECT
	if( item = NULL ) then
		fbSemanticModelFailAt("aggregate layout subject allocation failed")
		return NULL
	end if
	layout_subject_count += 1
	item->key = key
	item->identity = identity
	item->ordinal = layout_subject_count
	item->created = FALSE
	item->finalized = abs(sym->udt.retdtype <> FB_DATATYPE_INVALID)
	for family as integer = 0 to SLF_COUNT - 1
		item->counts(family) = 0
	next
	item->next_subject = NULL
	if( layout_tail = NULL ) then
		layout_head = item
	else
		layout_tail->next_subject = item
	end if
	layout_tail = item
	if( layout_hash_ready = FALSE ) then
		hashInit(@layout_subjects, LAYOUT_BUCKETS, FALSE)
		layout_hash_ready = TRUE
	end if
	if( hashAdd(@layout_subjects, strptr(item->key), item, hashHash(strptr(item->key))) = NULL ) then
		fbSemanticModelFailAt("aggregate layout index allocation failed")
		return NULL
	end if
	return item
end function

private function hNext( byval family as SEMANTIC_LAYOUT_FAMILY, byval item as SEMANTIC_LAYOUT_SUBJECT ptr ) as longint
	if( item = NULL ) then return 0
	if( layout_counts(family) >= LAYOUT_LIMIT ) then
		fbSemanticModelFailAt("aggregate layout observation limit exceeded")
		return 0
	end if
	layout_counts(family) += 1
	item->counts(family) += 1
	return layout_counts(family)
end function

private function hDecision( byref decision as FB_SEMANTIC_LAYOUT_DECISION ) as string
	dim as string reason
	select case decision.reason
	case FB_SEMLAYOUT_NONE: reason = "none"
	case FB_SEMLAYOUT_NATURAL: reason = "natural"
	case FB_SEMLAYOUT_PACKING: reason = "packing"
	case FB_SEMLAYOUT_MANAGED: reason = "managed-descriptor"
	case else
		fbSemanticModelFailAt("unknown native aggregate alignment decision")
		return ""
	end select
	return fbSemanticModelNumber(decision.offset) + TABCHAR + fbSemanticModelNumber(decision.packing) + _
		TABCHAR + fbSemanticModelTypeNumber(culng(decision.dtype)) + _
		TABCHAR + fbSemanticModelNumber(fbSemanticModelSymbolId(decision.subtype)) + _
		TABCHAR + fbSemanticModelNumber(decision.natural) + TABCHAR + fbSemanticModelNumber(decision.effective) + _
		TABCHAR + reason + TABCHAR + fbSemanticModelNumber(decision.padding)
end function

sub fbSemanticModelCapturePacking( byref request as FB_SEMANTIC_PACKING_INPUT, byval has_field as integer )
	request.statementid = 0
	request.sourceid = 0
	request.compoundid = 0
	request.has_field = has_field
	request.expressionid = 0
	request.requested = 0
	request.selected = 0
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	request.statementid = fbSemanticModelCurrentStatement( )
	request.sourceid = fbSemanticModelCurrentSource( )
	request.compoundid = fbSemanticModelCurrentCompound( )
	request.has_field = has_field
end sub

sub fbSemanticModelPackingInput( byval sym as FBSYMBOL ptr, byval parent as FBSYMBOL ptr, _
	byref request as FB_SEMANTIC_PACKING_INPUT, byval passed_alignment as integer )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	dim as SEMANTIC_LAYOUT_SUBJECT ptr item = hSubject(sym)
	dim as longint ordinal = hNext(SLF_PACKING, item)
	if( ordinal = 0 ) then exit sub
	dim as string requested
	if( request.has_field ) then requested = fbSemanticModelNumber(request.requested)
	hProperty("aggregate-packing-input:" + fbSemanticModelNumber(ordinal), item->key + TABCHAR + _
		fbSemanticModelNumber(request.statementid) + TABCHAR + fbSemanticModelNumber(request.sourceid) + _
		TABCHAR + fbSemanticModelNumber(request.compoundid) + TABCHAR + fbSemanticModelNumber(abs(request.has_field <> FALSE)) + _
		TABCHAR + fbSemanticModelNumber(request.expressionid) + TABCHAR + requested + _
		TABCHAR + fbSemanticModelNumber(request.selected) + TABCHAR + fbSemanticModelNumber(passed_alignment) + _
		TABCHAR + fbSemanticModelNumber(sym->udt.align) + TABCHAR + fbSemanticModelNumber(fbSemanticModelSymbolId(parent)))
end sub

sub fbSemanticModelLayoutBegin( byval sym as FBSYMBOL ptr, byval parent as FBSYMBOL ptr, byval passed_alignment as integer )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	dim as SEMANTIC_LAYOUT_SUBJECT ptr item = hSubject(sym)
	if( item = NULL ) then exit sub
	if( item->counts(SLF_BEGIN) <> 0 ) then
		fbSemanticModelFailAt("aggregate layout creation was observed twice")
		exit sub
	end if
	item->created = TRUE
	dim as longint ordinal = hNext(SLF_BEGIN, item)
	if( ordinal = 0 ) then exit sub
	hProperty("aggregate-layout-begin:" + fbSemanticModelNumber(ordinal), item->key + TABCHAR + _
		fbSemanticModelNumber(passed_alignment) + TABCHAR + fbSemanticModelNumber(sym->udt.align) + _
		TABCHAR + fbSemanticModelNumber(abs(symbGetUDTIsUnion(sym) <> FALSE)) + _
		TABCHAR + fbSemanticModelNumber(fbSemanticModelSymbolId(parent)) + TABCHAR + hContext( ))
end sub

sub fbSemanticModelLayoutField( byval parent as FBSYMBOL ptr, byval field_sym as FBSYMBOL ptr, _
	byval allocates as integer, byref decision as FB_SEMANTIC_LAYOUT_DECISION, byval padding as integer, byval storage_bytes as longint )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( (field_sym = NULL) orElse (field_sym->class <> FB_SYMBCLASS_FIELD) ) then
		fbSemanticModelFailAt("native layout field has another symbol class")
		exit sub
	end if
	dim as SEMANTIC_LAYOUT_SUBJECT ptr item = hSubject(parent)
	dim as longint ordinal = hNext(SLF_FIELD, item)
	if( ordinal = 0 ) then exit sub
	dim as string storage_kind
	if( field_sym->attrib and FB_SYMBATTRIB_DYNAMIC ) then
		storage_kind = "dynamic-alias"
	elseif( allocates ) then
		storage_kind = "allocated"
	elseif( field_sym->var_.bits > 0 ) then
		storage_kind = "shared-bitfield"
	else
		'' Native size-error recovery can retain a nonallocating field.
		'' The ordinary compiler failure still discards the module transaction.
		storage_kind = "not-allocated"
	end if
	hProperty("aggregate-layout-field:" + fbSemanticModelNumber(ordinal), item->key + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(field_sym)) + TABCHAR + storage_kind + TABCHAR + hDecision(decision) + _
		TABCHAR + fbSemanticModelNumber(padding) + TABCHAR + fbSemanticModelTypeNumber(culng(field_sym->typ)) + _
		TABCHAR + fbSemanticModelNumber(fbSemanticModelSymbolId(field_sym->subtype)) + _
		TABCHAR + fbSemanticModelNumber(field_sym->lgt) + TABCHAR + fbSemanticModelNumber(field_sym->ofs) + _
		TABCHAR + fbSemanticModelNumber(storage_bytes) + TABCHAR + fbSemanticModelNumber(field_sym->var_.bitpos) + _
		TABCHAR + fbSemanticModelNumber(field_sym->var_.bits) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(field_sym->var_.array.desc)) + _
		TABCHAR + fbSemanticModelNumber(parent->ofs) + TABCHAR + fbSemanticModelNumber(parent->lgt) + _
		TABCHAR + fbSemanticModelNumber(parent->udt.natalign))
end sub

function fbSemanticModelLayoutInsertBegin( byval parent as FBSYMBOL ptr, byval inner as FBSYMBOL ptr, _
	byref decision as FB_SEMANTIC_LAYOUT_DECISION, byval old_offset as longint, byval old_length as longint, _
	byval old_natural as integer ) as longint
	if( fbSemanticModelFullEnabled( ) = FALSE ) then return 0
	if( active_insert <> 0 ) then
		fbSemanticModelFailAt("overlapping anonymous aggregate insertions")
		return 0
	end if
	dim as SEMANTIC_LAYOUT_SUBJECT ptr item = hSubject(parent), child = hSubject(inner)
	if( (item = NULL) or (child = NULL) ) then return 0
	dim as longint ordinal = hNext(SLF_INSERT, item)
	if( ordinal = 0 ) then return 0
	active_insert = ordinal
	active_parent = item->identity
	promotion_start = layout_counts(SLF_PROMOTE)
	hProperty("aggregate-layout-insert:" + fbSemanticModelNumber(ordinal), item->key + TABCHAR + child->key + _
		TABCHAR + hDecision(decision) + TABCHAR + fbSemanticModelNumber(old_offset) + _
		TABCHAR + fbSemanticModelNumber(old_length) + TABCHAR + fbSemanticModelNumber(old_natural) + _
		TABCHAR + fbSemanticModelNumber(parent->ofs) + TABCHAR + fbSemanticModelNumber(inner->lgt) + _
		TABCHAR + fbSemanticModelNumber(inner->udt.natalign) + _
		TABCHAR + fbSemanticModelNumber(abs(symbGetUDTIsUnion(parent) <> FALSE)))
	return ordinal
end function

sub fbSemanticModelLayoutPromote( byval insertion as longint, byval member as FBSYMBOL ptr, byval old_offset as longint )
	if( insertion = 0 ) then exit sub
	if( (insertion <> active_insert) or (member = NULL) ) then
		fbSemanticModelFailAt("anonymous member relocation has another insertion")
		exit sub
	end if
	dim as string key = fbSemanticModelNumber(active_parent)
	dim as SEMANTIC_LAYOUT_SUBJECT ptr item = hashLookup(@layout_subjects, strptr(key))
	dim as longint ordinal = hNext(SLF_PROMOTE, item)
	if( ordinal = 0 ) then exit sub
	dim as longint owner = 0
	if( member->symtb <> NULL ) then owner = fbSemanticModelSymbolId(member->symtb->owner)
	hProperty("aggregate-layout-promote:" + fbSemanticModelNumber(ordinal), fbSemanticModelNumber(insertion) + _
		TABCHAR + fbSemanticModelNumber(fbSemanticModelSymbolId(member)) + TABCHAR + fbSemanticModelNumber(old_offset) + _
		TABCHAR + fbSemanticModelNumber(member->ofs) + TABCHAR + fbSemanticModelNumber(owner) + _
		TABCHAR + fbSemanticModelNumber(abs(member->class = FB_SYMBCLASS_FIELD)))
end sub

sub fbSemanticModelLayoutInsertEnd( byval insertion as longint, byval parent as FBSYMBOL ptr )
	if( insertion = 0 ) then exit sub
	if( (insertion <> active_insert) orElse (fbSemanticModelSymbolId(parent) <> active_parent) ) then
		fbSemanticModelFailAt("anonymous insertion closes another layout")
		exit sub
	end if
	hProperty("aggregate-layout-insert-result:" + fbSemanticModelNumber(insertion), _
		fbSemanticModelNumber(parent->ofs) + TABCHAR + fbSemanticModelNumber(parent->lgt) + _
		TABCHAR + fbSemanticModelNumber(parent->udt.natalign) + _
		TABCHAR + fbSemanticModelNumber(layout_counts(SLF_PROMOTE) - promotion_start))
	active_insert = 0
	active_parent = 0
	promotion_start = 0
end sub

sub fbSemanticModelLayoutEnd( byval sym as FBSYMBOL ptr, byref decision as FB_SEMANTIC_LAYOUT_DECISION )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	dim as SEMANTIC_LAYOUT_SUBJECT ptr item = hSubject(sym)
	dim as longint ordinal = hNext(SLF_END, item)
	if( ordinal = 0 ) then exit sub
	if( sym->udt.retdtype = FB_DATATYPE_INVALID ) then
		fbSemanticModelFailAt("native aggregate completion lacks its initialized classifier")
		exit sub
	end if
	hProperty("aggregate-layout-end:" + fbSemanticModelNumber(ordinal), item->key + TABCHAR + hDecision(decision) + _
		TABCHAR + fbSemanticModelNumber(sym->udt.unpadlgt) + TABCHAR + fbSemanticModelNumber(sym->lgt) + _
		TABCHAR + fbSemanticModelNumber(sym->udt.natalign) + TABCHAR + fbSemanticModelNumber(sym->udt.align))
end sub

sub fbSemanticModelLayoutSnapshot( byval sym as FBSYMBOL ptr, byval identity as longint )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	dim as SEMANTIC_LAYOUT_SUBJECT ptr item = hSubject(sym)
	dim as longint ordinal = hNext(SLF_SNAPSHOT, item)
	if( ordinal = 0 ) then exit sub
	if( item->identity <> identity ) then
		fbSemanticModelFailAt("aggregate layout snapshot has another native identity")
		exit sub
	end if
	dim as longint base_storage = 0
	if( sym->udt.base <> NULL ) then base_storage = fbSemanticModelSymbolId(sym->udt.base)
	hProperty("aggregate-layout-snapshot:" + fbSemanticModelNumber(ordinal), item->key + TABCHAR + _
		fbSemanticModelNumber(sym->udt.natalign) + TABCHAR + fbSemanticModelNumber(sym->udt.align) + _
		TABCHAR + fbSemanticModelNumber(sym->lgt) + TABCHAR + fbSemanticModelNumber(sym->udt.unpadlgt) + _
		TABCHAR + fbSemanticModelNumber(item->finalized) + TABCHAR + fbSemanticModelNumber(base_storage))
end sub

sub fbSemanticModelFinishLayouts( byval commit as integer )
	if( commit and fbSemanticModelFullEnabled( ) ) then
		if( active_insert <> 0 ) then fbSemanticModelFailAt("anonymous aggregate insertion remains open")
		dim as SEMANTIC_LAYOUT_SUBJECT ptr item = layout_head
		while( item <> NULL )
			dim as string payload = item->key + TABCHAR + iif(item->created, "created", "preexisting") + _
				TABCHAR + fbSemanticModelNumber(item->finalized)
			for family as integer = 0 to SLF_COUNT - 1
				payload += TABCHAR + fbSemanticModelNumber(item->counts(family))
			next
			hProperty("aggregate-layout-subject:" + fbSemanticModelNumber(item->ordinal), payload)
			item = item->next_subject
		wend
		dim as string counts
		for family as integer = 0 to SLF_COUNT - 1
			if( family > 0 ) then counts += TABCHAR
			counts += fbSemanticModelNumber(layout_counts(family))
		next
		counts += TABCHAR + fbSemanticModelNumber(layout_subject_count)
		hProperty("aggregate-layout-counts:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )), counts)
	end if
	fbSemanticModelResetLayouts( )
end sub

'' end of tooling/semantic-layout.bas
