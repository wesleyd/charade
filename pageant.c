/* The code in here is mostly lifted from Simon Tatham's putty code, in
 * particular windows/winpgntc.c.
 *
 * TODO: Simon Tatham's copyright notice!!!
 *
 * Copyright (c) 2009, Wesley Darlington. All Rights Reserved.
 */

#include <stdio.h>
#include <windows.h>

#include "eprintf.h"
#include "pageant.h"

// TODO: is there actually something magical about this number?
#define AGENT_COPYDATA_ID 0x804e50ba

#define AGENT_MAX_MSGLEN 8192

#define GET_32BIT_MSB_FIRST(cp) \
    (((unsigned long)(unsigned char)(cp)[0] << 24) | \
     ((unsigned long)(unsigned char)(cp)[1] << 16) | \
     ((unsigned long)(unsigned char)(cp)[2] <<  8) | \
     ((unsigned long)(unsigned char)(cp)[3]      ))

#define GET_32BIT(cp) GET_32BIT_MSB_FIRST(cp)

int
send_request_to_pageant(byte *buf, int numbytes, int bufsize)
{
    EPRINTF(3, "Sending %d bytes to pageant.\n", numbytes);

    // Now, let's just *assume* that it'll arrive in one big
    // chunk and just *send* it to pageant...
    HWND hwnd;
    hwnd = FindWindow("Pageant", "Pageant");
    if (!hwnd) {
        EPRINTF(0, "Can't FindWindow(\"Pageant\"...) - "
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

    byte *shmem = MapViewOfFile(filemap, FILE_MAP_WRITE, 0, 0, 0);
    memcpy(shmem, buf, numbytes);
    COPYDATASTRUCT cds;
    cds.dwData = AGENT_COPYDATA_ID;
    cds.cbData = 1 + strlen(mapname);
    cds.lpData = mapname;

    int id = SendMessage(hwnd, WM_COPYDATA, (WPARAM) NULL, (LPARAM) &cds);
    int retlen = 0;
    if (id > 0) {
        retlen = 4 + GET_32BIT(shmem);
        if (retlen > bufsize) {
            EPRINTF(0, "Buffer too small to contain reply from pageant.\n");
            return 0;
        }

        memcpy(buf, shmem, retlen);
    } else {
        EPRINTF(0, "Couldn't SendMessage().\n");
        return 0;
    }

    UnmapViewOfFile(shmem);
    CloseHandle(filemap);

    EPRINTF(3, "Got %d bytes back from pageant.\n", retlen);

    return retlen;
}
