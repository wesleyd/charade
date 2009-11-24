#ifndef PAGEANT_H
#define PAGEANT_H

/* The code in here is mostly lifted from Simon Tatham's putty code, in
 * particular windows/winpgntc.c.
 *
 * TODO: Simon Tatham's copyright notice!!!
 *
 * Copyright (c) 2009, Wesley Darlington. All Rights Reserved.
 */

// Send a numbytes-sized message (in buf) to pageant, synchronously.
// Up to numbytes will be written into buf of the returned message. If
// the returned message would overflow buf, or if any other error occurs,
// 0 will be returned.
int send_request_to_pageant(byte *buf, int numbytes, int bufsize);

#endif // #ifndef PAGEANT_H
