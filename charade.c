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
#include "eprintf.h"
#include "pageant.h"
#include "todo.h"

#define LISTEN_BACKLOG 5
#define BUFSIZE 8192

// TODO: is there actually something magical about this number?
#define AGENT_COPYDATA_ID 0x804e50ba 

#define SSH_AUTHSOCKET_ENV_NAME "SSH_AUTH_SOCK"
#define SSH_AGENTPID_ENV_NAME "SSH_AGENT_PID"

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

int remove_socket_at_exit = 0;

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

    EPRINTF(5, "adding socket %d to socket list.\n", sock);

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

    EPRINTF(5, "num sockets in list: %d.\n", count);

    return count;
}

void
remove_socket_dir(void)  /* atexit handler */
{
    if (remove_socket_at_exit) {
        int ret = rmdir(socket_dir);

        if (ret) {
            EPRINTF(0, "error removing socket dir '%s': %s.\n", 
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
            EPRINTF(0, "error removing socket '%s': %s.\n", 
                    socket_name, strerror(errno));
            /* atexit! */
        }
    }
}


void
create_socket(void)
{
    if (atexit(remove_socket_dir)) {
        EPRINTF(0, "Can't install atexit handler to delete socket dir.\n");
        exit(1);
    }

    /* Create private directory for agent socket */
    strlcpy(socket_dir, "/tmp/ssh-XXXXXXXXXX", sizeof socket_dir);
    if (mkdtemp(socket_dir) == NULL) {
        EPRINTF(0, "Can't mkdir socket directory: %s\n", 
                strerror(errno));
        exit(1);
    }

    remove_socket_at_exit = 1;

    int ret = snprintf(socket_name, sizeof(socket_name), 
                       "%s/agent.%ld", socket_dir, (long)getpid());
    if (ret >= sizeof(socket_name)) {
        // Would have liked to print more...
        EPRINTF(0, "socket_name too long (%d >= %d).\n", 
                ret, sizeof(socket_name));
        exit(1);
    }

    listen_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listen_sock < 0) {
        EPRINTF(0, "socket error: %s.\n", strerror(errno));
        exit(1);
    }

    struct sockaddr_un sunaddr;
    memset(&sunaddr, 0, sizeof(sunaddr));
    sunaddr.sun_family = AF_UNIX;
    strlcpy(sunaddr.sun_path, socket_name, sizeof(sunaddr.sun_path));
    int prev_mask = umask(0177);
    if (bind(listen_sock, (struct sockaddr *) &sunaddr, sizeof(sunaddr)) < 0) {
        EPRINTF(0, "bind() error: %s.\n", strerror(errno));
        umask(prev_mask);
        exit(1);
    }

    if (atexit(remove_socket)) {
        EPRINTF(0, "Can't install atexit handler to delete socket '%s'; "
                        "do it yourself!\n", socket_name);
        exit(1);
    }

    umask(prev_mask);
    if (listen(listen_sock, LISTEN_BACKLOG) < 0) {
        EPRINTF(0, "listen() error: %s", strerror(errno));
        exit(1);
    }
}

void
kill_old_agent(void)
{
    char *pidstr = getenv(SSH_AGENTPID_ENV_NAME);
    if (pidstr == NULL) {
        EPRINTF(0, "%s not set, can't kill agent.\n", SSH_AGENTPID_ENV_NAME);
        exit(1);
    }

    const char *errstr = NULL;
    pid_t pid = strtol(pidstr, NULL, 10);  // base 10
    if (errstr) {
        EPRINTF(0, "Invalid PID %s=\"%s\".\n", SSH_AGENTPID_ENV_NAME, pidstr);
        exit(1);
    }

    if (kill(pid, SIGTERM) == -1) {
        EPRINTF(0, "error from kill -TERM %ld: %s.\n", (long) pid, strerror(errno));
        exit(1);
    }

    const char *format = g_csh_flag 
                       ? "unsetenv %s;\n"
                       : "unset %s;\n";
    printf(format, SSH_AUTHSOCKET_ENV_NAME);
    printf(format, SSH_AGENTPID_ENV_NAME);
    printf("echo Agent pid %ld killed;\n", (long)pid);
    exit(0);
}

void
print_env_var(char *key, char *value)
{
    if (g_csh_flag) {
        printf("setenv %s %s;\n", key, value);
    } else {
        printf("%s=%s; export %s\n", key, value, key);
    }
}

#define ITOA_BUFSIZE 64
char *
itoa_unsafe(int i)
{
    static char buf[ITOA_BUFSIZE];  // Unsafeness number one
    if (snprintf(buf, sizeof(buf), "%d", i) > sizeof(buf)) {
        // Silently ignore: unsafeness number two
    }
    return buf;
}
#undef ITOA_BUFSIZE

void
print_env_stuff(int pid)
{
    print_env_var(SSH_AUTHSOCKET_ENV_NAME, socket_name);
    print_env_var(SSH_AGENTPID_ENV_NAME, itoa_unsafe(pid));
}

int
make_poll_fds(struct pollfd **fds)
{
    int nfds = 1                       // The listening socket...
             + num_sockets_in_list();  // ...plus all the others.

    struct pollfd *pollfds = calloc(sizeof(struct pollfd), nfds);

    if (!pollfds) {
        EPRINTF(0, "Can't calloc() %d struct pollfd's for poll().\n", nfds);
        exit(1);
    }

    pollfds[0].fd = listen_sock;
    pollfds[0].events = POLLIN | POLLHUP;

    struct socklist_node_t *p;
    int i = 1;
    for (p = socklist.tqh_first; p != NULL; p = p->next.tqe_next) {

        if (i >= nfds) {
            EPRINTF(0, "Internal error: socket list changed beneath us.\n");
            exit(1);
        }

        // TODO: Do we want POLLOUT and the close notifications too???

        pollfds[i].events = POLLIN | POLLHUP;
        pollfds[i].fd = p->fd;
        ++i;
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
        EPRINTF(0, "Can't fcntl(%d, F_GETFL, 0): %s.\n", sock, strerror(errno));
        exit(1);
    }

    flags |= O_NONBLOCK;

    if (fcntl(sock, F_SETFL, flags) == -1) {
        EPRINTF(0, "fcntl(%d, F_SETFL, O_NONBLOCK): %s.\n", sock, strerror(errno));
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
        EPRINTF(0, "accept error: %s", strerror(errno));
        exit(1);
    }

    set_nonblock(newsock);

    add_socket_to_socket_list(newsock);
}

void
fd_is_closed(int fd)
{
    EPRINTF(3, "Removing fd %d from list.\n", fd);
    // Remove it from the list...

    // Remove the fd from the list...
    struct socklist_node_t *p;
    for (p = socklist.tqh_first; p != NULL; p = p->next.tqe_next) {
        if (p->fd == fd) {
            TAILQ_REMOVE(&socklist, p, next);
            break;
        }
    }

    // ...and close it I guess...
    close(fd);
}

void
deal_with_ready_fds(struct pollfd *fds, int nfds)
{
    EPRINTF(3, "nfds=%d.\n", nfds);

    int i;
    for (i = 0; i < nfds; ++i) {
        if (!fds[i].revents)
            continue;

        // Don't just *assume* that entry 0 is listen_sock...
        if (listen_sock == fds[i].fd) {
            accept_new_socket();
        } else if (fds[i].revents & POLLIN) {
            // Deal with data from fds[i].fd...
            const size_t count = BUFSIZE;
            byte buf[BUFSIZE];

            ssize_t numbytes = read(fds[i].fd, buf, count);

            if (0 == numbytes) {
                close(fds[i].fd);
                fd_is_closed(fds[i].fd);
            } else if (-1 == numbytes) {
                // TODO: Should we handle EAGAIN/EWOULDBLOCK specially?
                EPRINTF(0, "internal error: read(fd=%d) => errno=%d/%s.\n", 
                        fds[i].fd, errno, strerror(errno));
                close(fds[i].fd);
                fd_is_closed(fds[i].fd);
            } else {
                int retlen = send_request_to_pageant(buf, numbytes, sizeof buf);

                // Now, send buf back to the socket. We should probably 
                // loop and retry or use poll properly since it's nonblocking...
                ssize_t byteswritten = write(fds[i].fd, buf, retlen);
                if (byteswritten != retlen) {
                    EPRINTF(0, "Tried to write %d bytes, "
                               "ended up writing %d.\n",
                            retlen, byteswritten);
                    close(fds[i].fd);
                    fd_is_closed(fds[i].fd);
                }
            }
        } else {
            EPRINTF(0, "Don't know how to deal with revents=0x%x on fd %d.\n",
                    fds[i].revents, fds[i].fd);
            close(fds[i].fd);
            fd_is_closed(fds[i].fd);
        }
    }
}

void
handle_key_requests_forever(void)
{
    EPRINTF(5, "entry.\n");

    for (;;) {
        struct pollfd *fds;
        int nfds = make_poll_fds(&fds);

        EPRINTF(3, "Entering poll with %d fds.\n", nfds);
        int numready = poll(fds, nfds, -1);
        EPRINTF(3, "poll() returned %d ready.\n", numready);

        if (numready > 0) {
            deal_with_ready_fds(fds, nfds);
        } else if (numready < 0) {
            if (EINTR == errno) {
                EPRINTF(3, "poll() was EINTR-ed.\n");
                continue;
            } else {
                EPRINTF(0, "poll error: %s.\n", strerror(errno));
                exit(1);
            }
        } else if (0 == numready) {
            EPRINTF(0, "Internal error: poll() => 0, but no timeout was set.\n");
            exit(1);
        }

        free_poll_fds(fds);
    }
}

void
redirect(FILE *f, char *basename)
{
    char buf[MAXPATHLEN] = "";

    int ret = snprintf(buf, sizeof(buf), "%s/%s.%ld", 
                       socket_dir, basename, (long)getpid());
    if (ret >= sizeof(buf)) {
        EPRINTF(0, "Can't open %s to redirect %s: Too long (%d >= %d).\n", 
                buf, basename, ret, sizeof(buf));
    } else {
        EPRINTF(1, "Redirecting %s to %s.\n", basename, buf);

        if (!freopen(buf, "w", f)) {
            EPRINTF(0, "can't freopen %s.\n", basename);
        }

        // Turn off buffering...
        setvbuf(f, NULL, _IONBF, 0);

        EPRINTF(1, "Redirected %s to %s.\n", basename, buf);
    }
}

void
redirect_stdall(void)
{
    fclose(stdin);

    /*
    if (get_loudness() <= 0) {
        fclose(stdout);
        fclose(stderr);
    } else {
        redirect(stdout, "stdout");
        redirect(stderr, "stderr");
    }
    */

    redirect(stdout, "stdout");
    redirect(stderr, "stderr");
}

pid_t
fork_off_key_handler(void)
{
    pid_t handler_pid = fork();
    if (handler_pid == -1) {
        EPRINTF(0, "Can't fork(): %s.\n", strerror(errno));
        exit(1);
    }

    if (handler_pid != 0) {
        return handler_pid;
    }

    // OK, we're the child agent process now...

    // Make this controllable by cmdline, do it only with -v
    redirect_stdall();

    // Do the setsid thing
    if (setsid() == -1) {
        EPRINTF(0, "error from setsid(): %s.\n", strerror(errno));
        exit(1);
    }

    chdir("/");

    // TODO: Do the filehandle thing

    // TODO: Do setrlimit thing

    // TODO: Do the signal thing

    handle_key_requests_forever();
    
    /* NOTREACHED */
    exit(0);
}

void
exec_subprocess(pid_t agent_pid)
{
    TODO();
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

    init_socket_list();

    create_socket();

    if (g_dontfork_flag) {
        // This is "debug" mode, but I prefer to call it dontfork mode...
        print_env_stuff(getpid());
        handle_key_requests_forever();
        /* NOTREACHED */
    } else {
        pid_t agent_pid = fork_off_key_handler();

        close(listen_sock);

        if (g_subprocess_argc) {
            exec_subprocess(agent_pid);
            /* NOTREACHED */
        } else {
            print_env_stuff(agent_pid);
            remove_socket_at_exit = 0;
            exit(0);
        }
        /* NOTREACHED */
    }
    /* NOTREACHED */

    exit(0);
}
