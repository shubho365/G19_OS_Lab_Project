// syscalltest.c
// -------------
// xv6 user program that exercises all five custom system calls.
// Copy this file into the xv6-public source directory and add it
// to UPROGS and EXTRA in the xv6 Makefile (see INSTRUCTIONS.md).

#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  printf(1, "\n========================================\n");
  printf(1, "  Custom System Call Test (Project 1)\n");
  printf(1, "========================================\n\n");

  // ---------- 1. getppid ----------
  printf(1, "[1] getppid()           -> %d (parent pid of this shell-child)\n",
         getppid());

  // ---------- 2. getprocs ----------
  printf(1, "[2] getprocs()          -> %d active processes\n", getprocs());

  // ---------- 3. set_priority on ourselves ----------
  int mypid = getpid();
  int oldp  = set_priority(mypid, 30);
  printf(1, "[3] set_priority(%d,30) -> previous priority was %d\n",
         mypid, oldp);

  // ---------- 4. cps  (ps-like listing) ----------
  printf(1, "[4] cps() output:\n");
  cps();

  // ---------- 5. waitx via fork ----------
  printf(1, "\n[5] forking a child for waitx() ...\n");
  int pid = fork();
  if(pid < 0){
    printf(1, "    fork failed!\n");
    exit();
  }
  if(pid == 0){
    // child: do some busy work so rtime > 0
    int i, j, x = 0;
    for(i = 0; i < 200; i++)
      for(j = 0; j < 100000; j++)
        x += i ^ j;
    printf(1, "    child %d done (junk=%d)\n", getpid(), x & 0xff);
    exit();
  } else {
    int wtime = 0, rtime = 0;
    int reaped = waitx(&wtime, &rtime);
    printf(1, "    waitx reaped pid=%d  wtime=%d ticks  rtime=%d ticks\n",
           reaped, wtime, rtime);
  }

  printf(1, "\n========================================\n");
  printf(1, "  All five syscalls executed successfully\n");
  printf(1, "========================================\n");
  exit();
}
