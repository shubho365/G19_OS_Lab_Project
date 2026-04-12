#include "types.h"
#include "stat.h"
#include "user.h"

int main() {
    int m = mutex_create();
    if(m < 0) {
        printf(1, "Failed to create mutex\n");
        exit();
    }
    
    printf(1, "Starting mutex test. Parent PID: %d\n", getpid());
    
    int pid = fork();
    if(pid < 0) {
        printf(1, "Fork failed\n");
        exit();
    }
    
    if(pid == 0) {
        // Child
        mutex_lock(m);
        printf(1, "Child %d acquired lock\n", getpid());
        sleep(50); // sleep a while to ensure parent blocks
        printf(1, "Child %d releasing lock\n", getpid());
        mutex_unlock(m);
        exit();
    } else {
        // Parent
        sleep(10); // give child a chance to grab the lock first
        printf(1, "Parent %d attempting to grab lock...\n", getpid());
        mutex_lock(m);
        printf(1, "Parent %d acquired lock\n", getpid());
        mutex_unlock(m);
        wait();
    }
    
    printf(1, "Mutex test passed!\n");
    exit();
}
