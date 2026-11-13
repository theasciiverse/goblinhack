#!/bin/sh
# Lays out the av archive in stage/: the goblinhack binary with the game's
# files in resources/ beside it (pictures, rooms, levels, sounds, fonts),
# and the licence, readme and change notes.
set -eu
rm -rf stage && mkdir -p stage
cp goblinhack stage/goblinhack
mkdir -p stage/resources
# the game's files only: no build files and no source compiled into the game
(cd src/resources && tar -cf - --exclude='Makefile*' --exclude='*.cpp' --exclude='*.o' --exclude='*.d' .) \
  | (cd stage/resources && tar -xf -)
cp LICENSE stage/LICENSE
cp README.md PATCHES.md UPSTREAM.md stage/
