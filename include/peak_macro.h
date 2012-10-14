#ifndef PEAK_MACRO_H
#define PEAK_MACRO_H

#define lengthof(__x__)		(sizeof(__x__)/sizeof((__x__)[0]))
#define unlikely(__cond__)	__builtin_expect(!!(__cond__), 0)
#define likely(__cond__)	__builtin_expect(!!(__cond__), 1)
#define msleep(__x__)		usleep((__x__)*1000)

#ifdef __APPLE__
#define __read_mostly
#else /* !__APPLE__ */
#define __read_mostly	__attribute__((__section__(".data.read_mostly")))
#define __dead			__attribute__((__noreturn__))
#endif /* __APPLE__ */
#define __packed		__attribute__((__packed__))

#ifdef __CHECKER__
#define UNITTEST static
#else /* !__CHECKER__ */
#define UNITTEST
#endif /* __CHECKER__ */

#endif /* !PEAK_MACRO_H */
