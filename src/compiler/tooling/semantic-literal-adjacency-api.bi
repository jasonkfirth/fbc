'' Project: FreeBASIC compiler semantic observations
'' File: tooling/semantic-literal-adjacency-api.bi
'' Purpose: Declare the narrow parser-to-exporter literal adjacency observation.
'' Responsibilities: Borrowed result and stable pre-fold child identities only.
'' This file intentionally does NOT contain: AST layout or parser semantic changes.

#ifndef FB_SEMANTIC_LITERAL_ADJACENCY_API_BI
#define FB_SEMANTIC_LITERAL_ADJACENCY_API_BI
declare sub fbSemanticModelStringLiteralJoin _
    (byval expr as ASTNODE ptr, byval left_id as longint, byval right_id as longint, _
     byref first_site as LEX_LOCATION, byref last_site as LEX_LOCATION, _
     byval first_nonphysical as longint, byval last_nonphysical as longint)
#endif
'' end of tooling/semantic-literal-adjacency-api.bi
