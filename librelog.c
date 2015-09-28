/*
 * librelog.c
 *
 *  Created on: Sep 28, 2015
 *      Author: ncarrier
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>

#define LIBRELOG_OUTFILE_ENVIRONMENT "RELOG_OUTFILE"
#define LIBRELOG_ERRFILE_ENVIRONMENT "RELOG_ERRFILE"

__attribute__((constructor)) void init_relog(void)
{
	int ret;
	int outfd = -1;
	int errfd = -1;
	const char *outfile = getenv(LIBRELOG_OUTFILE_ENVIRONMENT);
	const char *errfile = getenv(LIBRELOG_ERRFILE_ENVIRONMENT);

	if (outfile != NULL) {
		outfd = open(outfile, O_WRONLY);
		if (outfd == -1) {
			fprintf(stderr, "%d open: %m\n", __LINE__);
			goto err;
		}

		ret = dup2(outfd, STDOUT_FILENO);
		if (ret == -1) {
			fprintf(stderr, "%d dup2: %m\n", __LINE__);
			goto err;
		}
		setlinebuf(stdout);
	}
	if (errfile != NULL) {
		errfd = open(errfile, O_WRONLY);
		if (errfd == -1) {
			fprintf(stderr, "%d open: %m\n", __LINE__);
			goto err;
		}

		ret = dup2(errfd, STDERR_FILENO);
		if (ret == -1) {
			fprintf(stderr, "%d dup2: %m\n", __LINE__);
			goto err;
		}
		setlinebuf(stderr);
	}

	return;
err:
	if (outfd != -1)
		close(outfd);
	if (errfd != -1)
		close(errfd);
}
