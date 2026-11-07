'' Project: FreeBASIC C error-owner lowering
'' File: backend/c/error-owner-types.bi
'' Purpose: Define procedure-local capture and handler graph ownership.
'' Responsibilities: Retain source symbols while cloned statements are emitted.
'' This file intentionally does NOT change public AST or symbol layouts.

type C_ERROR_HANDLER
	label as FBSYMBOL ptr
	callback as FBSYMBOL ptr
	live as ubyte ptr
	clones as ASTNODE ptr
	clonetail as ASTNODE ptr
end type

type C_ERROR_RECOVERY
	owner as FBSYMBOL ptr
	label as FBSYMBOL ptr
	branch as ASTNODE ptr
	target as FBSYMBOL ptr
	target_id as integer
	next as C_ERROR_RECOVERY ptr
end type

type C_ERROR_PLAN
	owner as FBSYMBOL ptr
	statements as ASTNODE ptr ptr
	count as integer
	capacity as integer
	handlers as C_ERROR_HANDLER ptr
	handlercount as integer
	can_transfer as integer
	captures as FBSYMBOL ptr ptr
	capturecount as integer
	capturecapacity as integer
	serial as integer
	observed as integer
	original_live as ubyte ptr
	original_labels as ubyte ptr
	original_cursor as integer
	recoveries as C_ERROR_RECOVERY ptr
	recovery_ids as integer ptr
	heap as integer
	exit_label as FBSYMBOL ptr
	exit_target_id as integer '' Completed callback cleanup transfers to the owner return.
	cleanup_label as FBSYMBOL ptr
	object_callback as FBSYMBOL ptr
	cleanup_callback as FBSYMBOL ptr
	checkpoint_api as FBSYMBOL ptr
	resources as FBSYMBOL ptr ptr
	resourcecount as integer
	resourcecapacity as integer
	unsupported_cleanup as integer
	owner_prologue_section as integer
	owner_prologue_bytes as uinteger
end type

type C_ERROR_BOUNDARY
	proc as FBSYMBOL ptr
	label as FBSYMBOL ptr
	next as C_ERROR_BOUNDARY ptr
end type

dim shared as C_ERROR_RECOVERY ptr errorrecoveries
dim shared as C_ERROR_BOUNDARY ptr errorboundaries

dim shared as C_ERROR_PLAN ptr errorplan
dim shared as integer errorhelper, errorserial, errorabiwritten, errordeclsection

declare sub fbSemanticModelExportProc( byval proc as FBSYMBOL ptr, byval body as ASTNODE ptr, byval emitted as integer )
declare function hCErrPrepare( byval proc as FBSYMBOL ptr ) as integer
declare function hCErrCapture( byval sym as FBSYMBOL ptr ) as integer
declare function hCErrSymbolText( byval sym as FBSYMBOL ptr ) as string
declare function hCErrCallHandler( byval expr as EXPRNODE ptr ) as integer
declare sub hCErrSelect( byval index as integer, byval vr as IRVREG ptr, byval level as integer )
declare sub hCErrOwnerLocals( )
declare sub hCErrRecordOwnerPrologue( )
declare sub hCErrMoveOwnerPrologue( )
declare sub hCErrRegisterFrame( )
declare function hCErrKeepAPI(byval api as FBSYMBOL ptr) as integer
declare sub hCErrLeave( )
declare sub hCErrEmitHelpers( )
declare sub hCErrDestroy( )
declare sub hCErrDestroyBoundaries( )

declare function hCErrTakeRecoveries(byval proc as FBSYMBOL ptr) as C_ERROR_RECOVERY ptr
declare sub hCErrDropRecoveries(byref first as C_ERROR_RECOVERY ptr)

'' end of backend/c/error-owner-types.bi
