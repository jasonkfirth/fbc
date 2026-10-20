/* FreeBASIC DOS graphics: gfx_vesa.s
 * Call the VBE protected-mode bank switcher while preserving the C ABI.
 * Mode selection, framebuffer conversion and IRQ dispatch live elsewhere.
 */

.intel_syntax noprefix


#define FUNC(name)              .globl _##name ; .balign 8, 0x90 ; _##name :
#define GLOBL(name)             _##name
#define LABEL(name)             .balign 4, 0x90 ; name :

.section .text

/* protected-mode bank switcher */

FUNC(fb_dos_vesa_set_bank_pm)
	/* This wrapper is called from C. In particular, selecting window A
	 * below changes EBX, which GCC expects its callee to preserve. Keep
	 * the firmware's register changes inside this void-returning wrapper.
	 */
	pushad
	push ds
	push es
	push fs
	push gs
	mov ecx, GLOBL(fb_dos_vesa_pm_bank_switcher)    /* ecx = pointer to pm bank switching code in our address space */
	xor ebx, ebx                                    /* set window A (bh = 00h, bl = 00h) */
	mov edx, GLOBL(fb_dos_vesa_bank_number)         /* window offset in window granularity units */
	call ecx
	pop gs
	pop fs
	pop es
	pop ds
	popad
	ret

FUNC(fb_dos_vesa_set_bank_pm_end)

/* end of gfx_vesa.s */
.end
