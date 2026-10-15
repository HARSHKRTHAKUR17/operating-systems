#include <stdio.h>
#include <stdlib.h>
#include "common.h"

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "usage: cpu <string>\n");
        exit(1);
    }

    char *str = argv[1];

    while (1) {
        printf("%s\n", str);
        Spin(1);
    }

    return 0;
}

// argc is the number of command-line arguments, including the program name.
// argv[1] is the string passed by the user.
// while (1) runs indefinitely.
// printf() prints the string.
// Spin(1) wastes approximately one second of CPU time by repeatedly checking the clock.