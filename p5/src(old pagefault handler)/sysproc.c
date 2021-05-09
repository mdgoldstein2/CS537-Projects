#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "ptentry.h"

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
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
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
 * Function that gets input for the mencrypt syscall, then calls it.
 *
 * Return: -1 if unable to get input (or num_pages < 0), 0 if num_pages = 0,
 *         else the value of mencrypt.
 */
int sys_mencrypt(void) {
    char *address;  // starting address to be encrypted
    int num_pages;  // number of pages past the address to also be encrypted

    // get number of pages
    if (argint(1, &num_pages) < 0) {
        return -1;  // return -1 if unable to get
    }

    // check if it is 0
    if (num_pages == 0) {
        return 0;  // return 0 if so
    } else if (num_pages < 0) {  // check if it is negative
        return -1;  // return -1 if so
    }

    // get address
    if (argptr(0, &address, 1)) {
        return -1;  // return -1 if unable to get
    }

    return mencrypt(address, num_pages);  // call mencrypt
}

/**
 * Function that gets input for the getpgtable syscall, then calls it.
 *
 * Return: -1 if unable to get input, else the value of getpgtable.
 */
int sys_getpgtable(void) {
    struct pt_entry *pgtable;  // pointer to array of ptentries
    int num_entries;  // number of entries in table

    // get number of entries
    if (argint(1, &num_entries) < 0) {
        return -1;  // return -1 if unable
    }

    // get pointer
    if (argptr(0, (char**) &pgtable, sizeof(struct pt_entry) * num_entries) < 0) {
        return -1;  // return -1 if unable
    }

    return getpgtable(pgtable, num_entries);  // call getpgtable
}

/**
 * Function that gets input for the dump_rawphymem syscall, then calls it.
 *
 * Return: -1 if unable to get input, else the value of dump_rawphymem.
 */
int sys_dump_rawphymem(void) {
    uint phys_addr;  // physical address whose page will be dumped
    char *buffer;  // buffer to print out page to

    // get phys_addr
    if (argint(0, (int*) &phys_addr) < 0) {
        return -1;  // return -1 if unable
    }

    // get buffer
    if (argptr(1, &buffer, PGSIZE) < 0) {
        return -1;  // return -1 if unable
    }

    return dump_rawphymem(phys_addr, buffer);  // call dump_rawphymem
}
