/*
 * ioccc-chaos: Alan De Smet's curses 3D wireframe viewer (IOCCC 1998,
 * 1998/chaos) turning its five objects in turn, for the Asciiverse
 * catalog.
 *
 *   ioccc-chaos [-t seconds] [-l] [object or file ...]
 *
 * The entry shows the object in a data file named on its command line and
 * turns it on the keys y, u, i, h, j and k (a and z zoom), or by itself
 * after three seconds without a key. Here it runs in a child process with
 * its keys coming from the launcher, which tumbles the object about
 * different axes in turn, moves on to the next object after its time and
 * keeps the real keyboard for itself: any key quits. -t sets every
 * object's time; names pick built-in objects (-l lists them) or data
 * files of your own, in the entry's format.
 *
 * Copyright (c) 2026 Asciiverse. Licensed under CC BY-SA 4.0, like the
 * entry it runs (https://creativecommons.org/licenses/by-sa/4.0/).
 */
#define _DEFAULT_SOURCE
#include "av.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "chaos-scenes.h"

#define NSCENES (sizeof av_scenes / sizeof av_scenes[0])

int chaos_main(int, char **);
extern const struct av_scene *av_chaos_scene;

/*
 * The tumbling: each move is a string of the entry's keys sent one at a
 * time, one every TICK ms, for MOVE ms, then the next move. Every key
 * turns the object by 6 degrees, and h and y, j and u, k and i turn it
 * opposite ways about the same axis (a and z zoom).
 */
#define TICK 80
#define MOVE 4000
static const char *moves[] = { "u", "uk", "h", "jh", "k", "ih", "y", "jk" };
#define NMOVES (sizeof moves / sizeof moves[0])

/*
 * How each built-in object starts (keys sent at once) and, where it is
 * not the tumbling above, how it moves (a pattern of keys, round and
 * round). The IOCCC letters lie edge-on at first: turned 90 degrees twice
 * and brought closer they read IOCCC, then sway 48 degrees each way.
 */
static const struct look {
	const char *name, *setup, *pattern;
} looks[] = {
	{ "cube", "hhhhuuuu", NULL },
	{ "pyramid", "hhhhhh", NULL },
	{ "desk", "hhhh", NULL },
	{ "xwing", "aaaaaaaahhhhhh", NULL },
	{ "ioccc", "uuuuuuuuuuuuuuukkkkkkkkkkkkkkkaaaaaaaaaaaaaaaaaaaa",
	    "uuuuuuuujjjjjjjjjjjjjjjjuuuuuuuu" },
};

static int
child(void *arg)
{
	char *argv[] = { "chaos", (char *)((const struct av_scene *)arg)->name,
	    NULL };

	av_chaos_scene = arg;
	return chaos_main(2, argv);
}

/* plays one object; returns 1 when a key (or a signal) ends the show */
static int
play(const struct av_scene *sc, int seconds)
{
	int in, ready, quit = 0, status;
	long long start = av_now(), end = start + (long long)seconds * 1000;
	long long next = start + TICK;
	unsigned long tick = 0;
	const struct look *look = NULL;
	pid_t pid;

	for (size_t i = 0; i < sizeof looks / sizeof looks[0]; i++)
		if (strcmp(sc->name, looks[i].name) == 0)
			look = &looks[i];
	/* CAN ends any escape sequence the last child left half written */
	av_puts("\030\033[0m\033[2J");
	if ((pid = av_spawn(child, (void *)sc, &in, NULL)) < 0)
		return 1;
	if (look && write(in, look->setup, strlen(look->setup)) < 0)
		quit = 0;
	while (!quit) {
		long long now = av_now(), wait;

		if (now >= end)
			break;
		if (now >= next) {
			const char *m = look && look->pattern ? look->pattern :
			    moves[((now - start) / MOVE) % NMOVES];
			char k = m[tick++ % strlen(m)];
			if (write(in, &k, 1) < 0)
				break;
			next += TICK;
		}
		wait = next - av_now();
		quit = av_wait(-1, wait < 0 ? 0 : wait > TICK ? TICK : (int)wait,
		    &ready);
		if (av_stopped())
			quit = 1;
		(void)av_resized();	/* the child's curses follows it */
		if (waitpid(pid, &status, WNOHANG) == pid) {
			/* the entry gave up (a bad data file): next object */
			pid = -1;
			break;
		}
	}
	close(in);
	av_reap(pid);
	av_term_raw();
	return quit;
}

static void
usage(void)
{
	fprintf(stderr, "usage: ioccc-chaos [-t seconds] [-l] [object or file ...]\n");
	exit(2);
}

int
main(int argc, char **argv)
{
	int seconds = 0, c;
	struct av_scene *list;
	size_t n = 0;

	while ((c = getopt(argc, argv, "t:l")) != -1) {
		switch (c) {
		case 't':
			seconds = atoi(optarg);
			if (seconds < 1)
				usage();
			break;
		case 'l':
			for (size_t i = 0; i < NSCENES; i++)
				printf("%s\n", av_scenes[i].name);
			return 0;
		default:
			usage();
		}
	}
	list = calloc(NSCENES + (size_t)argc, sizeof *list);
	if (list == NULL)
		return 1;
	for (int i = optind; i < argc; i++) {
		size_t k;
		for (k = 0; k < NSCENES; k++)
			if (strcmp(argv[i], av_scenes[k].name) == 0)
				break;
		if (k < NSCENES)
			list[n++] = av_scenes[k];
		else if (av_load(&list[n], argv[i], 30))
			n++;
		else {
			fprintf(stderr, "ioccc-chaos: no object or file %s\n", argv[i]);
			return 1;
		}
	}
	if (n == 0)
		for (; n < NSCENES; n++)
			list[n] = av_scenes[n];

	av_term_start();
	for (size_t i = 0;; i = (i + 1) % n)
		if (play(&list[i], seconds ? seconds : list[i].seconds))
			break;
	av_term_end();
	free(list);
	return 0;
}
