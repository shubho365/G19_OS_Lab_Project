struct stat;
struct rtcdate;
#include "msgqueue.h"


// system calls
int fork(void);
int exit(void) __attribute__((noreturn));
int wait(void);
int pipe(int*);
int write(int, const void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(char*, char**);
int open(const char*, int);
int mknod(const char*, short, short);
int unlink(const char*);
int fstat(int fd, struct stat*);
int link(const char*, const char*);
int mkdir(const char*);
int chdir(const char*);
int dup(int);
int getpid(void);
char* sbrk(int);
int sleep(int);
int uptime(void);

int getppid(void);
int getprocs(void);
int set_priority(int pid, int prio);
int cps(void);
int waitx(int *wtime, int *rtime);
int waitpid(int pid);
int sigkill(int pid);
int shmem(int key);
int fork2(int priority);
int shmget(int id);
int shmem_count(int id);
int mutex_create(void);
int mutex_lock(int mutex_id);
int mutex_unlock(int mutex_id);
int msgget(key_t key, int msgflg);
int msgsnd(int msqid, void *msgp, size_t msgsz, int msgflg);
int msgrcv(int msqid, void *msgp, size_t msgsz, long int msgtyp, int msgflg);
int msgctl(int msqid, int cmd, struct msqid_ds *buf);

// ulib.c
int stat(const char*, struct stat*);
char* strcpy(char*, const char*);
void *memmove(void*, const void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
void printf(int, const char*, ...);
char* gets(char*, int max);
uint strlen(const char*);
void* memset(void*, int, uint);
void* malloc(uint);
void free(void*);
int atoi(const char*);
