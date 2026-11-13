# Asciiverse terminal edition of Goblin Hack: no window, no OpenGL, the game
# drawn in the terminal. Run from the repository root:
#
#   make -f asciiverse/Makefile            # with sound (SDL2_mixer)
#   make -f asciiverse/Makefile SOUND=0    # silent, no SDL at all
#
# The upstream autoconf build (RUNME) is untouched and still builds the
# windowed game.

CXX      ?= c++
OUT      := build-terminal
SOUND    ?= 1
OPT      ?= -O2 -g

# the game's sources, as listed in the upstream makefile
SRCS := $(shell sed -n '/^OBJS_SUBDIRS/,/^\$$(TARGET)/p' src/Makefile.in | \
          grep -o '\./[^ ]*\.o' | sed 's|^\./|src/|; s|\.o$$|.cpp|')
PLATFORM := asciiverse/platform/sdl.cpp asciiverse/platform/terminal.cpp \
            asciiverse/platform/render.cpp

CPPFLAGS += -Iasciiverse/platform/include -Isrc/include -DGHACK_TERMINAL \
            -DGH_INSTALLED_DIR='"$(DATADIR)"'
DATADIR  ?= /usr/share/goblinhack
CXXFLAGS += $(OPT) -std=gnu++11 -fpermissive -w -fno-strict-aliasing
LDLIBS   += -lz -lpthread

ifeq ($(SOUND),1)
AUDIO_CFLAGS := $(shell pkg-config --cflags SDL2_mixer sdl2)
AUDIO_LIBS   ?= $(shell pkg-config --libs SDL2_mixer sdl2)
PLATFORM_AUDIO := asciiverse/platform/audio.cpp
else
PLATFORM_AUDIO := asciiverse/platform/audio_silent.cpp
endif

OBJS := $(patsubst %.cpp,$(OUT)/obj/%.o,$(SRCS) $(PLATFORM) $(PLATFORM_AUDIO))

all: build/config.h $(OUT)/goblinhack

# ghack.h includes ../../build/config.h, which configure would write
build/config.h: asciiverse/platform/config.h
	cp $< $@

$(OBJS): build/config.h

$(OUT)/goblinhack: $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(AUDIO_LIBS) $(LDLIBS)

# the one file that sees the real SDL2 headers
$(OUT)/obj/asciiverse/platform/audio.o: asciiverse/platform/audio.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(OPT) -std=gnu++11 $(AUDIO_CFLAGS) -c -o $@ $<

$(OUT)/obj/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c -o $@ $<

clean:
	rm -rf build-terminal

-include $(OBJS:.o=.d)
.PHONY: all clean
