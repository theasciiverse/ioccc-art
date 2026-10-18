/*
 * The author's 256-colour version, 2012/endoh1/endoh1_color.c, compiled
 * unchanged like fluid_bw.c. Its one-letter globals are renamed so that
 * both versions link into one program; the system headers come first so
 * the renaming reaches only the entry.
 */
#include <stdio.h>
#include <unistd.h>
#include <complex.h>

int av_usleep(unsigned int us);
#define usleep av_usleep
#define main endoh1_color_main
#define a fc_a
#define b fc_b
#define d fc_d
#define g fc_g
#define h fc_h
#define j fc_j
#define o fc_o
#define p fc_p
#define q fc_q
#define r fc_r
#define t fc_t
#define w fc_w
#define x fc_x
#define y fc_y
#include "../2012/endoh1/endoh1_color.c"
