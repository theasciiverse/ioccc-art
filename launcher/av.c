/*
 * Asciiverse launcher for IOCCC art: terminal, keys and children (av.h).
 *
 * Copyright (c) 2026 Asciiverse. Licensed under CC BY-SA 4.0, like the
 * entries it runs (https://creativecommons.org/licenses/by-sa/4.0/).
 */
#define _DEFAULT_SOURCE
#define _DARWIN_C_SOURCE
#include "av.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif
#include <sys/wait.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

pid_t av_parent;
static struct termios av_orig;
static int av_started;
static volatile sig_atomic_t av_winch, av_stop;

static void
av_onwinch(int sig)
{
	(void)sig;
	av_winch = 1;
}

static void
av_onstop(int sig)
{
	(void)sig;
	av_stop = 1;
}

void
av_write(const void *buf, size_t len)
{
	const char *p = buf;

	while (len > 0) {
		ssize_t n = write(STDOUT_FILENO, p, len);
		if (n < 0) {
			if (errno == EINTR)
				continue;
			return;
		}
		p += n;
		len -= (size_t)n;
	}
}

void
av_puts(const char *s)
{
	av_write(s, strlen(s));
}

void
av_term_raw(void)
{
	struct termios t = av_orig;

	t.c_lflag &= ~(ICANON | ECHO | ISIG | IEXTEN);
	t.c_iflag &= ~(IXON | ICRNL);
	t.c_cc[VMIN] = 1;
	t.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

void
av_term_start(void)
{
	struct sigaction sa;

	if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
		fprintf(stderr, "needs a terminal\n");
		exit(1);
	}
	tcgetattr(STDIN_FILENO, &av_orig);
	av_started = 1;
	av_term_raw();

	memset(&sa, 0, sizeof sa);
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = av_onwinch;
	sigaction(SIGWINCH, &sa, NULL);
	sa.sa_handler = av_onstop;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGHUP, &sa, NULL);
	signal(SIGPIPE, SIG_IGN);

	/* alternate screen, cursor hidden, no line wrap, blank */
	av_puts("\033[?1049h\033[?25l\033[?7l\033[0m\033[2J");
}

void
av_term_end(void)
{
	if (!av_started)
		return;
	av_started = 0;
	/* CAN ends any escape sequence a killed child left half written */
	av_puts("\030\033[0m\033[2J\033[?7h\033[?25h\033[?1l\033>\033[?1049l");
	tcsetattr(STDIN_FILENO, TCSANOW, &av_orig);
}

void
av_size(int *rows, int *cols)
{
	struct winsize ws;

	*rows = 24;
	*cols = 80;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
		if (ws.ws_row > 0)
			*rows = ws.ws_row;
		if (ws.ws_col > 0)
			*cols = ws.ws_col;
	}
}

int
av_resized(void)
{
	if (!av_winch)
		return 0;
	av_winch = 0;
	return 1;
}

int
av_stopped(void)
{
	return av_stop != 0;
}

long long
av_now(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

int
av_wait(int fd, int ms, int *ready)
{
	struct pollfd p[2];
	int n = 1;

	*ready = 0;
	p[0].fd = STDIN_FILENO;
	p[0].events = POLLIN;
	p[0].revents = 0;
	if (fd >= 0) {
		p[1].fd = fd;
		p[1].events = POLLIN;
		p[1].revents = 0;
		n = 2;
	}
	if (poll(p, (nfds_t)n, ms < 0 ? 0 : ms) <= 0)
		return 0;
	if (n == 2 && (p[1].revents & (POLLIN | POLLHUP)))
		*ready = 1;
	if (p[0].revents & (POLLIN | POLLHUP | POLLERR)) {
		char buf[64];
		/* a key, or the terminal went away: both end the show */
		if (read(STDIN_FILENO, buf, sizeof buf) < 0)
			buf[0] = 0;
		return 1;
	}
	return 0;
}

pid_t
av_spawn(int (*fn)(void *), void *arg, int *in, int *out)
{
	int ip[2] = { -1, -1 }, op[2] = { -1, -1 };
	pid_t pid;

	if ((in && pipe(ip) < 0) || (out && pipe(op) < 0))
		return -1;
	av_parent = getpid();
	pid = fork();
	if (pid < 0)
		return -1;
	if (pid == 0) {
#ifdef __linux__
		/* the launcher killed outright takes its child along */
		prctl(PR_SET_PDEATHSIG, SIGKILL);
#endif
		signal(SIGWINCH, SIG_DFL);
		signal(SIGINT, SIG_DFL);
		signal(SIGTERM, SIG_DFL);
		signal(SIGHUP, SIG_DFL);
		signal(SIGPIPE, SIG_DFL);
		if (in) {
			dup2(ip[0], STDIN_FILENO);
			close(ip[0]);
			close(ip[1]);
		} else {
			int fd = open("/dev/null", O_RDONLY);
			if (fd >= 0) {
				dup2(fd, STDIN_FILENO);
				close(fd);
			}
		}
		if (out) {
			dup2(op[1], STDOUT_FILENO);
			close(op[0]);
			close(op[1]);
		}
		{
			/* the entries' complaints would land on the picture */
			int fd = open("/dev/null", O_WRONLY);
			if (fd >= 0) {
				dup2(fd, STDERR_FILENO);
				close(fd);
			}
		}
		_exit(fn(arg));
	}
	if (in) {
		close(ip[0]);
		*in = ip[1];
	}
	if (out) {
		close(op[1]);
		*out = op[0];
	}
	return pid;
}

void
av_reap(pid_t pid)
{
	if (pid <= 0)
		return;
	kill(pid, SIGKILL);
	while (waitpid(pid, NULL, 0) < 0 && errno == EINTR)
		;
}

int
av_load(struct av_scene *sc, const char *path, int seconds)
{
	FILE *f = fopen(path, "rb");
	unsigned char *buf = NULL;
	size_t len = 0, cap = 0, n;

	if (f == NULL)
		return 0;
	for (;;) {
		if (len == cap) {
			unsigned char *nb;
			cap = cap ? cap * 2 : 65536;
			if ((nb = realloc(buf, cap)) == NULL) {
				free(buf);
				fclose(f);
				return 0;
			}
			buf = nb;
		}
		n = fread(buf + len, 1, cap - len, f);
		if (n == 0)
			break;
		len += n;
	}
	fclose(f);
	sc->name = path;
	sc->data = buf;
	sc->len = len;
	sc->seconds = seconds;
	return 1;
}
