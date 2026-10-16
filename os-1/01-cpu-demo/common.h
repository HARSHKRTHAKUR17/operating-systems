#ifndef COMMON_H
#define COMMON_H

#include <assert.h>
#include <sys/time.h>

double GetTime(void)
{
    struct timeval t;
    int rc = gettimeofday(&t, NULL);
    assert(rc == 0);

    return (double)t.tv_sec + (double)t.tv_usec / 1e6;
}

void Spin(int howlong)
{
    double t = GetTime();

    while ((GetTime() - t) < (double)howlong)
        ; // Busy wait
}

#endif

// Record the starting time in t.
// Keep checking the current time.
// Subtract the starting time from the current time.
// Exit only when the elapsed time reaches howlong.

/* These include guards prevent the header 
from being processed multiple times in a single 
compilation unit. This example defines the functions 
directly in the header, which is acceptable for this 
small, single-file demonstration. */

