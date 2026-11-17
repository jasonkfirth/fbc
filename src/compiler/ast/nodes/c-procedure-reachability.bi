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
'' A resolved ASM operand also needs its original native entry and body.
#define CPROC_ASM_REFERENCE (&h40)
'' Partial object retirement can call a generated child destructor after AST flushing.
#define CPROC_CLEANUP_ROOT  (&h80)

declare sub astCProcRecordAddress( byval proc as FBSYMBOL ptr )
declare sub astCProcRecordAsmReference( byval proc as FBSYMBOL ptr )

declare sub astCProcRecordFieldCleanup( byval resource as FBSYMBOL ptr )
declare function astCProcHasAsmReference( byval proc as FBSYMBOL ptr ) as integer

#endif
'' end of ast/nodes/c-procedure-reachability.bi
