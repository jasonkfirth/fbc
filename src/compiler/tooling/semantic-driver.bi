'' Project: FreeBASIC compiler - semantic driver inputs
'' File: tooling/semantic-driver.bi
'' Purpose: Observe the driver's selected module role and compilation inputs.
'' Responsibilities: Define bounded snapshots independent of driver storage.
'' This file intentionally does NOT contain: option parsing or linker policy.

#ifndef __FB_SEMANTIC_DRIVER_BI__
#define __FB_SEMANTIC_DRIVER_BI__

declare sub fbSemanticModelDriverReset( )
declare sub fbSemanticModelDriverBegin _
	( byref role as const string, byref source as const string, byref object_file as const string, _
	  byval is_main as integer, byval ordinal as integer )
declare sub fbSemanticModelDriverInput( byref kind as const string, byref value as const string )
declare function fbSemanticModelDriverReady( ) as integer
declare function fbSemanticModelDriverGenerated( ) as integer
declare sub fbSemanticModelExportDriverInputs( )
declare sub fbSemanticModelExportModuleLibraries( )

#endif

'' end of tooling/semantic-driver.bi
