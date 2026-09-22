// -----------------------------------------------------------------------
// File lwp.c - pre-emptive LWP C source code.
// Copyright (C) 1997 Paolo De Marino
//
// Original Source Code by Sengan Short (sengan.short@durham.ac.uk)
// and Josh Turpen (snarfy@goodnet.com).
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Library General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version,
//  with the only exception that all the people in the THANKS file
//  must receive credit.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Library General Public License for more details.
//
//  You should have received a copy of the GNU Library General Public
//  License along with this library; see the file COPYING.LIB.
//  If not, write to the Free Software Foundation, Inc., 675 Mass Ave,
//  Cambridge, MA 02139, USA.
//
//  For contacting the author send electronic mail to
//     paolodemarino@usa.net
//
//  Or paper mail to
//
//     Paolo De Marino
//     Via Donizetti 1/E
//     80127 Naples
//     Italy
//
// History: See history.txt.
// -----------------------------------------------------------------------

#ifdef DEBUG
#define FORTIFY
#include "../../fortify/fortify.h"
#include <stdio.h>              /* Needed for fprintf(stderr,...) */

//#define PARANOID
/*
 * Includes really paranoid tests of coherence that will dramatically
 * decrease performances, if uncommented
 */
#endif

#include "lwp.h"
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <sys/timeb.h>
#include <sys/segments.h>
#include <bios.h>
#include <dos.h>
#include <conio.h>

#define LWP_EXCPTN 0x99         /* arbitrary exception number */

#define IRQ8 0x70               /* used to start the 1khz timer */
#define IRQ0 0x8                /* used to start the variable timer */
#define PIT0 0x40
#define PIT1 0x41
#define PIT2 0x42
#define PITMODE 			0x43
#define PITCONST 			1193180L
#define PIT0DEF 			18.2067597

#define LOCK_VARS(FIRST,LAST) \
        _lwp_lock_data( (void *) &(FIRST), (unsigned long) &(LAST)-   \
								(unsigned long) &(FIRST) + sizeof(LAST) )

#ifdef DEBUG
#define errprintf(string , args... ) cprintf(string , ##args )
#else
#define errprintf(string , args... )
#endif

/* Global Variables ------------------------------------------------------ */

/*
 * Everything between TOP_OF_VARS and BOTTOM_OF_VARS will be locked
 */

#define TOP_OF_VARS 	tick_per_ms
static volatile float tick_per_ms = 0.0182068;
static volatile float ms_per_tick = 54.9246551;
static volatile float freq8h = 18.2067597;
static volatile int counter_8h;
static volatile int counter_reset;
static int _lwp_irq_used = 0;
static int _lwp_speed_used = 0;
static int _lwp_on = 0;
static unsigned char saved_rtc_a, saved_rtc_b;
static unsigned char saved_pic_master, saved_pic_slave;

/* The fast consumer shares IRQ8 without shortening application time slices.
 * Its stack is part of the locked provider data, independent of the stack
 * (and SS selector) on which the DPMI host delivered the interrupt.
 */
void (* volatile _lwp_tick_hook)(void);
volatile unsigned int _lwp_tick_divisor = 1;
volatile unsigned int _lwp_tick_count = 1;
unsigned char _lwp_tick_stack[4096] __attribute__((aligned(16)));

volatile unsigned _lwp_pcount = 1;
/* Priority counter. Used in conjunction with lwp.priority. */
volatile static unsigned lwp_dead_threads = 0;
/* Set to 1 when a thread is lwp_kill'ed */

char _fpu_init_state[108];

extern __dpmi_paddr _lwp_pm_old_handler;

//__dpmi_regs _lwp_regs;
void (*_lwp_old_handler) (int);

int _lwp_heavy_dbg = 0;         /* NOTE - TO BE REMOVED IN FINAL RELEASE! */

lwp *_lwp_cur = (lwp *) 0;
static volatile int _lwp_count;
volatile int lwp_active_threads;
volatile int _lwp_enable = 1;
volatile int _lwp_reschedule;
volatile int _lwp_interrupt_pending;
/* Scheduling ticks, independent of DOS calendar calls and the PIT rate. */
volatile unsigned long _lwp_ticks;

#define BOTTOM_OF_VARS _lwp_ticks

/* ----------------------------------------------------------------------- */

/* Some function prototypes */

extern void _lwp_scheduler (int signum);
extern void _init_fpu (void);
void _lwp_findnext (void);      // Called by the scheduler, does the real job.

void lwp_deinit (void);
static void lwp_tcp (void *);
static int lwp_install (int irq, int speed);
static int _lwp_lock_memory (void);
static void _lwp_unlock_memory (void);
static void setRTC (int value);
static void startIRQ8 (void);
static void stopIRQ8 (void);
char *get_cmostime (void);
#ifdef DEBUG
static void printRing (void);
#endif

#define TOP_OF_FUNCTIONS _lwp_dead_thread

void
_lwp_dead_thread (void)
{
  lwp_kill (lwp_getpid ());
/*
 * Should never come here: lwp_kill(lwp_getpid()) never returns!
 */
  cprintf ("PANIC!\n\r");
}
int
lwp_getactive (void)
{
  return lwp_active_threads;
}
int static
lwp_install (int irq, int speed)
{
  volatile unsigned int pit0_set, pit0_value;
  __dpmi_paddr _lwp_pm_new_handler;

  _lwp_old_handler = signal (SIGILL, _lwp_scheduler);
  if (_lwp_old_handler == SIG_ERR)
    return 0;
  if (irq == 8)
    {
      _lwp_pm_new_handler.offset32 = (int) _lwp_pm_irq8_timer_hook;
      _lwp_pm_new_handler.selector = _my_cs ();
      if (__dpmi_get_protected_mode_interrupt_vector (IRQ8, &_lwp_pm_old_handler) != 0 ||
          __dpmi_set_protected_mode_interrupt_vector (IRQ8, &_lwp_pm_new_handler) != 0)
        {
          signal (SIGILL, _lwp_old_handler);
          return 0;
        }
      _lwp_on = 1;
      _lwp_enable = 0;
      setRTC (speed);
      startIRQ8 ();
      return (1);
    }
  else if (irq == 0)
    {
      if (speed < 1)
        return (0);

      _lwp_pm_new_handler.offset32 = (int) _lwp_pm_irq0_timer_hook;
      _lwp_pm_new_handler.selector = _my_cs ();

      __dpmi_get_protected_mode_interrupt_vector (IRQ0, &_lwp_pm_old_handler);
      __dpmi_set_protected_mode_interrupt_vector (IRQ0, &_lwp_pm_new_handler);

      outportb (PITMODE, 0x36);
      pit0_value = PITCONST / speed;
      pit0_set = (pit0_value & 0x00ff);
      outportb (PIT0, pit0_set);
      pit0_set = (pit0_value >> 8);
      outportb (PIT0, pit0_set);
      freq8h = speed;
      counter_8h = 0;
      counter_reset = freq8h / PIT0DEF;
      tick_per_ms = freq8h / 1000;
      ms_per_tick = 1000 / freq8h;
      _lwp_on = 1;
      _lwp_enable = 0;
      return (1);
    }
  else
    return (0);
}

int
lwp_init (int irq, int speed)
{
  /* This provider owns IRQ8 only. Leave the PIT used by clocks, graphics,
     and the PC speaker alone. Refuse an existing periodic RTC owner. */
  if (_lwp_on)
    return 1;
  if (irq != 8 || speed < RTC8192 || speed > RTC2)
    return 0;
  outportb (0x70, 0x0B);
  if (inportb (0x71) & 0x40)
    return 0;
  _lwp_irq_used = irq;
  _lwp_speed_used = speed;
  _lwp_enable = 1;
  if (!_lwp_lock_memory ())
    return 0;
  _lwp_cur = (lwp *) calloc (1, sizeof (lwp));
  if (!_lwp_cur)
    {
      _lwp_unlock_memory ();
      return 0;
    }
  if (_lwp_lock_data (_lwp_cur, sizeof (lwp)))
    {
      free (_lwp_cur);
      _lwp_cur = NULL;
      _lwp_unlock_memory ();
      return 0;
    }
  _init_fpu ();
  _lwp_cur->lwpid = LWP_MAIN;
  _lwp_cur->next = _lwp_cur;
  _lwp_cur->priority = 1;
  _lwp_cur->status = LWP_RUNNING;
  _lwp_interrupt_pending = 0;
  _lwp_count = 0;
  lwp_active_threads = 1;
  /* The cleanup worker releases another thread's stack after it has exited. */
  if (lwp_spawn (lwp_tcp, NULL, 65536, 1) < 0)
    goto fail_main;
  _lwp_cur->next->lwpid = LWP_TCP;
  _lwp_count = 0;
  if (atexit (lwp_deinit) || !lwp_install (irq, speed))
    {
      lwp *cleanup = _lwp_cur->next;
      _lwp_unlock_data (cleanup->stackTop, cleanup->stklen);
      _lwp_unlock_data (cleanup, sizeof (lwp));
      free (cleanup->stackTop);
      free (cleanup);
      goto fail_main;
    }
  return 1;

fail_main:
  _lwp_unlock_data (_lwp_cur, sizeof (lwp));
  free (_lwp_cur);
  _lwp_cur = NULL;
  lwp_active_threads = 0;
  _lwp_unlock_memory ();
  return 0;
}

#undef cprintf
#ifdef DEBUG
static void
lwp_generate_exception (void)
  __attribute__ ((noreturn));
     static void lwp_generate_exception (void)
{
  int a = 5, b;
  b = 1 / (a - a);
  printf ("%i\n", b);           /* Force use of b */
}
#endif
#ifdef PARANOID
static void
lwp_paranoid (void)
{
  lwp *curs = _lwp_cur;

  _lwp_enable = 1;

  Fortify_CheckAllMemory ();    /* Check overall memory coherence */
  do                            /* Check our chain      */
    {
      if (!Fortify_CheckPointer (curs))
        {
          printf ("Invalid pointer detected in thread chain(%p)\n",
                  curs);
          printf ("--Press a key and we will bail out - Use Symify --");
          while (!kbhit ());

          lwp_generate_exception ();
        }
//      else
      //        fprintf (stderr, "Thread %p - PID %i STATUS %i\n",
      //                 curs, curs->lwpid, curs->status);
      curs = curs->next;
    }
  while (curs != _lwp_cur);
  _lwp_enable = 0;
}
#endif

void
lwp_findnext (void)
{
  int sleepers = 0;             // Are there any sleepers?

  lwp *old = _lwp_cur;

  old->saved_errno = errno;
  old->saved_dpmi_error = __dpmi_error;

  _lwp_cur = _lwp_cur->next;

#ifdef PARANOID
  Fortify_OutputAllMemory ();

  if (_lwp_heavy_dbg)
    printRing ();
  else
    lwp_paranoid ();
#endif

  while (1)
    {
      switch (_lwp_cur->status)
        {
        case LWP_RUNNING:
/*          cprintf("(%i) to run\n\r",_lwp_cur->lwpid); */
          _lwp_pcount = _lwp_cur->priority;
          errno = _lwp_cur->saved_errno;
          __dpmi_error = _lwp_cur->saved_dpmi_error;
          return;               /* Found! */
        case LWP_SLEEPING:
          sleepers = 1;
          if (_lwp_ticks - _lwp_cur->waiting.wakeup_tick > LONG_MAX)
            break;
          _lwp_cur->status = LWP_RUNNING;  /* Else... */
          _lwp_pcount = _lwp_cur->priority;
          errno = _lwp_cur->saved_errno;
          __dpmi_error = _lwp_cur->saved_dpmi_error;
          return;
        case LWP_WAIT_SEMAPHORE:
          if (_lwp_cur->waiting.what_sema->owned)
            break;
          _lwp_cur->waiting.what_sema->owned = 1;
          _lwp_cur->waiting.what_sema->owner_id = _lwp_cur->lwpid;
          _lwp_cur->status = LWP_RUNNING;
          _lwp_pcount = _lwp_cur->priority;
          errno = _lwp_cur->saved_errno;
          __dpmi_error = _lwp_cur->saved_dpmi_error;
          return;
        case LWP_WAIT_TRUE:
/*
 *          cprintf("Queue %i waiting for int at %p to become != 0\n\r",
 *                  _lwp_cur->lwpid,_lwp_cur->waiting.what_int);
 */
          if (*(_lwp_cur->waiting.what_int) == 0)
            break;
          _lwp_cur->status = LWP_RUNNING;
          _lwp_pcount = _lwp_cur->priority;
          errno = _lwp_cur->saved_errno;
          __dpmi_error = _lwp_cur->saved_dpmi_error;
          return;
        case LWP_WAIT_FALSE:
/*
 *          cprintf("Queue %i waiting for int at %p to become == 0\n\r",
 *                  _lwp_cur->lwpid,_lwp_cur->waiting.what_int);
 */
          if (*(_lwp_cur->waiting.what_int) != 0)
            break;
          _lwp_cur->status = LWP_RUNNING;
          _lwp_pcount = _lwp_cur->priority;
          errno = _lwp_cur->saved_errno;
          __dpmi_error = _lwp_cur->saved_dpmi_error;
          return;
        case LWP_DEAD:
          break;                /* Just skip this thread. */
        default:
          break;
        };
/*
 * Look if the task we've just tried to clean is the current one,
 * and if there are no sleeping tasks.
 * If it were so, it was a grievous fault: deadlock!
 */
      if (!sleepers && _lwp_cur == old)
        {
          cprintf ("Deadlock!\n\r");
          abort ();
        }
      _lwp_cur = _lwp_cur->next;
    };
#ifdef PARANOID
  lwp_paranoid ();
#endif

}

int
lwp_getpid (void)
{
  return (_lwp_cur->lwpid);
}
void
lwp_setuserptr (void *usrdata)
{
  _lwp_cur->userptr = usrdata;  // No need to lock multitasker

}
void *
lwp_getuserptr (void)
{
  return (_lwp_cur->userptr);
}

void
lwp_set_priority (unsigned priority)
{
  if (priority != 0)
    _lwp_cur->priority = priority;
/*
 * Will become effective at next task
 * switch. Otherwise could be used to
 * run endlessly.
 */
}

unsigned
lwp_get_priority (void)
{
  return _lwp_cur->priority;
}

/* Spawns a light weight process.
   Returns the lwpid of the proc or 0 on fail */

unsigned long static
_lwp_flags (void)
{
  long i;

  __asm__ volatile ("pushfl \n"
                    "popl %0"
                    :"=g" (i));
  return i;
}

int
lwp_spawn (void (*proc) (void *), void *args, int stack_length,
           unsigned priority)
{
  struct startupStack
  {
    int FPUStatus[27];          // 108 bytes to initialize the stack

    int flags;                  // Space reserved for flags

    int gs, fs, es, ds;         // Segment registers

    int regs[8];                // and 8 registers

    int ip,                     // Here is the ip of the procedure
      retaddr,                  // Its return address,
      parms;                    // And all the requested parameters

  }
   *stackBot;
  lwp *tmp;
  int tmp2;

  tmp2 = _lwp_enable;
  _lwp_enable = 1;

  if ((proc == NULL) || (stack_length < 4096) || (stack_length > INT_MAX - 31) ||
      (_lwp_count == INT_MAX) || (priority == 0))
    {
      _lwp_enable = tmp2;
      return (-1);
    }
  tmp = (lwp *) calloc (1, sizeof (lwp));

  if (tmp == NULL)
    {
      _lwp_enable = tmp2;
      return (-1);
    }

/*
 * Reset atkill() functions.
 */
#ifdef PARANOID
  lwp_paranoid ();
#endif
  memset (tmp->atkill, 0, sizeof (tmp->atkill));
#ifdef PARANOID
  lwp_paranoid ();
#endif

  stack_length = (stack_length + 15) & ~15;  /* Make divisible by 4 */

// V. 2.1: calloc() instead of malloc() allocates zeroed memory
  tmp->stack = (unsigned int *) calloc (stack_length, 1);
  if (!tmp->stack)
    {
      free (tmp);
      _lwp_enable = tmp2;
      return -1;
    }
  if (_lwp_lock_data (tmp, sizeof (lwp)))
    {
      free (tmp->stack);
      free (tmp);
      _lwp_enable = tmp2;
      return -1;
    }
  if (_lwp_lock_data (tmp->stack, stack_length))
    {
      _lwp_unlock_data (tmp, sizeof (lwp));
      free (tmp->stack);
      free (tmp);
      _lwp_enable = tmp2;
      return -1;
    }

  tmp->stklen = stack_length;
  tmp->stackTop = tmp->stack;

  stackBot = (struct startupStack *)
    ((((unsigned long) tmp->stack + stack_length -
       sizeof (struct startupStack) - 8) & ~15UL) + 8);
  /* After restoring registers and RET, (ESP + 4) is 16-byte aligned, as
     required by modern GCC. Keep the original allocation for freeing. */

  stackBot->ds = stackBot->es = stackBot->fs = stackBot->gs = _my_ds ();
  memcpy (stackBot->FPUStatus, _fpu_init_state, sizeof (_fpu_init_state));
  stackBot->flags = _lwp_flags ();

  memset (stackBot->regs, 0, sizeof (stackBot->regs));
  stackBot->ip = (long) proc;
  stackBot->retaddr = (long) _lwp_dead_thread;
  stackBot->parms = (long) args;

  tmp->stack = (void *) stackBot;

  tmp->lwpid = ++_lwp_count;
  tmp->next = _lwp_cur->next;
  tmp->priority = priority;
  tmp->status = LWP_RUNNING;
  tmp->userptr = (void *) 0;
  lwp_active_threads++;

  _lwp_cur->next = tmp;

#ifdef PARANOID
  fprintf (stderr, "Spawn...\n");
  lwp_paranoid ();
#endif

  _lwp_enable = tmp2;
  return (tmp->lwpid);
}

static void
lwp_kill_ptr (lwp * thread)
{
  thread->status = LWP_DEAD;
  lwp_dead_threads = 1;
}

/*
 * Kills a lwp.
 * Returns: 1 if killed, 0 if thread was LWP_MAIN/LWP_TCP.
 * Pay attention: it doesn't return if you call lwp_kill(lwp_getpid())
 */

int
lwp_kill (int lwpid)
{
  lwp *curs;
  int tmp;

  if (lwpid == LWP_MAIN || lwpid == LWP_TCP)
    return 0;
  /* Main and TCP can't be killed */
  tmp = _lwp_enable;
  _lwp_enable = 1;

/* Are WE going to die? */
  if (_lwp_cur->lwpid == lwpid)
    {
      lwp_kill_ptr (_lwp_cur);
      _lwp_enable = tmp;        /* Re-enable multithreading */
      lwp_yield ();             /* And run like hell!     */

      cprintf ("PANIC!\n\r");   /* Should never get here! */
      return 1;
    }

  curs = _lwp_cur;

  if (curs == NULL)
    return (0);

  while ((curs->next != _lwp_cur) && (curs->lwpid != lwpid))
    curs = curs->next;

  /* Went through all of them and wasn't found */
  if (curs->lwpid != lwpid)
    {
      _lwp_enable = tmp;
      return 0;
    }

  /* Now curs IS the right one to kill */

  lwp_kill_ptr (curs);

#ifdef PARANOID
  lwp_paranoid ();
#endif

  _lwp_enable = tmp;
  return (1);
}
int
lwp_atkill (void (*proc) (void))
{
  int idx;
  for (idx = 0; idx < MAX_ATKILL; idx++)
    {
      if (_lwp_cur->atkill[idx] == (lwp_atkill_fct) 0)
        {
          _lwp_cur->atkill[idx] = proc;
          return 0;
        }
    }
  return -1;
}
#ifdef DEBUG
static void
printRing (void)
{
  lwp *curs = _lwp_cur;
  printf ("Thread dump----\n");
  do
    {
      printf ("Thread pointed by %8p -	pointer is %s ",
              curs, Fortify_CheckPointer (curs) ? "Valid  " : "Invalid");
      if (!Fortify_CheckPointer (curs))
        lwp_generate_exception ();
      printf ("Thread %i Status %i\n", curs->lwpid, curs->status);
      curs = curs->next;
    }
  while (curs != _lwp_cur);
  printf ("End Thread dump----\n");
}
#endif
/*
 *  This is our TCP, i.e. a never-dying thread. It just waits for the death
 *  of a thread. when such an event occurs, it frees that thread's stack
 *  and resources.
 */
static void
lwp_tcp (void *unused)
{
  lwp *curs, *trailer;
  int tmp;

#ifdef PARANOID
  if (_lwp_cur->lwpid != LWP_TCP || _lwp_cur->status != LWP_RUNNING)
    lwp_generate_exception ();
#endif

  while (1)                     /* Should run until program completion */
    {
      lwp_wait_true (&lwp_dead_threads);  /* When a thread is dead... */
      tmp = _lwp_enable;
      _lwp_enable = 1;
#ifdef DEBUG
      fprintf (stderr, "TCP Started...\n");
//      printRing ();
      Fortify_OutputAllMemory ();
#endif
      trailer = _lwp_cur;
      curs = trailer->next;
      do
        {
#ifdef PARANOID
          lwp_paranoid ();
#endif
#ifdef DEBUG
          fprintf (stderr, "Killing thread pointed by %p\n"
                   "Pointer is %s\n",
                   curs, Fortify_CheckPointer (curs) ? "Valid" : "Invalid");
          if (!Fortify_CheckPointer (curs))
            lwp_generate_exception ();
          if (!Fortify_CheckPointer (curs))
            fprintf (stderr, "Status %i PID %i\n", curs->status, curs->lwpid);
#endif
          if (curs->status == LWP_DEAD)
            {
              signed int idx;
              for (idx = MAX_ATKILL - 1; idx >= 0; idx--)
                {
                  if (curs->atkill[idx] != (lwp_atkill_fct) 0)
                    (curs->atkill[idx]) ();
                }
              trailer->next = curs->next;
              /* Free everything that thread owns */
              _lwp_unlock_data (curs->stackTop, curs->stklen);
              _lwp_unlock_data (curs, sizeof (lwp));
              free (curs->stackTop);
              free (curs);
              lwp_active_threads--;
              curs = trailer;   /* Back up one unit */
            }
          trailer = curs;
          curs = curs->next;
        }
      while (curs != _lwp_cur);
      lwp_dead_threads = 0;     /* Reset dead threads counter */
#ifdef DEBUG
      printRing ();
      fprintf (stderr, "TCP ended\n");
#endif
      _lwp_enable = tmp;
    }
}
/*
 * It's the user's responsibility to ensure that all the threads have been
 * lwp_kill'ed now!
 */
void
lwp_deinit (void)
{
  volatile unsigned long tick;
  volatile char *cmostime;
  lwp *curs;

  if (!_lwp_on)
    return;

  if (_lwp_cur->lwpid != LWP_MAIN)
    return;
  if (_lwp_irq_used != 8 && _lwp_irq_used != 0)
    return;

/*
 * Now, find and kill all alive threads with the exception of LWP_MAIN
 * (that's the calling thread!) and LWP_TCP.
 */

  _lwp_enable = 1;              /* Disable multithreading */

#ifdef PARANOID
  printf ("lwp_deinit called!\n");
  lwp_paranoid ();
  printRing ();
#endif
  for (curs = _lwp_cur->next; curs != _lwp_cur; curs = curs->next)
    {
      if (curs->lwpid != LWP_MAIN &&  /* Impossible, let's check it anyhow */
          curs->lwpid != LWP_TCP)
        lwp_kill_ptr (curs);
    }
#ifdef PARANOID
  printf ("All threads killed!\n");
  lwp_paranoid ();
  printRing ();
#endif
  _lwp_enable = 0;              /* Re-enable multithreading (i.e. launch lwp_tcp) */

  lwp_wait_false (&lwp_dead_threads);  /* Wait lwp_tcp to finish ... */

  _lwp_enable = 1;              /* Now, disable multithreading */

#ifdef DEBUG
/*
 * Count active threads
 */
  do
    {
      int count = 0;
      lwp *curs = _lwp_cur;
      cprintf ("Active threads:\n\r");
      do
        {
          count++;
          cprintf ("Thread %i - PID %i\n\r", count, curs->lwpid);
          curs = curs->next;
        }
      while (curs != _lwp_cur);
      if (count == 2)
        cprintf ("All right! 2 threads active!\n\r");
      else
        cprintf ("Hey! >2 threads active! Can\'t be!\n\r");
    }
  while (0);
#endif
/*
 * Now, find and kill lwp_tcp.
 */
  do
    {
      lwp *curs, *trail;

      trail = _lwp_cur;
      curs = _lwp_cur->next;
      for (; curs != _lwp_cur; trail = curs, curs = curs->next)
        if (curs->lwpid == LWP_TCP)
          {
            trail->next = curs->next;
/* Free everything that thread owns */
            _lwp_unlock_data (curs->stackTop, curs->stklen);
            _lwp_unlock_data (curs, sizeof (lwp));
            free (curs->stackTop);
            free (curs);
#ifdef DEBUG
            cprintf ("TCP Killed\n\r");
#endif
            break;
          }
    }
  while (0);

  if (_lwp_irq_used == 8)
    {
      stopIRQ8 ();
      __dpmi_set_protected_mode_interrupt_vector (IRQ8, &_lwp_pm_old_handler);
      signal (SIGILL, _lwp_old_handler);
      _lwp_unlock_data (_lwp_cur, sizeof (lwp));
      free (_lwp_cur);
      _lwp_cur = NULL;

#ifdef DEBUG
      cprintf ("IRQ8 successfully uninstalled.\n\r");
#endif
      _lwp_on = 0;
      _lwp_unlock_memory ();
      return;
    }
  else if (_lwp_irq_used == 0)
    {
      outportb (PITMODE, 0x36);
      outportb (PIT0, 0x00);
      outportb (PIT0, 0x00);
      __dpmi_set_protected_mode_interrupt_vector (IRQ0, &_lwp_pm_old_handler);
      signal (SIGILL, _lwp_old_handler);
      _lwp_unlock_data (_lwp_cur, sizeof (lwp));
      free (_lwp_cur);
      cmostime = get_cmostime ();
      tick = PIT0DEF *
        (
          (((float) *cmostime) * 3600) +
          (((float) *(cmostime + 1)) * 60) +
          (((float) *(cmostime + 2)))
        );
      biostime (1, tick);

      freq8h = PIT0DEF;
      counter_reset = freq8h / PIT0DEF;
      tick_per_ms = freq8h / 1000;
      ms_per_tick = 1000 / freq8h;
#ifdef DEBUG
      cprintf ("IRQ0 successfully uninstalled.\n\r");
#endif
      _lwp_on = 0;
      return;
    }
  return;
}

char *
get_cmostime (void)
{
  /* used to fix the fast timer */
  __dpmi_regs _lwp_regs;
  static char buffer[6];
  char ch;

  memset (&_lwp_regs, 0, sizeof (_lwp_regs));
  _lwp_regs.h.ah = 0x02;
  __dpmi_int (0x1a, &_lwp_regs);

  ch = _lwp_regs.h.ch;
  buffer[0] = (char) ((int) (ch & 0x0f) + (int) ((ch >> 4) & 0x0f) * 10);
  ch = _lwp_regs.h.cl;
  buffer[1] = (char) ((int) (ch & 0x0f) + (int) ((ch >> 4) & 0x0f) * 10);
  ch = _lwp_regs.h.dh;
  buffer[2] = (char) ((int) (ch & 0x0f) + (int) ((ch >> 4) & 0x0f) * 10);
  buffer[3] = _lwp_regs.h.dl;
  buffer[4] = (char) (_lwp_regs.x.flags & 0x0001);
  buffer[5] = 0x00;

  return buffer;
}
static void
_lwp_bottom_of_functions ()
{
};
#define BOTTOM_OF_FUNCTIONS _lwp_bottom_of_functions

static int
_lwp_lock_memory ()
{
  if (_lwp_lock_code (TOP_OF_FUNCTIONS,
   (unsigned long) &BOTTOM_OF_FUNCTIONS - (unsigned long) &TOP_OF_FUNCTIONS)
    )
    {
      /* fail */
#ifdef DEBUG
      cprintf ("ERROR:  DPMI error locking code segment functions\n\r");
#endif
      return (0);
    }
  if (_lwp_lock_code (&_lwpasm_start, (long) &_lwpasm_end - (long) &_lwpasm_start))
    {
      _lwp_unlock_code (TOP_OF_FUNCTIONS,
        (unsigned long) &BOTTOM_OF_FUNCTIONS - (unsigned long) &TOP_OF_FUNCTIONS);
      /* fail */
#ifdef DEBUG
      cprintf ("ERROR:  DPMI error locking code segment IRQ handlers\n\r");
#endif
      return (0);
    }
  if (LOCK_VARS (TOP_OF_VARS, BOTTOM_OF_VARS))
    {
      _lwp_unlock_code (&_lwpasm_start, (long) &_lwpasm_end - (long) &_lwpasm_start);
      _lwp_unlock_code (TOP_OF_FUNCTIONS,
        (unsigned long) &BOTTOM_OF_FUNCTIONS - (unsigned long) &TOP_OF_FUNCTIONS);
#ifdef DEBUG
      cprintf ("ERROR:  DPMI error locking data segment variables\n\r");
#endif
      return (0);
    }
  return (1);
}
/* Called only with the scheduler uninstalled or after failed initialization. */
static void
_lwp_unlock_memory (void)
{
  _lwp_unlock_data ((void *) &TOP_OF_VARS,
    (unsigned long) &BOTTOM_OF_VARS - (unsigned long) &TOP_OF_VARS + sizeof (BOTTOM_OF_VARS));
  _lwp_unlock_code (&_lwpasm_start, (long) &_lwpasm_end - (long) &_lwpasm_start);
  _lwp_unlock_code (TOP_OF_FUNCTIONS,
    (unsigned long) &BOTTOM_OF_FUNCTIONS - (unsigned long) &TOP_OF_FUNCTIONS);
}
/* copied from go32 lib funcs */
int
_lwp_lock_data (void *lockaddr, unsigned long locksize)
{
  unsigned long baseaddr;
  __dpmi_meminfo memregion;
  if (__dpmi_get_segment_base_address (_my_ds (), &baseaddr) == -1)
    return (-1);

  memset (&memregion, 0, sizeof (memregion));

  memregion.address = baseaddr + (unsigned long) lockaddr;
  memregion.size = locksize;

  if (__dpmi_lock_linear_region (&memregion) == -1)
    return (-1);

  return (0);
}

int
_lwp_unlock_data (void *lockaddr, unsigned long locksize)
{
  unsigned long baseaddr;
  __dpmi_meminfo memregion;
  if (__dpmi_get_segment_base_address (_my_ds (), &baseaddr) == -1)
    return (-1);

  memset (&memregion, 0, sizeof (memregion));

  memregion.address = baseaddr + (unsigned long) lockaddr;
  memregion.size = locksize;

  if (__dpmi_unlock_linear_region (&memregion) == -1)
    return (-1);

  return (0);
}

int
_lwp_lock_code (void *lockaddr, unsigned long locksize)
{
  unsigned long baseaddr;
  __dpmi_meminfo memregion;
  if (__dpmi_get_segment_base_address (_my_cs (), &baseaddr) == -1)
    return (-1);

  memset (&memregion, 0, sizeof (memregion));

  memregion.address = baseaddr + (unsigned long) lockaddr;
  memregion.size = locksize;

  if (__dpmi_lock_linear_region (&memregion) == -1)
    return (-1);

  return (0);
}

int
_lwp_unlock_code (void *lockaddr, unsigned long locksize)
{
  unsigned long baseaddr;
  __dpmi_meminfo memregion;
  if (__dpmi_get_segment_base_address (_my_cs (), &baseaddr) == -1)
    return (-1);

  memset (&memregion, 0, sizeof (memregion));

  memregion.address = baseaddr + (unsigned long) lockaddr;
  memregion.size = locksize;

  if (__dpmi_unlock_linear_region (&memregion) == -1)
    return (-1);

  return (0);
}

static void
setRTC (int value)
{
  unsigned char value_a;
  int interrupt_state = __dpmi_get_and_disable_virtual_interrupt_state ();
  outportb (0x70, 0x0A);
  saved_rtc_a = inportb (0x71);
  value_a = (saved_rtc_a & 0xF0) | (value & 0x0F);
  outportb (0x70, 0x0A);
  outportb (0x71, value_a);
  __dpmi_get_and_set_virtual_interrupt_state (interrupt_state);
}

int
lwp_set_tick_hook (void (*hook)(void), int speed)
{
  unsigned char value;
  int interrupt_state = __dpmi_get_and_disable_virtual_interrupt_state ();
  if (!_lwp_on || _lwp_irq_used != 8 || !hook || _lwp_tick_hook ||
      speed < RTC8192 || speed > _lwp_speed_used)
    goto unavailable;
  outportb (0x70, 0x0B);
  value = inportb (0x71);
  /* Fast ticks acknowledge register C locally. Do not consume another
   * client's alarm/update interrupts, or alter an enabled square-wave pin.
   */
  if ((value & 0x78) != 0x40)
    goto unavailable;
  _lwp_tick_divisor = 1U << (_lwp_speed_used - speed);
  _lwp_tick_count = _lwp_tick_divisor;
  _lwp_tick_hook = hook;
  outportb (0x70, 0x0A);
  value = inportb (0x71);
  outportb (0x70, 0x0A);
  outportb (0x71, (value & 0xF0) | speed);
  __dpmi_get_and_set_virtual_interrupt_state (interrupt_state);
  return 1;

unavailable:
  __dpmi_get_and_set_virtual_interrupt_state (interrupt_state);
  return 0;
}

int
lwp_clear_tick_hook (void (*hook)(void))
{
  unsigned char value;
  int interrupt_state = __dpmi_get_and_disable_virtual_interrupt_state ();
  if (!hook || _lwp_tick_hook != hook)
    {
      __dpmi_get_and_set_virtual_interrupt_state (interrupt_state);
      return 0;
    }
  _lwp_tick_hook = NULL;
  _lwp_tick_divisor = _lwp_tick_count = 1;
  outportb (0x70, 0x0A);
  value = inportb (0x71);
  outportb (0x70, 0x0A);
  outportb (0x71, (value & 0xF0) | _lwp_speed_used);
  __dpmi_get_and_set_virtual_interrupt_state (interrupt_state);
  return 1;
}

static void
startIRQ8 (void)
{
  int interrupt_state = __dpmi_get_and_disable_virtual_interrupt_state ();
  outportb (0x70, 0x0B);
  saved_rtc_b = inportb (0x71);
  saved_pic_master = inportb (0x21);
  saved_pic_slave = inportb (0xA1);
  outportb (0x70, 0x0B);
  outportb (0x71, saved_rtc_b | 0x40);
  outportb (0x70, 0x0C);
  inportb (0x71);
  /* IRQ8 reaches the CPU through IRQ2 on the master PIC. */
  outportb (0x21, saved_pic_master & ~0x04);
  outportb (0xA1, saved_pic_slave & ~0x01);
  __dpmi_get_and_set_virtual_interrupt_state (interrupt_state);
}
static void
stopIRQ8 (void)
{
  unsigned char value;
  int interrupt_state = __dpmi_get_and_disable_virtual_interrupt_state ();
  _lwp_tick_hook = NULL;
  _lwp_tick_divisor = _lwp_tick_count = 1;
  outportb (0x70, 0x0B);
  value = inportb (0x71);
  outportb (0x70, 0x0B);
  outportb (0x71, (value & ~0x40) | (saved_rtc_b & 0x40));
  outportb (0x70, 0x0A);
  outportb (0x71, saved_rtc_a);
  outportb (0x70, 0x0C);
  inportb (0x71);
  outportb (0xA1, (inportb (0xA1) & ~0x01) | (saved_pic_slave & 0x01));
  outportb (0x21, (inportb (0x21) & ~0x04) | (saved_pic_master & 0x04));
  __dpmi_get_and_set_virtual_interrupt_state (interrupt_state);
}

void
lwp_sleep (unsigned int secs, unsigned short msecs)
{
  /* Dispatch must not enter DOS or calendar conversion through ftime().
     Count scheduling ticks, including those which defer a task switch. The
     fast speaker consumer divides its IRQ rate before incrementing this
     clock. Split long waits so unsigned deadline subtraction handles wrap.
     RTC rate code 3 means 8192 Hz; each larger code halves that rate. */
  unsigned int hz = 32768U >> (_lwp_speed_used - 1);
  unsigned long long ticks = (unsigned long long) secs * hz +
    ((unsigned long long) msecs * hz + 999) / 1000;
  /* The current tick can be almost over. One extra tick prevents an early
     wake when the requested interval starts just before the next IRQ. */
  if (ticks)
    ticks++;
  do {
    unsigned long part = ticks > LONG_MAX ? LONG_MAX : (unsigned long) ticks;
    int tmp = 1;
    __asm__ __volatile__ ("xchgl %0, %1"
                         : "+r" (tmp), "+m" (_lwp_enable) : : "memory");
    _lwp_cur->waiting.wakeup_tick = _lwp_ticks + part;
    _lwp_cur->status = LWP_SLEEPING;
    _lwp_enable = tmp;
    lwp_yield ();
    ticks -= part;
  } while (ticks);
}

void
lwp_wait_true (volatile int *what)  // Wait until what != 0
 {
  volatile int tmp = _lwp_enable;
  _lwp_enable = 1;              // Lock multitasking engine

  if (*what == 0)
    {
      _lwp_cur->status = LWP_WAIT_TRUE;
      _lwp_cur->waiting.what_int = what;

      _lwp_enable = tmp;
      lwp_yield ();
    }
  else
    _lwp_enable = tmp;          // Already != 0

}
void
lwp_wait_false (volatile int *what)  // Wait until what == 0
 {
  volatile int tmp = _lwp_enable;
  _lwp_enable = 1;              // Lock multitasking engine

  if (*what != 0)
    {
      _lwp_cur->status = LWP_WAIT_FALSE;
      _lwp_cur->waiting.what_int = what;

      _lwp_enable = tmp;
      lwp_yield ();
    }
  else
    _lwp_enable = tmp;          // Alreay == 0

}
// When a task waits for an integer to become true/false, it can be useful to
// force all the tasks waiting for that integer to pass as if it were become
// true/false.
void
lwp_pulse_true (volatile int *what)
{
  lwp *cursor = _lwp_cur->next;
  volatile int tmp = _lwp_enable;
  _lwp_enable = 1;              // Lock multitasking engine

  while (cursor != _lwp_cur)
    {
      if ((cursor->status == LWP_WAIT_TRUE) &&
          (cursor->waiting.what_int == what))
        cursor->status = LWP_RUNNING;  // If the tasks waits for this integer,
      // let it run.

      cursor = cursor->next;
    }

  _lwp_enable = tmp;
}
void
lwp_pulse_false (volatile int *what)
{
  lwp *cursor = _lwp_cur->next;
  volatile int tmp = _lwp_enable;
  _lwp_enable = 1;              // Lock multitasking engine

  while (cursor != _lwp_cur)
    {
      if ((cursor->status == LWP_WAIT_FALSE) &&
          (cursor->waiting.what_int == what))
        cursor->status = LWP_RUNNING;
      cursor = cursor->next;
    }

  _lwp_enable = tmp;
}

void
lwp_wait_semaphore (lwp_semaphore * sema)
{
  volatile int tmp = _lwp_enable;
  _lwp_enable = 1;              // Lock multitasking engine

  _lwp_cur->status = LWP_WAIT_SEMAPHORE;
  _lwp_cur->waiting.what_sema = sema;

  _lwp_enable = tmp;
  lwp_yield ();
}

void
lwp_init_semaphore (lwp_semaphore * sema)
{
  sema->owned = 0;
}

int
lwp_release_semaphore (lwp_semaphore * sema)
{
  if (sema->owned)
    {
      if (sema->owner_id != _lwp_cur->lwpid)
        return -1;
      sema->owned = 0;
    };
  return 0;
}

_Static_assert (offsetof (lwp, stack) == 12, "PDMLWP assembly stack offset");
/* end of lwp.c */
