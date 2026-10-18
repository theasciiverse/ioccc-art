#!/bin/sh
# Records a title's catalog preview at 100x30 (hub/RECORDING.md), ended by
# SIGTERM, which the launcher answers by giving the terminal back. The
# previews show a few scenes each, shorter than av's endless round:
#
#   asciiverse/record-preview.sh fluid   # asciiverse/ascii-fluid.cast
#   asciiverse/record-preview.sh chaos   # asciiverse/ioccc-chaos.cast
#
# Needs asciinema 3 and out/ from asciiverse/build.sh.
set -eu
cd "$(dirname "$0")/.."
case "$1" in
fluid) id=ascii-fluid title="ASCII Fluid" seconds=49
	args="-t 12 endoh1 pour-out tanada clock" ;;
chaos) id=ioccc-chaos title="Chaos" seconds=46
	args="-t 9 cube pyramid desk xwing ioccc" ;;
*) echo "usage: $0 fluid|chaos" >&2; exit 2 ;;
esac
repo=$PWD
home=$(mktemp -d)
trap 'rm -rf "$home"' EXIT
mkdir "$home/bin" && cp "out/$id" "$home/bin/"
cd "$home"  # nothing in the recording names a local path
HOME="$home" PATH="$home/bin:$PATH" TERM=xterm-256color COLORTERM=truecolor LANG=C.UTF-8 \
asciinema rec --headless --overwrite -q \
  --window-size 100x30 --idle-time-limit 2 \
  --capture-env "TERM,COLORTERM,LANG" --title "$title" \
  --command "timeout --foreground -s TERM $seconds $id $args" \
  "$repo/asciiverse/$id.cast"
ls -la "$repo/asciiverse/$id.cast"
