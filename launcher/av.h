/*
 * Asciiverse launcher for IOCCC art: the part both titles share.
 *
 * The entry runs in a forked child, unchanged; the launcher keeps the
 * terminal: it switches to the alternate screen, reads the keyboard itself
 * (any key quits), ends each scene after its time and starts the next,
 * and puts the terminal back however it ends (a key, SIGINT, SIGTERM,
 * SIGHUP).
 *
 * Copyright (c) 2026 Asciiverse. Licensed under CC BY-SA 4.0, like the
 * entries it runs (https://creativecommons.org/licenses/by-sa/4.0/).
 */
#ifndef AV_H
#define AV_H

#include <stddef.h>
#include <sys/types.h>

/* a scene built into the binary (asciiverse/embed.sh), or read from disk */
struct av_scene {
	const char *name;
	const unsigned char *data;
	size_t len;
	int seconds;		/* how long it plays before the next */
};

/* puts the terminal in raw mode on the alternate screen; exits on error */
void av_term_start(void);
/* puts it back; safe to call twice */
void av_term_end(void);
/* raw mode again, after a child (curses) changed the modes */
void av_term_raw(void);

/* the terminal's size, at least 1x1 */
void av_size(int *rows, int *cols);
/* true once after each SIGWINCH */
int av_resized(void);
/* true once SIGINT, SIGTERM or SIGHUP came */
int av_stopped(void);

/*
 * Waits up to ms milliseconds for a key or for fd (-1 for none) to have
 * data. Returns 1 when a key was pressed (it is read and dropped), else 0;
 * *ready is set when fd has data.
 */
int av_wait(int fd, int ms, int *ready);

/* monotonic milliseconds */
long long av_now(void);

/*
 * Forks a child that runs fn(arg) and exits with its result. With in, the
 * child's stdin is a pipe and *in its write end, else /dev/null; with out,
 * its stdout is a pipe and *out the read end, else the terminal.
 */
pid_t av_spawn(int (*fn)(void *), void *arg, int *in, int *out);
/* in a child: the launcher's pid, to notice when it is gone */
extern pid_t av_parent;
/* kills a child and waits for it */
void av_reap(pid_t pid);

/* writes all of buf to the terminal */
void av_write(const void *buf, size_t len);
void av_puts(const char *s);

/* reads a whole file into a scene; 0 when it cannot be read */
int av_load(struct av_scene *sc, const char *path, int seconds);

#endif
