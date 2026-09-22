                               DISCLAMER
This document describes the terms for distributing the source code of and any
derived work based on the PDMLWP multitasking library. Such source code is
marked with the copyright notice:

 "Copyright (C) 1997 Paolo De Marino"

All the files marked with the above copyright fall either under LGPL
(Library General Public License) or under GPL (General Public License) as
stated at the beginning of each file, with the following exception, that ALL
the people in the THANKS file must receive credit. The example*.* files are
FREEWARE. You can do whatever you want with them.

    All these files are distributed in the hope that they will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    and of the GNU Library General Public License along with this
    program; see the files COPYING and COPYING.LIB. If not, write to
    the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
    MA 02139, USA.

Enough legal stuff.

A couple of months ago, I found on the Net the excellent LWP
package written by Josh Turpen and Sengan Short. It provided basic
services such as starting a new thread, killing a thread, regulating
the time slice, and so on. But soon I discovered that it didn't provide
any synchronization facility between the MANY threads it could handle.
Thus I decided to add it a set of BASIC synchronization capabilities.
Anyhow, as I needed a C++ interface for a C++ program I was (am!) going
to write, I decided to add a complete set of objects to handle not only
synchronization, but also message posting, readers/writers locks, gates,
Thread objects to the thing.

So came out this package. It is basically divided in three sections:
  lwpasm.S (Capital "S"!)   The assembler stuff
  lwp.c                     The "C" part of the business
  threads.cc                The C++ interface.

Here are brief descriptions of what each section does. Let's begin with
the "C" part, exactly with the original LWP functions (I've slightly
modified almost all of them, but I have left an identical interface).

lwp_init(irq, speed)

This function hooks all the stuff that needs to be hooked.  After you call
this function, main() is now operating as a task, but since it is the only
task you won't notice. Both timer IRQs are supported (0 and 8).  IRQ8 only
supports certain speeds (2hz, 4hz, 8hz, 16hz...8192hz, see lwp.h for more).
IRQ0 is programmable, and any speed can be implemented.  10hz < speed
< 1024hz is recommended. This function automagically cleans up after
itself when your program exits.

lwp_spawn(proc, stack_size,priority)
This function spawns off other tasks.  Proc is the name of a function of
type void proc(void).  Stack size is used to create a stack that is
used by your proc.  It won't spawn if stack_size < 255.  I'd suggest at least
4k, just to be safe.  Your proc will lwp_kill() itself when it returns, so
there is no need to worry about threads joining. Priority specifies the
number of consecutive time slices the thread will have. Any number >0 will
fit.
PAY ATTENTION: lwp_yield is executed anyhow! It doesn't care about priorities.

Example:

void proc(void)
{
   printf("My proc is kewl!\n");
}
main()
{
   lwp_init(0,100);
   lwp_spawn(proc, 4096,1);
   /* do some stuff...*/
}

lwp_kill(lwpid)
This function kills a process.  Simple.  Pass it the pid of the process
and blahmo, it's dead.

lwp_getpid()
This returns the current process's pid, useful in killing the current process.
Example:

void proc1()
{
   ...do stuff
   lwp_kill(lwp_getpid());
   /* never returns */
}

Note:  Processes that return automagically kill themselves.

lwp_thread_disable()
This function disables task switching.  It's mostly used to fix
non re-entrant functions like printf, malloc, etc.

lwp_thread_enable()
This function enables task switching after a previous lwp_thread_disable().


lwp_thread_disable and lwp_thread_enable are not available, or needed,
in the cooperative version.

Example:

lwp_thread_disable();
printf("foo = %d\n", foo);
lwp_thread_enable();

Take a look at the lwpstdio.h, lwpconio.h, and lwppc.h that are included with
this distribution.  It takes care of SOME of libc's non-reentrant functions.
You don't need to wrap functions like printf with thread_disable and
thread enable, because it's automagically done in the included header files.
If there is a function you are using that isn't in one of the above header
files and you aren't sure that it is re-entrant, wrap it with thread_disable
and thread_enable, just to be safe.

lwp_yield()
This function causes a task switch to happen.  You might need to use
this here and there.

*Note*
This library uses the IRQ8 real time clock timer.  It interrupts at a rate
of 2hz..8192hz.  I chose this interrupt specifically for the reason
that nobody else uses it, so this tasking library should be compatible
with just about anything.  It took a lot of blood, sweat, and tears to
get it to work with IRQ8!  IRQ0 has also been implemented for its
programmable capabilities.


These were the services provided by LWP.

void lwp_sleep(unsigned int secs,unsigned short msecs)

Puts to bed your task for a given time, given at the precision of
1 thousandth of a second.
IMPORTANT: While the thread "sleeps", IT DOESN'T EAT UP CYCLES. It simply
won't receive time slices.

Example:

void periodic_check(void)
{
 while(!finished)
 {
  lwp_sleep(60*15,0);    /* Wait 15 minutes */

  autosave();
 }
}
This task wakes up every 15 minutes, and auto-saves the situtation.

void lwp_setuserptr(void *usrdata);
void *lwp_getuserptr(void);

These two member functions allow the user to set a per-thread user pointer,
that can be read/written by every single thread. This lets you write code
accessing differentl data depending on the thread it executes under, without
having to check the pid()s to find what to operate on.

void lwp_wait_true(volatile int *what)
void lwp_wait_false(volatile int *what)

These two functions play a very important role in mutitasking: they allow
a thread to wait for an integer to become true or false, without consuming
time slices. Their most immediate use is in the building of a gate, it is
of a "door" that can be opened or closed by another process.

Example:

volatile int trigger = 0;

void triggered_task(void)
{
 lwp_wait_true(&trigger);

... Does what it has to do...
}

void main()
{
 lwp_spawn(triggered_task,8192);

...Waits for some event...

 trigger = 1;     // Here it is. From this moment, the triggered_task
                  // can proceed!

 ... Does the rest of its stuff...
}

void lwp_pulse_true(volatile int *what)
void lwp_pulse_false(volatile int *what)

These two functions are useful to implement event raisers: they let any
thread waiting for "*what" to become true/false pass, WITHOUT CHANGING
THE VALUE OF *what! It is, they'll set all the threads NOW waiting free,
but they'll stop any other thread calling lwp_wait_true/false afterwards.

void lwp_wait_semaphore(lwp_semaphore *sema)
void lwp_init_semaphore(lwp_semaphore *sema)
int lwp_release_semaphore(lwp_semaphore *sema)

These are perhaps the most important functions in the package.
They allow a process to wait for a Mutex semaphore, i.e. for a semaphore that
can be owned by JUST ONE THREAD AT A TIME. Examples of such situations
are resources such as the keyboard, the screen: only one process at a time
may own one of these resources. Another kind of "resource" that needs to
be locked is data: it IS useful to implement locks on particular memory
areas that cannot be safely accessed by two threads at the same time.

Example:

lwp_semaphore sema;
int count = 0;

void thread(void)
{
 ... Does some stuff ...

 lwp_wait_semaphore(&sema); //Wait until semaphore is free
 count++;                   // Do something...
 lwp_release_semaphore(&sema); // Make the semaphore free again.

 ... Does some other stuff ...
}

void lwp_set_priority(unsigned priority)
Sets the priority at the value you specify for it.	Don't rely on this
function to protect the non-reentrant parts of your program: the priority
counter is set only at next task switch.

unsigned lwp_get_priority(void)
Returns the priority count, i.e. the number of consecutive time slices the
thread will receive.

C++ INTERFACE

Here it comes the nicest part of the package: the C++ interface.
We implement the following classes, which we will discuss.

class CritSect;
class MutexSemaphore;
class MutexSemaphoreLock;
class CountSemaphore;
class SerializedQueue<T>;
class Gate;
class Event;
class ReadWriteSemaphore;
class WriteLock;
class ReadLock;
class Thread;
class Message;

Class CritSect.

Is just a shorthand for lwp_thread_disable/enable: CritSect objects stop
multithreading within their scope of existence:

int count=0;
void func(void)
{
 ... Some stuff...

 do {  CritSect sect;
       count++;
 } while(0);
}

Classes MutexSemaphore / MutexSemaphoreLock

This is the simplest kind of synchronization object: it allows only
one thread at a time access to a resource. It implements a recursively
lockable mutex, which is its safest form.

Example

MutexSemaphore sema; // Initializes itself on its own.

void func(void)
{
 do{
     MutexSemaphoreLock lock(sema);
     ... Do your private stuff with the semaphore locked...
 } while(0);
 ... Now semaphore is automagically unlocked...
}

But, as it is recursively lockable, even this code would have worked without
hanging the machine.

MutexSemaphore sema;

void func(void)
{
 do{
     MutexSemaphoreLock lock(sema);

     do_stuff();
 } while(0);    // The semaphore is unlocked only here, as one might expect.
 ... Now semaphore is automagically unlocked...
}

void do_stuff(void)
{
 MutexSemaphoreLock lock(sema);  // The semaphore was already locked

 ... What this function does...
}                                // And is NOT unlocked here!


class CountSemaphore

This class allows for a different kind of semaphore: a thread may allow
a definite number of processes to pass. If you were writing a game,
you might want the monsters to show up at certaing moments, ONE AFTER THE
OTHER, or perhaps two at the same time, but you surely don't want all of them
to materialize simultaneously, neither do you want to wait for each monster to
die before making another appear. So...

CountSemaphore monsters_sema;

void monster(void)
{
 monsters_sema.Wait();  // Wait for ok from theGame()

 ...Boo! Eat the player! ...
}

main()
{
 if(level == 1 )
 {                    // Spawn 10 (sleeping) monsters.
  for(int i = 0; i < 10; i++) lwp_spawn(monster,8192);
 } else
 {                    // Spawn 20 (sleeping) monsters.
  for(int i = 0; i < 20; i++) lwp_spawn(monster,8192);
 };

 ... At a certain point, you call the main game loop ...

 theGame();
}
void theGame(void)
{
 ... You want to materialize a certain number of monsters ...
 if(level == 1)
 {
  monsters_sema.Clear();   // Let one process pass on...
 } else
 {
  monsters_sema.Clear(2)   // Wake up two processes!
 };
}

Class SerializedQueue<T>

This class is used by the Thread class to send/receive messages in a
serialized (FIFO) way, but can also be used as an example of how semaphores
can be used to serialize access to objects. The serialization of the access
is obtained with two semaphores: a first mutex semaphore that enqueues all the
accesses (remember that in a queue there usually is no "peek" function,
but only "push" and "pop", and thus all accesses modify it), and a Count
semaphore for "poppers": when a thread wants to pop an object from the bottom
of the queue, and there is no object there, it goes into a waiting state,
cleared by the "pushing" of an object by another thread.

class Gate

Gates are checkpoints that can either allow all threads to pass through
a point, or to stop them all from proceeding.

Example: (see the file test/example6.cc too)
If in that game you wanted to introduce a "pause" key, you could write:

Gate pauseGate(1);   // Initialize it to be open!

main()
{

   ... Some stuff ...
 if( pause_key_pressed() )
 {
  pauseGate.Close(); // Stop all the other game threads!

  while(!kbhit()) ;  // Wait for another key pressure...

  pauseGate.Open();  // Re-open the gate
 }

 ... Rest of the game ...
}

void game(void)
{
 ... There is some code...
 while(!game_over)
 {
  pauseGate.Wait();  // Wait for the gate to be opened, or pass on if it
                     // is already open.

  ... Play ...
 };
}

void monster(void)
{
 ... There is some code to draw the monster ...
 while(alive)
 {
  pauseGate.Wait();  // Wait for the gate to be opened, or pass on if it
                     // is already open.

  ... Move the monster ...
 };
 ... Make the monster disappear ...
}

Class Event

Events allow to let all the threads waiting for it to pass simultaneously
when the event is Raise()d.

See the file test/example9.cc for an example of using the Event class.

Classes ReadWriteSemaphore/ReadLock/Writelock

Often in multithreading there are resources that many threads can "read"
simultaneously without problems, but that can be accessed for writing by
one thread at a time, ant can't be read when they're being written to.
An example of such a resource is a big structure: many can read, but
only one can write. In these cases, the best thing to do is using a
ReadWriteLock.

Interface to this class is very easy, as the example shows:

Example (see the file test/example5.cc)

ReadWriteSemaphore sema;
int ToBeProtected;

void thread1(void)
{
  int tmp;
  ... Some stuff ...
  do { ReadLock lock(sema);
       tmp = ToBeProtected;
  } while(0);
  ... Rest of the stuff ...
}
void thread2(void)
{
  int tmp;
  ... Some stuff ...
  do { WriteLock lock(sema);
       ToBeProtected++;
  } while(0);
  ... Rest of the stuff ...

}

ATTENTION: Readers/Writers lock ARE NOT FAIR LOCKS! If there are many writers,
readers will probably starve, as Writers have precedence on Readers!

Class Thread
This is the hearth of the C++ package: the Thread class allows for a simple
C++ creation of idependent light weight processes (whence the name LWP!).
The interface to the Thread class is quite easy: to create a new Thread
you have to derive a new class from the base class:

class MyThread : public Thread
{
public:
       MyThread(unsigned int stacksize) : Thread(stacksize) { };

       virtual void prepare(void); // Starts as soon as the thread is created
       virtual void execute(void); // Starts when the thread is "start()"ed
       virtual void cleanup(void); // Starts when execute() returns
}

The three virtual methods prepare(), execute() and cleanup(), overwritten,
provide the actual thread code. They default to doing nothing.
There are three members to make life easier to programmers. In fact, they
are all called at very specific times.
The prepare() function you provide is called as soon as the Thread-derived
object is created, i.e. at the end of the constructor.
The execute() function is called ONLY when another thread calls the "start"
member function. This allows for synchronization in thread starting.
The cleanup() function instead is called when the "execute()" function has
returned; i.e., when the "job" has been performed, and you might need to
"clean up" the situation. NOTE: the cleanup() function is NOT called when
the kill() member is called, and any objects created by the thread REMAIN
EXISTENT UNLESS YOU EXPLICITLY DESTROY THEM SOMEWHERE.

But Thread object don't provide only these services: their perhaps most
important job is allowing threads to exchange messages.
A Message is simply a pointer to void, that you "post()" to another thread.
The Thread objects provides two methods to do it:

void postMessage(void *) and
static void postMessage(void *,Thread& dest).

The first can be used so:

    myThread.postMessage("Hello!");

Sends the message "Hello!" to the thread myThread. In the same way, you
might call

    Thread::postMessage("Hello",myThread);

These two forms are synonimous.

Receiving a message is very simple too, but one thing must be understood:
you send just void*, but you receive Message objects, containing another
important datum, i.e. the sender. Thus, to receive that message, and print
all the relevant data, you should write:

void myThread::execute(void)
{
  do
  {
      Message theMessage = getMessage(); // Inside execute(), you have
                                         // access to member functions!
      if(theMessage.Contents() == 0) break;
      printf("Message is: %s from thread with pid(%i)\n",
             (char*)(theMessage.Contents()),
             theMessage.Source()->getPid());
  } while(1);
}

NOTES:
1)    the getMessage() method waits for a message to be sent,
      THEN returns it. Pay attention to deadlocks!
2)    Messages arrive in the order in which they were posted. I was very
      careful about it: they are put in a queue (obviously, in a serialized
      queue).
3)    In fact, you are not limited at all to sending char*! You can send
      ANY pointer with the postMessage()/getMessage() method.
4)    When you perform a getMessage(), the message is removed from the
      queue. If you want to check howe many message are waiting to be
      processed, use the waitingMessages() method, or the noMessages() to
      check whether the messages queue is empty.

Just one more thing: there is a global MainThread object, named theMainThread,
which is exactly what it pretends to be: the object pointed to when you call
Thread::currentThread() from the main(). It is initialised by

void InitLwp(speed)

which uses IRQ 8, the most programmable IRQ, to give/receive control, and
destroyed by

void DoneLwp(void)

that should be more or less the first and the last things you do in your main.

WARNINGS:
This package is pretty stable: it has never crashed in its current version,
which I tested on my 486DX2/50. It doesn't eat up much memory, neither does
it eat too many cycles. BUT: there are lots of non-reentrant functions in
libc and in the C++ libraries, beginning with raise()/signal(), which means
that you cannot use exceptions, for they could be processed by ANY thread.
Another example of non-reentrant procs are all the I/O routines,
malloc,free,realloc (new and delete are non-reentrant too!). The true solution
to this problem would be rewriting all the libraries, but I don't have
enought time for such an enormous job. The only, obviously imperfect solution
is creating small patch header files, i.e. lwpconio.h, lwppc.h, lwpstdio.h,
lwpstdlib.h.

Mainly, PAY ATTENTION TO WHAT YOU DO! I have tried to shield the thing up
as much as I could, isolating operator new, operator delete, malloc,
realloc, free, calloc, cfree, but anyhow PAY ATTENTION to non-reentrant
routines, and remember THERE ARE MANY!.

Anyhow, if you have suggestions, ideas, comments, or just want to say hello,
don't hesitate to mail me.

Paolo De Marino (paolodemarino@usa.net)
Via Donizetti 1/E
80127 Naples
Italy
