/* cmdline.c
 *
 * Parse command line arguments for charade.
 */

#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "copyright.h"
#include "cmdline.h"
#include "eprintf.h"

int g_csh_flag = 0;
int g_sh_flag = 0;
int g_kill_flag = 0;
int g_dontfork_flag = 0;
char *g_socket_name = 0;

int g_subprocess_argc;
char **g_subprocess_argv;

static void
usage(void)
{
    EPRINTF(0, "usage: charade [options] [command [arg ...]]\n");
    EPRINTF(0, "Options:\n");
    EPRINTF(0, "  -c          Generate C-shell commands on stdout.\n");
    EPRINTF(0, "  -s          Generate Bourne shell commands on stdout.\n");
    EPRINTF(0, "  -k          Kill the current agent.\n");
    EPRINTF(0, "  -d          Don't fork.\n");
    EPRINTF(0, "  -v [-v ...] More trace output.\n");
    EPRINTF(0, "  -a socket   Bind agent socket to given name.\n");
    EPRINTF(0, "  --copyright Print out copyright message & licence.\n");
    exit(1);
}

static struct option opts[] = {
    {"copyright", 0, NULL, 'C'},
    {NULL, 0, NULL, 0}
};

void
parse_cmdline(int argc, char **argv)
{
    int ch;

    /* I seem to get link errors when I declare these getopt variables.
       Things seem to work fine when I don't declare them, so I'm
       just commenting them out!
    extern int optind, opterr;
    extern char *optarg;
    */

    while (-1 != (ch = getopt_long(argc, argv, "cskdva:", opts, NULL))) {
        switch (ch) {
            case 'c': ++g_csh_flag;
                      break;
            case 's': ++g_sh_flag;
                      break;
            case 'k': ++g_kill_flag;
                      break;
            case 'd': g_dontfork_flag = 1;
                      break;
            case 'v': louder();
                      break;
            case 'a': g_socket_name = optarg;
                      break;
            case 'C': print_copyright();
                      break;
            default: usage();
        }
    }
    argc -= optind;
    argv += optind;

    // If any flags given, there better not be process-to-run-args...
    if (argc > 0 && (g_csh_flag || g_sh_flag || g_kill_flag)) {
        EPRINTF(1, "Can't produce shell commands *and* run subprocess!\n");
        usage();
    }

    // If there's no shell and no process-to-run-args, assume C shell...
    if (argc == 0 && !g_csh_flag && !g_sh_flag) {
        char *shell = getenv("SHELL");
        size_t len;
        if (shell != NULL && (len = strlen(shell)) > 2
                          && strncmp(shell + len - 3, "csh", 3) == 0)
            g_csh_flag = 1;
    }

    if (argc > 0 && g_dontfork_flag) {
        EPRINTF(1, "Can't not fork *and* run a subprocess!\n");
        usage();
    }

    g_subprocess_argc = argc;
    g_subprocess_argv = argv;
}
