#!/bin/sh
# Packs each title into its own archive for av: dist/<id>-<target>.tar.gz
# with the binary, NOTICE (the attribution CC BY-SA 4.0 asks for) and
# LICENSE (its legal code).
#
#   asciiverse/package.sh x86_64-unknown-linux-musl
set -eu
cd "$(dirname "$0")/.."
target="$1"
mkdir -p dist
for id in ascii-fluid ioccc-chaos; do
	stage="stage/$id"
	rm -rf "$stage" && mkdir -p "$stage"
	cp "out/$id" NOTICE LICENSE "$stage/"
	tar -czf "dist/$id-$target.tar.gz" -C "$stage" .
done
ls dist
