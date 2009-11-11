/* cmdline.h
 *
 * Copyright (c) 2009, Wesley Darlington. All Rights Reserved.
 *
 */

#ifndef CMDLINE_H
#define CMDLINE_H

extern int g_csh_flag;
extern int g_sh_flag;
extern int g_kill_flag;
extern int g_debug_flag;
extern char *g_socket_name;

extern int g_subprocess_argc;
extern char **g_subprocess_argv;

void parse_cmdline(int argc, char **argv);

#endif  // #ifndef CMDLINE_H
