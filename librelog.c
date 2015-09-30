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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "relog.h"

struct relog_process {
	const char *cmd;
	FILE *pipe;
	int old_fd_value;
	int old_fd_backup;
};

// TODO replace popen by fork exec to gain one process
// TODO use only one global

static struct relog_process out_process = {
		.old_fd_value = STDOUT_FILENO,
		.old_fd_backup = -1,
};
static struct relog_process err_process = {
		.old_fd_value = STDERR_FILENO,
		.old_fd_backup = -1,
};

static bool same_errprocess;
static int stderr_fd_backup = -1;

/* file: file to redirect to, fd: stream redirected */
/* no _cleanup, the normal C standard streams cleanup will take place */
static int relog_file_init(const char *file, int fd)
{
	int newfd;
	int ret = -1;

	if (file == NULL || fd < 0)
		return -1;

	newfd = open(file, O_WRONLY);
	if (newfd == -1) {
		fprintf(stderr, "%d open: %m\n", __LINE__);
		goto err;
	}

	ret = dup2(newfd, fd);
	if (ret == -1) {
		fprintf(stderr, "%d dup2: %m\n", __LINE__);
		goto err;
	}

	return 0;
err:
	if (newfd != -1)
		close(newfd);

	return ret;
}

static void relog_process_clean(struct relog_process *process)
{
	if (process->pipe == NULL)
		return;

	/* close the dup'ed fd of the process, for pclose will terminate it */
	close(process->old_fd_value);
	pclose(process->pipe);

	if (process->old_fd_backup == -1)
		return;

	dup2(process->old_fd_backup, process->old_fd_value);
	close(process->old_fd_backup);
}

static int relog_process_init(struct relog_process *process)
{
	int ret = -1;

	if (process->cmd == NULL || *process->cmd == '\0') {
		fprintf(stderr, "%d %s: %s\n", __LINE__, __func__,
				strerror(EINVAL));
		return -1;
	}
	process->pipe = popen(process->cmd, "w");
	if (process->pipe == NULL) {
		fprintf(stderr, "%d popen: %m\n", __LINE__);
		return -1;
	}
	/* backup the standard file descriptor */
	process->old_fd_backup = dup(process->old_fd_value);
	if (process->old_fd_backup == -1) {
		fprintf(stderr, "%d dup: %m\n", __LINE__);
		goto err;
	}
	ret = dup2(fileno(process->pipe), process->old_fd_value);
	if (ret == -1) {
		fprintf(stderr, "%d dup2: %m\n", __LINE__);
		goto err;
	}

	return 0;
err:
	relog_process_clean(process);

	return ret;
}

static __attribute__((constructor)) void relog_init(void)
{
	int ret;
	const char *old_ld_preload;

	relog_file_init(getenv(RELOG_OUTFILE_ENV), STDOUT_FILENO);
	relog_file_init(getenv(RELOG_ERRFILE_ENV), STDERR_FILENO);

	/* avoid creating a fork bomb, hahem... */
	// TODO properly remove relog from LD_PRELOAD, don't reinstall it
	old_ld_preload = getenv("LD_PRELOAD");
	if (old_ld_preload != NULL)
		unsetenv("LD_PRELOAD");

	same_errprocess = getenv(RELOG_SAME_ERRPROCESS_ENV) != NULL;
	out_process.cmd = getenv(RELOG_OUTPROCESS_ENV);
	if (out_process.cmd != NULL) {
		relog_process_init(&out_process);
		if (same_errprocess) {
			stderr_fd_backup = dup(STDERR_FILENO);
			if (stderr_fd_backup == -1) {
				fprintf(stderr, "%d dup: %m\n", __LINE__);
			} else {
				ret = dup2(fileno(out_process.pipe),
						STDERR_FILENO);
				if (ret < 0)
					fprintf(stderr, "%d dup2: %m\n",
							__LINE__);
			}
		}
	}
	err_process.cmd = getenv(RELOG_ERRPROCESS_ENV);
	if (err_process.cmd != NULL && !same_errprocess)
		relog_process_init(&err_process);

	setlinebuf(stderr);
	setlinebuf(stdout);

	if (old_ld_preload != NULL)
		setenv("LD_PRELOAD", old_ld_preload, 1);
}

static __attribute__((destructor)) void relog_clean(void)
{
	/* restores stderr plus closes the snd ref on the process' pipe */
	if (same_errprocess && out_process.cmd != NULL)
		dup2(stderr_fd_backup, STDERR_FILENO);

	relog_process_clean(&out_process);
	relog_process_clean(&err_process);
}
