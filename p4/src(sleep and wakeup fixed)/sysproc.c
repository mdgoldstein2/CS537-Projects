#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "pstat.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;

  if(argint(0, &n) < 0)
    return -1;

  // this section has beel slightly rewritten
  acquire(&tickslock);
  if(n == 0) {
    release(&tickslock);
    return 0;
  }

  sleepfortime(&ticks, &tickslock, n);
  if(myproc()->killed){
    release(&tickslock);
    return -1;
  }

  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

/**
 * Function that sets the time slice length of the process with the given pid.
 * Gets arguments from kernel, then calls setslice() from proc.c
 *
 * Return: -1 if unable to fetch arguments, else the value of
 *         setslice(pid, slice)
 */
int
sys_setslice(void) {
    int pid;  // pid of the process whose slice will be set
    int slice;  // slice length from process

    // get pid
    if (argint(0, &pid) < 0) {
        return -1;  // return -1 if unable
    }

    // get slice
    if (argint(1, &slice) < 0) {
        return -1;  // return -1 if unable
    }

    return setslice(pid, slice);  // call and return value of setslice
}

/**
 * Function that returns the time slice length of a the process with the given
 * pid. Gets argument from kernel, then calls getslice() from proc.c
 *
 * Return: -1 if unable to fetch argument, else the value of getslice(pid)
 */
int
sys_getslice(void) {
    int pid;  // pid of process whose slice length will be returned

    // get pid
    if (argint(0, &pid) < 0) {
        return -1;  // return -1 if unable
    }

    return getslice(pid);  // call and return value of getslice
}

/**
 * Function that creates a child process with the given time slice length. Gets
 * argument from kernel, then calls fork2() from proc.c
 *
 * Return: -1 if unable to fetch argument, else the value of fork2(pid)
 */
int
sys_fork2(void) {
    int slice;  // time slice length of new process

    // get slice
    if (argint(0, &slice) < 0) {
        return -1;  // return -1 if unable
    }

    return fork2(slice);  // call and return value of fork2()
}

/**
 * Function that fills a struct pstat with information about each process. Gets
 * argument from kernel, then calls getpinfo() from proc.
 *
 * Return: -1 if unable to fetch argument, else the value of
 * getpinfo(pstat_address).
 */
int
sys_getpinfo(void) {
    struct pstat *struct_pointer;  // pointer to struct

    // get pointer
    if (argptr(0, (char **)&struct_pointer, sizeof(struct pstat)) < 0 ) {
        return -1;  // return -1 if unable
    }

    // call and return value of getpinfo()
    return getpinfo((struct pstat*)struct_pointer);
}
