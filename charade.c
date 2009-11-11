/* charade.c
 *
 * ssh-agent clone for cygwin to proxy all ssh-ish requests to pageant.
 *
 * Copyright (c) 2009, Wesley Darlington. All Rights Reserved.
 */

#include <stdlib.h>

#include "cmdline.h"

int sock;


void
create_socket(void)
{
    if (agentsocket == NULL) {
        /* Create private directory for agent socket */
        strlcpy(socket_dir, "/tmp/ssh-XXXXXXXXXX", sizeof socket_dir);
        if (mkdtemp(socket_dir) == NULL) {
            perror("mkdtemp: private socket dir");
            exit(1);
        }
        snprintf(socket_name, sizeof socket_name, "%s/agent.%ld", socket_dir,
            (long)parent_pid);
    } else {
        /* Try to use specified agent socket */
        socket_dir[0] = '\0';
        strlcpy(socket_name, agentsocket, sizeof socket_name);
    }


    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_un sunaddr;
    memset(&sunaddr, 0, sizeof(sunaddr));
    sunaddr.sun_family = AF_UNIX;
    strlcpy(sunaddr.sun_path, socket_name, sizeof(sunaddr.sun_path));
    prev_mask = umask(0177);
    if (bind(sock, (struct sockaddr *) &sunaddr, sizeof(sunaddr)) < 0) {
        perror("bind");
        *socket_name = '\0'; /* Don't unlink any existing file */
        umask(prev_mask);
            cleanup_exit(1);
    }
    umask(prev_mask);
    if (listen(sock, SSH_LISTEN_BACKLOG) < 0) {
        perror("listen");
        cleanup_exit(1);
    }
}

void
loop(void)
{

}

void
kill_old_agent(void)
{
}

void
fork_subprocess(void)
{
}

void
go(void)
{
}

int
main(int argc, char **argv)
{
    /* Ensure that fds 0, 1 and 2 are open or directed to /dev/null */
    // TODO: sanitise_stdfd();

    parse_cmdline(argc, argv);

    if (g_kill_flag) {
        kill_old_agent();
        exit(0);
    }

    create_socket();

    if (! g_debug_flag) {
        fork_subprocess();
    }

    go();

    /* NOTREACHED */

    exit(0);
}
