/*
 * Project: FreeBASIC private native error bridge
 * File: drivers/win32/x86_64/error_set_handler_win64.s
 * Purpose: Capture the GAS caller's frame base before entering C runtime code.
 * Responsibilities: Preserve the one-argument public setter and return address.
 * This file intentionally does NOT contain: stack switching or metadata allocation.
 */
.intel_syntax noprefix
.text
.globl fb_ErrorSetHandler
.def fb_ErrorSetHandler; .scl 2; .type 32; .endef
.seh_proc fb_ErrorSetHandler
fb_ErrorSetHandler:
.seh_endprologue
/* Win64 passes the handler in RCX. RDX adds the caller's GAS frame base.
 * A tail branch preserves the original return address and shadow space. */
mov rdx, rbp
lea r8, [rsp+8]
jmp fb_ErrorSetNativeHandlerLegacy
.seh_endproc
/* GAS calls this leaf hook before its own frame expires. The native frame
 * base must be captured before a C prologue can replace RBP. */
.globl fb_ErrorNativeOwnerLeaveCurrent
.def fb_ErrorNativeOwnerLeaveCurrent; .scl 2; .type 32; .endef
.seh_proc fb_ErrorNativeOwnerLeaveCurrent
fb_ErrorNativeOwnerLeaveCurrent:
.seh_endprologue
mov rcx, rbp
jmp fb_ErrorNativeOwnerLeaveChecked
.seh_endproc
/* The pre-destruction hook retains the source handler while retiring
 * abandoned C children. Final owner retirement remains a separate hook. */
.globl fb_ErrorNativeOwnerCleanupCurrent
.def fb_ErrorNativeOwnerCleanupCurrent; .scl 2; .type 32; .endef
.seh_proc fb_ErrorNativeOwnerCleanupCurrent
fb_ErrorNativeOwnerCleanupCurrent:
.seh_endprologue
mov rcx, rbp
jmp fb_ErrorNativeOwnerCleanupChecked
.seh_endproc
/* The fixed prologue reserves separate local slots above the Win64 home
 * area. The probe helper may clobber volatile registers, so reload all live
 * values from those slots before entering the handler. A saved C checkpoint
 * restores its original stack; this trampoline never returns normally. */
.globl fb_ErrorNativeDispatchRaw
.def fb_ErrorNativeDispatchRaw; .scl 2; .type 32; .endef
.seh_proc fb_ErrorNativeDispatchRaw
fb_ErrorNativeDispatchRaw:
push rbp
.seh_pushreg rbp
sub rsp, 64
.seh_stackalloc 64
lea rbp, [rsp+64]
.seh_setframe rbp, 64
.seh_endprologue
mov [rbp-8], rcx
mov [rbp-16], rdx
mov rax, r8
add rax, 15
and rax, -16
mov [rbp-24], rax
call ___chkstk_ms
mov rax, [rbp-24]
sub rsp, rax
mov r11, [rbp-8]
mov r10, [rbp-16]
mov rbp, r10
jmp r11
.seh_endproc
/* end of drivers/win32/x86_64/error_set_handler_win64.s */
