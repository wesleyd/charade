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
     ((unsigned long)(unsigned char)(cp)[2] <<  8) | \
     ((unsigned long)(unsigned char)(cp)[3]      ))

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
#if 0
    if (agentsocket == NULL) {
#endif

    if (atexit(remove_socket_dir)) {
        EPRINTF(0, "Can't install atexit handler to delete socket dir.\n");
        exit(1);
    }

    /* Create private directory for agent socket */
    strlcpy(socket_dir, "/tmp/ssh-XXXXXXXXXX", sizeof socket_dir);
    if (mkdtemp(socket_dir) == NULL) {
        EPRINTF(0, "Can't mkdir private socket directory: %s\n", 
                strerror(errno));
        //perror("mkdtemp: private socket dir");
        exit(1);
    }

    remove_socket_at_exit = 1;

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

void
fork_subprocess(void)
{
    if (! g_dontfork_flag) {
        long pid = fork();

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
    } else {
        int pid = getpid();

        printf("%s=%s; export %s\n", SSH_AUTHSOCKET_ENV_NAME, socket_name,
                                   SSH_AUTHSOCKET_ENV_NAME);
        printf("%s=%ld; export %s\n", SSH_AGENTPID_ENV_NAME, (long) pid,
                                   SSH_AGENTPID_ENV_NAME);
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
fd_is_closed(int fd)
{
    // Remove it from the list...

    // TODO: Remove the fd from the big list!
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

// buf is used for input and output, which is why we have
// a bufsize *and* a numbytes.
int
send_request_to_pageant(byte *buf, int numbytes, int bufsize)
{
    // Now, let's just *assume* that it'll arrive in one big
    // chunk and just *send* it to pageant...
    HWND hwnd;
    hwnd = FindWindow("Pageant", "Pageant");
    if (!hwnd) {
        EPRINTF(1, "Can't FindWindow(\"Pageant\"...) - "
                   "is pageant running?.\n");
        return 0;
    }

    char mapname[512];
    sprintf(mapname, "PageantRequest%08x", (unsigned)GetCurrentThreadId());

    HANDLE filemap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, 
                                       PAGE_READWRITE, 0, 
                                       AGENT_MAX_MSGLEN, mapname);
    if (filemap == NULL || filemap == INVALID_HANDLE_VALUE) {
        EPRINTF(0, "Can't CreateFileMapping.\n");
        return 0;
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
        if (retlen > bufsize) {
            EPRINTF(0, "Buffer too small to contain reply from pageant.\n");
            return 0;
        }

        // TODO: are there miscounting errors here?

        memcpy(buf, p, retlen);
    } else {
        EPRINTF(0, "Couldn't SendMessage().\n");
        return 0;
    }
    UnmapViewOfFile(p);
    CloseHandle(filemap);

    return retlen;
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
        } else {
            // Deal with data from fds[i].fd...
            const size_t count = BUFSIZE;
            byte buf[BUFSIZE];

            ssize_t numbytes = read(fds[i].fd, buf, count);

            if (0 == numbytes) {
                fd_is_closed(fds[i].fd);
            } else if (-1 == numbytes) {
                fprintf(stderr, "TODO: fd %d error, errno is %s.\n", fds[i].fd, strerror(errno));
                exit(1);
            } else {
                int retlen = send_request_to_pageant(buf, numbytes, sizeof(buf));

                // Now, send buf back to the socket. We should probably 
                // loop and retry or use poll properly since it's nonblocking...
                ssize_t byteswritten = write(fds[i].fd, buf, retlen);
                if (byteswritten != retlen) {
                    fprintf(stderr, "Tried to write %d bytes, "
                                    "ended up writing %d. Quitting.\n",
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
        EPRINTF(3, "poll() returned %d ready.\n", numready);

        if (numready > 0) {
            deal_with_ready_fds(fds, nfds);
        } else if (numready < 0) {
            if (EINTR == errno) {
                EPRINTF(3, "poll() was EINTR-ed.\n");
                continue;
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

pid_t
fork_off_key_handler(void)
{
    fprintf(stderr, "TODO: fork_off_key_handler.\n");
    exit(1);
}

void
exec_subprocess(void)
{
    fprintf(stderr, "TODO: exec_subprocess.\n");
    exit(1);
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

        if (g_subprocess_argc) {
            exec_subprocess();
            /* NOTREACHED */
        } else {
            print_env_stuff(agent_pid);
            exit(0);
        }
        /* NOTREACHED */
    }
    /* NOTREACHED */

    exit(0);
}
