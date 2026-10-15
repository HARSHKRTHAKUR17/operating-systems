
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

volatile int value;

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <initial-value>\n", argv[0]);
        return EXIT_FAILURE;
    }

    volatile int *p = &value;
    *p = atoi(argv[1]);

    printf("PID: %ld\n", (long)getpid());
    printf("Address stored in p: %p\n", (void *)p);
    fflush(stdout);

    while (1) {
        sleep(1);
        *p = *p + 1;
        printf("PID: %ld | value of p: %d\n", (long)getpid(), *p);
        fflush(stdout);
    }

    return EXIT_SUCCESS;
}
