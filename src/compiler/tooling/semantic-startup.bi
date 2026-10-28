'' Project: FreeBASIC compiler - startup and instance lifecycle observations
'' File: tooling/semantic-startup.bi
'' Purpose: Copy native registrations, wrapper ownership and instance action order.
'' Responsibilities: Bounded source/list/guard and selected-emission receipts.
'' This file intentionally does NOT contain: initialization or cleanup lowering.

#ifndef __FB_SEMANTIC_STARTUP_BI__
#define __FB_SEMANTIC_STARTUP_BI__
#include once "core/fbint.bi"

declare sub fbSemanticModelResetStartup( )
declare sub fbSemanticModelFinishStartup( byval commit as integer )
declare sub fbSemanticModelStartupRegistration( byval proc as FBSYMBOL ptr, byref kind as const string )
declare sub fbSemanticModelStartupPriority( byval proc as FBSYMBOL ptr, byval priority as integer )
declare sub fbSemanticModelDeclarationOrder( byval sym as longint, byval owner as longint, byval groupid as longint, _
	byval ordinal as integer, byval statement as longint, byval sourceid as longint )
declare function fbSemanticModelGlobalInstance( byval sym as FBSYMBOL ptr, byval initialized as integer, _
	byval cleanup as integer ) as longint
declare function fbSemanticModelStaticInstance( byval sym as FBSYMBOL ptr, byval parent as FBSYMBOL ptr, _
	byval wrapper as FBSYMBOL ptr, byval initialized as integer, byval cleanup as integer, byval registered as integer, _
	byref purpose as const string ) as longint
declare sub fbSemanticModelStaticGuard( byval identity as longint, byval flag as FBSYMBOL ptr, byval exit_label as FBSYMBOL ptr )
declare sub fbSemanticModelStartupWrapper( byval proc as FBSYMBOL ptr, byref kind as const string )
declare sub fbSemanticModelGlobalInstanceAction( byval identity as longint, byval wrapper as FBSYMBOL ptr, _
	byref kind as const string, byval ordinal as integer )
declare sub fbSemanticModelStaticCleanupBody( byval sym as FBSYMBOL ptr, byval parent as FBSYMBOL ptr, byval wrapper as FBSYMBOL ptr )
declare sub fbSemanticModelStartupEmission( byval proc as FBSYMBOL ptr, byref kind as const string, _
	byref stage as const string, byref role as const string, byref selected_priority as const string, _
	byref placement as const string, byref spelling as const string )

#endif

'' end of tooling/semantic-startup.bi
