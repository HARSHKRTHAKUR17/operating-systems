# Instructor Concurrency Demo: Threads and Mutexes

This folder contains the instructor-style thread-counter example and its mutex-protected version. The goal is to understand how POSIX threads share process memory, why `counter++` is not a safe concurrent operation by itself, how a mutex protects a critical section, and how to measure runtime.

## Files

```text
instructor-demo/
├── common.h
├── threads.c
├── lock_threads.c
├── threads          # generated executable; do not commit
└── lock_threads     # generated executable; do not commit
```

- `threads.c`: two worker threads increment a shared counter without synchronization. This demonstrates a data race.
- `lock_threads.c`: protects each increment with a mutex.
- `common.h`: shared helper functions and checked wrappers around pthread operations. Keep one definition of each helper in this file; do not append duplicate copies.

Executables are produced by compiling the source files. They are not source files and normally should not be committed.

## Environment: use Ubuntu/WSL

These examples use POSIX threads (`pthread.h`). Build and run them in Ubuntu/WSL, rather than a typical Git Bash/MinGW environment. Git Bash remains useful for Git commands.

Your repository can stay in your Windows folder. From Ubuntu/WSL, the Windows path `C:\Users\Lenovo\Desktop\Operating-System` is accessible as:

```bash
/mnt/c/Users/Lenovo/Desktop/Operating-System
```

Navigate to this directory:

```bash
cd /mnt/c/Users/Lenovo/Desktop/Operating-System/os-1/03-concurrency-demo/instructor-demo
```

If GCC and the build tools are not installed in Ubuntu/WSL:

```bash
sudo apt update
sudo apt install build-essential
```

## Build

Compile both programs from this directory:

```bash
gcc -Wall -Wextra -Wpedantic -g -pthread -o threads threads.c
gcc -Wall -Wextra -Wpedantic -g -pthread -o lock_threads lock_threads.c
```

Then run them:

```bash
./threads 100000
./lock_threads 100000
```

If a worker function does not use its `void *arg` parameter, the compiler may warn that it is unused. This is a warning, not necessarily a build failure. Add `(void)arg;` near the beginning of the function to explicitly mark it unused.

### Compiler flags

- `-Wall`: enables a broad set of common warnings.
- `-Wextra`: enables additional warnings.
- `-Wpedantic`: warns about code that does not conform to the selected ISO C standard.
- `-g`: includes debugging information for tools such as GDB.
- `-pthread`: enables the appropriate POSIX thread compilation and linking options.
- `-o threads` / `-o lock_threads`: chooses the output executable's name.

GCC can print warnings and still successfully produce an executable. Check its exit status or whether the executable was created if unsure.

## Command-line arguments: `argc`, `argv`, and `atoi`

The programs expect exactly one command-line argument: the number of increments each worker should attempt.

```c
int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "usage: threads <value>\\n");
        exit(1);
    }

    loops = atoi(argv[1]);
    /* create threads, join them, print result */
}
```

- `argc` is the argument count. It includes the program name.
- `argv` is an array of strings containing the arguments.
- `argv[0]` is the program name/path, and `argv[1]` is the first user-provided argument.
- `./lock_threads 100000` normally has `argc == 2`, and `argv[1]` is the string `"100000"`.
- `atoi(argv[1])` converts that string to an `int`.
- `argc != 2` rejects both a missing value and extra arguments.

The argument is the number of increments **per worker**, not the total across both workers. With two workers and `loops = 100000`, the intended total is `2 × 100000 = 200000` increments.

`atoi()` is convenient for a teaching example, but it does not robustly report invalid input or overflow. Production-quality code should use `strtol()`, check conversion errors, and reject negative or out-of-range values.

## The unsynchronized version: `threads.c`

Both worker threads access the same global counter:

```c
volatile int counter = 0;
int loops;
```

Each worker repeats `counter++`. The global counter is shared because both threads run in the same process and share its address space.

### Why `counter++` is unsafe

`counter++` is a read-modify-write operation: conceptually, it reads the old value, computes the next value, and stores it. The operation is not guaranteed to be atomic. Two threads can interfere with one another, causing updates to be lost.

In C, concurrent unsynchronized accesses to a non-atomic object, when at least one access is a write, constitute a **data race** and lead to **undefined behavior**. Therefore, the final value is not guaranteed to be `2 * loops`, nor is it guaranteed to be any particular lower value. The program may happen to print the expected result on a particular run, but that does not make it correct.

`volatile` does **not** make an increment atomic and does not provide thread synchronization. It is not a fix for a data race.

### Why a race can happen on one or multiple CPU cores

A scheduler can switch between threads on a single core, allowing their operations to interleave. On multiple cores, threads may also execute simultaneously. The race is caused by unsynchronized access to shared state, not simply by having multiple cores.

## The mutex-protected version: `lock_threads.c`

The improved worker protects the counter update:

```c
Pthread_mutex_lock(&m);
counter++;
Pthread_mutex_unlock(&m);
```

The mutex `m` must be initialized before the workers use it:

```c
Pthread_mutex_init(&m, NULL);
```

A mutex provides mutual exclusion: while one thread holds the mutex, another thread attempting to lock that same mutex must wait until it becomes available. This makes the increment a protected critical section, provided every concurrent access that needs protection follows the same locking protocol.

With two workers and a non-negative `loops` value, the expected final counter is `2 * loops`. For `./lock_threads 100000`, the final counter should be `200000`, assuming thread creation, joining, and mutex operations succeed.

### Important details

- The mutex is locked and unlocked inside the loop. Each increment is protected separately; the whole loop is not one large critical section.
- `Pthread_mutex_init(&m, NULL)` initializes the mutex with default attributes.
- `Pthread_join(p1, NULL)` and `Pthread_join(p2, NULL)` make `main` wait for both workers to finish before printing the final counter.
- `common.h` defines wrappers such as `Pthread_create`, `Pthread_join`, and the mutex wrappers. They check pthread return codes and terminate if an operation reports an error.
- The program should not destroy the mutex until no thread can use it. These demos can be extended to call `pthread_mutex_destroy(&m)` after both joins.

## What is `common.h` for?

`common.h` is a shared header for helpers used by the demo programs.

### Header guard

```c
#ifndef COMMON_H
#define COMMON_H
/* declarations and helper definitions */
#endif
```

The guard prevents the same header from being processed repeatedly within a single translation unit. It does **not** prevent duplicate function definitions written twice inside the header itself. If editing `common.h`, replace its contents carefully instead of pasting a second copy at the bottom.

### `GetTime()`

`GetTime()` uses `gettimeofday()` and returns wall-clock time as seconds, including fractional microseconds:

```c
return (double)t.tv_sec + (double)t.tv_usec / 1000000.0;
```

It can be used for simple elapsed-time demonstrations. For robust elapsed-time measurements, a monotonic clock such as `clock_gettime(CLOCK_MONOTONIC, ...)` is generally preferable because wall-clock time can change.

### `Spin(int howlong)`

In the version used here, `Spin()` busy-waits for approximately `howlong` **seconds**. It repeatedly checks the clock rather than sleeping, so it consumes CPU while waiting. This helper is not needed by the counter loop unless a program explicitly calls it.

### Pthread wrappers

The wrappers call the corresponding POSIX API and check its returned error code. POSIX pthread functions generally return an error number directly rather than setting `errno` in the usual system-call style.

```c
Pthread_create(&p1, NULL, worker, NULL);
Pthread_join(p1, NULL);
Pthread_mutex_init(&m, NULL);
Pthread_mutex_lock(&m);
Pthread_mutex_unlock(&m);
```

The fourth argument to `Pthread_create` is passed to the worker's `void *arg` parameter. These demos pass `NULL` because the worker uses the global `loops` variable instead of receiving per-thread data.

## Measure runtime with `time`

The shell's `time` keyword reports elapsed time and CPU time:

```bash
time ./threads 100000
time ./lock_threads 100000
```

Typical output format:

```text
Initial value : 0
Final value   : 200000

real    0m0.120s
user    0m0.150s
sys     0m0.090s
```

These numbers are illustrative only. Actual timings depend on the computer, WSL configuration, scheduler, system load, compiler settings, and run-to-run variation.

- `real`: elapsed wall-clock time from start to finish.
- `user`: CPU time spent executing user-space code.
- `sys`: CPU time spent executing kernel code on behalf of the process, such as parts of thread synchronization.

`user + sys` can exceed `real` when multiple threads execute on different CPU cores. Measured values vary with scheduling and system activity.

### Compare performance carefully

Try a larger count:

```bash
time ./threads 1000000
time ./lock_threads 1000000
```

The mutex version has lock/unlock overhead on every increment. However, a single timing is not a definitive benchmark. The unsynchronized version is incorrect due to its data race, so this is an illustrative comparison rather than a fair comparison between two correct implementations. Run multiple trials and compare results cautiously.

## Common problems

### `fatal error: pthread.h: No such file or directory`

Build the code in Ubuntu/WSL with a suitable GCC toolchain. A typical Git Bash/MinGW installation does not provide the POSIX threading headers expected by this example.

### `redefinition of GetTime` or `redefinition of Spin`

The helper was pasted into `common.h` twice. Keep only one definition of each function.

### `unused parameter 'arg'`

This is a warning when the worker does not use its argument. Add `(void)arg;` in the worker to explicitly mark it as intentionally unused.

### The compiler prints a warning

A warning is not necessarily a compilation failure. If there is no `error:` message and GCC exits successfully, it may still have created the executable. Address warnings rather than ignoring them.

### The final counter from `threads` happens to be correct

That does not prove the program is thread-safe. Data races have undefined behavior, and their visible effects depend on timing, compiler optimizations, and the environment.

## Git hygiene

From the repository root, stage source and documentation, not compiled binaries. A suitable `.gitignore` entry for these generated executables is:

```gitignore
os-1/03-concurrency-demo/instructor-demo/threads
os-1/03-concurrency-demo/instructor-demo/lock_threads
```

Then inspect the changes before committing:

```bash
git status
git add os-1/03-concurrency-demo/instructor-demo/common.h \
        os-1/03-concurrency-demo/instructor-demo/threads.c \
        os-1/03-concurrency-demo/instructor-demo/lock_threads.c \
        os-1/03-concurrency-demo/instructor-demo/README.md
git diff --cached
git commit -m "Document instructor thread and mutex demos"
git push
```

Run these Git commands from the repository root. Adjust paths if your repository layout differs.

## Key takeaways

1. Threads in the same process share global variables and other process memory.
2. `counter++` is not inherently atomic.
3. `volatile` is not a synchronization mechanism.
4. A data race on a non-atomic object in C causes undefined behavior.
5. A mutex protects a critical section when all relevant accesses follow the same locking protocol.
6. `pthread_join()` ensures the main thread waits for workers to finish.
7. `argc` counts command-line arguments including the program name; `argv` contains those arguments as strings.
8. `time` helps compare elapsed and CPU time, but benchmark results need careful interpretation.
9. Use Ubuntu/WSL for these POSIX thread examples, and keep source code separate from generated executables.

## Experiments

1. Run `./threads 100000` several times. Record the final counter, but remember that a correct-looking result does not remove the data race.
2. Run `./lock_threads 100000` and verify that the final counter is `200000`.
3. Increase the argument to `1000000` and compare several `time` runs.
4. Temporarily remove the mutex lock/unlock calls from `lock_threads.c`, rebuild, and observe what changes. Restore the mutex calls afterward.
5. Read the `Pthread_create` wrapper and trace how `NULL` becomes the worker's `arg` parameter.
6. Use GDB with the `-g` build to set breakpoints in `worker()` and observe thread execution.

These experiments connect the code to scheduling, shared memory, mutual exclusion, race conditions, and synchronization overhead.
