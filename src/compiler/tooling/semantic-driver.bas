'' Project: FreeBASIC compiler - semantic driver inputs
'' File: tooling/semantic-driver.bas
'' Purpose: Retain the selected driver inputs with each committed source module.
'' Responsibilities: Own snapshots, ordered input receipts, and library closure.
'' This file intentionally does NOT contain: argument parsing or link resolution.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-driver.bi"

'' The driver compiles modules serially. A snapshot is copied before fbCompile
'' opens its transaction, then emitted with that module. Parser restarts replace
'' the snapshot and discard their previous staged facts. No driver pointer is
'' retained after the observation call.
type SEMANTIC_DRIVER_INPUT
	kind as string
	value as string
	next_input as SEMANTIC_DRIVER_INPUT ptr
end type

'' Large response files can contain many inputs. Keep observation storage
'' bounded separately from the exporter's total record and staging budgets.
const SEMANTIC_DRIVER_MAX_INPUTS = 65536
dim shared as SEMANTIC_DRIVER_INPUT ptr driver_inputs, driver_last_input
dim shared as integer driver_input_count, driver_ready, driver_is_main, driver_ordinal
dim shared as string driver_role, driver_source, driver_object
dim shared as longint driver_owner

sub fbSemanticModelDriverReset( )
	dim as SEMANTIC_DRIVER_INPUT ptr item = driver_inputs
	while( item <> NULL )
		dim as SEMANTIC_DRIVER_INPUT ptr next_input = item->next_input
		delete item
		item = next_input
	wend
	driver_inputs = NULL
	driver_last_input = NULL
	driver_input_count = 0
	driver_ready = FALSE
	driver_owner = 0
	driver_role = ""
	driver_source = ""
	driver_object = ""
end sub

sub fbSemanticModelDriverBegin _
	( byref role as const string, byref source as const string, byref object_file as const string, _
	  byval is_main as integer, byval ordinal as integer )
	fbSemanticModelDriverReset( )
	if( fbSemanticModelEnabled( ) = FALSE ) then exit sub
	if( (role <> "main") and (role <> "secondary") and (role <> "archive-metadata") and (role <> "resource") ) then
		fbSemanticModelFailAt("unknown driver module role")
		exit sub
	end if
	'' The native driver uses -1 for generated resource and archive modules.
	'' Preserve that sentinel; it is not a source input ordinal.
	if( (len(source) = 0) or (ordinal < -1) or (ordinal = 0) or _
	    ((ordinal = -1) and ((role = "main") or (role = "secondary"))) ) then
		fbSemanticModelFailAt("invalid driver module identity")
		exit sub
	end if
	driver_role = role
	driver_source = source
	driver_object = object_file
	driver_is_main = abs(is_main <> FALSE)
	driver_ordinal = ordinal
	driver_ready = TRUE
end sub

function fbSemanticModelDriverReady( ) as integer
	return driver_ready
end function

function fbSemanticModelDriverGenerated( ) as integer
	return driver_ready and ((driver_role = "resource") or (driver_role = "archive-metadata"))
end function

sub fbSemanticModelDriverInput( byref kind as const string, byref value as const string )
	if( driver_ready = FALSE ) then exit sub
	if( driver_input_count >= SEMANTIC_DRIVER_MAX_INPUTS ) then
		fbSemanticModelFailAt("driver input observation limit exceeded")
		exit sub
	end if
	dim as SEMANTIC_DRIVER_INPUT ptr item = new SEMANTIC_DRIVER_INPUT
	if( item = NULL ) then
		fbSemanticModelFailAt("driver input observation allocation failed")
		exit sub
	end if
	item->kind = kind
	item->value = value
	item->next_input = NULL
	if( driver_last_input = NULL ) then
		driver_inputs = item
	else
		driver_last_input->next_input = item
	end if
	driver_last_input = item
	driver_input_count += 1
end sub

private sub hProperty( byref key as const string, byref value as const string )
	fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + fbSemanticModelNumber(driver_owner) + _
		TABCHAR + key + TABCHAR + fbSemanticModelEscape(value))
end sub

private sub hStringInputs( byval inputs as TLIST ptr, byref kind as const string )
	dim as string ptr item = listGetHead(inputs)
	while( item <> NULL )
		fbSemanticModelDriverInput(kind, *item)
		item = listGetNext(item)
	wend
end sub

sub fbSemanticModelExportDriverInputs( )
	if( (driver_ready = FALSE) or (fbSemanticModelFullEnabled( ) = FALSE) ) then exit sub
	driver_owner = fbSemanticModelSymbolId(@symbGetGlobalNamespc( ))
	if( driver_owner = 0 ) then
		fbSemanticModelFailAt("driver input namespace is unavailable")
		exit sub
	end if
	'' These lists retain effective search order, including repeated paths.
	'' A requested preinclude is an input even when parsing later never opens
	'' it; FILE/SRC independently prove which source files were consumed.
	hStringInputs(@env.predefines, "predefine")
	hStringInputs(@env.preincludes, "preinclude")
	hStringInputs(@env.includepaths, "include-path")
	hProperty("driver-module", fbSemanticModelNumber(fbSemanticModelModuleIdentity( )))
	hProperty("driver-role", driver_role)
	hProperty("driver-source", driver_source)
	hProperty("driver-object", driver_object)
	hProperty("driver-main", fbSemanticModelNumber(driver_is_main))
	hProperty("driver-ordinal", fbSemanticModelNumber(driver_ordinal))
	hProperty("driver-entry", env.entry)
	hProperty("driver-outtype", fbSemanticModelNumber(env.clopt.outtype))
	hProperty("producer-version", FB_VERSION)
	hProperty("producer-build-date", FB_BUILD_DATE_ISO)
	hProperty("producer-source-sha1", FB_BUILD_SHA1)
	hProperty("producer-fork-id", FB_BUILD_FORK_ID)
	dim as integer ordinal = 0
	dim as SEMANTIC_DRIVER_INPUT ptr item = driver_inputs
	while( item <> NULL )
		hProperty("driver-input-" + fbSemanticModelNumber(ordinal), item->kind + TABCHAR + item->value)
		ordinal += 1
		item = item->next_input
	wend
	hProperty("driver-input-count", fbSemanticModelNumber(ordinal))
end sub

private sub hLibraries( byval inputs as TSTRSET ptr, byref kind as const string )
	dim as integer ordinal = 0
	dim as TSTRSETITEM ptr item = listGetHead(@inputs->list)
	while( item <> NULL )
		hProperty("driver-module-" + kind + "-" + fbSemanticModelNumber(ordinal), item->s)
		ordinal += 1
		item = listGetNext(item)
	wend
	hProperty("driver-module-" + kind + "-count", fbSemanticModelNumber(ordinal))
end sub

sub fbSemanticModelExportModuleLibraries( )
	if( (driver_owner = 0) or (fbSemanticModelFullEnabled( ) = FALSE) ) then exit sub
	'' These are the module's requested libraries, after accepted source
	'' directives. Their presence does not establish that a linker searched
	'' or selected an archive; native link observations have that separate role.
	hLibraries(@env.libs, "library")
	hLibraries(@env.libpaths, "library-path")
end sub

'' end of tooling/semantic-driver.bas
