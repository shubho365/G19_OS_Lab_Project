/*
 * ncputest.c — User program to demonstrate the getncpu() system call.
 *
 * Usage (inside xv6 shell):
 *   $ ncputest
 *
 * This program invokes the custom getncpu() system call, which queries
 * the kernel for the number of active CPUs detected during boot.
 */
#include "types.h"
#include "stat.h"
#include "user.h"

int
main(void)
{
  int n = getncpu();
  if(n < 0){
    printf(1, "getncpu() failed\n");
    exit();
  }
  printf(1, "Number of active CPUs: %d\n", n);
  exit();
}
