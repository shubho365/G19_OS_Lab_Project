#ifndef MSGQUEUE_H
#define MSGQUEUE_H

#include "types.h"

typedef int key_t;
typedef uint size_t;

#define IPC_CREAT  00001000
#define IPC_EXCL   00002000
#define IPC_NOWAIT 00004000

#define IPC_RMID   0

struct msqid_ds
{
    uint msg_qnum;      // no of messages on queue
    uint msg_qbytes;    // bytes max on a queue
    int  msg_lspid;     // pid of last msgsnd
    int  msg_lrpid;     // pid of last msgrcv
};

struct msgbuf
{
    long mtype;
    char mtext[1];
};

#endif
