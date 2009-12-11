#ifndef PAGEANT_H
#define PAGEANT_H

/* The code in here is mostly lifted from Simon Tatham's putty code, in
 * particular windows/winpgntc.c.
 *
 * Copyright (c) 2009, Wesley Darlington. All Rights Reserved.
 *
 * Large swathes of the pageant interface taken from the putty source
 * code...
 *
 * Copyright 1997-2007 Simon Tatham.
 *
 * Portions copyright Robert de Bath, Joris van Rantwijk, Delian
 * Delchev, Andreas Schultz, Jeroen Massar, Wez Furlong, Nicolas Barry,
 * Justin Bradford, Ben Harris, Malcolm Smith, Ahmad Khalifa, Markus
 * Kuhn, and CORE SDI S.A.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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
