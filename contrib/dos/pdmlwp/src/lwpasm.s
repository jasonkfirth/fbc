/* PDMLWP DOS thread provider: lwpasm.s
 * Save CPU/x87 state, dispatch RTC scheduling signals and restore threads.
 * Allocation, thread selection and synchronization live in the C runtime.
 */
/****************************************************************************
*  Copyright (C) 1997 Paolo De Marino
*
*  Original code by Sengan Short (sengan.short@durham.ac.uk) and Josh Turpen
*  (snarfy@goodnet.com).
*
*  This library is free software; you can redistribute it and/or
*  modify it under the terms of the GNU Library General Public
*  License as published by the Free Software Foundation; either
*  version 2 of the License, or (at your option) any later version,
*  with the only exception that all the people in the THANKS file must
*  receive credit.
*
*  This library is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*  Library General Public License for more details.
*
*  You should have received a copy of the GNU Library General Public
*  License along with this library; see the file COPYING.LIB.
*  If not, write to the Free Software Foundation, Inc., 675 Mass Ave,
*  Cambridge, MA 02139, USA.
*
*  For contacting the author send electronic mail to
*     paolodemarino@usa.net
*
*  Or paper mail to
*
*     Paolo De Marino
*     Via Donizetti 1/E
*     80127 Naples
*     Italy
*
*  History: see history.txt
****************************************************************************/
.globl __lwp_pm_old_handler
.globl __lwp_pm_irq8_timer_hook
.globl __lwp_pm_irq0_timer_hook
.globl __lwpasm_start
.globl __lwpasm_end
.globl _lwp_yield
.globl __lwp_scheduler
.globl _lwp_thread_enable
.globl _lwp_thread_disable
.globl __init_fpu
.globl __lwp_pcount
tmp1: .long 0
tmp2: .long 0
tmp3: .long 0
.globl tmp1
.globl tmp2
.globl tmp3

.align 4
.text

__lwpasm_start:

__lwp_pm_old_handler: .long 0,0    /* __dpmi_paddr */

__lwp_pm_irq8_timer_hook:
        pushl %eax
        pushl %ds
        .byte 0x2E   /* CS: */
        movw ___djgpp_ds_alias, %ds

        /* Service queued output even when a runtime lock excludes task
           switches. The callback is integer-only and cannot enable IRQs.
           Preserve the interrupted ES/DF and use a locked, aligned stack. */
        cmpl $0, __lwp_tick_hook
        je tick_schedule
        pushfl
        pushal
        pushl %es
        movl %esp, %ebx
        movw %ss, %dx
        movw %ds, %ax
        movw %ax, %es
        movw %ax, %ss
        movl $__lwp_tick_stack+4096, %esp
        subl $16, %esp
        movl %ebx, 0(%esp)
        movw %dx, 4(%esp)
        cld
        call *__lwp_tick_hook
        movl 0(%esp), %ebx
        movw 4(%esp), %dx
        movw %dx, %ss
        movl %ebx, %esp
        popl %es
        popal
        popfl

        decl __lwp_tick_count
        jnz tick_ack
        movl __lwp_tick_divisor, %eax
        movl %eax, __lwp_tick_count

tick_schedule:
        incl __lwp_ticks
        cmp $0, __lwp_interrupt_pending
        jne L2

        movl $1, __lwp_interrupt_pending

        cmp $0, __lwp_enable
        jne  L1

        movb $0x99,%al
        call ___djgpp_hw_exception
        jmp L2

L1:     movl $1, __lwp_reschedule
        movl $0, __lwp_interrupt_pending
L2:
        popl %ds
        popl %eax
        ljmp *%cs:__lwp_pm_old_handler

tick_ack:
        movb $0x0C, %al
        outb %al, $0x70
        inb $0x71, %al
        movb $0x20, %al
        outb %al, $0xA0
        outb %al, $0x20
        popl %ds
        popl %eax
        sti
        iret



__lwp_pm_irq0_timer_hook:
        pushl %eax
        pushl %ds

        .byte 0x2E  /* CS: */
        movw ___djgpp_ds_alias, %ds
        cmp $0, __lwp_interrupt_pending
        jne L4

        movl $1, __lwp_interrupt_pending
        cmp $0, __lwp_enable
        jne  L3

        movb $0x99,%al
        call ___djgpp_hw_exception
        jmp L4

L3:     movl $0, __lwp_interrupt_pending
L4:
#if 0
		  movl $0x20, %eax
        outb %eax, $0x20

        popl %ds
        popl %eax
        sti
        iret
#else
		  popl %ds
		  popl %eax
		  ljmp *%cs:__lwp_pm_old_handler
#endif
.align 4
__lwp_scheduler:
        movl  ___djgpp_exception_state_ptr, %edi
        cmpl $0, %edi
        je raised               /* SIGILL was raise()'d, not by interrupt */

/*	From lwp 2.0 */
		  cmpl $0x99, 56(%edi)    /* our exception or real exception? */
		  jne raised
/* End from lwp 2.0 */

        /* DJGPP may defer delivery of the synthetic SIGILL until after the
           IRQ hook returns. The interrupted thread can enter a runtime
           critical section in that interval. Recheck at dispatch, otherwise
           it is suspended holding the global scheduling exclusion and the
           next worker inherits a permanently disabled scheduler. */
        cmpl $0, __lwp_enable
        je dispatch_unlocked
        movl $1, __lwp_reschedule
        jmp raised
dispatch_unlocked:
        movl $0, __lwp_reschedule

        /* This path bypasses DJGPP's return through longjmp(), which would
           restore PREVEXC (offset 60 of jmp_buf). Do the same bookkeeping
           before abandoning the signal frame. Otherwise later exceptions
           refer to a stale frame on an application or retired worker stack. */
        movl 60(%edi), %eax
        movl %eax, ___djgpp_exception_state_ptr

        /* get reg values from __djgpp_exception_state */
        movl 0(%edi), %eax
        pushl %eax
        movl 4(%edi), %ebx
        movl 8(%edi), %ecx
        movl 12(%edi), %edx
        movl 16(%edi), %esi
        movl 24(%edi), %ebp
        movl 28(%edi), %eax
        movl %eax, tmp1       /* esp */
        movl 32(%edi), %eax
        movl %eax, tmp2       /* eip */
        movl 36(%edi), %eax
        movl %eax, tmp3       /* eflags */

		  movl 20(%edi), %eax   /* edi */

		  movw 44(%edi), %es
		  movw 46(%edi), %fs
		  movw 48(%edi), %gs
		  movw 42(%edi), %ds		/* From this line, referencing ds is deadly */
										/* dangerous */

		  movl %eax, %edi			/* Here di is placed */

        popl %eax					/* Get eax from the first load */
		  .byte 0x2e
        movl tmp1, %esp       /* stack switched to exception state stack */
		  .byte 0x2e
        pushl tmp2            /* push exeception state ret address */

        /* now it's setup to look just like a yield happened in the code  */

        pushal
		  pushl %ds				/* Save segment registers */
		  pushl %es
		  pushl %fs
		  pushl %gs

		  .byte 0x2e				/* NEW */
		  movw ___djgpp_ds_alias,%ds /*NEW*/

		  .byte 0x2e
        pushl tmp3   /* eflags */


        decl __lwp_pcount       /* Decrement priority counter   */
        cmp $0,__lwp_pcount     /* If > 0                       */
        ja P1                   /* Skip this section            */

        subl $108, %esp
        fwait
        fnsave (%esp)   /* FPU pushal ;) */
/*        fwait         No need to wait for completion! */

        movl __lwp_cur, %esi
        movl %esp, 12(%esi)

        /* The interrupted code may be in movedata with ES naming a DMA
           selector, or may have DF set. C requires ES=DS and forward string
           operations. The saved segment registers/flags restore the caller. */
        pushl %ds
        popl %es
        cld
   call _lwp_findnext

   movl __lwp_cur, %esi

        movl 12(%esi), %esp

        cli
        fwait
        frstor (%esp)   /* FPU popal ;) */
/*        fwait		   No need to wait for completion! */
        addl $108, %esp

        popl %eax
        andb $0xfe, %ah   /* mask off debug flag */
        pushl %eax

P1:     popfl
		  popl %gs				/* Restore segment registers */
		  popl %fs
		  popl %es
		  popl %ds

		  fwait			/* We must wait HERE! */
        sti
        popal
raised:
		  pushl %ds
        .byte 0x2E  /* CS: */
        movw ___djgpp_ds_alias, %ds
		  movl $0, __lwp_interrupt_pending
		  popl %ds
        ret

/*
 * _lwp_yield is imperative! It doesn't care about priorities: if a threads
 * asks for it, it must be IMMEDIATELY performed.
*/
.align 4
_lwp_yield:
		  pushl %ds									/*JT*/
	     .byte 0x2e								/*JT*/
	     movw ___djgpp_ds_alias, %ds			/*JT*/
        movl $1, __lwp_interrupt_pending
        movl $0, __lwp_reschedule

		  popl %ds									/*JT*/

        pushal
		  pushl %ds				/* Save segment registers */
		  pushl %es
		  pushl %fs
		  pushl %gs
        pushfl

        subl $108, %esp
        fwait
        fnsave (%esp)   /* FPU pushal ;) */
/*        fwait	 - No need to fwait here, can multiprocess! */

	     .byte 0x2e								/*JT*/
	     movw ___djgpp_ds_alias, %ds			/*JT*/
        movl __lwp_cur, %esi
        movl %esp, 12(%esi)

        pushl %ds
        popl %es
        cld
        call _lwp_findnext
        movl __lwp_cur, %esi

        movl 12(%esi), %esp

        cli
        fwait
        frstor (%esp)   /* FPU popal ;) */
        addl $108, %esp

        popl %eax
        andb $0xfe, %ah   /* mask off debug flag before poping */
        pushl %eax        /* the flags */
        popfl

		  popl %gs				/* Restore segment registers */
		  popl %fs
		  popl %es
		  popl %ds

        fwait				 /* Here we must wait for the coprocessor */
        sti

        popal
		  pushl %ds
        .byte 0x2E  /* CS: */
        movw ___djgpp_ds_alias, %ds
        movl $0, __lwp_interrupt_pending
		  popl %ds
        ret

__init_fpu:
        pushl %eax
        movl $__fpu_init_state, %eax
        fwait
        fnsave (%eax)
        frstor (%eax)
        popl %eax	   /* HERE we MUST fwait - we don't know what the stack */
        fwait    		/* will do! */
        ret

_lwp_thread_enable:     movl $0, __lwp_enable
                        ret
_lwp_thread_disable:    movl $1, __lwp_enable
                        ret
__lwpasm_end:

/* end of lwpasm.s */
