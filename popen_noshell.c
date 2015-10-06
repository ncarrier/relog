/*
 * wordexp.c
 *
 *  Created on: Oct 1, 2015
 *      Author: ncarrier
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif /* _GNU_SOURCE */
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <wordexp.h>

#include "popen_noshell.h"

#ifndef POPEN_NOSHELL_MAX
#define POPEN_NOSHELL_MAX 10
#endif /* POPEN_NOSHELL_MAX */

static struct {
	pid_t pid;
	FILE *file;
} map[POPEN_NOSHELL_MAX];

struct popen_noshell_ctx {
	int flags;
	/* in direction is the standard input from the forked process' POV */
	bool in;
	int pipefd[2];
};

static int popen_noshell_configure_ctx(struct popen_noshell_ctx *ctx, const char *type)
{
	char c0;
	char c1;

	if (type == NULL || type[0] == '\0')
		return -EINVAL;

	c0 = type[0];
	c1 = type[1];
	if (c0 == 'w') {
		ctx->in = true;
		if (c1 == 'e')
			ctx->flags = FD_CLOEXEC;
		else if (c1 != '\0')
			return -EINVAL;
	} else if (c0 == 'r') {
		ctx->in = false;
		if (c1 == 'e')
			ctx->flags = FD_CLOEXEC;
		else if (c1 != '\0')
			return -EINVAL;
	} else if (c0 == 'e') {
		ctx->flags = FD_CLOEXEC;
		if (c1 == 'w')
			ctx->in = true;
		else if (c1 == 'r')
			ctx->in = false;
		else if (c1 != '\0')
			return -EINVAL;
	} else {
		return -EINVAL;
	}

	return 0;
}

__attribute__((noreturn))
static void launch_command(const struct popen_noshell_ctx *ctx,
		const wordexp_t *we)
{
	int ret;
	int oldfd;
	int newfd;

	if (ctx->in) {
		oldfd = ctx->pipefd[0];
		newfd = STDIN_FILENO;
	} else {
		oldfd = ctx->pipefd[1];
		newfd = STDOUT_FILENO;
	}
	ret = dup2(oldfd, newfd);
	close(ctx->pipefd[0]);
	close(ctx->pipefd[1]);
	if (ret == -1)
		_exit(EXIT_FAILURE);

	ret = execvp(we->we_wordv[0], we->we_wordv);
	if (ret == -1)
		if (errno == ENOENT)
			_exit(127);

	_exit(EXIT_FAILURE);
}

static int popen_noshell_store(FILE *file, pid_t pid)
{
	int i;

	for (i = 0; i < POPEN_NOSHELL_MAX; i++)
		if (map[i].file == NULL) {
			map[i].file = file;
			map[i].pid = pid;
			return i;
		}

	return -ENOMEM;
}

static int popen_noshell_find(FILE *file)
{
	int i;

	for (i = 0; i < POPEN_NOSHELL_MAX; i++)
		if (map[i].file == file)
			return i;

	return -ENOENT;
}

static void pclose_noshell_unstore(int i)
{
	int upper_bound = POPEN_NOSHELL_MAX - 2;
	map[i].file = NULL;
	map[i].pid = 0;

	for (; i < upper_bound; i++)
		map[i] = map[i + 1];
}

FILE *popen_noshell(const char *command, const char *type)
{
	struct popen_noshell_ctx ctx = {.pipefd = {-1, -1},};
	wordexp_t we;
	int ret;
	FILE *result = NULL;
	pid_t pid = -1;
	int fd;

	ret = popen_noshell_configure_ctx(&ctx, type);
	if (ret < 0) {
		errno = ret;
		return NULL;
	}

	ret = wordexp(command, &we, 0);
	if (ret != 0) {
		errno = ret == WRDE_NOSPACE ? ENOMEM : EINVAL;
		return NULL;
	}

	ret = pipe2(ctx.pipefd, ctx.flags);
	if (ret < 0)
		return NULL;

	fd = ctx.in ? ctx.pipefd[1] : ctx.pipefd[0];
	result = fdopen(fd, ctx.in ? "w" : "r");
	if (result  == NULL)
		goto err;

	pid = fork();
	if (pid == -1)
		goto err;
	if (pid == 0) /* in child */
		launch_command(&ctx, &we);

	close(ctx.in ? ctx.pipefd[0] : ctx.pipefd[1]);
	ret = popen_noshell_store(result, pid);
	if (ret < 0)
		goto err;

	return result;
err:
	if (pid != -1) {
		kill(pid, SIGKILL);
		waitpid(pid, NULL, 0);
	}
	if (result != NULL)
		fclose(result);
	if (ctx.pipefd[0] != -1)
		close(ctx.pipefd[0]);
	if (ctx.pipefd[1] != -1)
		close(ctx.pipefd[1]);

	return NULL;
}

int pclose_noshell(FILE *file)
{
	int ret;
	int i;
	int status;
	pid_t pid;

	i = popen_noshell_find(file);
	if (i < 0) {
		errno = -i;
		return -1;
	}

	pid = map[i].pid;
	pclose_noshell_unstore(i);

	fclose(file);

	ret = waitpid(pid, &status, 0);
	if (ret == -1)
		return -1;

	return status;
}
