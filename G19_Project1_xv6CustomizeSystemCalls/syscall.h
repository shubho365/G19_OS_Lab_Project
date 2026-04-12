// System call numbers
#define SYS_fork    1
#define SYS_exit    2
#define SYS_wait    3
#define SYS_pipe    4
#define SYS_read    5
#define SYS_kill    6
#define SYS_exec    7
#define SYS_fstat   8
#define SYS_chdir   9
#define SYS_dup    10
#define SYS_getpid 11
#define SYS_sbrk   12
#define SYS_sleep  13
#define SYS_uptime 14
#define SYS_open   15
#define SYS_write  16
#define SYS_mknod  17
#define SYS_unlink 18
#define SYS_link   19
#define SYS_mkdir  20
#define SYS_close  21

#define SYS_getppid       22
#define SYS_getprocs      23
#define SYS_set_priority  24
#define SYS_cps           25
#define SYS_waitx         26

// New syscalls — Project 1 (covering Process Creation, Signals, IPC)
#define SYS_waitpid       27
#define SYS_sigkill       28
#define SYS_shmem         29
#define SYS_fork2         30
#define SYS_shmget        31
#define SYS_shmem_count   32
#define SYS_mutex_create  33
#define SYS_mutex_lock    34
#define SYS_mutex_unlock  35
