#!/bin/sh
# Builds Goblin Hack's terminal edition for the Asciiverse catalog as
# ./goblinhack, with the game's files in ./resources beside it.
#
# asciiverse/Makefile builds the game against asciiverse/platform: no
# window, no OpenGL. Sound goes through SDL2 and SDL2_mixer, built here from
# source without video and linked in statically (WAV only: every sound and
# tune is WAV). SDL loads the system's sound library (PipeWire, PulseAudio
# or ALSA) at run time, so Linux builds are glibc: run in ubuntu:22.04 for
# glibc 2.35 and newer,
#   docker run --rm -v "$PWD":/src -w /src ubuntu:22.04 sh asciiverse/build.sh
# On macOS run on the runner with CC_ARCH set to x86_64 or arm64.
set -eu
SDL2=2.32.10
SDL2_MIXER=2.8.1

if [ -f /etc/os-release ] && grep -q '^ID=ubuntu' /etc/os-release && [ "$(id -u)" = 0 ]; then
  export DEBIAN_FRONTEND=noninteractive
  apt-get update -qq
  apt-get install -y -qq build-essential cmake git pkg-config file zlib1g-dev \
    libasound2-dev libpulse-dev libpipewire-0.3-dev >/dev/null
fi

root=$(pwd)
deps="$root/avbuild"
prefix="$deps/prefix"
mkdir -p "$deps"
arch=${CC_ARCH:+-DCMAKE_OSX_ARCHITECTURES=$CC_ARCH}
jobs=${JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)}

if [ ! -f "$prefix/lib/libSDL2.a" ]; then
  [ -d "$deps/SDL" ] || git clone -q --depth 1 -b "release-$SDL2" https://github.com/libsdl-org/SDL "$deps/SDL"
  cmake -S "$deps/SDL" -B "$deps/SDL-build" -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$prefix" -DSDL_SHARED=OFF -DSDL_STATIC=ON \
    -DSDL_TEST=OFF -DSDL_VIDEO=OFF -DSDL_RENDER=OFF -DSDL_JOYSTICK=OFF \
    -DSDL_HAPTIC=OFF -DSDL_HIDAPI=OFF -DSDL_SENSOR=OFF -DSDL_DLOPEN=ON \
    -DSDL_STATIC_PIC=ON $arch >/dev/null
  cmake --build "$deps/SDL-build" -j "$jobs" >/dev/null
  cmake --install "$deps/SDL-build" >/dev/null
fi

if [ ! -f "$prefix/lib/libSDL2_mixer.a" ]; then
  [ -d "$deps/SDL_mixer" ] || git clone -q --depth 1 -b "release-$SDL2_MIXER" https://github.com/libsdl-org/SDL_mixer "$deps/SDL_mixer"
  cmake -S "$deps/SDL_mixer" -B "$deps/SDL_mixer-build" -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$prefix" -DCMAKE_PREFIX_PATH="$prefix" \
    -DBUILD_SHARED_LIBS=OFF -DSDL2MIXER_SAMPLES=OFF -DSDL2MIXER_VORBIS=OFF \
    -DSDL2MIXER_OPUS=OFF -DSDL2MIXER_FLAC=OFF -DSDL2MIXER_MOD=OFF \
    -DSDL2MIXER_MIDI=OFF -DSDL2MIXER_MP3=OFF -DSDL2MIXER_WAVPACK=OFF \
    -DSDL2MIXER_GME=OFF $arch >/dev/null
  cmake --build "$deps/SDL_mixer-build" -j "$jobs" >/dev/null
  cmake --install "$deps/SDL_mixer-build" >/dev/null
fi

export PKG_CONFIG_PATH="$prefix/lib/pkgconfig"
audio_cflags=$(pkg-config --cflags SDL2_mixer sdl2)
audio_libs=$(pkg-config --static --libs SDL2_mixer sdl2)
archflag=${CC_ARCH:+-arch $CC_ARCH}
static_cxx=""
[ "$(uname)" = Darwin ] || static_cxx="-static-libstdc++ -static-libgcc"

make -f asciiverse/Makefile -j "$jobs" SOUND=1 OPT="-O2 $archflag" \
  AUDIO_CFLAGS="$audio_cflags" AUDIO_LIBS="$audio_libs" \
  LDFLAGS="$archflag $static_cxx"

cp build-terminal/goblinhack goblinhack
strip goblinhack
file goblinhack
ls -l goblinhack
