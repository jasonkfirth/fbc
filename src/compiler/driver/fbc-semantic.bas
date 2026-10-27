'' Project: FreeBASIC compiler driver
'' File: driver/fbc-semantic.bas
'' Purpose: Observe selected driver inputs before compiling a source module.
'' Responsibilities: Copy module roles, input order, and effective path policy.
'' This file intentionally does NOT contain: option parsing or serialization.

#include once "driver/fbc-private.bi"
#include once "tooling/semantic-driver.bi"

'' XPM conversion changes FBCIOFILE.srcfile to the generated BASIC filename.
'' Retain the original resource independently until invocation shutdown.
dim shared as string semantic_xpm_source

sub fbcDriverSemanticResource( byref filename as string )
	semantic_xpm_source = filename
	'' Protect the consumed resource before conversion replaces its driver name.
	'' Final publication must preserve this input for each requested artifact.
	fbSemanticModelProtectFile(filename)
	fbSemanticDiagnosticsProtectFile(filename)
	fbSemanticLinkProtect(filename)
end sub

private function hObjectKind( byval object_file as string ptr ) as string
	dim as FBCIOFILE ptr module = listGetHead(@fbc.modules)
	while( module <> NULL )
		if( module->objfile = object_file ) then return "source-object"
		module = listGetNext(module)
	wend
	module = listGetHead(@fbc.rcs)
	while( module <> NULL )
		if( module->objfile = object_file ) then return "resource-object"
		module = listGetNext(module)
	wend
	if( fbc.xpm.objfile = object_file ) then return "resource-object"
	return "object-input"
end function

private sub hStringSet( byval inputs as TSTRSET ptr, byref kind as const string )
	dim as TSTRSETITEM ptr item = listGetHead(@inputs->list)
	while( item <> NULL )
		fbSemanticModelDriverInput(kind, item->s)
		item = listGetNext(item)
	wend
end sub

sub fbcDriverSemanticModule( byval module as FBCIOFILE ptr, byval is_main as integer, _
	byval is_fbctinf as integer, byval module_count as integer )
	if( len(fbc.semanticmodel) = 0 ) then exit sub
	dim as string role = "secondary"
	if( is_main ) then role = "main"
	if( module = @fbc.xpm ) then role = "resource"
	if( is_fbctinf ) then role = "archive-metadata"
	fbSemanticModelDriverBegin(role, module->srcfile, *module->objfile, is_main, module_count)
	if( fbc.semanticmodel_bindings or fbc.semanticmodel_expressions ) then exit sub
	fbSemanticModelDriverInput("compiler-prefix", fbc.prefix)
	fbSemanticModelDriverInput("compiler-include", fbc.incpath)
	fbSemanticModelDriverInput("compiler-library", fbc.libpath)
	fbSemanticModelDriverInput("sysroot", fbc.sysroot)
	fbSemanticModelDriverInput("source-options", fbc.sourcecmdline)
	fbSemanticModelDriverInput("assembler-options", fbc.extopt.gas)
	fbSemanticModelDriverInput("compiler-options", fbc.extopt.gcc)
	fbSemanticModelDriverInput("linker-options", fbc.extopt.ld)
	fbSemanticModelDriverInput("tool-prefix", fbc.buildprefix)
	fbSemanticModelDriverInput("main-input", fbc.mainname)
	fbSemanticModelDriverInput("subsystem", fbc.subsystem)
#ifndef ENABLE_STANDALONE
	fbSemanticModelDriverInput("target-prefix", fbc.targetprefix)
#endif
	fbSemanticModelDriverInput("no-default-libraries", str(abs(fbc.nodeflibs <> FALSE)))
	fbSemanticModelDriverInput("no-runtime-startup", str(abs(fbc.nofbrt0 <> FALSE)))
	fbSemanticModelDriverInput("static-link", str(abs(fbc.staticlink <> FALSE)))
	fbSemanticModelDriverInput("strip-symbols", str(abs(fbc.stripsymbols <> FALSE)))
	fbSemanticModelDriverInput("retain-object", str(abs(fbc.keepobj <> FALSE)))
	fbSemanticModelDriverInput("source-emission-only", str(abs(fbc.emitasmonly <> FALSE)))
	fbSemanticModelDriverInput("assembly-emission-only", str(abs(fbc.emitfinalasmonly <> FALSE)))
	dim as FBCIOFILE ptr source_file = listGetHead(@fbc.modules)
	while( source_file <> NULL )
		fbSemanticModelDriverInput("source-input", source_file->srcfile)
		source_file = listGetNext(source_file)
	wend
	source_file = listGetHead(@fbc.rcs)
	while( source_file <> NULL )
		fbSemanticModelDriverInput("resource-input", source_file->srcfile)
		source_file = listGetNext(source_file)
	wend
	if( len(semantic_xpm_source) > 0 ) then
		fbSemanticModelDriverInput("resource-input", semantic_xpm_source)
	elseif( len(fbc.xpm.srcfile) > 0 ) then
		fbSemanticModelDriverInput("resource-input", fbc.xpm.srcfile)
	end if
	dim as string ptr item = listGetHead(@fbc.objlist)
	while( item <> NULL )
		fbSemanticModelDriverInput(hObjectKind(item), *item)
		item = listGetNext(item)
	wend
	item = listGetHead(@fbc.libfiles)
	while( item <> NULL )
		fbSemanticModelDriverInput("archive-input", *item)
		item = listGetNext(item)
	wend
	hStringSet(@fbc.libs, "library-input")
	hStringSet(@fbc.libpaths, "library-path")
	hStringSet(@fbc.excludedlibs, "excluded-library")
end sub

'' end of driver/fbc-semantic.bas
