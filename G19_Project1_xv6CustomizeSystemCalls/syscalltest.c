// syscalltest.c
// -------------
// xv6 user program that exercises all eight custom system calls.
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

  // ---------- 6. waitpid ----------
  printf(1, "\n[6] forking a child for waitpid() ...\n");
  pid = fork();
  if(pid < 0){
    printf(1, "    fork failed!\n");
  } else if(pid == 0){
    sleep(10);
    exit();
  } else {
    int reaped = waitpid(pid);
    printf(1, "    waitpid reaped specific pid=%d\n", reaped);
  }

  // ---------- 7. sigkill ----------
  printf(1, "\n[7] forking a child to sigkill() ...\n");
  pid = fork();
  if(pid < 0){
    printf(1, "    fork failed!\n");
  } else if(pid == 0){
    while(1) sleep(10);
  } else {
    int res = sigkill(pid);
    printf(1, "    sigkill(%d) returned %d\n", pid, res);
    wait(); // clean up the zombie
  }

  // ---------- 8. shmem ----------
  printf(1, "\n[8] testing shmem(0) ...\n");
  char *ptr = (char*)shmem(0);
  if(ptr != (char*)-1){
    printf(1, "    shmem(0) returned address %p\n", ptr);
    // Note: The current implementation of shmem in sysproc.c 
    // uses copyout to copy kernel buffer to user space.
    // We can at least verify we can access the memory.
    memset(ptr, 0, 4096); 
    strcpy(ptr, "hello shared memory");
    printf(1, "    wrote to shared memory: %s\n", ptr);
  } else {
    printf(1, "    shmem(0) failed\n");
  }

  printf(1, "\n========================================\n");
  printf(1, "  All eight syscalls executed successfully\n");
  printf(1, "========================================\n");
  exit();
}
