#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "pstat.h"

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
  struct queue proc_queue;
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

/**
 * Function that adds a new process to the process queue. Make sure this is
 * only called with the ptable lock acquired.
 *
 * new_proc: process to add to queue
 */
void
enqueue(struct proc *new_proc) {
    static int initialized = 0;  // static var to track if queue initialized

    if (initialized == 0) {  // if uninitialized, set head and tail to 0
        ptable.proc_queue.head = 0;
        ptable.proc_queue.tail = 0;
        initialized = 1;  // mark as initialized
    } 

    // check if queue empty
    if (ptable.proc_queue.head == 0) {
        // have head and tail point to new proc
        ptable.proc_queue.head = new_proc;
        ptable.proc_queue.tail = new_proc;
    } else {  // add to tail if not empty
        ptable.proc_queue.tail->next = new_proc;
        ptable.proc_queue.tail = new_proc;
    }

    ptable.proc_queue.tail->next = 0;  // set next to null
}

/**
 * Function that returns the head of the process queue and moves the head
 * to the next process. Make sure this is only called with ptable lock acquired
 */
struct proc*
dequeue() {
    // if head is null, return
    if (ptable.proc_queue.head == 0) {
        return 0;
    }

    struct proc *temp= ptable.proc_queue.head;  // get head
    ptable.proc_queue.head = ptable.proc_queue.head->next;  // move head to next
    
    // if new head is null, list is empty so set tail to null
    if (ptable.proc_queue.head == 0) {
        ptable.proc_queue.tail = 0;
    }

    return temp;  // return old head
}    


//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  // set timeslice to 1, all other time trackers to 0
  p->runticks = 0;
  p->sleepfor = 0;
  p->currentcomp = 0;
  p->timeslice = 1;
  p->compticks = 0;
  p->schedticks = 0;
  p->sleepticks = 0;
  p->switches = 0;

  p->state = RUNNABLE;
  enqueue(p);  // add p to queue

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  // set timeslice to parent's slice, all other time trackers to 0
  np->runticks = 0;
  np->sleepfor = 0;
  np->currentcomp = 0;
  np->timeslice = np->parent->timeslice;
  np->compticks = 0;
  np->schedticks = 0;
  np->sleepticks = 0;
  np->switches = 0;

  np->state = RUNNABLE;
  enqueue(np);  // add p to queue

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
    struct cpu *c = mycpu();  // get cpu

    for (;;) {  // infinite loop
        sti();  // Enable interrputs on this processor.
        acquire(&ptable.lock);  // get lock 
        struct proc *p = c->proc;  // get current proc

        // check if there is a previous process to run again
        if (p != 0 && p->state == RUNNABLE &&
                p->runticks < p->timeslice + p->currentcomp) {
            // increment run and scheduled ticks
            p->runticks++;
            p->schedticks++;

            // check if compensation slices will be used
            if (p->runticks > p->timeslice) {
                p->compticks++;  // increment if so
            }
        } else {
            // if process did existi and ran previously, reset runticks
            //  and currentcomp, then enqueue
            if (p != 0) {
                p->runticks = 0;
                p->currentcomp = 0;
                enqueue(p);
            }

            p = dequeue();  // get process from queue
            int sleep_check = 0;  // checks how many times while loop has run
        
            // go through queue until runnable process found or loop has run over
            // all possible processes
            while (p != 0 && p->state != RUNNABLE && sleep_check < NPROC) {
                // add sleeping processes to end of queue
                if (p->state == SLEEPING) {
                    enqueue(p);
                    sleep_check++;
                }
                p = dequeue();  // get next process from queue
            }

            if (p == 0 || p->state != RUNNABLE) {  // no runnable processes in queue
                // enqueue p if sleeping
                if (p != 0 && p->state == SLEEPING) {
                    enqueue(p);
                    c->proc = 0;  // clear proc to avoid double resets
                }
                release(&ptable.lock);  // release lock
                continue;  // skip incrementing or executing since not runnable
            }

            // mark this runnable process as switched to, increment run and
            // scheduled ticks
            p->switches++;
            p->runticks++;
            p->schedticks++;
        }

        // switch to chosen process
        c->proc = p;
        switchuvm(p);
        p->state = RUNNING;
        swtch(&(c->scheduler), p->context);

        switchkvm();  // return from process
        release(&ptable.lock);  // release lock after running
    }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;
  p->currentcomp = 0;
  p->runticks = 0;
  p->sleepfor = 0;  // clear sleep timer (it is not used in regular sleep)

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

/**
 * Function that is otherwise identical to sleep except sets a process
 * to sleep for a given amount of time.
 *
 * chan: arbitrary channel for process to sleep on
 * lk: current lock held by the process
 * sleeptime: how many ticks the process should be asleep for
 */
void sleepfortime(void *chan, struct spinlock *lk, int sleeptime) {
    struct proc *p = myproc();  // get current process
    if (p == 0) {  // panic of no current process
        panic("sleep");
    }

    if(lk == 0) {  // panic if no lock
        panic("sleep without lk");
    }

    // Must acquire ptable.lock in order to
    // change p->state and then call sched.
    // Once we hold ptable.lock, we can be
    // guaranteed that we won't miss any wakeup
    // (wakeup runs with ptable.lock locked),
    // so it's okay to release lk.
    if(lk != &ptable.lock){  //DOC: sleeplock0
        acquire(&ptable.lock);  //DOC: sleeplock1
        release(lk);
    }

    // Go to sleep.
    p->chan = chan;
    p->state = SLEEPING;
    p->currentcomp = 0;
    p->runticks = 0;
    p->sleepfor = sleeptime;  // set sleep timer

    sched();  // call scheduler

    // Tidy up.
    p->chan = 0;

    // Reacquire original lock.
    if(lk != &ptable.lock){  //DOC: sleeplock2
        release(&ptable.lock);
        acquire(lk);
    }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
    struct proc *p;  // process used for iterating over ptable

    // iterate over ptable
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        // wakeup1(&ticks) is called on every tick, so increment sleepticks and
        // compensation ticks for all processes
        if (chan == &ticks && p->state == SLEEPING) {
            p->currentcomp++;
            p->sleepticks++;

            // decrement sleeptime ticks for processes put to sleep by syscall
            if (p->chan == &ticks) {
                p->sleepfor--;
            }
        }

        // wake all processes on the given channel who no longer need to sleep
        // NOTE: processes not put to sleep by syscall always have sleepfor = 0
        if (p->state == SLEEPING && p->chan == chan && p->sleepfor == 0) {
            p->state = RUNNABLE;
        }
    }
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

/**
 * Function that sets the time slice length of a process.
 *
 * pid: pid of process whose slice length will be set
 * slice: the time slice length for the process
 * Return: -1 if pid or slice invalid, else 0
 */
int
setslice (int pid, int slice) {
    // check if slice < 1
    if (slice < 1) {
        return -1;  // return -1 if so
    }    

    acquire(&ptable.lock);  // get lock

    // iterate over ptable to find process with matching pid
    for (struct proc *p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        // check if pid matches
        if (p->pid == pid) {  // if it does, update timeslice and release
            p->timeslice = slice;
            release(&ptable.lock);
            return 0;  // return 0 on success
        }
    }

    release(&ptable.lock);  // release lock
    return -1;  // if reached end, pid invalid
}

/**
 * Function that returns the time slice of the process with the given pid
 *
 * pid: pid of process whose time slice will be returned
 * Return: -1 if pid invalid, else slice length for the process
 */
int
getslice (int pid) {
    acquire(&ptable.lock);  // acquire lock to make sure values not altered    

     // iterate over ptable to find process with matching pid
    for (struct proc *p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        // check if pid matches
        if (p->pid == pid) {  // if it does, get time slice and release
            int slice = p->timeslice;  // store time slice in temp
            release(&ptable.lock);  // release lock
            return slice;  // return time slice
        }
    }

    release(&ptable.lock);  // release lock
    return -1;  // if reached end, pid invalid
}

/**
 * Function that creates a new process with the given slice length.
 *
 * slice: time slice length of the new process
 * return: -1 on failure, 0 to child process, pid of child process to parent
 *         process
 */
int
fork2(int slice) {
    // check if slice < 1
    if (slice < 1) {
        return -1;  // return -1 if so
    }

    // rest is copied from slice except slice of new process is set by function
    int i, pid;
    struct proc *np;
    struct proc *curproc = myproc();

    // Allocate process.
    if ((np = allocproc()) == 0) {
        return -1;
    }

    // Copy process state from proc.
    if ((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0) {
        kfree(np->kstack);
        np->kstack = 0;
        np->state = UNUSED;
        return -1;
    }
    np->sz = curproc->sz;
    np->parent = curproc;
    *np->tf = *curproc->tf;

    // Clear %eax so that fork returns 0 in the child.
    np->tf->eax = 0;

    for (i = 0; i < NOFILE; i++)
        if (curproc->ofile[i])
            np->ofile[i] = filedup(curproc->ofile[i]);
    np->cwd = idup(curproc->cwd);

    safestrcpy(np->name, curproc->name, sizeof(curproc->name));

    pid = np->pid;

    acquire(&ptable.lock);

    // set timeslice to given slice, all other time trackers to 0
    np->runticks = 0;
    np->sleepfor = 0;
    np->currentcomp = 0;
    np->timeslice = slice;
    np->compticks = 0;
    np->schedticks = 0;
    np->sleepticks = 0;
    np->switches = 0;

    np->state = RUNNABLE;
    enqueue(np);  // add p to queue

    release(&ptable.lock);

    return pid;
}

/**
 * Function that fills a pstat structure with information on all processes on the ptable.
 *
 * pointer: pointer to the pstat structure
 * Return: -1 if pointer not valid, else 0
 */
int
getpinfo(struct pstat *pointer) {
    // if pointer is null, return -1
    if (pointer == 0) {
        return -1;
    }

    acquire(&ptable.lock);  // get lock

    // copy over info from ptable to pstat
    for (struct proc *p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        // copy over in use
        if (p->state == UNUSED) {
            pointer->inuse[p - ptable.proc] = 0;
        } else {
            pointer->inuse[p - ptable.proc] = 1;
        }

        // copy all other info
        pointer->pid[p - ptable.proc] = p->pid;
        pointer->timeslice[p - ptable.proc] = p->timeslice;
        pointer->compticks[p - ptable.proc] = p->compticks;
        pointer->schedticks[p - ptable.proc] = p->schedticks;
        pointer->sleepticks[p - ptable.proc] = p->sleepticks;
        pointer->switches[p - ptable.proc] = p->switches;
    }

    release(&ptable.lock);
    
    return 0;  // function successful, return 0
}
