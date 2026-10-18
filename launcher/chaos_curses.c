/*
 * What chaos_hooks.h points chaos.c at: its data files from the built-in
 * scenes, and its 79x24 picture shown centred in the terminal.
 *
 * chaos draws on stdscr at fixed places (columns 0 to 78, rows 0 to 23).
 * stdscr becomes a pad of that size as soon as curses starts, so every
 * drawing call works unchanged; refresh() shows the pad in the middle of
 * the terminal, and a resize, which getch() sees as KEY_RESIZE, places it
 * again and blanks the old place.
 *
 * Copyright (c) 2026 Asciiverse. Licensed under CC BY-SA 4.0, like the
 * entry (https://creativecommons.org/licenses/by-sa/4.0/).
 */
#define _DEFAULT_SOURCE
#define _DARWIN_C_SOURCE
#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "av.h"

#define PIC_H 24
#define PIC_W 79

/* set by the launcher before the child runs chaos_main() */
const struct av_scene *av_chaos_scene;

static WINDOW *pad;
static int shown_oy = -1, shown_ox = -1, shown_lines, shown_cols;

FILE *
av_fopen(const char *name, const char *mode)
{
	(void)name;
	(void)mode;
	/* chaos opens only its data file */
	return fmemopen((void *)av_chaos_scene->data, av_chaos_scene->len, "r");
}

WINDOW *
av_initscr(void)
{
	if (initscr() == NULL)
		return NULL;
	pad = newpad(PIC_H, PIC_W);
	if (pad != NULL)
		stdscr = pad;
	return stdscr;
}

int
av_refresh(void)
{
	struct winsize ws;
	int oy, ox, r;

	if (pad == NULL)
		return wrefresh(stdscr);
	/* follow a resize even before getch() reports it */
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 0 &&
	    ws.ws_col > 0 && (ws.ws_row != LINES || ws.ws_col != COLS))
		resize_term(ws.ws_row, ws.ws_col);
	oy = LINES > PIC_H ? (LINES - PIC_H) / 2 : 0;
	ox = COLS > PIC_W ? (COLS - PIC_W) / 2 : 0;
	if (oy != shown_oy || ox != shown_ox || LINES != shown_lines ||
	    COLS != shown_cols) {
		/* moved: blank the whole screen first */
		WINDOW *screen = newwin(0, 0, 0, 0);
		if (screen != NULL) {
			wnoutrefresh(screen);
			delwin(screen);
		}
		shown_oy = oy;
		shown_ox = ox;
		shown_lines = LINES;
		shown_cols = COLS;
	}
	pnoutrefresh(pad, 0, 0, oy, ox,
	    oy + (PIC_H < LINES ? PIC_H : LINES) - 1,
	    ox + (PIC_W < COLS ? PIC_W : COLS) - 1);
	r = doupdate();
	/* ncurses points stdscr back at the screen in doupdate */
	stdscr = pad;
	return r;
}

int
av_getch(void)
{
	int c;

	/*
	 * the launcher gone (killed outright, on macOS where no signal says
	 * so): stop, rather than turn for ever on a terminal nobody owns
	 */
	if (getppid() != av_parent) {
		endwin();
		exit(0);
	}
	if (pad == NULL)
		return wgetch(stdscr);
	/* wgetch does not refresh a pad by itself */
	while ((c = wgetch(pad)) == KEY_RESIZE)
		av_refresh();
	stdscr = pad;
	return c;
}
