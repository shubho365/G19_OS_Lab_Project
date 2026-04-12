#include "msgqueue.h"
#include "stat.h"
#include "types.h"
#include "user.h"

struct test_msg
{
    long mtype;
    char mtext[100];
};

void printf(int fd, const char *fmt, ...);

int main(void)
{
    int msqid;
    key_t key = 1234;
    int pid;
    struct test_msg send_msg;
    struct test_msg recv_msg;

    printf(1, "Starting Message Queue test...\n");

    msqid = msgget(key, IPC_CREAT);
    if (msqid < 0)
    {
        printf(1, "msgget failed\n");
        exit();
    }
    printf(1, "Message queue created, id: %d\n", msqid);

    pid = fork();
    if (pid < 0)
    {
        printf(1, "fork failed\n");
        exit();
    }

    if (pid > 0)
    {
        // Parent
        send_msg.mtype = 1;
        strcpy(send_msg.mtext, "Hello from parent via Message Queue!");

        printf(1, "Parent sending message: %s\n", send_msg.mtext);
        if (msgsnd(msqid, &send_msg, strlen(send_msg.mtext) + 1, 0) < 0)
        {
            printf(1, "msgsnd failed\n");
        }
        else
        {
            printf(1, "Parent message sent.\n");
        }

        // Wait for child to exit
        wait();

        // Clean up
        if (msgctl(msqid, IPC_RMID, 0) < 0)
        {
            printf(1, "msgctl IPC_RMID failed\n");
        }
        else
        {
            printf(1, "Message queue destroyed.\n");
        }

        printf(1, "Message Queue test passed!\n");
        exit();
    }
    else
    {
        // Child
        printf(1, "Child waiting for message...\n");
        int bytes = msgrcv(msqid, &recv_msg, sizeof(recv_msg.mtext), 1, 0);
        if (bytes < 0)
        {
            printf(1, "msgrcv failed\n");
        }
        else
        {
            printf(1, "Child received message: %s (%d bytes)\n", recv_msg.mtext, bytes);
        }
        exit();
    }
}
