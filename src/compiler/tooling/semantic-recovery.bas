'' Project: FreeBASIC compiler - native expression recovery
'' File: tooling/semantic-recovery.bas
'' Purpose: Preserve the actual parser recovery steps behind typed expressions.
'' Responsibilities: Copy native sites, roots, contexts and finite step counts.
'' This file intentionally does NOT contain: replacement construction or final AST identity.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-recovery.bi"
#include once "tooling/semantic-constructs.bi"
#include once "tooling/semantic-source.bi"
#include once "diagnostics/error.bi"

'' The parser runs serially. Only scalars and copied strings enter the module
'' buffer; an observed root can be folded or freed immediately after return.
private const RECOVERY_STEP_LIMIT = 100000
private dim shared as longint recovery_step_count
private dim shared as longint recovery_skip_count
private dim shared as longint recovery_type_count, recovery_symbol_count

sub fbSemanticModelResetRecovery( )
	recovery_step_count = 0
	recovery_skip_count = 0
	recovery_type_count = 0
	recovery_symbol_count = 0
end sub

private sub hRecoveryProperty( byref key as const string, byref value as const string )
	fbSemanticModelAppendProvenance("K" + TABCHAR + "module" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelModuleIdentity( )) + TABCHAR + key + TABCHAR + fbSemanticModelEscape(value))
end sub

function fbSemanticModelRecoveryNode( byval node as ASTNODE ptr, _
	byref kind as const string, byref route as const string, byref native_file as const string, _
	byval native_line as integer ) as ASTNODE ptr
	if( fbSemanticModelTypedExpressionsEnabled( ) = FALSE ) then return node
	if( lex.ctx = NULL ) then
		fbSemanticModelFailAt("recovery observation has no lexer")
		return node
	end if
	if( lex.ctx->semantic_probe ) then return node
	if( (recovery_step_count >= RECOVERY_STEP_LIMIT) or (native_line < 1) or _
		(len(route) = 0) or (len(native_file) = 0) ) then
		fbSemanticModelFailAt("invalid native expression recovery site")
		return node
	end if
	select case kind
	case "replacement-constant", "replacement-zero", "retained-prefix", "replacement-expression"
	case else
		fbSemanticModelFailAt("unknown native expression recovery kind")
		return node
	end select

	dim as integer root_class = 0
	dim as longint dtype = FB_DATATYPE_INVALID
	if( node <> NULL ) then
		root_class = node->class
		'' Raw type bits have the same unsigned width on 32- and 64-bit hosts.
		dtype = culng(node->dtype)
		if( (root_class < 0) or (root_class >= AST_CLASSES) or _
			((kind = "replacement-constant") and (root_class <> AST_NODECLASS_CONST)) ) then
			fbSemanticModelFailAt("invalid native recovery root class")
			return node
		end if
	end if
	dim as LEX_LOCATION source = lexGetLastLocation( )
	dim as integer physical = source.is_physical and source.raw_valid
	recovery_step_count += 1
	hRecoveryProperty("native-expression-recovery:" + fbSemanticModelNumber(recovery_step_count), _
		kind + TABCHAR + fbSemanticModelEscape(route) + TABCHAR + fbSemanticModelEscape(native_file) + TABCHAR + _
		fbSemanticModelNumber(native_line) + TABCHAR + fbSemanticModelNumber(fbSemanticModelCurrentStatement( )) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelCurrentSource( )) + TABCHAR + fbSemanticModelNumber(fbSemanticModelCurrentContext( )) + TABCHAR + _
		fbSemanticModelNumber(errGetCount( )) + TABCHAR + fbSemanticModelNumber(abs(node <> NULL)) + TABCHAR + _
		fbSemanticModelNumber(root_class) + TABCHAR + fbSemanticModelTypeNumber(dtype) + TABCHAR + _
		fbSemanticModelEscape(source.source_file) + TABCHAR + fbSemanticModelNumber(source.start_line) + TABCHAR + _
		fbSemanticModelNumber(source.start_column) + TABCHAR + fbSemanticModelNumber(iif(physical, source.raw_start_line, 0)) + TABCHAR + _
		fbSemanticModelNumber(iif(physical, source.raw_start_column, 0)) + TABCHAR + fbSemanticModelNumber(abs(physical)))
	return node
end function

function fbSemanticModelRecoveryEnabled( ) as integer
	if( fbSemanticModelTypedExpressionsEnabled( ) = FALSE ) then return FALSE
	if( lex.ctx = NULL ) then return FALSE
	return lex.ctx->semantic_probe = FALSE
end function

private function hSkipPoint( byref source as LEX_LOCATION ) as string
	dim as integer physical = source.is_physical and source.raw_valid and _
		(source.raw_start_line > 0) and (source.raw_start_column >= 0)
	return fbSemanticModelEscape(source.source_file) + TABCHAR + fbSemanticModelNumber(source.source_context) + TABCHAR + _
		fbSemanticModelNumber(source.start_line) + TABCHAR + fbSemanticModelNumber(source.start_column) + TABCHAR + _
		fbSemanticModelNumber(iif(physical, source.raw_start_line, 0)) + TABCHAR + _
		fbSemanticModelNumber(iif(physical, source.raw_start_column, 0)) + TABCHAR + fbSemanticModelNumber(abs(physical))
end function

sub fbSemanticModelRecoverySkip( byval token as integer, byval eat as integer, _
	byval flags as integer, byval comma as integer, byval skipped as longint, _
	byval stopped_token as integer, byval consumed_stop as integer, byval nesting as integer, _
	byval errors_before as integer, byref first as LEX_LOCATION, byref stopping as LEX_LOCATION, _
	byref reason as const string )
	if( fbSemanticModelRecoveryEnabled( ) = FALSE ) then exit sub
	if( (recovery_skip_count >= RECOVERY_STEP_LIMIT) or (skipped < 0) or (nesting < 0) or _
		(errors_before < 0) or (errGetCount( ) < errors_before) ) then
		fbSemanticModelFailAt("invalid native token skip observation")
		exit sub
	end if
	select case reason
	case "target", "end-of-line", "end-of-file", "statement-boundary", "comma"
	case else
		fbSemanticModelFailAt("unknown native token skip outcome")
		exit sub
	end select
	recovery_skip_count += 1
	hRecoveryProperty("native-token-skip:" + fbSemanticModelNumber(recovery_skip_count), _
		fbSemanticModelNumber(token) + TABCHAR + fbSemanticModelNumber(abs(eat <> FALSE)) + TABCHAR + _
		fbSemanticModelNumber(flags) + TABCHAR + fbSemanticModelNumber(abs(comma <> FALSE)) + TABCHAR + _
		fbSemanticModelNumber(skipped) + TABCHAR + fbSemanticModelNumber(stopped_token) + TABCHAR + _
		fbSemanticModelNumber(abs(consumed_stop <> FALSE)) + TABCHAR + fbSemanticModelNumber(nesting) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelCurrentStatement( )) + TABCHAR + fbSemanticModelNumber(fbSemanticModelCurrentContext( )) + TABCHAR + _
		fbSemanticModelNumber(errors_before) + TABCHAR + fbSemanticModelNumber(errGetCount( )) + TABCHAR + _
		hSkipPoint(first) + TABCHAR + hSkipPoint(stopping) + TABCHAR + reason)
end sub

private function hRecoverySite( byref route as const string, byref native_file as const string, _
	byval native_line as integer ) as string
	dim as LEX_LOCATION source = lexGetLastLocation( )
	return fbSemanticModelEscape(route) + TABCHAR + fbSemanticModelEscape(native_file) + TABCHAR + _
		fbSemanticModelNumber(native_line) + TABCHAR + fbSemanticModelNumber(fbSemanticModelCurrentStatement( )) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelCurrentContext( )) + TABCHAR + fbSemanticModelNumber(errGetCount( )) + TABCHAR + hSkipPoint(source)
end function

function fbSemanticModelRecoveryType( byval dtype as integer, byref kind as const string, _
	byref route as const string, byref native_file as const string, byval native_line as integer ) as integer
	if( fbSemanticModelRecoveryEnabled( ) = FALSE ) then return dtype
	if( (recovery_type_count >= RECOVERY_STEP_LIMIT) or (native_line < 1) or _
		(len(route) = 0) or (len(native_file) = 0) ) then
		fbSemanticModelFailAt("invalid native type recovery site")
		return dtype
	end if
	select case kind
	case "replacement"
	case "pointer"
		if( typeIsPtr(dtype) = FALSE ) then
			fbSemanticModelFailAt("recovery pointer type has no pointer depth")
			return dtype
		end if
	case "discarded"
		if( dtype <> FB_DATATYPE_INVALID ) then
			fbSemanticModelFailAt("discarded recovery type is not invalid")
			return dtype
		end if
	case else
		fbSemanticModelFailAt("unknown native type recovery kind")
		return dtype
	end select
	recovery_type_count += 1
	'' Only chosen type bits are observed here. Following native assignments
	'' still own subtype and length, which can be stale at this exact point.
	hRecoveryProperty("native-type-recovery:" + fbSemanticModelNumber(recovery_type_count), _
		kind + TABCHAR + fbSemanticModelTypeNumber(culng(dtype)) + TABCHAR + hRecoverySite(route, native_file, native_line))
	return dtype
end function

private sub hRecoverySymbol( byval sym as FBSYMBOL ptr, byref spelling as const string, _
	byref kind as const string, byref route as const string, byref native_file as const string, byval native_line as integer )
	if( fbSemanticModelRecoveryEnabled( ) = FALSE ) then exit sub
	if( (recovery_symbol_count >= RECOVERY_STEP_LIMIT) or (native_line < 1) or _
		(len(route) = 0) or (len(native_file) = 0) ) then
		fbSemanticModelFailAt("invalid native symbol recovery site")
		exit sub
	end if
	if( sym <> NULL ) then
		if( sym->class <> FB_SYMBCLASS_PARAM ) then
			fbSemanticModelFailAt("recovery parameter has another symbol class")
			exit sub
		end if
	elseif( (kind = "generated-name") and (len(spelling) = 0) ) then
		fbSemanticModelFailAt("generated recovery name is empty")
		exit sub
	end if
	dim as integer raw_class
	dim as longint dtype = FB_DATATYPE_INVALID, bytes, attrib, pattrib, stats
	dim as string copied_name = spelling, owner_name
	if( sym <> NULL ) then
		raw_class = sym->class
		dtype = culng(sym->typ)
		bytes = sym->lgt
		attrib = sym->attrib
		pattrib = sym->pattrib
		stats = sym->stats
		if( sym->id.name <> NULL ) then copied_name = *sym->id.name
		if( sym->symtb <> NULL ) then
			if( sym->symtb->owner <> NULL ) then
				if( sym->symtb->owner->id.name <> NULL ) then owner_name = *sym->symtb->owner->id.name
			end if
		end if
	end if
	recovery_symbol_count += 1
	'' These transient snapshots do not establish source bindings or retain
	'' symbol IDs from a rejected module. Names are copied while still live.
	hRecoveryProperty("native-symbol-recovery:" + fbSemanticModelNumber(recovery_symbol_count), _
		kind + TABCHAR + fbSemanticModelNumber(abs(sym <> NULL)) + TABCHAR + fbSemanticModelNumber(raw_class) + TABCHAR + _
		fbSemanticModelTypeNumber(dtype) + TABCHAR + fbSemanticModelNumber(bytes) + TABCHAR + _
		fbSemanticModelNumber(attrib) + TABCHAR + fbSemanticModelNumber(pattrib) + TABCHAR + fbSemanticModelNumber(stats) + TABCHAR + _
		fbSemanticModelEscape(copied_name) + TABCHAR + fbSemanticModelEscape(owner_name) + TABCHAR + hRecoverySite(route, native_file, native_line))
end sub

function fbSemanticModelRecoverySymbol( byval sym as FBSYMBOL ptr, _
	byref route as const string, byref native_file as const string, byval native_line as integer ) as FBSYMBOL ptr
	hRecoverySymbol(sym, "", "mock-parameter", route, native_file, native_line)
	return sym
end function

sub fbSemanticModelRecoveryName( byref spelling as const string, _
	byref route as const string, byref native_file as const string, byval native_line as integer )
	hRecoverySymbol(NULL, spelling, "generated-name", route, native_file, native_line)
end sub

sub fbSemanticModelFinishRecovery( byval commit as integer )
	if( commit and fbSemanticModelTypedExpressionsEnabled( ) ) then
		hRecoveryProperty("native-expression-recovery-count", fbSemanticModelNumber(recovery_step_count))
		hRecoveryProperty("native-token-skip-count", fbSemanticModelNumber(recovery_skip_count))
		hRecoveryProperty("native-type-recovery-count", fbSemanticModelNumber(recovery_type_count))
		hRecoveryProperty("native-symbol-recovery-count", fbSemanticModelNumber(recovery_symbol_count))
	end if
	fbSemanticModelResetRecovery( )
end sub

'' end of tooling/semantic-recovery.bas
