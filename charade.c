/* charade.c
 *
 * ssh-agent clone for cygwin to proxy all ssh-ish requests to pageant.
 *
 * Copyright (c) 2009, Wesley Darlington. All Rights Reserved.
 */

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <windows.h>

#include "cmdline.h"

#define LISTEN_BACKLOG 5
#define BUFSIZE 8192
#define AGENT_MAX_MSGLEN 8192

// TODO: is there actually something magical about this number?
#define AGENT_COPYDATA_ID 0x804e50ba 

#define SSH_AUTHSOCKET_ENV_NAME "SSH_AUTH_SOCK"
#define SSH_AGENTPID_ENV_NAME "SSH_AGENT_PID"


#define GET_32BIT_MSB_FIRST(cp) \
    (((unsigned long)(unsigned char)(cp)[0] << 24) | \
    ((unsigned long)(unsigned char)(cp)[1] << 16) | \
    ((unsigned long)(unsigned char)(cp)[2] << 8) | \
    ((unsigned long)(unsigned char)(cp)[3]))

#define GET_32BIT(cp) GET_32BIT_MSB_FIRST(cp)



typedef unsigned char byte;

int listen_sock;
struct socklist_node_t {
    TAILQ_ENTRY(socklist_node_t) next;
    int fd;
    byte *data;
};
TAILQ_HEAD(socklist_queue, socklist_node_t) socklist;

char socket_dir[MAXPATHLEN] = "";
char socket_name[MAXPATHLEN] = "";

int remove_socket_at_exit = 1;

void
init_socket_list(void)
{
    TAILQ_INIT(&socklist);
}

void
add_socket_to_socket_list(int sock)
{
    struct socklist_node_t *p = calloc(sizeof(struct socklist_node_t), 1);

    p->fd = sock;
    p->data = NULL;

    fprintf(stderr, "Adding socket %d to socket list.\n", sock);

    TAILQ_INSERT_TAIL(&socklist, p, next);
}

int
num_sockets_in_list(void)
{
    struct socklist_node_t *p;
    int count = 0;

    for (p = socklist.tqh_first; p != NULL; p = p->next.tqe_next) {
        ++count;
    }

    fprintf(stderr, "Log: there are %d sockets in the list.\n", count);

    return count;
}

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


    listen_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listen_sock < 0) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_un sunaddr;
    memset(&sunaddr, 0, sizeof(sunaddr));
    sunaddr.sun_family = AF_UNIX;
    strlcpy(sunaddr.sun_path, socket_name, sizeof(sunaddr.sun_path));
    int prev_mask = umask(0177);
    if (bind(listen_sock, (struct sockaddr *) &sunaddr, sizeof(sunaddr)) < 0) {
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
    if (listen(listen_sock, LISTEN_BACKLOG) < 0) {
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

int
make_poll_fds(struct pollfd **fds)
{
    int nfds = 1                       // The listening socket...
             + num_sockets_in_list();  // ...plus all the others.

    struct pollfd *pollfds = calloc(sizeof(struct pollfd), nfds);

    if (!pollfds) {
        fprintf(stderr, "Can't calloc struct pollfd's for poll().\n");
        exit(1);
    }

    pollfds[0].fd = listen_sock;
    pollfds[0].events = POLLIN | POLLHUP;

    struct socklist_node_t *p;
    int i = 1;
    for (p = socklist.tqh_first; p != NULL; p = p->next.tqe_next) {

        if (i >= nfds) {
            fprintf(stderr, "Fatal: socket list changed underneath us.\n");
            exit(1);
        }

        // TODO: Do we want POLLOUT and the close notifications too???

        pollfds[i].events = POLLIN | POLLHUP;
        pollfds[i].fd = p->fd;
    }

    *fds = pollfds;
    return nfds;
}

void
free_poll_fds(struct pollfd *fds)
{
    free(fds);
}

void
set_nonblock(int sock)
{
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0) {
        fprintf(stderr, "fcntl(%d, F_GETFL, 0): %s.\n", sock, strerror(errno));
        exit(1);
    }

    flags |= O_NONBLOCK;

    if (fcntl(sock, F_SETFL, flags) == -1) {
        fprintf(stderr, "fcntl(%d, F_SETFL, O_NONBLOCK): %s.\n", 
                sock, strerror(errno));
        exit(1);
    }
}

void
accept_new_socket(void)
{
    struct sockaddr_un sunaddr;
    socklen_t socksize = sizeof(sunaddr);
    int newsock = accept(listen_sock, (struct sockaddr *) &sunaddr, &socksize);

    if (-1 == newsock) {
        perror("accept");
        exit(1);
    }

    set_nonblock(newsock);

    add_socket_to_socket_list(newsock);
}

void
deal_with_ready_fds(struct pollfd *fds, int nfds)
{
    fprintf(stderr, "%s: nfds=%d.\n", __func__, nfds);

    int i;
    for (i = 0; i < nfds; ++i) {
        if (!fds[i].revents)
            continue;

        // Don't just *assume* that entry 0 is listen_sock...
        if (listen_sock == fds[i].fd) {
            accept_new_socket();
        } else {
            // Deal with data from fds[i].fd...
            const size_t count = BUFSIZE;
            byte buf[BUFSIZE];

            ssize_t numbytes = read(fds[i].fd, buf, count);

            if (0 == numbytes) {
                fprintf(stderr, "TODO: fd %d closed.\n", fds[i].fd);
                exit(1);
            } else if (-1 == numbytes) {
                fprintf(stderr, "TODO: fd %d error, errno is %s.\n", fds[i].fd, strerror(errno));
                exit(1);
            } else {
                // Now, let's just *assume* that it'll arrive in one big
                // chunk and just *send* it to pageant...
                HWND hwnd;
                hwnd = FindWindow("Pageant", "Pageant");
                if (!hwnd) {
                    fprintf(stderr, "Error: couldn't find pageant window.\n");
                    exit(1);
                }
                char mapname[512];
                // TODO: FFS don't use sprintf!!!!!
                sprintf(mapname, "PageantRequest%08x", (unsigned)GetCurrentThreadId());
                HANDLE filemap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
                                            0, AGENT_MAX_MSGLEN, mapname);
                if (filemap == NULL || filemap == INVALID_HANDLE_VALUE) {
                    fprintf(stderr, "TODO: What do we do here? I have no idea yet.\n");
                    exit(1);
                }
                byte *p = MapViewOfFile(filemap, FILE_MAP_WRITE, 0, 0, 0);
                memcpy(p, buf, numbytes);
                COPYDATASTRUCT cds;
                cds.dwData = AGENT_COPYDATA_ID;
                cds.cbData = 1 + strlen(mapname);
                cds.lpData = mapname;

                int id = SendMessage(hwnd, WM_COPYDATA, (WPARAM) NULL, (LPARAM) &cds);
                int retlen = 0;
                if (id > 0) {
                    retlen = 4 + GET_32BIT(p);
                    if (retlen > sizeof(buf)) {
                        fprintf(stderr, "Nearly a buffer overflow, oh yeah! Quitting.\n");
                        exit(1);
                    }
                    memcpy(buf, p, retlen);
                } else {
                    fprintf(stderr, "TODO: Couldn't SendMessage. Quitting.\n");
                    exit(1);
                }
                UnmapViewOfFile(p);
                CloseHandle(filemap);

                // Now, send buf back to the socket. We should probably loop and retry
                // or use poll properly since it's nonblocking...
                ssize_t byteswritten = write(fds[i].fd, buf, retlen);
                if (byteswritten != retlen) {
                    fprintf(stderr, "Tried to write %d bytes, ended up writing %d. Quitting.\n",
                            retlen, byteswritten);
                    exit(1);
                }
            }
        }
    }
}

void
handle_key_requests_forever(void)
{
    for (;;) {
        struct pollfd *fds;
        int nfds = make_poll_fds(&fds);
        int numready = poll(fds, nfds, -1);
        fprintf(stderr, "Poll said %d ready.\n", numready);

        if (numready > 0) {
            deal_with_ready_fds(fds, nfds);
        } else if (numready < 0) {
            if (EINTR == errno) {
                fprintf(stderr, "Info: poll() => EINTR.\n");
                return;
            } else {
                perror("poll error");
                exit(1);
            }
        } else if (0 == numready) {
            fprintf(stderr, "Error: poll() => 0, but no timeout was set.\n");
            exit(1);
        }

        free_poll_fds(fds);
    }
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

    init_socket_list();

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
