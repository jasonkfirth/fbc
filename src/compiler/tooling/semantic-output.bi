'' Project: FreeBASIC compiler - semantic output
'' File: tooling/semantic-output.bi
'' Purpose: Define checked staging and publication of semantic sidecars.
'' Responsibilities: Expose an opaque writer and protected artifact paths.
'' This file intentionally does NOT contain: serialization or filesystem layouts.

#ifndef __FB_SEMANTIC_OUTPUT_BI__
#define __FB_SEMANTIC_OUTPUT_BI__

'' The writer owns its stream, private directory, and protected path snapshots.
'' Finish releases it exactly once, whether publication succeeds or fails.
declare function fbSemanticOutputOpen( byval filename as const zstring ptr ) as any ptr
declare function fbSemanticOutputProtect _
	( byval writer as any ptr, byval filename as const zstring ptr ) as long
declare function fbSemanticOutputWrite _
	( byval writer as any ptr, byval buffer as const any ptr, byval bytes as uinteger ) as long

'' A writer may own one private journal beside its staging stream. The caller
'' closes the returned CRT stream; Finish removes the journal on either outcome.
declare function fbSemanticOutputJournal _
	( byval writer as any ptr, byval filename as zstring ptr, byval capacity as uinteger ) as any ptr
declare function fbSemanticOutputFinish( byval writer as any ptr, byval publish as long ) as long

#endif

'' end of tooling/semantic-output.bi
