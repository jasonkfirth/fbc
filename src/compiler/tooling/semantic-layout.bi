'' Project: FreeBASIC compiler - aggregate layout observations
'' File: tooling/semantic-layout.bi
'' Purpose: Observe source packing and native layout decisions before they disappear.
'' Responsibilities: Transient capture values and serial module observer interfaces.
'' This file intentionally does NOT contain: layout policy or retained symbol pointers.

#ifndef __FB_SEMANTIC_LAYOUT_BI__
#define __FB_SEMANTIC_LAYOUT_BI__

#include once "core/fbint.bi"

enum FB_SEMANTIC_LAYOUT_REASON
	FB_SEMLAYOUT_NONE
	FB_SEMLAYOUT_NATURAL
	FB_SEMLAYOUT_PACKING
	FB_SEMLAYOUT_MANAGED
end enum

'' Native callers own these values on their stack. A zero decision means
'' that no alignment calculation took place, as for a shared bitfield unit.
'' The subtype pointer is consumed during that same native call, never saved.
type FB_SEMANTIC_LAYOUT_DECISION
	reason as FB_SEMANTIC_LAYOUT_REASON
	dtype as integer
	subtype as FBSYMBOL ptr
	offset as longint
	packing as integer
	natural as integer
	effective as integer
	padding as integer
end type

type FB_SEMANTIC_PACKING_INPUT
	statementid as longint
	sourceid as longint
	compoundid as longint
	has_field as integer
	expressionid as longint
	requested as longint
	selected as integer
end type

declare sub fbSemanticModelResetLayouts( )
declare sub fbSemanticModelFinishLayouts( byval commit as integer )
declare sub fbSemanticModelCapturePacking( byref request as FB_SEMANTIC_PACKING_INPUT, byval has_field as integer )
declare sub fbSemanticModelPackingInput( byval sym as FBSYMBOL ptr, byval parent as FBSYMBOL ptr, _
	byref request as FB_SEMANTIC_PACKING_INPUT, byval passed_alignment as integer )
declare sub fbSemanticModelLayoutBegin( byval sym as FBSYMBOL ptr, byval parent as FBSYMBOL ptr, byval passed_alignment as integer )
declare sub fbSemanticModelLayoutField( byval parent as FBSYMBOL ptr, byval field_sym as FBSYMBOL ptr, _
	byval allocates as integer, byref decision as FB_SEMANTIC_LAYOUT_DECISION, byval padding as integer, byval storage_bytes as longint )
declare function fbSemanticModelLayoutInsertBegin( byval parent as FBSYMBOL ptr, byval inner as FBSYMBOL ptr, _
	byref decision as FB_SEMANTIC_LAYOUT_DECISION, byval old_offset as longint, byval old_length as longint, _
	byval old_natural as integer ) as longint
declare sub fbSemanticModelLayoutPromote( byval insertion as longint, byval member as FBSYMBOL ptr, byval old_offset as longint )
declare sub fbSemanticModelLayoutInsertEnd( byval insertion as longint, byval parent as FBSYMBOL ptr )
declare sub fbSemanticModelLayoutEnd( byval sym as FBSYMBOL ptr, byref decision as FB_SEMANTIC_LAYOUT_DECISION )
declare sub fbSemanticModelLayoutSnapshot( byval sym as FBSYMBOL ptr, byval identity as longint )

#endif

'' end of tooling/semantic-layout.bi
