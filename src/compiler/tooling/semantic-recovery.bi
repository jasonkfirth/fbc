'' Project: FreeBASIC compiler - native expression recovery
'' File: tooling/semantic-recovery.bi
'' Purpose: Observe parser-created replacement and retained expression roots.
'' Responsibilities: Copied recovery steps and module-owned count closure.
'' This file intentionally does NOT contain: parser recovery decisions or retained AST pointers.

#ifndef __FB_SEMANTIC_RECOVERY_BI__
#define __FB_SEMANTIC_RECOVERY_BI__

#include once "ast/ast.bi"
#include once "lexer/lex.bi"

declare function fbSemanticModelRecoveryNode( byval node as ASTNODE ptr, _
	byref kind as const string, byref route as const string, byref native_file as const string, _
	byval native_line as integer ) as ASTNODE ptr

declare sub fbSemanticModelResetRecovery( )
declare sub fbSemanticModelFinishRecovery( byval commit as integer )

declare function fbSemanticModelRecoveryEnabled( ) as integer
declare function fbSemanticModelRecoveryType( byval dtype as integer, byref kind as const string, _
	byref route as const string, byref native_file as const string, byval native_line as integer ) as integer
declare function fbSemanticModelRecoverySymbol( byval sym as FBSYMBOL ptr, _
	byref route as const string, byref native_file as const string, byval native_line as integer ) as FBSYMBOL ptr
declare sub fbSemanticModelRecoveryName( byref spelling as const string, _
	byref route as const string, byref native_file as const string, byval native_line as integer )
declare sub fbSemanticModelRecoverySkip( byval token as integer, byval eat as integer, _
	byval flags as integer, byval comma as integer, byval skipped as longint, _
	byval stopped_token as integer, byval consumed_stop as integer, byval nesting as integer, _
	byval errors_before as integer, byref first as LEX_LOCATION, byref stopping as LEX_LOCATION, _
	byref reason as const string )

#endif
'' end of tooling/semantic-recovery.bi
