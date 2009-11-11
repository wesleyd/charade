#ifndef EPRINTF_H
#define EPRINTF_H

/* eprintf.h
 *
 */

#ifdef __cplusplus
extern "C" {
#endif  // #ifdef __cplusplus

extern unsigned int g_volume;

#define EPRINTF(Level, Format, Args...) \
    eprintf(Level, "%d %s: " Format, Level, __func__, ##Args)

int eprintf(int level, const char *fmt, ...)
        __attribute__ ((format (printf, 2, 3)));

// Increase the verbosity...
void louder(void);

#ifdef __cplusplus
}
#endif  // #ifdef __cplusplus

#endif // #define EPRINTF_H
