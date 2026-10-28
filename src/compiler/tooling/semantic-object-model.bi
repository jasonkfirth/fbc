'' Project: FreeBASIC compiler - native object model observations
'' File: tooling/semantic-object-model.bi
'' Purpose: Copy initialized RTTI and virtual table decisions into the sidecar.
'' Responsibilities: Type snapshots, selected slots and native initializer states.
'' This file intentionally does NOT contain: object layout or dispatch selection.

#ifndef __FB_SEMANTIC_OBJECT_MODEL_BI__
#define __FB_SEMANTIC_OBJECT_MODEL_BI__
#include once "core/fbint.bi"

declare sub fbSemanticModelResetObjectModel( )
declare sub fbSemanticModelFinishObjectModel( byval commit as integer )
declare sub fbSemanticModelObjectSnapshot( byval udt as FBSYMBOL ptr, byval identity as longint )
declare sub fbSemanticModelObjectVtable( byval udt as FBSYMBOL ptr, byval table as FBSYMBOL ptr, _
	byval base_elements as integer, byval copied_elements as integer, byval tree as ASTNODE ptr )
declare sub fbSemanticModelObjectRtti( byval udt as FBSYMBOL ptr, byval rtti as FBSYMBOL ptr, _
	byref state as const string, byval tree as ASTNODE ptr )

#endif

'' end of tooling/semantic-object-model.bi
