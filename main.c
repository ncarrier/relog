/*
 * main.c
 *
 *  Created on: Sep 25, 2015
 *      Author: ncarrier
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif /* _GNU_SOURCE */
#include <unistd.h>

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifndef RELOG_DEFAULT_OUT_FILE
#define RELOG_DEFAULT_OUT_FILE "/dev/kmsg"
#endif /* RELOG_DEFAULT_OUT_FILE */

#ifndef RELOG_LIBRELOG_PATH
#define RELOG_LIBRELOG_PATH "/usr/lib/librelog.so"
#endif /* RELOG_LIBRELOG_PATH */

#define RELOG_OUTFILE_OPT "--outfile="
#define RELOG_ERRFILE_OPT "--errfile="

#define RELOG_OUTFILE_ENV "RELOG_OUTFILE"
#define RELOG_ERRFILE_ENV "RELOG_ERRFILE"

static bool str_starts_with(const char *string, const char *prefix)
{
	return strncmp(string, prefix, strlen(prefix)) == 0;
}

static const char *str_strip_prefix(const char *string, const char *prefix)
{
	return string + strlen(prefix);
}

__attribute__((noreturn))
static void usage(int status)
{
	printf("usage : relog [--outfile=output_file] [--errfile=error_file] "
			"relog PROGRAM WITH ITS ARGUMENTS\n"
			"\tstarts PROGRAM with the arguments list "
			"\"WITH ITS ARGUMENTS\" and redirects it's standard "
			"output to output_file if given and it's standard error"
			" to error_file, if given. Both streams are configured "
			"with line mode buffering.\n");
#ifdef RELOG_FORCE_DEFAULT_OUTPUT_TO
	printf("\tIf no output_file is given, "RELOG_FORCE_DEFAULT_OUTPUT_TO
			" will be used for standard output redirection.\n");
#endif
#ifdef RELOG_FORCE_DEFAULT_ERROR_TO
	printf("\tIf no error_file is given, "RELOG_FORCE_DEFAULT_ERROR_TO
			" will be used for standard output redirection.\n");
#endif

	exit(status);
}

static void str_free(char **str)
{
	if (str == NULL || *str == NULL)
		return;

	free(*str);
	*str = NULL;
}

static int prepend_librelog_path_to_ld_preload(void)
{
	int ret;
	char __attribute__((cleanup(str_free)))*str = NULL;

	ret = asprintf(&str, RELOG_LIBRELOG_PATH":%s", getenv("LD_PRELOAD"));
	if (ret == -1) {
		fprintf(stderr, "asprintf error\n");
		errno = ENOMEM;
		return -1;
	}

	return setenv("LD_PRELOAD", str, 1);
}

int main(int argc, char *argv[])
{
	int ret;
#ifdef RELOG_FORCE_DEFAULT_OUTPUT_TO
	const char *outfile = RELOG_FORCE_DEFAULT_OUTPUT_TO;
#else
	const char *outfile = NULL;
#endif /* RELOG_FORCE_DEFAULT_OUTPUT_TO */
#ifdef RELOG_FORCE_DEFAULT_ERROR_TO
	const char *errfile = RELOG_FORCE_DEFAULT_ERROR_TO;
#else
	const char *errfile = NULL;
#endif /* RELOG_FORCE_DEFAULT_ERROR_TO */

	if (strcmp(argv[1], "-h") == 0)
		usage(EXIT_SUCCESS);

parse_options:
	if (argc >= 2) {
		if (str_starts_with(argv[1], RELOG_OUTFILE_OPT)) {
			outfile = str_strip_prefix(argv[1], RELOG_OUTFILE_OPT);
			argc--;
			argv++;
			goto parse_options;
		}
		if (str_starts_with(argv[1], RELOG_ERRFILE_OPT)) {
			errfile = str_strip_prefix(argv[1], RELOG_ERRFILE_OPT);
			argc--;
			argv++;
			goto parse_options;
		}
	}
	if (argc < 2)
		usage(EXIT_FAILURE);

	if (outfile != NULL)
		setenv(RELOG_OUTFILE_ENV, outfile, 1);
	if (errfile != NULL)
		setenv(RELOG_ERRFILE_ENV, errfile, 1);

	if (getenv("LD_PRELOAD") == NULL)
		ret = setenv("LD_PRELOAD", RELOG_LIBRELOG_PATH, 1);
	else
		ret = prepend_librelog_path_to_ld_preload();
	if (ret == -1) {
		fprintf(stderr, "setting LD_PRELOAD: %m\n");
		return EXIT_FAILURE;
	}

	argc--;
	argv++;
	ret = execvp(*argv, argv);
	if (ret == -1) {
		fprintf(stderr, "setting LD_PRELOAD: %s\n", strerror(-ret));
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
