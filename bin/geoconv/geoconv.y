%{
/*
 * Copyright (c) 2012-2014 Franco Fichtner <franco@packetwerk.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/stat.h>
#include <fcntl.h>
#include <peak.h>
#include <limits.h>
#include <unistd.h>

#define YYERROR_VERBOSE	/* yacc(1) compliance */

output_init();

void	yyerror(const char *);
int	yyparse(void);
int	yywrap(void);
int	yylex(void);

extern FILE *yyin;

void yyerror(const char *str)
{
	perr("%s\n", str);
	exit(1);
}

int yywrap(void)
{
	return (1);
}

struct geo_helper {
	RB_ENTRY(geo_helper) entry;
	struct peak_locate data;
};

static RB_HEAD(geo_tree, geo_helper) geo_head = RB_INITIALIZER(&geo_head);
static prealloc_t geo_mem;

#define locatecmp(x, y)	peak_locate_cmp(&(x)->data, &(y)->data)

RB_GENERATE_STATIC(geo_tree, geo_helper, entry, locatecmp);
%}
%union {
	struct netaddr addr;
	char loc[8];
}
%token <addr> IPV4_STR IPV6_STR
%token <loc> LOC_SHORT
%token IP_RAW LOC_LONG
%type <addr> ip_str
%type <loc> loc
%start line
%%
line	: /* empty */
	| line '\n'
	| line entry '\n'
	;

entry	: ip_str ip_str ip_raw ip_raw loc {
		struct geo_helper *elm = prealloc_get(&geo_mem);
		if (!elm) {
			perr("too many entries\n");
			exit(1);
		}

		memset(elm, 0, sizeof(*elm));
		memcpy(elm->data.location, $5, sizeof(elm->data.location));
		elm->data.min = $1;
		elm->data.max = $2;

		if (RB_INSERT(geo_tree, &geo_head, elm)) {
			perr("found duplicated entry\n");
			exit(1);
		}
	}
	;

ip_str	: '"' IPV4_STR '"' ',' {
		$$ = $2;
	}
	| '"' IPV6_STR '"' ',' {
		$$ = $2;
	}
	;

ip_raw	: '"' IP_RAW '"' ','
	;

loc	: '"' LOC_SHORT '"' ',' '"' LOC_LONG '"' {
		memcpy($$, $2, sizeof($$));
	}
	;
%%
static void
finalise(const char *plain_fn)
{
	struct peak_locate_hdr hdr = {
		.count = prealloc_used(&geo_mem),
		.revision = LOCATE_REVISION,
		.magic = LOCATE_MAGIC,
	};
	char binary_fn[PATH_MAX];
	struct geo_helper *elm;
	int fd;

	if (RB_EMPTY(&geo_head)) {
		/* don't need output without input */
		return;
	}

	/* automatically pick name from first plain config file */
	snprintf(binary_fn, sizeof(binary_fn), "%s.bin", plain_fn);

	fd = open(binary_fn, O_WRONLY|O_CREAT|O_TRUNC,
	    S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	if (fd < 0) {
		perr("output open failed\n");
		exit(1);
	}

	if (write(fd, &hdr, sizeof(hdr)) != sizeof(hdr)) {
		perr("header write failed\n");
		exit(1);
	}

	RB_FOREACH(elm, geo_tree, &geo_head) {
		if (write(fd, &elm->data, sizeof(elm->data)) !=
		    sizeof(elm->data)) {
			perr("entry write failed\n");
			exit(1);
		}

		/* this is normally unsafe, but we do it anyway */
		prealloc_put(&geo_mem, elm);
	}

	close(fd);
}

static void
usage(void)
{
	extern char *__progname;

	perr("usage: %s file ...\n", __progname);

	exit(EXIT_FAILURE);
}

int
main(int argc, char **argv)
{
	const char *fn = NULL;
	struct geo_helper *elm;
	int i;

	if (argc <= 1) {
		usage();
		/* NOTREACHED */
	}

	if (!prealloc_init(&geo_mem, 150000, sizeof(*elm))) {
		perr("could not allocate memory\n");
		return (1);
	}

	for (i = 1; i < argc; ++i) {
		FILE *file = fopen(argv[i], "r");
		if (!file) {
			perr("could not open file %s\n", argv[i]);
			prealloc_exit(&geo_mem);
			return (1);
		}
		yyin = file;
		yyparse();
		fclose(file);

		fn = fn ? : argv[i];
	}

	finalise(fn);

	prealloc_exit(&geo_mem);

	return (0);
}
