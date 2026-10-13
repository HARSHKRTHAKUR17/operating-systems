#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

int main(void)
{
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        printf("Child: PID = %d\n", getpid());
        exit(0);
    }

    printf("Parent: PID = %d\n", getpid());
    printf("Parent: child PID = %d\n", pid);

    int status;
    pid_t child_pid = wait(&status);

    printf("Parent: child %d finished\n", child_pid);
    return 0;
}
