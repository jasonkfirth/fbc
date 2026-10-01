'' Project: FreeBASIC compiler - semantic output
'' File: tooling/semantic-output.bi
'' Purpose: Define checked staging and publication of semantic sidecars.
'' Responsibilities: Expose an opaque writer and protected artifact paths.
'' This file intentionally does NOT contain: serialization or filesystem layouts.

#ifndef __FB_SEMANTIC_OUTPUT_BI__
#define __FB_SEMANTIC_OUTPUT_BI__

'' The writer owns its stream, private directory, and protected path snapshots.
'' Finish releases it exactly once, whether publication succeeds or fails.
extern "c"
declare function fbSemanticOutputOpen( byval filename as const zstring ptr ) as any ptr
declare function fbSemanticOutputProtect _
	( byval output as any ptr, byval filename as const zstring ptr ) as long
declare function fbSemanticOutputWrite _
	( byval output as any ptr, byval data as const any ptr, byval bytes as uinteger ) as long
declare function fbSemanticOutputFinish( byval output as any ptr, byval publish as long ) as long
end extern

#endif

'' end of tooling/semantic-output.bi
