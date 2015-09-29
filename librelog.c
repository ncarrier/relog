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

#define LIBRELOG_OUTPROCESS_ENVIRONMENT "RELOG_OUTPROCESS"

static FILE *out_process_pipe = NULL;
static int old_stdout_fileno = -1;

__attribute__((constructor)) void init_relog(void)
{
	const char *old_ld_preload;
	int ret;
	int outfd = -1;
	int errfd = -1;
	const char *outfile = getenv(LIBRELOG_OUTFILE_ENVIRONMENT);
	const char *errfile = getenv(LIBRELOG_ERRFILE_ENVIRONMENT);
	const char *outprocess = getenv(LIBRELOG_OUTPROCESS_ENVIRONMENT);

	/* avoid creating a fork bomb, hahem... */
	old_ld_preload = getenv("LD_PRELOAD");
	if (old_ld_preload != NULL)
		unsetenv("LD_PRELOAD");

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
	}
	if (outprocess != NULL) {

		out_process_pipe = popen(outprocess, "w");
		if (outprocess == NULL) {
			fprintf(stderr, "%d popen: %m\n", __LINE__);
			goto err;
		}
		/* backup 1 */
		old_stdout_fileno = dup(STDOUT_FILENO);
		if (old_stdout_fileno == -1) {
			fprintf(stderr, "%d dup: %m\n", __LINE__);
			goto err;
		}
		ret = dup2(fileno(out_process_pipe), STDOUT_FILENO);
		if (ret == -1) {
			fprintf(stderr, "%d dup2: %m\n", __LINE__);
			goto err;
		}
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
	}

	setlinebuf(stderr);
	setlinebuf(stdout);

	if (old_ld_preload != NULL)
		setenv("LD_PRELOAD", old_ld_preload, 1);

	return;
err:
	if (outfd != -1)
		close(outfd);
	if (errfd != -1)
		close(errfd);
}

__attribute__((destructor)) void clean_relog(void)
{
	if (out_process_pipe != NULL)
		pclose(out_process_pipe);
	dup2(old_stdout_fileno, STDOUT_FILENO);
}
