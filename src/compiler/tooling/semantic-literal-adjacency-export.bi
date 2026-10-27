'' Project: FreeBASIC compiler semantic observations
'' File: tooling/semantic-literal-adjacency-export.bi
'' Purpose: Preserve implicit literal adjacency before constant folding loses its inputs.
'' Responsibilities: Counted canonical input edges, distinct result snapshots and reset state.
'' This file intentionally does NOT contain: retained AST pointers or generated program code.

#ifndef FB_SEMANTIC_LITERAL_ADJACENCY_EXPORT_BI
#define FB_SEMANTIC_LITERAL_ADJACENCY_EXPORT_BI

'' The exporter is synchronous. This scalar belongs to the current module,
'' resets at BeginModule and is serialized only for committed full models.
dim shared as longint semantic_literal_adjacency_count

sub fbSemanticModelStringLiteralJoin _
    (byval expr as ASTNODE ptr, byval left_id as longint, byval right_id as longint, _
     byref first_site as LEX_LOCATION, byref last_site as LEX_LOCATION, _
     byval first_nonphysical as longint, byval last_nonphysical as longint)

    if( fbSemanticExtraFactsEnabled(FB_SEMEXTRA_LITERAL_GROUPS) = FALSE ) then exit sub
    if( fbSemanticModelFullEnabled( ) = FALSE or lex.ctx->semantic_probe ) then exit sub
    dim as longint statement_id = fbSemanticModelCurrentStatement( )
    if( statement_id = 0 ) then exit sub
    if( expr = NULL or left_id <= 0 or right_id <= 0 or left_id = right_id or _
        semantic_literal_adjacency_count >= SEMANTIC_MODEL_MAX_EXPRESSIONS_PER_MODEL ) then
        fbSemanticModelFailAt("literal adjacency input bounds")
        exit sub
    end if
    '' astNewBOP may already have freed both input nodes. The parser captured
    '' their IDs beforehand; only the resulting borrowed AST is read here.
    fbSemanticModelExportExpressionDistinct(expr, first_site, last_site, first_nonphysical, last_nonphysical)
    dim as longint result_id = expr->semantic_expression
    if( result_id <= left_id or result_id <= right_id ) then
        fbSemanticModelFailAt("literal adjacency result identity")
        exit sub
    end if
    semantic_literal_adjacency_count += 1
    dim as string result_text = fbSemanticModelNumber(result_id)
    dim as string payload = fbSemanticModelNumber(left_id) + TABCHAR + fbSemanticModelNumber(right_id) + _
        TABCHAR + fbSemanticModelNumber(statement_id)
    fbSemanticModelAppendDetail("K" + TABCHAR + "expression" + TABCHAR + result_text + _
        TABCHAR + "string-literal-adjacency-input" + TABCHAR + fbSemanticModelEscape(payload))
    fbSemanticModelAppendDetail("H" + TABCHAR + "expression" + TABCHAR + result_text + TABCHAR + _
        "expression" + TABCHAR + fbSemanticModelNumber(left_id) + TABCHAR + "string-literal-adjacency-left" + TABCHAR + "0")
    fbSemanticModelAppendDetail("H" + TABCHAR + "expression" + TABCHAR + result_text + TABCHAR + _
        "expression" + TABCHAR + fbSemanticModelNumber(right_id) + TABCHAR + "string-literal-adjacency-right" + TABCHAR + "1")
end sub

#endif
'' end of tooling/semantic-literal-adjacency-export.bi
