/*
 * Yusuke Endoh's IOCCC 2012 entry, 2012/endoh1/endoh1.c, compiled
 * unchanged as a function the launcher calls in a child process: its
 * main() is renamed, and each frame is flushed at its usleep() so the
 * launcher receives whole frames through the pipe. Built with -DG=1
 * -DP=4 -DV=8, the author's gravity, pressure and viscosity.
 */
#include <stdio.h>
#include <unistd.h>
#include <complex.h>

int av_usleep(unsigned int us);
#define usleep av_usleep
#define main endoh1_main
#include "../2012/endoh1/endoh1.c"
