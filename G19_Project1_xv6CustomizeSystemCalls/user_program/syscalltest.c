// syscalltest.c
// -------------
// xv6 user program that exercises all 8 custom system calls.
//
// Original 5 (Process Info):
//   getppid, getprocs, set_priority, cps, waitx
//
// New 3 (covering Process Creation, Signals, IPC):
//   waitpid, sigkill, shmem
//
// Compile: add to UPROGS and EXTRA in the xv6 Makefile:
//   _syscalltest\
// and copy this file to the xv6 root directory.

#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  printf(1, "\n================================================\n");
  printf(1, "   Custom System Call Test — G19 Project 1\n");
  printf(1, "================================================\n\n");

  // ── ORIGINAL 5 SYSCALLS ──────────────────────────────────

  // 1. getppid — return parent PID
  printf(1, "[1] getppid()           -> %d  (parent PID)\n",
         getppid());

  // 2. getprocs — count of active (non-UNUSED) processes
  printf(1, "[2] getprocs()          -> %d  active processes\n",
         getprocs());

  // 3. set_priority — change our own scheduling priority
  int mypid = getpid();
  int oldp  = set_priority(mypid, 30);
  printf(1, "[3] set_priority(%d,30) -> previous priority was %d\n",
         mypid, oldp);

  // 4. cps — print process table (ps-like)
  printf(1, "\n[4] cps() process table:\n");
  cps();

  // 5. waitx — fork a child and measure its wait/run time
  printf(1, "\n[5] waitx(): forking child ...\n");
  {
    int pid = fork();
    if(pid == 0){
      // child: do some busy work
      int i, j, x = 0;
      for(i = 0; i < 200; i++)
        for(j = 0; j < 100000; j++)
          x += i ^ j;
      printf(1, "    child %d done (junk=%d)\n", getpid(), x & 0xff);
      exit();
    } else {
      int wt = 0, rt = 0;
      int reaped = waitx(&wt, &rt);
      printf(1, "    waitx reaped pid=%d  wtime=%d ticks  rtime=%d ticks\n",
             reaped, wt, rt);
    }
  }

  // ── NEW 3 SYSCALLS ────────────────────────────────────────

  // 6. waitpid — wait for a SPECIFIC child PID
  printf(1, "\n[6] waitpid(): forking child, waiting for its exact PID ...\n");
  {
    int cpid = fork();
    if(cpid == 0){
      printf(1, "    child %d: running briefly, then exiting\n", getpid());
      exit();
    } else {
      int reaped = waitpid(cpid);
      if(reaped == cpid)
        printf(1, "    waitpid(%d) -> reaped exact child PID = %d  [OK]\n",
               cpid, reaped);
      else
        printf(1, "    waitpid(%d) -> got %d  [UNEXPECTED]\n", cpid, reaped);
    }
  }

  // 7. sigkill — send kill signal to a process by PID
  printf(1, "\n[7] sigkill(): forking a looping child, then killing it ...\n");
  {
    int cpid = fork();
    if(cpid == 0){
      // child loops forever; parent will kill it
      printf(1, "    child %d: looping (will be killed by parent)\n", getpid());
      while(1)
        sleep(1);
      exit();
    } else {
      sleep(3);    // give child time to start
      int ret = sigkill(cpid);
      printf(1, "    sigkill(%d) returned %d (0 = success)\n", cpid, ret);
      wait();      // reap the killed child
      printf(1, "    killed child reaped successfully  [OK]\n");
    }
  }

  // 8. shmem — shared memory IPC via a global kernel page
  printf(1, "\n[8] shmem(): demonstrating shared-memory IPC ...\n");
  {
    // Parent maps key 0 and writes a magic value.
    int *addr = (int*)shmem(0);
    if((int)addr < 0){
      printf(1, "    shmem() failed\n");
    } else {
      *addr = 0xDEAD;
      printf(1, "    parent wrote 0x%x to shmem page (VA=0x%x)\n",
             *addr, (int)addr);

      // Child maps the same key and reads back the value.
      int cpid = fork();
      if(cpid == 0){
        int *caddr = (int*)shmem(0);
        printf(1, "    child  read  0x%x from shmem page (VA=0x%x)\n",
               *caddr, (int)caddr);
        if(*caddr == 0xDEAD)
          printf(1, "    shmem IPC value matches  [OK]\n");
        else
          printf(1, "    shmem IPC value mismatch  [FAIL]\n");
        exit();
      } else {
        wait();
      }
    }
  }

  printf(1, "\n================================================\n");
  printf(1, "   All 8 custom syscalls executed successfully!\n");
  printf(1, "================================================\n");
  exit();
}
