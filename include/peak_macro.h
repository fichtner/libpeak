#ifndef PEAK_MACROS_H
#define PEAK_MACROS_H

#define unlikely(__cond__)	__builtin_expect(!!(__cond__), 0)
#define likely(__cond__)	__builtin_expect(!!(__cond__), 1)

#define __read_mostly __attribute__((__section__(".data.read_mostly")))

#endif /* PEAK_MACROS_H */
