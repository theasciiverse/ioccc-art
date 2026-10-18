# Changes from the IOCCC entries

This repository packages two IOCCC winners as Art & Demos titles in the
Asciiverse catalog (collection "ioccc"): Yusuke Endoh's ASCII fluid
simulator (2012/endoh1) as **ascii-fluid** and Alan De Smet's 3D
wireframe viewer (1998/chaos) as **ioccc-chaos**.

The first commit holds the two entry folders exactly as the IOCCC
publishes them, from github.com/ioccc-src/winner at
`9142d5fbc6bdacbd3cf06d6b28767a19b418c956` (2026-10-07), with the
repository's `LICENSE` (the CC BY-SA 4.0 legal code); only the entry
tarballs (`*.tar.bz2`, the same files packed) are left out. Each folder
holds the author's original source (`*.orig.c`) beside the IOCCC's
maintained one, which is what is built: endoh1.c is identical to
endoh1.orig.c, and chaos.c adds the `endwin()` the original left out
when q quits. Nothing in `2012/` or `1998/` is changed afterwards;
everything of ours is in `launcher/` and `asciiverse/`.

Licence and attribution: `NOTICE`. Both entries are CC BY-SA 4.0,
copyright Landon Curt Noll, and so is all of ours.

## The launcher (`launcher/av.c`)

Both titles share it. It switches to the alternate screen with the
cursor hidden, sets the keyboard to raw mode and reads it itself: any
key (Ctrl+C too) ends the show, as do SIGINT, SIGTERM and SIGHUP, and the
terminal is put back every way. Each scene or object runs in a forked
child that is killed (SIGKILL) when its time is up, and the next starts;
round and round. A child does not outlive the launcher: on Linux it is
killed with it (`PR_SET_PDEATHSIG`); the fluid child dies on its broken
pipe; the chaos child notices its parent is gone at its next key read.
The entries' error output goes to /dev/null so it cannot land on the
picture. No Windows build: the launcher needs `fork()`, pipes and
termios.

## ascii-fluid (2012/endoh1)

- `launcher/fluid_bw.c` and `launcher/fluid_color.c` `#include` the
  entry's `endoh1.c` and `endoh1_color.c` unchanged, with `main` renamed
  and `usleep` pointed at `av_usleep()`, which flushes the frame before
  sleeping, so the launcher gets whole pictures through the pipe. The
  colour version's one-letter globals are renamed by macros so both
  versions link into one binary. Built with the author's `-DG=1 -DP=4
  -DV=8`.
- `launcher/fluid.c`: the 256-colour version runs by default, `-m` runs
  the original black and white one. The scene goes to the entry on its
  stdin; the entry's output (an 80x25 picture redrawn from home, with
  colour codes) is read back and redrawn centred: its "home" and line
  ends become cursor moves to the centred place, the screen is blanked
  and the picture placed again on a resize, and on a terminal smaller
  than 79x25 the middle columns and the bottom lines show. The stray
  "GO" the colour version prints before its first clear is not shown.
- Scenes, built into the binary by `asciiverse/embed.sh` from
  `asciiverse/fluid-scenes.txt`: the entry's own source first (the
  author's own first demo), then its 17 scene files, each for as long as
  it stays lively (12 to 45 seconds, measured; 439 seconds a round).
  `-t` sets one time for all, `-l` lists them, and names on the command
  line pick scenes or read scene files from disk.

## ioccc-chaos (1998/chaos)

- `launcher/chaos_hooks.h` is included before `chaos.c` (with
  `-include`), which is otherwise compiled unchanged: `main` is renamed,
  `fopen` goes to `av_fopen()` in `launcher/chaos_curses.c`, which opens
  the scene's data built into the binary with `fmemopen()`, and
  `initscr`, `refresh` and `getch` go to wrappers there that draw on a
  79x24 pad shown centred in the terminal, placed again after a resize.
- `launcher/chaos.c`: chaos reads its keys from a pipe the launcher
  writes to (curses keeps the terminal for drawing), so the real keyboard
  belongs to the launcher. It sends one of the entry's turning keys every
  80 ms, changing the axis every 4 seconds, so the object tumbles instead
  of spinning about one axis as the entry does by itself. Each object
  starts from a chosen view; the IOCCC letters, which lie edge-on at
  first, are turned to read IOCCC, brought closer and sway 48 degrees
  each way instead of tumbling.
- Objects, built in from `asciiverse/chaos-scenes.txt`: the author's
  cube, pyramid, desk, X-wing and IOCCC letters, 15 to 25 seconds each.
  `-t`, `-l` and names work as for ascii-fluid; a data file of your own
  in the entry's format can be named too.

## Packaging

- `asciiverse/build.sh` builds both into `out/` with the compiler,
  flags and curses library from the environment; `asciiverse/package.sh`
  packs one archive per title, `dist/<id>-<target>.tar.gz`, with
  `NOTICE` and `LICENSE`; `.github/workflows/release.yml` builds static
  Linux in Alpine (x86_64, aarch64) and macOS (x86_64, arm64) on a tag.
- `asciiverse/record-preview.sh` records each title's catalog preview.
