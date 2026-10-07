'' Project: FreeBASIC compiler - semantic sidecar
'' File: tooling/semantic-nodes.bas
'' Purpose: Describe typed AST node payloads without exposing union layouts.
'' Responsibilities: Export stable node kinds, values, calls, and branch targets.
'' This file intentionally does NOT contain: AST construction or code generation.

#include once "tooling/semantic-private.bi"

'' -------------------------------------------------------------------------
'' Stable node names
'' -------------------------------------------------------------------------

private function hNodeKind(byval node as ASTNODE ptr) as string
	select case node->class
	case AST_NODECLASS_NOP: return "nop"
	case AST_NODECLASS_LOAD: return "load"
	case AST_NODECLASS_ASSIGN: return "assignment"
	case AST_NODECLASS_BOP: return "binary-operator"
	case AST_NODECLASS_UOP: return "unary-operator"
	case AST_NODECLASS_CONV: return "conversion"
	case AST_NODECLASS_ADDROF: return "address-of"
	case AST_NODECLASS_BRANCH: return "branch"
	case AST_NODECLASS_JMPTB: return "jump-table"
	case AST_NODECLASS_CALL: return "call"
	case AST_NODECLASS_CALLCTOR: return "construction"
	case AST_NODECLASS_STACK: return "stack"
	case AST_NODECLASS_MEM: return "memory"
	case AST_NODECLASS_LOOP: return "loop"
	case AST_NODECLASS_COMP: return "comparison"
	case AST_NODECLASS_LINK: return "sequence"
	case AST_NODECLASS_CONST: return "constant"
	case AST_NODECLASS_VAR: return "variable"
	case AST_NODECLASS_IDX: return "index"
	case AST_NODECLASS_FIELD: return "field"
	case AST_NODECLASS_DEREF: return "dereference"
	case AST_NODECLASS_LABEL: return "label"
	case AST_NODECLASS_ARG: return "argument"
	case AST_NODECLASS_OFFSET: return "offset"
	case AST_NODECLASS_DECL: return "declaration"
	case AST_NODECLASS_NIDXARRAY: return "array"
	case AST_NODECLASS_IIF: return "conditional"
	case AST_NODECLASS_LIT: return "literal-text"
	case AST_NODECLASS_ASM: return "assembly"
	case AST_NODECLASS_DATASTMT: return "data"
	case AST_NODECLASS_DBG: return "debug"
	case AST_NODECLASS_BOUNDCHK: return "bounds-check"
	case AST_NODECLASS_PTRCHK: return "pointer-check"
	case AST_NODECLASS_SCOPEBEGIN: return "scope-begin"
	case AST_NODECLASS_SCOPEEND: return "scope-end"
	case AST_NODECLASS_SCOPE_BREAK: return "scope-exit"
	case AST_NODECLASS_TYPEINI: return "initializer"
	case AST_NODECLASS_TYPEINI_PAD: return "initializer-padding"
	case AST_NODECLASS_TYPEINI_ASSIGN: return "initializer-assignment"
	case AST_NODECLASS_TYPEINI_CTORCALL: return "initializer-construction"
	case AST_NODECLASS_TYPEINI_CTORLIST: return "initializer-construction-list"
	case AST_NODECLASS_TYPEINI_SCOPEINI: return "initializer-scope-begin"
	case AST_NODECLASS_TYPEINI_SCOPEEND: return "initializer-scope-end"
	case AST_NODECLASS_PROC: return "procedure"
	case AST_NODECLASS_MACRO: return "runtime-macro"
	case AST_NODECLASS_USTRINDEX: return "unicode-index"
	case else: return "unknown"
	end select
end function

private sub hProperty(byval nodeid as longint, byref key as const string, byval value as string)
	fbSemanticModelAppendDetail("K" + TABCHAR + "node" + TABCHAR + _
		fbSemanticModelNumber(nodeid) + TABCHAR + key + TABCHAR + fbSemanticModelEscape(value))
end sub

private sub hNumber(byval nodeid as longint, byref key as const string, byval value as longint)
	hProperty(nodeid, key, fbSemanticModelNumber(value))
end sub

'' -------------------------------------------------------------------------
'' Class-specific payloads
'' -------------------------------------------------------------------------

sub fbSemanticModelExportNodeDetails(byval node as ASTNODE ptr, byval nodeid as longint)
	if( (fbSemanticModelFullEnabled( ) = FALSE) or (node = NULL) ) then exit sub
	hProperty(nodeid, "kind", hNodeKind(node))
	if( node->vector <> 0 ) then hNumber(nodeid, "vector-width", node->vector)
	'' Read only the union member belonging to this node class. In particular,
	'' conversion flags are not an operator and CALLCTOR is not AST_NODE_CALL.
	select case node->class
	case AST_NODECLASS_CONST
		fbSemanticModelExportValue("node", nodeid, node->dtype, @node->val.value)
		hNumber(nodeid, "literal-suffix", abs(node->val.hassuffix <> FALSE))
	case AST_NODECLASS_LOAD
		hNumber(nodeid, "result-load", abs(node->lod.isres <> FALSE))
	case AST_NODECLASS_ASM
		'' The parser has already distinguished BASIC symbols from assembler
		'' text. Preserve that sequence without interpreting instructions or
		'' treating registers and assembler-local labels as BASIC references.
		dim as ASTASMTOK ptr token = node->asm.tokhead
		dim as longint ordinal = 0
		do while( (token <> NULL) and fbSemanticModelFullEnabled( ) )
			if( ordinal >= 1000000 ) then
				fbSemanticModelFailAt("semantic-nodes.bas:98")
				exit sub
			end if
			dim as string token_kind, token_text
			dim as longint token_symbol = 0
			select case token->type
			case AST_ASMTOK_TEXT
				token_kind = "text"
				if( token->text <> NULL ) then token_text = *token->text
			case AST_ASMTOK_SYMB
				token_kind = "symbol"
				token_symbol = fbSemanticModelSymbolId(token->sym)
				if( token_symbol = 0 ) then
					fbSemanticModelFailAt("semantic-nodes.bas:111")
					exit sub
				end if
			case else
				fbSemanticModelFailAt("semantic-nodes.bas:115")
				exit sub
			end select
			fbSemanticModelAppendDetail("ASM" + TABCHAR + fbSemanticModelNumber(nodeid) + _
				TABCHAR + fbSemanticModelNumber(ordinal) + TABCHAR + token_kind + _
				TABCHAR + fbSemanticModelNumber(token_symbol) + TABCHAR + fbSemanticModelEscape(token_text))
			ordinal += 1
			token = token->next
		loop
		hNumber(nodeid, "assembly-token-count", ordinal)
		hProperty(nodeid, "assembly-effects", "unknown-memory-registers-control")
	case AST_NODECLASS_ASSIGN
		hNumber(nodeid, "operator-options", node->op.options)
		hNumber(nodeid, "initialization", abs((node->op.options and AST_OPOPT_ISINI) <> 0))
	case AST_NODECLASS_CALL
		dim as string call_kind = "direct"
		if( node->l <> NULL ) then
			if( node->call.semantic_target <> NULL ) then
				call_kind = "virtual"
				fbSemanticModelExportRelation("node", nodeid, node->call.semantic_target, "static-target")
			else
				call_kind = "indirect"
			end if
		elseif( node->call.isrtl ) then
			call_kind = "runtime"
		end if
		hProperty(nodeid, "call-kind", call_kind)
		hNumber(nodeid, "argument-count", node->call.args)
		fbSemanticModelExportRelation("node", nodeid, node->call.tmpres, "result-temporary")
	case AST_NODECLASS_ARG
		'' Passing convention and omitted source arguments are independent.
		'' Capture omission before the optional initializer is cloned, because
		'' its lowered value can be identical to an explicitly supplied value.
		hNumber(nodeid, "default-argument", abs(node->arg.semantic_defaulted <> FALSE))
		if( (node->semantic_expression > 0) and (node->arg.semantic_defaulted = FALSE) ) then
			hNumber(nodeid, "call-argument-expression", node->semantic_expression)
		end if
		select case node->arg.mode
		case FB_PARAMMODE_BYVAL: hProperty(nodeid, "passing-mode", "byval")
		case FB_PARAMMODE_BYREF: hProperty(nodeid, "passing-mode", "byref")
		case FB_PARAMMODE_BYDESC: hProperty(nodeid, "passing-mode", "bydesc")
		case else: hProperty(nodeid, "passing-mode", "default")
		end select
		hNumber(nodeid, "bytes", node->arg.lgt)
	case AST_NODECLASS_CONV
		hNumber(nodeid, "conversion", abs(node->cast.doconv <> FALSE))
		hNumber(nodeid, "float-narrowing", abs(node->cast.do_convfd2fs <> FALSE))
		hNumber(nodeid, "const-conversion", abs(node->cast.convconst <> FALSE))
	case AST_NODECLASS_VAR
		hNumber(nodeid, "byte-offset", node->var_.ofs)
	case AST_NODECLASS_IDX
		hNumber(nodeid, "byte-offset", node->idx.ofs)
		hNumber(nodeid, "index-scale", node->idx.mult)
	case AST_NODECLASS_DEREF
		hNumber(nodeid, "byte-offset", node->ptr.ofs)
	case AST_NODECLASS_OFFSET
		hNumber(nodeid, "byte-offset", node->ofs.ofs)
	case AST_NODECLASS_BOP, AST_NODECLASS_UOP, AST_NODECLASS_BRANCH
		hNumber(nodeid, "operator-options", node->op.options)
		hNumber(nodeid, "left-pointer-arithmetic", abs((node->op.options and AST_OPOPT_LPTRARITH) <> 0))
		hNumber(nodeid, "right-pointer-arithmetic", abs((node->op.options and AST_OPOPT_RPTRARITH) <> 0))
		hNumber(nodeid, "inverse-branch", abs((node->op.options and AST_OPOPT_DOINVERSE) <> 0))
		'' ADDROF initializes the operator number only. Its unused ex slot
		'' can still contain a previous pool node's payload, not a symbol.
		fbSemanticModelExportRelation("node", nodeid, node->op.ex, "branch-target")
		if( node->class = AST_NODECLASS_BRANCH ) then
			fbSemanticModelExportRelation("node", nodeid, node->sym, "branch-target")
			select case node->op.op
			case AST_OP_JMP: hProperty(nodeid, "branch-kind", "unconditional")
			case AST_OP_JEQ: hProperty(nodeid, "branch-kind", "equal")
			case AST_OP_JNE: hProperty(nodeid, "branch-kind", "not-equal")
			case AST_OP_JGT: hProperty(nodeid, "branch-kind", "greater-than")
			case AST_OP_JLT: hProperty(nodeid, "branch-kind", "less-than")
			case AST_OP_JGE: hProperty(nodeid, "branch-kind", "greater-or-equal")
			case AST_OP_JLE: hProperty(nodeid, "branch-kind", "less-or-equal")
			case AST_OP_CALL: hProperty(nodeid, "branch-kind", "subroutine-call")
			case AST_OP_RET: hProperty(nodeid, "branch-kind", "subroutine-return")
			case AST_OP_JUMPPTR: hProperty(nodeid, "branch-kind", "indirect")
			case AST_OP_CALLPTR: hProperty(nodeid, "branch-kind", "indirect-call")
			case else: hProperty(nodeid, "branch-kind", "unknown")
			end select
		end if
	case AST_NODECLASS_JMPTB
		fbSemanticModelExportRelation("node", nodeid, node->jmptb.deflabel, "default-target")
		hProperty(nodeid, "jump-bias", ltrim(str(node->jmptb.bias)))
		hProperty(nodeid, "jump-span", ltrim(str(node->jmptb.span)))
		'' Sparse table pairs are compiler-resolved labels and unsigned values.
		'' Preserve the pairs rather than inventing source IF/CASE expressions.
		if( (node->jmptb.labelcount < 0) or (node->jmptb.labelcount > 1000000) ) then
			fbSemanticModelFailAt("semantic-nodes.bas:204")
			exit sub
		end if
		if( (node->jmptb.labelcount > 0) and _
			((node->jmptb.values = NULL) or (node->jmptb.labels = NULL)) ) then
			fbSemanticModelFailAt("semantic-nodes.bas:209")
			exit sub
		end if
		for index as integer = 0 to node->jmptb.labelcount - 1
			fbSemanticModelAppendDetail("J" + TABCHAR + fbSemanticModelNumber(nodeid) + _
				TABCHAR + fbSemanticModelNumber(index) + TABCHAR + _
				ltrim(str(node->jmptb.values[index])) + TABCHAR + _
				fbSemanticModelNumber(fbSemanticModelSymbolId(node->jmptb.labels[index])))
		next
	case AST_NODECLASS_MEM
		hNumber(nodeid, "bytes", node->mem.bytes)
		hNumber(nodeid, "fill-byte", node->mem.fillchar)
		select case node->mem.op
		case AST_OP_MEMMOVE: hProperty(nodeid, "memory-operation", "move")
		case AST_OP_MEMSWAP: hProperty(nodeid, "memory-operation", "swap")
		case AST_OP_MEMFILL: hProperty(nodeid, "memory-operation", "fill")
		case AST_OP_STKCLEAR: hProperty(nodeid, "memory-operation", "stack-clear")
		case else: hProperty(nodeid, "memory-operation", "unknown")
		end select
	case AST_NODECLASS_STACK
		select case node->stack.op
		case AST_OP_PUSH: hProperty(nodeid, "stack-operation", "push")
		case AST_OP_POP: hProperty(nodeid, "stack-operation", "pop")
		case AST_OP_PUSHUDT: hProperty(nodeid, "stack-operation", "push-aggregate")
		case AST_OP_STACKALIGN: hProperty(nodeid, "stack-operation", "align")
		case else: hProperty(nodeid, "stack-operation", "unknown")
		end select
	case AST_NODECLASS_MACRO
		select case node->op.op
		case AST_OP_VA_START: hProperty(nodeid, "macro-operation", "va-start")
		case AST_OP_VA_END: hProperty(nodeid, "macro-operation", "va-end")
		case AST_OP_VA_COPY: hProperty(nodeid, "macro-operation", "va-copy")
		case AST_OP_VA_ARG: hProperty(nodeid, "macro-operation", "va-arg")
		case else: hProperty(nodeid, "macro-operation", "unknown")
		end select
	case AST_NODECLASS_LINK
		select case node->link.ret
		case AST_LINK_RETURN_LEFT: hProperty(nodeid, "result-edge", "left")
		case AST_LINK_RETURN_RIGHT: hProperty(nodeid, "result-edge", "right")
		case else: hProperty(nodeid, "result-edge", "none")
		end select
	case AST_NODECLASS_IIF
		fbSemanticModelExportRelation("node", nodeid, node->iif.falselabel, "false-target")
	case AST_NODECLASS_TYPEINI, AST_NODECLASS_TYPEINI_PAD, AST_NODECLASS_TYPEINI_ASSIGN, _
		AST_NODECLASS_TYPEINI_CTORCALL, AST_NODECLASS_TYPEINI_CTORLIST
		hNumber(nodeid, "byte-offset", node->typeini.ofs)
		if( node->class = AST_NODECLASS_TYPEINI_CTORLIST ) then
			hNumber(nodeid, "element-count", node->typeini.elements)
		else
			hNumber(nodeid, "bytes", node->typeini.bytes)
		end if
	case AST_NODECLASS_TYPEINI_SCOPEINI
		hNumber(nodeid, "byte-offset", node->typeiniscope.ofs)
		hNumber(nodeid, "bytes", node->typeiniscope.bytes)
		hNumber(nodeid, "array-initializer", abs(node->typeiniscope.is_array <> FALSE))
	case AST_NODECLASS_TYPEINI_SCOPEEND
		hNumber(nodeid, "byte-offset", node->typeiniscope.ofs)
		hNumber(nodeid, "bytes", node->typeiniscope.bytes)
	case AST_NODECLASS_LIT
		if( node->lit.text <> NULL ) then hProperty(nodeid, "text", *node->lit.text)
	end select
end sub

'' end of tooling/semantic-nodes.bas
