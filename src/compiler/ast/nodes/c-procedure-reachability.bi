/'
    Project: FreeBASIC compiler C back end
    File: ast/nodes/c-procedure-reachability.bi
    Purpose: Share procedure reachability observations between AST passes.
    Responsibilities: Keep observations outside symbols and preserve their ABI.
    This file intentionally does NOT contain: AST traversal or emitted C.
'/
#ifndef __FB_C_PROCEDURE_REACHABILITY_BI__
#define __FB_C_PROCEDURE_REACHABILITY_BI__

#define CPROC_ADDRESS_TAKEN (&h01)
#define CPROC_DIRECT_CALL   (&h02)
#define CPROC_REACHABLE     (&h04)
#define CPROC_CANDIDATE     (&h08)
#define CPROC_STATIC_QUEUED (&h10)
'' A static cleanup wrapper is emitted after body reachability settles.
#define CPROC_STATIC_ROOT   (&h20)

declare sub astCProcRecordAddress( byval proc as FBSYMBOL ptr )

#endif
'' end of ast/nodes/c-procedure-reachability.bi
