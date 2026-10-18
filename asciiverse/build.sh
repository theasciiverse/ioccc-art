#!/bin/sh
# Builds ascii-fluid and ioccc-chaos into out/ with the compiler, flags and
# curses library from the environment, so one script serves every target
# (see .github/workflows/release.yml):
#
#   asciiverse/build.sh                                   # Debian, Ubuntu
#   LIBS=-lncursesw LDFLAGS=-static asciiverse/build.sh   # Alpine, static
#   LIBS=-lncurses asciiverse/build.sh                    # macOS
#
# The entries' sources are compiled as they are in 2012/ and 1998/, with
# warnings off (they are obfuscated on purpose); launcher/ wraps them.
set -eu
cd "$(dirname "$0")/.."
CC="${CC:-cc}"
CFLAGS="${CFLAGS:--O2}"
LDFLAGS="${LDFLAGS:-}"
LIBS="${LIBS:--lncursesw}"
STD="-std=gnu11"
WARN="-Wall -Wextra"
O=out
mkdir -p $O

asciiverse/embed.sh asciiverse/fluid-scenes.txt $O/fluid-scenes.h
asciiverse/embed.sh asciiverse/chaos-scenes.txt $O/chaos-scenes.h

$CC $STD $CFLAGS $WARN -c launcher/av.c -o $O/av.o

# ascii-fluid: the author's gravity, pressure and viscosity factors
ENDOH="-DG=1 -DP=4 -DV=8"
$CC $STD $CFLAGS -w $ENDOH -c launcher/fluid_bw.c -o $O/fluid_bw.o
$CC $STD $CFLAGS -w $ENDOH -c launcher/fluid_color.c -o $O/fluid_color.o
$CC $STD $CFLAGS $WARN -I$O -c launcher/fluid.c -o $O/fluid.o
$CC $CFLAGS $O/fluid.o $O/av.o $O/fluid_bw.o $O/fluid_color.o $LDFLAGS -lm \
	-o $O/ascii-fluid
echo "built ascii-fluid"

# ioccc-chaos
$CC $STD $CFLAGS -w -include launcher/chaos_hooks.h -c 1998/chaos/chaos.c \
	-o $O/chaos.o
$CC $STD $CFLAGS $WARN -Ilauncher -c launcher/chaos_curses.c -o $O/chaos_curses.o
$CC $STD $CFLAGS $WARN -I$O -Ilauncher -c launcher/chaos.c -o $O/chaos-main.o
$CC $CFLAGS $O/chaos-main.o $O/av.o $O/chaos_curses.o $O/chaos.o $LDFLAGS \
	$LIBS -o $O/ioccc-chaos 2>/dev/null ||
	$CC $CFLAGS $O/chaos-main.o $O/av.o $O/chaos_curses.o $O/chaos.o $LDFLAGS \
		$LIBS -ltinfo -o $O/ioccc-chaos
echo "built ioccc-chaos"
