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

/*
 * Dynamically linked functions. These come in two flavours:
 *
 *  - GET_WINDOWS_FUNCTION does not expose "name" to the preprocessor,
 *    so will always dynamically link against exactly what is specified
 *    in "name". If you're not sure, use this one.
 *
 *  - GET_WINDOWS_FUNCTION_PP allows "name" to be redirected via
 *    preprocessor definitions like "#define foo bar"; this is principally
 *    intended for the ANSI/Unicode DoSomething/DoSomethingA/DoSomethingW.
 *    If your function has an argument of type "LPTSTR" or similar, this
 *    is the variant to use.
 *    (However, it can't always be used, as it trips over more complicated
 *    macro trickery such as the WspiapiGetAddrInfo wrapper for getaddrinfo.)
 *
 * (DECL_WINDOWS_FUNCTION works with both these variants.)
 */
#define DECL_WINDOWS_FUNCTION(linkage, rettype, name, params) \
    typedef rettype (WINAPI *t_##name) params; \
    linkage t_##name p_##name
#define STR1(x) #x
#define STR(x) STR1(x)
#define GET_WINDOWS_FUNCTION_PP(module, name) \
    (p_##name = module ? (t_##name) GetProcAddress(module, STR(name)) : NULL)
#define GET_WINDOWS_FUNCTION(module, name) \
    (p_##name = module ? (t_##name) GetProcAddress(module, #name) : NULL)

HMODULE load_system32_dll(const char *libname);

// Send a numbytes-sized message to pageant, synchronously.
// Up to outbuflen bytes of response will be written into outbuf.
// If the returned message would overflow buf, or if any other 
// error occurs, 0 will be returned.
int send_request_to_pageant(byte *inbuf, int inbytes, 
                            byte *outbuf, int outbuflen);

#endif // #ifndef PAGEANT_H
