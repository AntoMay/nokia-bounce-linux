CC ?= cc
PKG_CONFIG ?= pkg-config

CPPFLAGS += -I../level -I../renderer
# STEP 14B-34-D: the audio player uses clock_gettime, nanosleep and
# pthread_cond_timedwait, which this project's strict -std=c11 does not expose.
# _POSIX_C_SOURCE declares the POSIX 2008 surface without relaxing -std=c11.
CPPFLAGS += -D_POSIX_C_SOURCE=200809L
CFLAGS ?= -O0 -g
CFLAGS += -std=c11 -Wall -Wextra -Werror -pedantic
X11_CFLAGS := $(shell $(PKG_CONFIG) --cflags x11)
X11_LIBS := $(shell $(PKG_CONFIG) --libs x11)
PNG_CFLAGS := $(shell $(PKG_CONFIG) --cflags libpng)
PNG_LIBS := $(shell $(PKG_CONFIG) --libs libpng)
# STEP 14B-34-C. ALSA is used ONLY by the standalone backend test. It is
# deliberately NOT added to the game binary link line: the backend is not yet
# integrated, and a self-test must never pull an audio library into the game.
# _POSIX_C_SOURCE is required here, and only here: <alsa/global.h> defines
# struct timeval/struct timespec itself unless _POSIX_C_SOURCE or _POSIX_SOURCE
# is set, which collides with glibc under this project's strict -std=c11.
# Defining it makes ALSA use the standard glibc definitions. This does not
# relax the project's -std=c11 -Wall -Wextra -Werror -pedantic flags.
ALSA_CFLAGS := -D_POSIX_C_SOURCE=200809L $(shell $(PKG_CONFIG) --cflags alsa 2>/dev/null)
ALSA_LIBS := $(shell $(PKG_CONFIG) --libs alsa 2>/dev/null)
ifeq ($(strip $(ALSA_LIBS)),)
ALSA_LIBS := -lasound
endif
# STEP 14B-34-D. -lm is now explicit: the game links audio_synth.c, whose
# deterministic renderer uses sin/cos. -lpthread is for the audio worker.
# STEP 14B-34-D. The game links the ALSA backend when pkg-config finds ALSA.
# BOUNCE_AUDIO_HAVE_ALSA=0 makes audio_player.c compile without it, so a host
# with no ALSA still builds the game; audio is simply unavailable there.
HAVE_ALSA := $(shell $(PKG_CONFIG) --exists alsa 2>/dev/null && echo 1 || echo 0)
ifeq ($(HAVE_ALSA),1)
GAME_AUDIO_CFLAGS := $(ALSA_CFLAGS) -DBOUNCE_AUDIO_HAVE_ALSA=1
GAME_AUDIO_LIBS := $(ALSA_LIBS)
GAME_AUDIO_SOURCES := audio_backend_alsa.c
else
GAME_AUDIO_CFLAGS :=
GAME_AUDIO_LIBS :=
GAME_AUDIO_SOURCES :=
endif

SOURCES = \
	vertical_slice.c \
	app_flow.c \
	persistence.c \
	game.c \
	player.c \
	game_timer.c \
	locale.c \
	script_font.c \
	asset_inventory.c \
	input_state.c \
	collision_query.c \
	visual_assets.c \
	audio_event.c \
	rtpl_decoder.c \
	audio_synth.c \
	audio_backend.c \
	audio_backend_null.c \
	audio_player.c \
	$(GAME_AUDIO_SOURCES) \
	debug_tracker.c \
	ui_shell.c \
	../level/level.c \
	../level/dyn_thorns.c \
	../assets/asset.c \
	../level/level_loader.c \
	../renderer/surface.c \
	../renderer/image.c \
	../renderer/renderer.c \
	../renderer/presentation.c

# STEP 38-RESET -- THE HEADERS ARE PREREQUISITES TOO.
#
# bounce_vertical_slice was built from $(SOURCES) alone, so make compared the binary
# against the .c files only. Every header in the tree was therefore invisible to the
# dependency check, and editing one left a stale binary in place with no warning.
#
# That is not a tidiness complaint. It is what makes a negative control lie: four of
# the controls for this feature change a default that lives in app_flow.h, and each
# one was reported as "not caught" simply because make declined to rebuild. A test
# suite whose injected bugs are silently ignored by the build cannot demonstrate that
# the tests would notice a real bug, so the build has to be honest before the tests
# can be.
#
# The whole program is compiled in one command rather than to objects, so -MMD would
# have no per-object .d file to produce. The wildcard is evaluated when the Makefile
# is read, which is correct here because the header set is fixed in the tree.
HEADERS := $(wildcard *.h ../level/*.h ../renderer/*.h ../assets/*.h)

.PHONY: all check check-rtpl check-audio-event check-audio-synth \
        check-audio-backend check-audio-backend-alsa check-audio-player \
        check-audio-player-alsa check-audio-lifecycle \
        check-audio-runtime check-audio-device-config assets run clean

all: bounce_vertical_slice

bounce_vertical_slice: $(SOURCES) $(HEADERS)
	$(CC) $(CPPFLAGS) $(X11_CFLAGS) $(PNG_CFLAGS) $(GAME_AUDIO_CFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $(SOURCES) $(X11_LIBS) $(PNG_LIBS) $(GAME_AUDIO_LIBS) -lpthread -lm $(LDLIBS)

check: bounce_vertical_slice
	cd ../.. && ./native/app/bounce_vertical_slice --check

# RTPL (.ott) decoder self-test. The runner links only the decoder and the
# standard library: it deliberately pulls in neither X11 nor libpng, so running
# it cannot touch any display, any audio backend, or anything else the game
# needs. No PCM is generated and no sound is played.
rtpl_decoder_check: rtpl_decoder_check.c rtpl_decoder.c rtpl_decoder.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ rtpl_decoder_check.c rtpl_decoder.c $(LDLIBS)

check-rtpl: rtpl_decoder_check
	cd ../.. && ./native/app/rtpl_decoder_check src/main/resources

# STEP 14B-27 audio event identity self-test. Like the RTPL runner above it
# links only its own module and libc: no X11, no libpng, no audio library. The
# test proves sequential emissions reach the consumer immediately and in order.
audio_event_check: audio_event_check.c audio_event.c audio_event.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ audio_event_check.c audio_event.c $(LDLIBS)

check-audio-event: audio_event_check
	./audio_event_check

# STEP 14B-34-A / 14B-34-B Layer C self-test (Target B). Like the two runners above
# it links only its own module, the Layer A decoder, and libc (+ -lm for the
# sine/cosine used by the deterministic renderer): no X11, no libpng, no audio
# library, and no Layer B object file. It therefore cannot open a device, write to
# a device, or play anything. STEP 14B-34-B generates PCM in memory and asserts
# it; it never plays it.
#
# audio_synth.c is deliberately NOT in SOURCES. Layer C is not linked into the
# game binary until STEP 14B-34-D installs the Layer B sink, and that step owns
# the only Makefile change to the game.
audio_synth_check: audio_synth_check.c audio_synth.c audio_synth.h rtpl_decoder.c rtpl_decoder.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ audio_synth_check.c audio_synth.c rtpl_decoder.c $(LDLIBS) -lm

check-audio-synth: audio_synth_check
	cd ../.. && ./native/app/audio_synth_check src/main/resources

# STEP 14B-34-C portable backend interface + Null backend self-test. Links the
# portable interface, the Null backend, the 14B-34-B renderer and the Layer A
# decoder -- and deliberately NOT libasound, so this test cannot open a device
# even by accident. It produces no sound. The renderer-to-backend integration
# case renders one ORIGINAL asset and writes it through the Null backend.
audio_backend_check: audio_backend_check.c audio_backend.c audio_backend.h \
			audio_backend_null.c audio_backend_null.h \
			audio_synth.c audio_synth.h rtpl_decoder.c rtpl_decoder.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ audio_backend_check.c audio_backend.c audio_backend_null.c audio_synth.c rtpl_decoder.c $(LDLIBS) -lm

check-audio-backend: audio_backend_check
	cd ../.. && ./native/app/audio_backend_check src/main/resources

# STEP 14B-34-C ALSA backend self-test. This is the only target that links
# libasound. It is SILENT by default: it exercises construction, format
# validation, the invalid-device path, and the close lifecycle without writing
# samples. The audible smoke test is opt-in via BOUNCE_AUDIO_ALSA_SMOKE=1 and is
# never run by `make check`.
audio_backend_alsa_check: audio_backend_alsa_check.c audio_backend.c audio_backend.h \
			audio_backend_alsa.c audio_backend_alsa.h
	$(CC) $(CPPFLAGS) $(ALSA_CFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ audio_backend_alsa_check.c audio_backend.c audio_backend_alsa.c $(ALSA_LIBS) $(LDLIBS)

check-audio-backend-alsa: audio_backend_alsa_check
	./audio_backend_alsa_check

# STEP 14B-34-D audio integration self-test. It links the player, Layer B, the
# renderer, the Layer A decoder, the portable backend, the Null backend and
# pthread -- and deliberately NOT libasound: BOUNCE_AUDIO_HAVE_ALSA=0 means it
# cannot open a device even by accident. It makes no sound. It drives the player
# through the real bounce_audio_event_emit() entry point, so the sink wiring
# itself is under test.
audio_player_check: audio_player_check.c audio_player.c audio_player.h \
		audio_event.c audio_event.h audio_synth.c audio_synth.h \
		audio_backend.c audio_backend.h audio_backend_null.c audio_backend_null.h \
		rtpl_decoder.c rtpl_decoder.h
	$(CC) $(CPPFLAGS) -DBOUNCE_AUDIO_HAVE_ALSA=0 $(CFLAGS) $(LDFLAGS) -o $@ audio_player_check.c audio_player.c audio_event.c audio_synth.c audio_backend.c audio_backend_null.c rtpl_decoder.c -lpthread $(LDLIBS) -lm

check-audio-player: audio_player_check
	cd ../.. && ./native/app/audio_player_check src/main/resources

# STEP 14B-34-D player self-test WITH ALSA. This is the regression test for the
# backend-teardown use-after-free, which lived only on the ALSA failure path and
# was therefore invisible to the Null-only runner above. It makes no sound: every
# case expects no usable device or an invalid device name.
audio_player_alsa_check: audio_player_alsa_check.c audio_player.c audio_player.h \
		audio_event.c audio_event.h audio_synth.c audio_synth.h \
		audio_backend.c audio_backend.h audio_backend_null.c audio_backend_null.h \
		audio_backend_alsa.c audio_backend_alsa.h rtpl_decoder.c rtpl_decoder.h
	$(CC) $(CPPFLAGS) $(ALSA_CFLAGS) -DBOUNCE_AUDIO_HAVE_ALSA=1 $(CFLAGS) $(LDFLAGS) -o $@ audio_player_alsa_check.c audio_player.c audio_event.c audio_synth.c audio_backend.c audio_backend_null.c audio_backend_alsa.c rtpl_decoder.c $(ALSA_LIBS) -lpthread $(LDLIBS) -lm

check-audio-player-alsa: audio_player_alsa_check
	cd ../.. && ./native/app/audio_player_alsa_check src/main/resources

# STEP 14B-34-E lifecycle and FIFO stress self-test. Links no ALSA and no device,
# and makes no sound. It is built to be run under ASan/UBSan/TSan, which is what
# turns a single happy-path run into a real use-after-free / leak / race check.
audio_lifecycle_check: audio_lifecycle_check.c audio_player.c audio_player.h \
		audio_event.c audio_event.h audio_synth.c audio_synth.h \
		audio_backend.c audio_backend.h audio_backend_null.c audio_backend_null.h \
		rtpl_decoder.c rtpl_decoder.h
	$(CC) $(CPPFLAGS) -DBOUNCE_AUDIO_HAVE_ALSA=0 $(CFLAGS) $(LDFLAGS) -o $@ audio_lifecycle_check.c audio_player.c audio_event.c audio_synth.c audio_backend.c audio_backend_null.c rtpl_decoder.c -lpthread $(LDLIBS) -lm

check-audio-lifecycle: audio_lifecycle_check
	cd ../.. && ./native/app/audio_lifecycle_check src/main/resources

# STEP 14B-34-F runtime / audibility verification. Deliberately NOT part of
# `check`: it links ALSA, and with BOUNCE_AUDIO_ALSA_SMOKE=1 it plays the three
# real game sounds on a real device. The default run makes no sound at all -- it
# measures the rendered PCM in memory, checks failure isolation, and reports
# which device names are openable.
#
# AUDIBILITY IS NEVER ESTABLISHED HERE. This target can reach snd_pcm_writei();
# it cannot listen. LEVEL C is reported NOT PROVEN unconditionally, and only
# the project owner can change that.
audio_runtime_check: audio_runtime_check.c audio_player.c audio_player.h \
		audio_event.c audio_event.h audio_synth.c audio_synth.h \
		audio_backend.c audio_backend.h audio_backend_null.c audio_backend_null.h \
		audio_backend_alsa.c audio_backend_alsa.h rtpl_decoder.c rtpl_decoder.h
	$(CC) $(CPPFLAGS) $(ALSA_CFLAGS) -DBOUNCE_AUDIO_HAVE_ALSA=1 $(CFLAGS) $(LDFLAGS) -o $@ audio_runtime_check.c audio_player.c audio_event.c audio_synth.c audio_backend.c audio_backend_null.c audio_backend_alsa.c rtpl_decoder.c $(ALSA_LIBS) -lpthread $(LDLIBS) -lm

check-audio-runtime: audio_runtime_check
	cd ../.. && ./native/app/audio_runtime_check src/main/resources

# STEP 14B-34-H explicit device configuration self-test. Verifies that the
# backend's own default is unchanged when no device is named, that a named
# device is recorded verbatim, and that an unusable name fails explicitly with
# no substitution and no Null degradation. It makes no sound: it only opens and
# closes devices, writing no sample to any of them. Host device availability is
# REPORTED, never asserted, so the test is meaningful on a host with no audio.
audio_device_config_check: audio_device_config_check.c audio_player.c audio_player.h \
		audio_event.c audio_event.h audio_synth.c audio_synth.h \
		audio_backend.c audio_backend.h audio_backend_null.c audio_backend_null.h \
		audio_backend_alsa.c audio_backend_alsa.h rtpl_decoder.c rtpl_decoder.h
	$(CC) $(CPPFLAGS) $(ALSA_CFLAGS) -DBOUNCE_AUDIO_HAVE_ALSA=1 $(CFLAGS) $(LDFLAGS) -o $@ audio_device_config_check.c audio_player.c audio_event.c audio_synth.c audio_backend.c audio_backend_null.c audio_backend_alsa.c rtpl_decoder.c $(ALSA_LIBS) -lpthread $(LDLIBS) -lm

check-audio-device-config: audio_device_config_check
	cd ../.. && ./native/app/audio_device_config_check src/main/resources plughw:1,0

assets: bounce_vertical_slice
	cd ../.. && ./native/app/bounce_vertical_slice --assets

run: bounce_vertical_slice
	cd ../.. && ./native/app/bounce_vertical_slice

clean:
	rm -f bounce_vertical_slice rtpl_decoder_check audio_event_check audio_synth_check \
	      audio_backend_check audio_backend_alsa_check audio_player_check \
	      audio_player_alsa_check audio_lifecycle_check audio_runtime_check \
	      audio_device_config_check
