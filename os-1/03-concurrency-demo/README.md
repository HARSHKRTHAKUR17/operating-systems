# CS 537 Concurrency: Threads, Shared Memory, Race Conditions, Atomics, and Mutexes

This folder contains the instructor-style thread counter demo and small follow-up experiments. The goal is to understand **how threads share memory, why unsynchronized operations can produce incorrect results, and how synchronization primitives help protect shared state**.

## Folder layout

```text
concurrency/
├── README.md
├── instructor-demo/
│   ├── threads.c
│   ├── common.h
│   └── threads                 # generated executable; do not commit
└── use-cases/
    ├── atomic-counter/
    │   ├── atomic_counter.c
    │   └── atomic_counter      # generated executable; do not commit
    └── bank-transfer/
        ├── transfer_unsafe.c
        ├── transfer_mutex.c
        ├── transfer_unsafe    # generated executable; do not commit
        └── transfer_mutex     # generated executable; do not commit
```

The executable files are created when you compile the programs. Keep source code and documentation in Git; generally do not commit generated binaries.

---

## 1. Concepts

### Process vs. thread

A **process** is a running program with its own virtual address space and operating-system resources. Threads are execution paths within a process. Threads in the same process share its global memory and heap, while each thread has its own stack and execution state.

This shared memory makes communication between threads efficient, but it also means threads can interfere with one another if they access shared data without proper coordination.

### Shared global variable

The instructor demo declares:

```c
volatile int counter = 0;
int loops;
```

`counter` is a global variable, so both worker threads in this process access the same object. `loops` is also global and is initialized before the threads are created; both workers read it.

Unlike separate processes, which normally have separate virtual address spaces, threads in one process share the same address space. Therefore, both threads refer to the same `counter` object.

### Thread creation and joining

```c
pthread_create(&p1, NULL, worker, NULL);
pthread_create(&p2, NULL, worker, NULL);

pthread_join(p1, NULL);
pthread_join(p2, NULL);
```

- `pthread_create()` starts a new thread that invokes the supplied start routine.
- The first argument receives the new thread's ID.
- `NULL` for the attributes requests default thread attributes.
- `worker` is the function each thread executes.
- The final `NULL` is the argument passed to `worker`.
- `pthread_join()` waits for the specified thread to finish.

The instructor's `Pthread_create()` and `Pthread_join()` names are wrappers around these POSIX functions. The `common.h` in this repository provides compatible wrappers that report an error and exit if the underlying call fails.

### What is a race condition?

A **race condition** occurs when program behavior depends on the timing or interleaving of concurrent operations.

In the instructor demo, both threads execute:

```c
counter++;
```

It is tempting to think this is one indivisible operation. Conceptually, an increment involves a read, an addition, and a write:

1. Read the current counter.
2. Add one.
3. Store the result.

For example, if `counter` is `5`:

| Step | Thread 1 | Thread 2 |
|---|---|---|
| 1 | Reads `5` | |
| 2 | | Reads `5` |
| 3 | Computes `6` | Computes `6` |
| 4 | Writes `6` | |
| 5 | | Writes `6` |

The counter ends at `6`, even though two increments were attempted. One update is lost.

The CPU is not intentionally skipping increments. Conflicting read-modify-write sequences can overwrite one another.

### Why the scheduler matters—but is not the root cause

The operating-system scheduler decides when runnable threads execute. On a single-core CPU, the scheduler can switch between threads between instructions. On a multicore CPU, threads may execute simultaneously on different cores.

The race is possible on both single-core and multicore systems. The underlying problem is the lack of synchronization around shared state; scheduling and timing influence which interleavings occur.

Small loop counts can appear correct simply because a problematic interleaving did not affect the observed result. That does **not** make the program safe.

### Why `volatile` does not solve the race

The instructor example uses:

```c
volatile int counter = 0;
```

In C, `volatile` affects how accesses to an object are treated by the compiler. It does **not** make `counter++` atomic and does not provide thread synchronization.

Concurrent unsynchronized accesses to this ordinary non-atomic variable, when at least one access writes, constitute a **data race and undefined behavior** in C. The exact result is not guaranteed; lost increments are one possible manifestation.

Use C atomics or mutexes for thread synchronization, not `volatile`.

---

## 2. Instructor demo: `instructor-demo/threads.c`

### Purpose

This is the main course-style example. It creates two threads, and each increments the shared counter `loops` times.

The expected mathematical result is:

```text
final counter = 2 * loops
```

For example, `./threads 100000` represents 100,000 increments from each of two threads, for 200,000 attempted increments in total. Because the code has a data race, the final result is not guaranteed to equal 200,000.

The source reproduces the core code shown in the lecture screenshot. It uses the standard `pthread_create()`/`pthread_join()` wrappers provided in `common.h`, rather than copying the instructor's complete original helper library byte-for-byte.

### `instructor-demo/common.h`

This header provides:

- `Pthread_create()` and `Pthread_join()` wrappers around POSIX thread functions.
- `GetTime()` and `Spin()` helpers used in the earlier CPU/memory demonstration.

`Spin()` is a busy-wait helper: it repeatedly checks elapsed time and can consume CPU while waiting. It is included for continuity with the earlier course example; the thread counter program does not call it.

A header's function declarations tell the compiler how a function is used. Header-defined helper functions in this small demo are marked `static inline` to avoid multiple-definition problems if included by more than one source file.

### Build and run

From the `concurrency/` directory in Ubuntu/WSL:

```bash
gcc -Wall -Wextra -Wpedantic -g -pthread \
    instructor-demo/threads.c -o instructor-demo/threads

./instructor-demo/threads 10
./instructor-demo/threads 100
./instructor-demo/threads 1000
./instructor-demo/threads 10000
./instructor-demo/threads 100000
```

The results may vary. A small run might show the expected count, while a larger run might show fewer increments. Even if a run is correct, the data race remains.

---

## 3. Atomic counter: `use-cases/atomic-counter/atomic_counter.c`

### Motivation

The instructor example demonstrates a shared counter that is updated unsafely. A counter is a simple case where an atomic operation is a good fit.

The extension uses:

```c
atomic_fetch_add(&counter, 1);
```

`atomic_fetch_add()` performs the increment atomically. Concurrent increments of this atomic object are not lost through the read-modify-write interleaving shown above.

The program still creates two threads and has each perform `loops` increments. Assuming both threads complete successfully and no other code modifies the counter, the final count is `2 * loops`.

### Build and run

```bash
gcc -Wall -Wextra -Wpedantic -g -pthread \
    use-cases/atomic-counter/atomic_counter.c \
    -o use-cases/atomic-counter/atomic_counter

./use-cases/atomic-counter/atomic_counter 100000
```

Expected output includes:

```text
Expected value: 200000
Final value:   200000
```

The exact formatting may differ slightly.

### What atomics do—and do not do

Atomics are useful for simple shared-state operations such as counters, flags, and reference counts. They do not automatically make a multi-step sequence involving several variables indivisible. More complex invariants may require a mutex or a carefully designed atomic algorithm.

---

## 4. Bank transfer without a mutex: `use-cases/bank-transfer/transfer_unsafe.c`

### Motivation

Suppose Account A has 500 and Account B has 300. A transfer moves 100 from A to B:

```text
Initial: A = 500, B = 300, total = 800
Debit:   A = 400, B = 300, total = 700
Credit:  A = 400, B = 400, total = 800
```

The intermediate total of 700 is not necessarily a lost amount; it is the state between the debit and credit. But a concurrent observer should not see that intermediate state if the application requires a transfer to appear as one indivisible operation.

The unsafe example uses **atomic account balances** but performs the debit and credit as separate atomic operations. A coordination variable makes the intermediate state observable for demonstration. This is deliberate: the example reliably shows why individual atomic updates do not make a complete transfer atomic.

### Build and run

```bash
gcc -Wall -Wextra -Wpedantic -g -pthread \
    use-cases/bank-transfer/transfer_unsafe.c \
    -o use-cases/bank-transfer/transfer_unsafe

./use-cases/bank-transfer/transfer_unsafe
```

The program should show an intermediate total of 700 and a final total of 800.

The busy-wait loops in this small teaching example intentionally spin. They are not recommended as a general synchronization design.

---

## 5. Bank transfer with a mutex: `use-cases/bank-transfer/transfer_mutex.c`

### Motivation

A **mutex** (mutual-exclusion lock) lets one thread at a time enter a protected critical section. In this example, the writer holds the mutex across both the debit and credit, and the reader uses the same mutex to take its snapshot.

```c
pthread_mutex_lock(&lock);

account_a -= 100;
account_b += 100;

pthread_mutex_unlock(&lock);
```

This prevents a reader that follows the same locking protocol from observing the transfer halfway through.

The example also uses a condition variable:

- `pthread_cond_wait()` waits for a condition while atomically releasing the mutex during the wait.
- When awakened, the thread reacquires the mutex before continuing.
- The condition is checked in a `while` loop because a thread must re-check the condition after waking.

### Build and run

```bash
gcc -Wall -Wextra -Wpedantic -g -pthread \
    use-cases/bank-transfer/transfer_mutex.c \
    -o use-cases/bank-transfer/transfer_mutex

./use-cases/bank-transfer/transfer_mutex
```

The reader should observe the completed transfer: A = 400, B = 400, total = 800.

**Important:** Locking only the writer is not enough if readers access the balances without acquiring the same lock. Every access that must respect the protected invariant needs to follow the synchronization protocol.

---

## 6. Atomic operations vs. mutexes

| Question | Atomic operation | Mutex |
|---|---|---|
| Protects a simple counter increment? | Yes, if the increment uses an atomic operation | Yes, if every access follows the lock |
| Makes multiple separate updates one indivisible transaction? | Not automatically | Yes, when all relevant readers/writers use the same lock |
| Good for | Counters, flags, reference counts, simple state transitions | Multi-step critical sections and invariants across several fields |
| Does it eliminate every concurrency bug? | No | No; lock misuse and deadlocks are still possible |

A mutex is not always required for every atomic variable. Choose the mechanism that correctly protects the required state and makes the program easiest to reason about.

### The check-then-act example

Even if `counter` is atomic, this sequence is not automatically atomic as a whole:

```c
if (atomic_load(&counter) == 0) {
    atomic_fetch_add(&counter, 1);
}
```

Two threads can both load zero before either increments, then both increment the counter. Each individual operation is atomic, but the combined check-and-update is not.

A compare-and-exchange operation can perform a conditional update atomically, for example changing a value from 0 to 1 only if it is still 0. A mutex is often simpler when several operations or variables must be coordinated.

---

## 7. Build flags

The examples use flags such as:

```bash
gcc -Wall -Wextra -Wpedantic -g -pthread source.c -o program
```

- `-Wall`: enables many useful compiler warnings.
- `-Wextra`: enables additional warnings.
- `-Wpedantic`: warns about certain non-standard language constructs.
- `-g`: includes debugging information for GDB; not required just to run the executable.
- `-pthread`: enables appropriate pthread compilation and linking support.
- `-o program`: names the output executable.

If compilation succeeds without warnings, GCC often prints nothing.

## 8. Ubuntu/WSL vs. Git Bash

Use **Ubuntu/WSL** to compile and run these POSIX thread examples. They use Linux/POSIX APIs such as pthreads.

Git Bash is a Windows shell, and its MinGW toolchain generally builds Windows executables. A Linux executable built in WSL is not a Windows executable, so Git Bash's GDB may report that it cannot recognize the executable format.

From VS Code, select the Ubuntu (WSL) terminal for compiling and debugging this work. Git Bash is fine for Git commands.

## 9. Git workflow

From Git Bash, at the repository root:

```bash
git add os-1/concurrency/
git status
git commit -m "Add CS 537 concurrency demonstrations"
git push
```

Run `git status` before committing to confirm that the intended source and README files are staged. Avoid committing generated executables.

## 10. Key takeaways

1. Threads within a process share global variables and heap memory.
2. A race condition arises when behavior depends on unsynchronized concurrent operations.
3. `counter++` is not guaranteed to be atomic.
4. The scheduler influences interleavings, but the lack of synchronization is the core problem.
5. `volatile` is not a thread-synchronization primitive.
6. A proper atomic increment protects a simple counter from lost updates.
7. Individually atomic operations do not automatically make a multi-step transaction atomic.
8. A mutex can protect a complete critical section when readers and writers follow the same locking protocol.
9. Condition variables let threads wait for a condition without repeatedly polling it.
10. Small tests that happen to return the expected result do not prove that concurrent code is correct.

## Suggested follow-up experiments

- Run the instructor counter with several loop counts and record the observed final values. Do not expect a particular incorrect value.
- Compare the instructor demo against the atomic counter with the same loop count.
- Compare the unsafe and mutex bank-transfer examples.
- Use GDB to set breakpoints in `worker()` and step through the program. Be aware that multithreaded execution can interleave between steps.
- Explain in your own words why atomicity of each balance update does not make the whole transfer indivisible.
