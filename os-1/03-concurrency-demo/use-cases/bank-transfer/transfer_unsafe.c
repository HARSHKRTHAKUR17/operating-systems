
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

static atomic_int account_a = 500;
static atomic_int account_b = 300;
static atomic_int stage = 0;

static void *transfer(void *arg)
{
    (void)arg;

    atomic_fetch_sub(&account_a, 100);
    atomic_store(&stage, 1);

    /* Wait until the reader observes the intermediate state. */
    while (atomic_load(&stage) != 2) {
    }

    atomic_fetch_add(&account_b, 100);
    return NULL;
}

static void *reader(void *arg)
{
    (void)arg;

    while (atomic_load(&stage) != 1) {
    }

    int a = atomic_load(&account_a);
    int b = atomic_load(&account_b);

    printf("Intermediate: A = %d, B = %d, total = %d\n",
           a, b, a + b);

    atomic_store(&stage, 2);
    return NULL;
}

int main(void)
{
    pthread_t writer, observer;

    if (pthread_create(&writer, NULL, transfer, NULL) != 0) {
        perror("pthread_create");
        return 1;
    }

    if (pthread_create(&observer, NULL, reader, NULL) != 0) {
        perror("pthread_create");
        pthread_join(writer, NULL);
        return 1;
    }

    pthread_join(writer, NULL);
    pthread_join(observer, NULL);

    printf("Final: A = %d, B = %d, total = %d\n",
           atomic_load(&account_a), atomic_load(&account_b),
           atomic_load(&account_a) + atomic_load(&account_b));

    return 0;
}

