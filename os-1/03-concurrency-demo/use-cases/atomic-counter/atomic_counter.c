
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

static atomic_int counter = 0;
static int loops;

static void *worker(void *arg)
{
    (void)arg;

    for (int i = 0; i < loops; i++) {
        atomic_fetch_add(&counter, 1);
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <loops>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *end;
    long parsed = strtol(argv[1], &end, 10);

    if (*argv[1] == '\0' || *end != '\0' ||
        parsed < 0 || parsed > 100000000L) {
        fprintf(stderr, "Please provide loops between 0 and 100000000.\n");
        return EXIT_FAILURE;
    }

    loops = (int)parsed;

    pthread_t p1, p2;

    if (pthread_create(&p1, NULL, worker, NULL) != 0) {
        perror("pthread_create");
        return EXIT_FAILURE;
    }

    if (pthread_create(&p2, NULL, worker, NULL) != 0) {
        perror("pthread_create");
        pthread_join(p1, NULL);
        return EXIT_FAILURE;
    }

    pthread_join(p1, NULL);
    pthread_join(p2, NULL);

    printf("Expected value: %d\n", 2 * loops);
    printf("Final value:   %d\n", atomic_load(&counter));

    return EXIT_SUCCESS;
}

