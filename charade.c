/* charade.c
 *
 * ssh-agent clone for cygwin to proxy all ssh-ish requests to pageant.
 *
 * Copyright (c) 2009, Wesley Darlington. All Rights Reserved.
 */

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "cmdline.h"

#define LISTEN_BACKLOG 5

#define SSH_AUTHSOCKET_ENV_NAME "SSH_AUTH_SOCK"
#define SSH_AGENTPID_ENV_NAME "SSH_AGENT_PID"

int sock;

char socket_dir[MAXPATHLEN] = "";
char socket_name[MAXPATHLEN] = "";

int remove_socket_at_exit = 1;

void
remove_socket_dir(void)  /* atexit handler */
{
    if (remove_socket_at_exit) {
        int ret = rmdir(socket_dir);

        if (ret) {
            fprintf(stderr, "Error removing socket directory '%s': %s.\n",
                    socket_dir, strerror(errno));
            /* atexit! */
        }
    }
}

void
remove_socket(void)  /* atexit handler */
{
    if (remove_socket_at_exit) {
        int ret = unlink(socket_name);

        if (ret) {
            fprintf(stderr, "Error removing socket '%s': %s.\n",
                    socket_name, strerror(errno));
            /* atexit! */
        }
    }
}


void
create_socket(void)
{
#if 0
    if (agentsocket == NULL) {
#endif

    /* Create private directory for agent socket */
    strlcpy(socket_dir, "/tmp/ssh-XXXXXXXXXX", sizeof socket_dir);
    if (mkdtemp(socket_dir) == NULL) {
        perror("mkdtemp: private socket dir");
        exit(1);
    }

    if (atexit(remove_socket_dir)) {
        fprintf(stderr, "Can't install atexit handler to delete socket '%s'. "
                        "Do it yourself!\n", socket_name);
        exit(1);
    }

    int ret = snprintf(socket_name, sizeof(socket_name), 
                       "%s/agent.%ld", socket_dir, (long)getpid());
    if (ret >= sizeof(socket_name)) {
        // Would have liked to print more...
        fprintf(stderr, "socket_name too long (%d >= %d).\n",
                ret, sizeof(socket_name));
        exit(1);
    }


#if 0
    } else {
        /* Try to use specified agent socket */
        socket_dir[0] = '\0';
        strlcpy(socket_name, agentsocket, sizeof socket_name);
    }
#endif


    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_un sunaddr;
    memset(&sunaddr, 0, sizeof(sunaddr));
    sunaddr.sun_family = AF_UNIX;
    strlcpy(sunaddr.sun_path, socket_name, sizeof(sunaddr.sun_path));
    int prev_mask = umask(0177);
    if (bind(sock, (struct sockaddr *) &sunaddr, sizeof(sunaddr)) < 0) {
        perror("bind");
        umask(prev_mask);
        exit(1);
    }

    if (atexit(remove_socket)) {
        fprintf(stderr, "Can't install atexit handler to delete socket '%s'. "
                        "Do it yourself!\n", socket_name);
        exit(1);
    }

    umask(prev_mask);
    if (listen(sock, LISTEN_BACKLOG) < 0) {
        perror("listen");
        exit(1);
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
    int pid = fork();

    if (-1 == pid) {
        perror("fork");
        exit(1);
    }

    if (pid) {  // Parent
        printf("%s=%s; export %s\n", SSH_AUTHSOCKET_ENV_NAME, socket_name,
                                   SSH_AUTHSOCKET_ENV_NAME);
        printf("%s=%ld; export %s\n", SSH_AGENTPID_ENV_NAME, (long) pid,
                                   SSH_AGENTPID_ENV_NAME);

        // TODO: If argv present, fork and exec it. Only do above if no args.

        remove_socket_at_exit = 0;
        exit(0);
    }

    // Child
    if (setsid() == -1) {
        perror("setsid");
    }
}

void
handle_key_requests_forever(void)
{
    // Select on sockets, etc etc
}

int
main(int argc, char **argv)
{
    /* Ensure that fds 0, 1 and 2 are open or directed to /dev/null */
    // TODO: sanitise_stdfd();

#if 0
    parse_cmdline(argc, argv);

    if (g_kill_flag) {
        kill_old_agent();
        exit(0);
    }
#endif

    create_socket();

#if 0
    if (! g_debug_flag) {
    }
#endif

    fork_subprocess();

    handle_key_requests_forever();

    /* NOTREACHED */

    exit(0);
}
