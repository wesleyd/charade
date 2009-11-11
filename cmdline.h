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

void parse_cmdline(int *pargc, char ***pargv);

#endif  // #ifndef CMDLINE_H
