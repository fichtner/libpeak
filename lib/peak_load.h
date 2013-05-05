#ifndef PEAK_LOAD_H
#define PEAK_LOAD_H

struct peak_load {
	unsigned int fmt;
	unsigned int len;
	int fd;

	uint32_t ts_off;
	uint32_t ts_ms;
	time_t ts_unix;

	uint8_t buf[32 * 1024];
};

void			 peak_load_exit(struct peak_load *);
unsigned int		 peak_load_next(struct peak_load *);
struct peak_load	*peak_load_init(const char *);

#endif /* !PEAK_LOAD_H */
