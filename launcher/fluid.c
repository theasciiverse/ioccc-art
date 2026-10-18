/*
 * ascii-fluid: Yusuke Endoh's ASCII fluid simulator (IOCCC 2012,
 * 2012/endoh1) running its scenes in turn, for the Asciiverse catalog.
 *
 *   ascii-fluid [-m] [-t seconds] [scene or file ...]
 *
 * The entry reads a scene on stdin and then draws for ever; here each
 * scene runs in a child process for its time, then the next starts, round
 * and round, until any key is pressed. The entry draws an 80x25 picture
 * from the top left corner; the launcher reads it through a pipe and puts
 * it in the middle of the terminal, following resizes; on a smaller
 * terminal it shows the middle columns and the bottom lines. -m runs the original black-and-white program instead
 * of the author's 256-colour one, -t sets every scene's time, and names
 * pick built-in scenes (-l lists them) or scene files of your own.
 *
 * Copyright (c) 2026 Asciiverse. Licensed under CC BY-SA 4.0, like the
 * entry it runs (https://creativecommons.org/licenses/by-sa/4.0/).
 */
#define _DEFAULT_SOURCE
#include "av.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fluid-scenes.h"

#define NSCENES (sizeof av_scenes / sizeof av_scenes[0])
/* the entry's picture: 79 columns and a newline, 25 lines */
#define PIC_W 79
#define PIC_H 25

int endoh1_main();
int endoh1_color_main();

int
av_usleep(unsigned int us)
{
	fflush(stdout);
	return usleep(us);
}

struct run {
	const struct av_scene *sc;
	int mono;
};

/* the child: the scene arrives on stdin, written by the parent */
static int
child(void *arg)
{
	struct run *r = arg;

	return r->mono ? endoh1_main() : endoh1_color_main();
}

/*
 * The filter from the entry's output to the terminal. The entry redraws
 * its whole picture every frame, from home, line by line, the colour
 * version with a colour code before every cell (some 24 KB a frame).
 * The filter reads each picture into cells and, when it is complete,
 * sends the terminal only the cells that changed, centred.
 */
struct cell {
	unsigned char ch;
	short colour;		/* 256-colour background, -1 for none */
};

static struct {
	int oy, ox;		/* where the picture goes */
	int sy, sx;		/* lines and columns of it cut off */
	int vh, vw;		/* how much of it fits */
	int line, col;		/* where the entry is writing in its picture */
	short colour;		/* the colour it set last */
	int esc;		/* 0, 1 after ESC, 2 inside CSI */
	char seq[32];
	int seqlen;
	int redraw;		/* the terminal's copy is gone: send all */
	struct cell pic[PIC_H][PIC_W], shown[PIC_H][PIC_W];
	char out[1 << 16];
	size_t outlen;
} f;

static void
emit(const char *s, size_t n)
{
	if (f.outlen + n > sizeof f.out) {
		av_write(f.out, f.outlen);
		f.outlen = 0;
	}
	memcpy(f.out + f.outlen, s, n);
	f.outlen += n;
}

static void
place(void)
{
	int rows, cols;

	av_size(&rows, &cols);
	f.oy = rows > PIC_H ? (rows - PIC_H) / 2 : 0;
	f.ox = cols > PIC_W ? (cols - PIC_W) / 2 : 0;
	/* too small: keep the middle columns and the bottom lines, where the
	   fluid settles */
	f.sy = rows < PIC_H ? PIC_H - rows : 0;
	f.sx = cols < PIC_W ? (PIC_W - cols) / 2 : 0;
	f.vh = rows < PIC_H ? rows : PIC_H;
	f.vw = cols < PIC_W ? cols : PIC_W;
	f.redraw = 1;
	av_puts("\033[0m\033[2J");
}

/* a whole picture has arrived: send what changed */
static void
show(void)
{
	char buf[48];
	int n, colour = -2;	/* -2: the terminal's colour is not known */

	for (int y = f.sy; y < f.sy + f.vh; y++) {
		int at = -1;	/* the column the cursor is at, if known */
		for (int x = f.sx; x < f.sx + f.vw; x++) {
			struct cell *c = &f.pic[y][x], *o = &f.shown[y][x];
			if (!f.redraw && c->ch == o->ch && c->colour == o->colour)
				continue;
			if (at != x) {
				n = snprintf(buf, sizeof buf, "\033[%d;%dH",
				    f.oy + y - f.sy + 1, f.ox + x - f.sx + 1);
				emit(buf, (size_t)n);
			}
			if (c->colour != colour) {
				if (c->colour < 0)
					n = snprintf(buf, sizeof buf, "\033[0m");
				else
					n = snprintf(buf, sizeof buf,
					    "\033[48;5;%dm", c->colour);
				emit(buf, (size_t)n);
				colour = c->colour;
			}
			emit((const char *)&c->ch, 1);
			*o = *c;
			at = x + 1;
		}
	}
	if (colour != -2 && colour != -1)
		emit("\033[0m", 4);
	f.redraw = 0;
	av_write(f.out, f.outlen);
	f.outlen = 0;
}

static void
filter(const unsigned char *s, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		unsigned char c = s[i];

		if (f.esc == 1) {
			f.esc = c == '[' ? 2 : 0;
			f.seqlen = 0;
			continue;
		}
		if (f.esc == 2) {
			if (f.seqlen < (int)sizeof f.seq - 1)
				f.seq[f.seqlen++] = (char)c;
			if (c < 0x40 || c > 0x7e)
				continue;
			f.esc = 0;
			f.seq[f.seqlen] = '\0';
			if (c == 'H') {
				/* the entry goes home: a new picture */
				f.line = 0;
				f.col = 0;
			} else if (c == 'm') {
				int v;
				f.colour = sscanf(f.seq, "48;5;%dm", &v) == 1 &&
				    v >= 0 && v < 256 ? (short)v : -1;
			}
			/* the clear before the first picture is ours already */
			continue;
		}
		if (c == 033) {
			f.esc = 1;
		} else if (c == '\n') {
			if (f.line < PIC_H && ++f.line == PIC_H)
				show();
			f.col = 0;
		} else if (f.line < PIC_H) {
			if (f.col < PIC_W) {
				f.pic[f.line][f.col].ch = c < ' ' || c > '~' ? ' ' : c;
				f.pic[f.line][f.col].colour = f.colour;
			}
			f.col++;
		}
	}
}

/* plays one scene; returns 1 when a key (or a signal) ends the show */
static int
play(const struct av_scene *sc, int mono, int seconds)
{
	struct run r = { sc, mono };
	int in, out, ready, quit = 0;
	size_t sent = 0;
	long long end = av_now() + (long long)seconds * 1000;
	pid_t pid = av_spawn(child, &r, &in, &out);

	if (pid < 0)
		return 1;
	/* nothing is drawn before the entry first goes home ("GO") */
	f.line = PIC_H;
	f.esc = 0;
	f.colour = -1;
	place();
	/* the scene goes in in pieces, so a big one cannot block us */
	while (!quit) {
		long long left = end - av_now();
		if (left <= 0)
			break;
		if (sent < sc->len) {
			ssize_t n = write(in, sc->data + sent,
			    sc->len - sent > 4096 ? 4096 : sc->len - sent);
			if (n > 0)
				sent += (size_t)n;
			if (sent >= sc->len || n < 0) {
				close(in);
				in = -1;
				sent = sc->len;
			}
		}
		quit = av_wait(out, left > 50 ? 50 : (int)left, &ready);
		if (av_stopped())
			quit = 1;
		if (av_resized())
			place();
		if (ready) {
			unsigned char buf[65536];
			ssize_t n = read(out, buf, sizeof buf);
			if (n <= 0)
				break;	/* the entry ended (an empty scene) */
			filter(buf, (size_t)n);
		}
	}
	if (in >= 0)
		close(in);
	close(out);
	av_reap(pid);
	return quit;
}

static void
usage(void)
{
	fprintf(stderr, "usage: ascii-fluid [-m] [-t seconds] [-l] [scene or file ...]\n");
	exit(2);
}

int
main(int argc, char **argv)
{
	int mono = 0, seconds = 0, c;
	struct av_scene *list;
	size_t n = 0;

	while ((c = getopt(argc, argv, "mt:l")) != -1) {
		switch (c) {
		case 'm':
			mono = 1;
			break;
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
			fprintf(stderr, "ascii-fluid: no scene or file %s\n", argv[i]);
			return 1;
		}
	}
	if (n == 0)
		for (; n < NSCENES; n++)
			list[n] = av_scenes[n];

	av_term_start();
	for (size_t i = 0;; i = (i + 1) % n)
		if (play(&list[i], mono, seconds ? seconds : list[i].seconds))
			break;
	av_term_end();
	free(list);
	return 0;
}
