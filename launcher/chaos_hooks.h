/*
 * Included before Alan De Smet's IOCCC 1998 entry, 1998/chaos/chaos.c,
 * which is compiled unchanged (-include launcher/chaos_hooks.h): its
 * main() becomes a function the launcher runs in a child process,
 * fopen() finds the built-in data files (chaos_curses.c), and curses
 * draws through a pad that is shown centred in the terminal.
 *
 * Copyright (c) 2026 Asciiverse. Licensed under CC BY-SA 4.0, like the
 * entry (https://creativecommons.org/licenses/by-sa/4.0/).
 */
#ifndef CHAOS_HOOKS_H
#define CHAOS_HOOKS_H

#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FILE *av_fopen(const char *name, const char *mode);
WINDOW *av_initscr(void);
int av_refresh(void);
int av_getch(void);

#define main chaos_main
#define fopen av_fopen
#undef initscr
#define initscr av_initscr
#undef refresh
#define refresh av_refresh
#undef getch
#define getch av_getch

#endif
