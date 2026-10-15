# Memory Demo: Virtual Addresses, Processes, and ASLR

## Objective

This experiment explores how separate processes use virtual addresses, why two processes can print the same address without sharing memory, and how Address Space Layout Randomization (ASLR) affects memory layouts.

The experiment was inspired by a CS 537 operating-systems demonstration.

## Program used

`mem.c`:

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

volatile int value;

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <initial-value>\\n", argv[0]);
        return EXIT_FAILURE;
    }

    volatile int *p = &value;
    *p = atoi(argv[1]);

    printf("PID: %ld\\n", (long)getpid());
    printf("Address stored in p: %p\\n", (void *)p);
    fflush(stdout);

    while (1) {
        sleep(1);
        *p = *p + 1;
        printf("PID: %ld | value of p: %d\\n", (long)getpid(), *p);
        fflush(stdout);
    }

    return EXIT_SUCCESS;
}
```

### What the code does

- `argc` is the number of command-line arguments, including the executable name.
- `argv[1]` is the initial value supplied on the command line.
- `atoi(argv[1])` converts that string to an integer.
- `volatile int value` is a global variable.
- `volatile int *p` is a pointer to a volatile integer; `&value` obtains the address of `value`.
- `*p` dereferences the pointer, accessing the integer at that address.
- `getpid()` returns the process ID (PID).
- The loop waits for about one second using `sleep(1)`, increments the value, and prints it.
- `fflush(stdout)` makes each line appear promptly, even when output is redirected.

`volatile` tells the compiler that accesses to the object must be performed as written rather than treating its value as an ordinary value that never changes unexpectedly. It is **not** a synchronization mechanism and does not make concurrent accesses safe.

This version uses `sleep(1)` rather than the original `Spin(1)` busy-wait helper. `sleep(1)` normally yields the CPU while waiting; a busy-wait loop repeatedly checks the time and can consume CPU. This is a useful contrast when observing processes with `top`.

## Build and run in Ubuntu / WSL

Use an Ubuntu (WSL) terminal, not Git Bash, for this Linux program.

From the project directory:

```bash
gcc -Wall -Wextra -Wpedantic -g -o mem mem.c
```

Compiler flags:

- `-Wall`: enables many useful warnings.
- `-Wextra`: enables additional warnings.
- `-Wpedantic`: warns about certain non-standard language constructs.
- `-g`: includes debugging information for GDB; it is not required just to run the program.
- `-o mem`: names the executable `mem`.

Run one instance:

```bash
./mem 0
```

Run another instance in a second Ubuntu terminal:

```bash
./mem 100
```

Stop each with **Ctrl+C**.

Use `./mem`, not just `mem`, because Linux generally does not search the current directory for commands unless it is explicitly included in `PATH`.

To observe CPU usage from another Ubuntu terminal:

```bash
top -o %CPU
```

This program sleeps between increments, so it should generally consume much less CPU than a version that continuously busy-waits.

## Virtual addresses and separate processes

Each process normally has its own virtual address space. A pointer printed by `%p` is a virtual address in the process's address space; it is not a physical RAM address.

Two separate processes can print the same virtual address while that address maps to different physical memory. For example:

```text
Process A: virtual address X -> physical page A
Process B: virtual address X -> physical page B
```

The physical addresses above are conceptual examples, not values printed by the program. The operating system configures page tables, and the processor's memory-management unit (MMU) uses them to translate virtual addresses.

Therefore:

- Same virtual address does **not** imply shared physical memory.
- If two processes deliberately map the same physical memory through shared memory, changes can be visible to both.
- Concurrent updates to shared data may require synchronization; otherwise, race conditions can occur.

## ASLR: Address Space Layout Randomization

ASLR is an operating-system security feature that randomizes parts of a process's virtual address layout, making memory locations less predictable.

With ASLR enabled, addresses may differ across executions. With ASLR disabled, the same executable can often use repeatable addresses across executions, provided relevant layout conditions remain the same.

Disabling ASLR does **not** make processes share memory. It affects address placement, not whether their memory is shared.

The program's code determines which address is printed—in this case, the address of `value`. ASLR and executable layout influence whether that address changes between executions.

### Global versus local variables

A global variable such as:

```c
int value = 10;

int main(void) {
    printf("%p\\n", (void *)&value);
}
```

typically resides in the program's data segment (`.data` for initialized data, `.bss` for zero-initialized or uninitialized global data).

A local variable such as:

```c
int main(void) {
    int value = 10;
    printf("%p\\n", (void *)&value);
}
```

typically has storage on the stack when it needs a memory location. A compiler may instead keep a local in a register or optimize it away.

Both global and local variables can have the same virtual address across separate executions when ASLR is disabled. For a global, the address is related to the executable's data layout and load address. For a local, it is commonly related to the stack location and the variable's offset in a stack frame. Repetition is possible, not guaranteed: PIE, compiler optimizations, environment changes, and other layout factors can matter.

A variable's **scope**, **lifetime**, and **address** are different concepts:

- Scope: where the variable can be named in source code.
- Lifetime: when the object exists.
- Address: where the object is located in a process's virtual address space.

A local variable's lifetime ending does not prevent a later process from reusing the same virtual address.

## GDB and disabling ASLR

The `-g` compiler option adds debug information that helps GDB associate machine instructions with source lines and variables. It does not disable ASLR.

On a Linux system where the operation is permitted, GDB can request that ASLR be disabled for programs it launches:

```gdb
set disable-randomization on
run 0
```

To request normal ASLR behavior instead:

```gdb
set disable-randomization off
run 0
```

GDB may report that disabling randomization is not permitted or unsupported, especially in constrained environments such as some WSL configurations. If that happens, the requested experiment did not successfully disable ASLR. The program can still run, but address comparisons may not show the expected repeatability.

The setting applies to debugged processes launched by GDB; it is not the same as permanently changing the system-wide ASLR configuration.

### Platform note: Ubuntu/WSL versus Git Bash

This experiment should be compiled and debugged in Ubuntu/WSL using Linux GCC and GDB. Git Bash is a Windows shell, and MinGW GDB generally expects Windows executables. A Linux ELF executable produced by Ubuntu GCC is not a Windows executable, so Git Bash's GDB may report that it is not in executable format. Use the Ubuntu terminal for Linux system-programming experiments.

## Key takeaways

1. A pointer stores an address; dereferencing it accesses the object at that address.
2. Each process normally has a separate virtual address space.
3. Two processes can use the same virtual address and still access different physical memory.
4. Shared physical memory can make updates visible across processes; unsynchronized concurrent updates can race.
5. ASLR randomizes memory layout to improve security. Disabling it can make addresses repeat, but does not create shared memory.
6. Global and local variables differ in typical storage, scope, and lifetime, but either can have a repeatable virtual address across executions.
7. `-g` adds debugging information; it does not affect ASLR.
8. Use Ubuntu/WSL—not Git Bash—to compile and debug this Linux program.
9. `sleep(1)` gives up the CPU while waiting; a busy-wait loop keeps executing instructions and can consume CPU.

## Suggested experiments

- Run two instances with different initial values and compare PIDs, addresses, and counters.
- Compare address output across multiple executions with ASLR enabled and, where permitted, disabled through GDB.
- Change `value` from a global to a local variable and compare results. Ensure the local variable's address is actually taken and printed.
- Compare `sleep(1)` with a busy-wait implementation and observe both using `top -o %CPU`.
- Explain why identical virtual addresses alone do not prove that memory is shared.
