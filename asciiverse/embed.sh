#!/bin/sh
# Writes a C header with the scenes of one title built in, from a list of
# "<file> <seconds>" lines (paths relative to the repo root, # comments):
#
#   asciiverse/embed.sh asciiverse/fluid-scenes.txt out/fluid-scenes.h
#
# Each file becomes a byte array; the table av_scenes[] names it by its
# file name without the extension, with the seconds it plays.
set -eu
cd "$(dirname "$0")/.."
list=$1
out=$2
mkdir -p "$(dirname "$out")"
{
	echo "/* made by asciiverse/embed.sh from $list; do not edit */"
	n=0
	grep -v '^#' "$list" | while read -r file secs; do
		[ -n "$file" ] || continue
		echo "static const unsigned char av_scene_$n[] = {"
		od -An -v -tx1 "$file" | sed 's/ *\([0-9a-f][0-9a-f]\)/0x\1,/g'
		echo "0 };"
		n=$((n + 1))
	done
	echo "static struct av_scene av_scenes[] = {"
	n=0
	grep -v '^#' "$list" | while read -r file secs; do
		[ -n "$file" ] || continue
		name=$(basename "$file"); name=${name%.*}
		size=$(wc -c < "$file" | tr -d ' ')
		echo "	{ \"$name\", av_scene_$n, $size, $secs },"
		n=$((n + 1))
	done
	echo "};"
} > "$out"
