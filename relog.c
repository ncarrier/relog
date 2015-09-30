/*
 * relog.c
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
#include <getopt.h>

#include "relog.h"

static const struct option long_options[] = {
		{
				.name = "errfile",
				.has_arg = required_argument,
				.flag = NULL,
				.val = 'e',
		},
		{
				.name = "help",
				.has_arg = no_argument,
				.flag = NULL,
				.val = 'h',
		},
		{
				.name = "outfile",
				.has_arg = required_argument,
				.flag = NULL,
				.val = 'o',
		},
		{
				.name = "same-errprocess",
				.has_arg = no_argument,
				.flag = NULL,
				.val = 's',
		},
		{
				.name = "errprocess",
				.has_arg = required_argument,
				.flag = NULL,
				.val = 'E',
		},
		{
				.name = "outprocess",
				.has_arg = required_argument,
				.flag = NULL,
				.val = 'O',
		},
		{0},
};

static const char short_options[] = "e:ho:sE:O:";

static const char *help[] = {
		['e'] = "FILE\n\t\t"
				"file which the program's standard error will "
				"be redirected to"
#ifdef RELOG_FORCE_DEFAULT_ERROR_TO
				", defaults to "RELOG_FORCE_DEFAULT_ERROR_TO
#endif
				,
		['o'] = "FILE\n\t\t"
				"file which the program's standard output will "
				"be redirected to"
#ifdef RELOG_FORCE_DEFAULT_OUTPUT_TO
				", defaults to "RELOG_FORCE_DEFAULT_OUTPUT_TO
#endif
				,
		['h'] = "\n\t\t"
				"this help",
		['s'] = "\n\t\t"
				"if the --outprocess is given, the standard "
				"error will be redirected to the same process' "
				"standard input, thus saving one process "
				"creation, otherwise, this option is ignored",
		['E'] = "CMDLINE\n\t\t"
				"filter which will be executed and fed with the"
				" program's standard error",
		['O'] = "CMDLINE\n\t\t"
				"filter which will be executed and fed with the"
				" program's standard output",
};

__attribute__((noreturn))
static void usage(int status)
{
	int i = 0;
	const char *h;
	const char *name;

	printf("usage : relog [options] PROGRAM WITH ITS ARGUMENTS\n\n"
			"\tStarts PROGRAM with the arguments list "
			"\"WITH ITS ARGUMENTS\". "
			"Both standard output and error streams are configured "
			"with line mode buffering. "
			"The options allow to redirect output and error to "
			"another log facility. "
			"If one option occur more than once, the latest "
			"occurrence takes precedence.\n\n");

	do {
		name = long_options[i].name;
		h = help[long_options[i].val];
		if (h != NULL)
			printf("\t--%s %s\n", name, h);
	} while (long_options[++i].name != NULL);

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

	return setenv("LD_PRELOAD", str, true);
}

int main(int argc, char *argv[])
{
	int c;
	int ret;

#ifdef RELOG_FORCE_DEFAULT_ERROR_TO
	setenv(RELOG_ERRFILE_ENV, RELOG_FORCE_DEFAULT_ERROR_TO, 1);
#endif /* RELOG_FORCE_DEFAULT_ERROR_TO */
#ifdef RELOG_FORCE_DEFAULT_OUTPUT_TO
	setenv(RELOG_OUTFILE_ENV, RELOG_FORCE_DEFAULT_OUTPUT_TO, 1);
#endif /* RELOG_FORCE_DEFAULT_OUTPUT_TO */

	while (true) {
		c = getopt_long(argc, argv, short_options, long_options, NULL);
		if (c == -1)
			break;

		switch (c) {
		case 'e':
			setenv(RELOG_ERRFILE_ENV, optarg, true);
			break;

		case 'h':
			usage(EXIT_SUCCESS);

		case 'o':
			setenv(RELOG_OUTFILE_ENV, optarg, true);
			break;

		case 's':
			setenv(RELOG_SAME_ERRPROCESS_ENV, optarg, true);
			break;

		case 'E':
			setenv(RELOG_ERRPROCESS_ENV, optarg, true);
			break;

		case 'O':
			setenv(RELOG_OUTPROCESS_ENV, optarg, true);
			break;

		default:
			usage(EXIT_FAILURE);
		}
	}

	if (getenv("LD_PRELOAD") == NULL)
		ret = setenv("LD_PRELOAD", RELOG_LIBRELOG_PATH, true);
	else
		ret = prepend_librelog_path_to_ld_preload();
	if (ret == -1) {
		fprintf(stderr, "setting LD_PRELOAD: %m\n");
		return EXIT_FAILURE;
	}

	argc -= optind;
	argv += optind;
	if (argc < 1)
		usage(EXIT_FAILURE);

	ret = execvp(*argv, argv);
	if (ret == -1) {
		perror("execvp");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
