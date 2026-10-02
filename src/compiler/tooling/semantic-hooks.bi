'' Project: FreeBASIC compiler - semantic observation hooks
'' ------------------------------------------------------
''
'' File: tooling/semantic-hooks.bi
''
'' Purpose:
''
''     Let parser, symbol, and AST code report compiler-selected semantic facts.
''
'' Responsibilities:
''
''     - expose observations needed before symbol and AST storage is released
''     - define capture and release calls for argument source locations
''
'' This file intentionally does NOT contain:
''
''     - exporter buffers, serialization helpers, or independent name resolution
''

#ifndef __FB_SEMANTIC_HOOKS_BI__
#define __FB_SEMANTIC_HOOKS_BI__

#include once "core/fbint.bi"
#include once "ast/ast.bi"
#include once "lexer/lex.bi"

'' These calls belong to the compiler's serial module lifecycle. Observations
'' are inert when export is disabled. Captured argument locations belong to
'' the argument until its release hook; callers never own the export buffer.

declare function fbSemanticModelSymbolId( byval sym as FBSYMBOL ptr ) as longint
declare sub fbSemanticModelInitializeSymbol( byval sym as FBSYMBOL ptr )
declare sub fbSemanticModelAllocateSymbol( byval sym as FBSYMBOL ptr )
declare sub fbSemanticModelMarkDeclared( byval sym as FBSYMBOL ptr )
declare function fbSemanticModelCurrentContext( ) as longint
declare function fbSemanticModelEnabled( ) as integer
declare function fbSemanticModelBindingCount( ) as longint
declare sub fbSemanticModelSetAccess(byval node as ASTNODE ptr, byref role as const string)
declare sub fbSemanticModelDiagnostic(byref severity as const string, byval code as integer, byval line_number as integer, byval message as const zstring ptr, byval detail as const zstring ptr, byval custom_text as const zstring ptr)
declare function fbSemanticModelLocationIsPhysical( byref source as LEX_LOCATION ) as integer
declare sub fbSemanticModelSetDeclarationName( byval sym as FBSYMBOL ptr, byval declared_name as const zstring ptr )
declare sub fbSemanticModelExportDeclaration _
	( byval sym as FBSYMBOL ptr, byref source as LEX_LOCATION, _
	  byref role as const string, byref declared_name as const string )

declare sub fbSemanticModelCaptureArgumentSource _
	( byval arg as FB_CALL_ARG ptr, byref source_range as AST_SEMANTIC_SOURCE_RANGE )

declare sub fbSemanticModelReleaseArgumentSource( byval arg as FB_CALL_ARG ptr )

declare sub fbSemanticModelExportArgumentConstructor _
	( byval owner as FBSYMBOL ptr, byval target as FBSYMBOL ptr, _
	  byval source_range as AST_SEMANTIC_SOURCE_RANGE ptr )
declare sub fbSemanticModelAssociateTemporaryDestructorsForRange _
	( byval generation as ulongint, byval source_range as AST_SEMANTIC_SOURCE_RANGE ptr )

declare sub fbSemanticModelExportSymbolDetails _
	( byval sym as FBSYMBOL ptr, byval variables_live as integer = FALSE )

declare sub fbSemanticModelExportSymbolReplacement _
	( byval previous as FBSYMBOL ptr, byval canonical as FBSYMBOL ptr )

declare sub fbSemanticModelExportProcPtrReplacement _
	( byval previous as FBSYMBOL ptr, byval canonical as FBSYMBOL ptr )

declare sub fbSemanticModelExportRelation _
	( byref domain as const string, byval identity as longint, _
	  byval target as FBSYMBOL ptr, byref kind as const string, _
	  byval ordinal as longint = 0 )

declare sub fbSemanticModelExportOperation _
	( byval expression as ASTNODE ptr, byval op as integer, _
	  byref source as LEX_LOCATION )

#endif

'' end of tooling/semantic-hooks.bi
