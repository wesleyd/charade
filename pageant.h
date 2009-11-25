#ifndef PAGEANT_H
#define PAGEANT_H

/* The code in here is mostly lifted from Simon Tatham's putty code, in
 * particular windows/winpgntc.c.
 *
 * TODO: Simon Tatham's copyright notice!!!
 *
 * Copyright (c) 2009, Wesley Darlington. All Rights Reserved.
 */

#define GET_32BIT_MSB_FIRST(cp) \
        (((unsigned long)(unsigned char)(cp)[0] << 24) | \
         ((unsigned long)(unsigned char)(cp)[1] << 16) | \
         ((unsigned long)(unsigned char)(cp)[2] <<  8) | \
         ((unsigned long)(unsigned char)(cp)[3]      ))

#define GET_32BIT(cp) GET_32BIT_MSB_FIRST(cp)

// Send a numbytes-sized message to pageant, synchronously.
// Up to outbuflen bytes of response will be written into outbuf.
// If the returned message would overflow buf, or if any other 
// error occurs, 0 will be returned.
int send_request_to_pageant(byte *inbuf, int inbytes, 
                            byte *outbuf, int outbuflen);

#endif // #ifndef PAGEANT_H
