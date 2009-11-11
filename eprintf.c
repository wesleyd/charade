/* eprintf.c
 *
 * Definitions of various clean output functions
 */

#include <stdarg.h>
#include <stdio.h>

#include "eprintf.h"

unsigned int g_volume = 1;

void
louder(void)
{
    ++g_volume;
}

int
eprintf(int level, const char *fmt, ...)
{
    if (level > g_volume)
        return 0;

    va_list ap;
    va_start(ap, fmt);

    int rv = vfprintf(stderr, fmt, ap);
    // fflush(stderr);

    va_end(ap);

    return rv;
}
