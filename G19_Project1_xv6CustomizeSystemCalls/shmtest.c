// shmtest.c
// ----------
// xv6 user program that tests fork2() and shared memory (shmget / shmem_count).

#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  printf(1, "\n========================================\n");
  printf(1, "  fork2 + Shared Memory Test\n");
  printf(1, "========================================\n\n");

  // ---------- 1. fork2 ----------
  printf(1, "[1] Testing fork2(priority=10) ...\n");
  int pid = fork2(10);
  if(pid < 0){
    printf(1, "    fork2 failed!\n");
    exit();
  }
  if(pid == 0){
    // child: verify our priority by calling cps
    printf(1, "    child pid=%d created with priority 10\n", getpid());
    printf(1, "    running cps() to verify:\n");
    cps();
    exit();
  } else {
    wait();
    printf(1, "    parent: child %d finished\n\n", pid);
  }

  // ---------- 2. shmget + shmem_count ----------
  printf(1, "[2] Testing shared memory (shmget / shmem_count) ...\n");

  // Before anyone maps it, refcount should be 0.
  printf(1, "    shmem_count(0) before shmget = %d\n", shmem_count(0));

  // Parent maps shared page 0.
  int addr = shmget(0);
  if(addr == -1){
    printf(1, "    shmget(0) failed in parent!\n");
    exit();
  }
  printf(1, "    parent: shmget(0) returned addr 0x%x\n", addr);
  printf(1, "    shmem_count(0) after parent maps = %d\n", shmem_count(0));

  // Write a magic value into the shared page.
  int *shared = (int*)addr;
  *shared = 42;
  printf(1, "    parent wrote %d to shared page\n", *shared);

  // Fork a child (regular fork) that also maps shared page 0.
  pid = fork();
  if(pid < 0){
    printf(1, "    fork failed!\n");
    exit();
  }
  if(pid == 0){
    // Child maps the same shared page.
    int caddr = shmget(0);
    if(caddr == -1){
      printf(1, "    shmget(0) failed in child!\n");
      exit();
    }
    int *cshared = (int*)caddr;
    printf(1, "    child:  shmget(0) returned addr 0x%x\n", caddr);
    printf(1, "    child:  read %d from shared page (expect 42)\n", *cshared);
    printf(1, "    shmem_count(0) inside child = %d\n", shmem_count(0));

    // Write back a different value so parent can see.
    *cshared = 99;
    printf(1, "    child:  wrote %d to shared page\n", *cshared);
    exit();
  } else {
    wait();
    printf(1, "    parent: read %d from shared page after child wrote (expect 99)\n", *shared);
    printf(1, "    shmem_count(0) after child exits = %d\n", shmem_count(0));
  }

  printf(1, "\n========================================\n");
  printf(1, "  fork2 + shared memory tests done\n");
  printf(1, "========================================\n");
  exit();
}
