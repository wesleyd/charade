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

void
print_buf(int level, byte *buf, int numbytes)
{
    int i;
    for (i = 0; i < numbytes;) {
        EPRINTF_RAW(level, "%02x ", buf[i]);
        ++i;
        if (!(i%8)) EPRINTF_RAW(level, " ");
        if (!(i%16) || i == numbytes) EPRINTF_RAW(level, "\n");
    }
}

BOOL CALLBACK
wnd_enum_proc(HWND hwnd, LPARAM lparam)
{
    char window_name[512] = "\0";
    GetWindowText(hwnd, window_name, 512);
    EPRINTF(0, "Window %p: title '%s'.\n", hwnd, window_name);
    return TRUE;
}

void
enum_windows(void)
{
    BOOL retval = EnumWindows(wnd_enum_proc, (LPARAM) 0);

    if (!retval) {
        EPRINTF(0, "EnumWindows failed; GetLastError() => %d.\n",
                (int) GetLastError());
    }
}

// "in" means "to pageant", "out" means "from pageant". Sorry.
int
send_request_to_pageant(byte *inbuf, int inbytes, byte *outbuf, int outbuflen)
{
    EPRINTF(3, "Sending %d bytes to pageant.\n", inbytes);

    if (inbytes < 4) {
        EPRINTF(0, "Pageant-bound message too short (%d bytes).\n", inbytes);
        return 0;
    }
    int claimed_inbytes = GET_32BIT(inbuf);
    if (inbytes != claimed_inbytes + 4) {
        EPRINTF(0, "Pageant-bound message is %d bytes long, but it "
                "*says* it has %d=%d+4 bytes in it.\n",
                inbytes, claimed_inbytes+4, claimed_inbytes);
        return 0;
    }

    EPRINTF(5, "Message to pageant (%d bytes):\n", inbytes);
    print_buf(5, inbuf, inbytes);

    HWND hwnd;
    hwnd = FindWindow("Pageant", "Pageant");
    if (!hwnd) {
        EPRINTF(0, "Can't FindWindow(\"Pageant\"...) - "
                   "is pageant running?. (GetLastError is %x)\n",
                   (unsigned) GetLastError());
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
    memcpy(shmem, inbuf, inbytes);
    COPYDATASTRUCT cds;
    cds.dwData = AGENT_COPYDATA_ID;
    cds.cbData = 1 + strlen(mapname);
    cds.lpData = mapname;

    int id = SendMessage(hwnd, WM_COPYDATA, (WPARAM) NULL, (LPARAM) &cds);
    int retlen = 0;
    if (id > 0) {
        retlen = 4 + GET_32BIT(shmem);
        if (retlen > outbuflen) {
            EPRINTF(0, "Buffer too small to contain reply from pageant.\n");
            return 0;
        }

        memcpy(outbuf, shmem, retlen);

        EPRINTF(5, "Reply from pageant (%d bytes):\n", retlen);
        print_buf(5, outbuf, retlen);
    } else {
        EPRINTF(0, "Couldn't SendMessage().\n");
        return 0;
    }

    // enum_windows();


    UnmapViewOfFile(shmem);
    CloseHandle(filemap);

    EPRINTF(3, "Got %d bytes back from pageant.\n", retlen);

    return retlen;
}
