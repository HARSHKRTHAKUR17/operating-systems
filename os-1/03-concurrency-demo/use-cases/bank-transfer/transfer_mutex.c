
#include <pthread.h>
#include <stdio.h>

static int account_a = 500;
static int account_b = 300;
static int stage = 0;

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t condition = PTHREAD_COND_INITIALIZER;

static void *transfer(void *arg)
{
    (void)arg;

    pthread_mutex_lock(&lock);

    account_a -= 100;
    stage = 1;
    pthread_cond_broadcast(&condition);

    account_b += 100;

    pthread_mutex_unlock(&lock);
    return NULL;
}

static void *reader(void *arg)
{
    (void)arg;

    pthread_mutex_lock(&lock);

    while (stage == 0) {
        pthread_cond_wait(&condition, &lock);
    }

    int a = account_a;
    int b = account_b;

    pthread_mutex_unlock(&lock);

    printf("Reader observed: A = %d, B = %d, total = %d\n",
           a, b, a + b);

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

    pthread_mutex_lock(&lock);
    printf("Final: A = %d, B = %d, total = %d\n",
           account_a, account_b, account_a + account_b);
    pthread_mutex_unlock(&lock);

    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&condition);

    return 0;
}

