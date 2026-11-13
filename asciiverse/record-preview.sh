#!/bin/sh
# Records the catalog preview, asciiverse/goblinhack.cast (100x30,
# asciinema 3), with hub's recorder (hub/tools/castrec.py, a sibling
# checkout at ../../hub): the title, the hero and dungeon choosers, then a
# walk through the first level, a look at the Tab view's pictures, and back.
#
# Plays the staged game (stage/goblinhack, from asciiverse/stage.sh) with
# sound off and a scratch state folder. The levels are random, so every
# take differs: record again and keep the best.
#
#   sh asciiverse/record-preview.sh
set -eu
root=$(cd "$(dirname "$0")/.." && pwd)
castrec=${CASTREC:-$root/../../hub/tools/castrec.py}
state=$(mktemp -d)
trap 'rm -rf "$state"' EXIT

walk="r14:right:0.07;w0.4;kspace;w0.3;r10:down:0.07;w0.4;r12:left:0.07;kspace;w0.3;r10:up:0.07;w0.3"
script="w3.5;kspace;w1.6;kspace;w1.6;kspace;w3.5;$walk;$walk;ktab;w2.5;r8:right:0.07;w1.5;ktab;w0.8;$walk;w1"

python3 "$castrec" "$root/asciiverse/goblinhack.cast" "$root/stage" "$script" -- \
  env XDG_STATE_HOME="$state" GOBLINHACK_SILENT=1 ./goblinhack --silent
