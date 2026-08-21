/*
 * FreeBASIC Windows CE MIPS runtime
 * ---------------------------------
 *
 * File: wince/mips32el/setjmp.s
 *
 * Purpose:
 *
 *     Implement setjmp and longjmp for the MIPS III software-float O32 ABI.
 *
 * Responsibilities:
 *
 *     - preserve the O32 callee-saved integer registers
 *     - restore the saved stack, frame, and return addresses
 *     - implement the standard nonzero longjmp return-value rule
 *
 * This file intentionally does NOT contain:
 *
 *     - COREDLL-private jump-buffer assumptions
 *     - floating-point register preservation
 *     - signal-mask or structured-exception handling
 *     - implementations for MIPS64, ARM, or desktop Windows
 *
 * ABI note:
 *
 *     The 11-word layout matches the long-established newlib soft-float MIPS
 *     implementation: s0-s7, sp, fp, and ra, in that order.
 */

	.text
	.set	noreorder

	.globl	setjmp
	.ent	setjmp
setjmp:
	.frame	$sp, 0, $31
	sw	$16,  0($4)
	sw	$17,  4($4)
	sw	$18,  8($4)
	sw	$19, 12($4)
	sw	$20, 16($4)
	sw	$21, 20($4)
	sw	$22, 24($4)
	sw	$23, 28($4)
	sw	$29, 32($4)
	sw	$30, 36($4)
	sw	$31, 40($4)
	move	$2, $0
	jr	$31
	nop
	.end	setjmp

	.globl	longjmp
	.ent	longjmp
longjmp:
	.frame	$sp, 0, $31
	lw	$16,  0($4)
	lw	$17,  4($4)
	lw	$18,  8($4)
	lw	$19, 12($4)
	lw	$20, 16($4)
	lw	$21, 20($4)
	lw	$22, 24($4)
	lw	$23, 28($4)
	lw	$29, 32($4)
	lw	$30, 36($4)
	lw	$31, 40($4)
	bne	$5, $0, .Llongjmp_value_ready
	nop
	li	$5, 1
.Llongjmp_value_ready:
	move	$2, $5
	jr	$31
	nop
	.end	longjmp

/* end of wince/mips32el/setjmp.s */
