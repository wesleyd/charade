/* charade.c
 *
 * Copyright (c) 2009, Wesley Darlington. All Rights Reserved.
 */

#include <stdlib.h>

#include "cmdline.h"

int
main(int argc, char **argv)
{
    /* Ensure that fds 0, 1 and 2 are open or directed to /dev/null */
    // TODO: sanitise_stdfd();

    parse_cmdline(&argc, &argv);

    exit(0);
}
