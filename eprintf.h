#ifndef EPRINTF_H
#define EPRINTF_H

/* eprintf.h
 *
 */

#ifdef __cplusplus
extern "C" {
#endif  // #ifdef __cplusplus

#define EPRINTF(Level, Format, Args...) \
    eprintf(Level, "%s: " Format, __func__, ##Args)

#define EPRINTF_RAW(Level, Format, Args...) \
    eprintf(Level, Format, ##Args)

int eprintf(int level, const char *fmt, ...)
        __attribute__ ((format (printf, 2, 3)));

void louder(void);
int get_loudness(void);

#ifdef __cplusplus
}
#endif  // #ifdef __cplusplus

#endif // #define EPRINTF_H
