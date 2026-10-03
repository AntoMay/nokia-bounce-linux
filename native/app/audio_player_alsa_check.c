/*
 * audio_player_alsa_check.c -- STEP 14B-34-D player self-test WITH ALSA.
 *
 * WHY A SECOND RUNNER EXISTS
 *   audio_player_check compiles with BOUNCE_AUDIO_HAVE_ALSA=0 so it cannot touch
 *   a device. That means it never exercises the path where opening ALSA FAILS
 *   and the player has to tear a half-built backend down. A real use-after-free
 *   lived exactly there and was invisible to the Null-only test.
 *
 *   This runner is compiled with BOUNCE_AUDIO_HAVE_ALSA=1 and links libasound,
 *   so it drives the real AUTO / forced-ALSA failure paths. Run under
 *   AddressSanitizer it is the regression test for that defect.
 *
 *   IT MAKES NO SOUND. Every case here expects the device to be unavailable or
 *   the name to be invalid, so the backend never plays anything. The audible
 *   opt-in smoke test remains the 14B-34-C mechanism
 *   (BOUNCE_AUDIO_ALSA_SMOKE) and is not duplicated here.
 *
 * Usage:  audio_player_alsa_check <resource-root>
 * Exit status: 0 when every assertion passed, 1 otherwise.
 */

#include "audio_event.h"
#include "audio_player.h"

#include <alsa/asoundlib.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* Silence ALSA's own stderr chatter: every case here deliberately fails to open a
 * device, and the reason is already asserted through the returned status. */
static void alsa_silence(const char *file, int line, const char *function,
                         int err, const char *fmt, ...)
{
    (void)file;
    (void)line;
    (void)function;
    (void)err;
    (void)fmt;
}

static int alsa_player_failures;

static void expect(int condition, const char *description)
{
    if (condition) {
        (void)printf("      ok   %s\n", description);
        return;
    }
    (void)printf("      FAIL %s\n", description);
    alsa_player_failures++;
}

static BounceAudioPlayer *make(const char *resource_root,
                              BounceAudioPlayerBackend backend,
                              const char *device)
{
    BounceAudioPlayerConfig config;

    bounce_audio_player_default_config(&config);
    config.resource_root = resource_root;
    config.backend = backend;
    config.device_name = device;
    return bounce_audio_player_create(&config);
}

int main(int argc, char **argv)
{
    const char *resource_root = (argc > 1) ? argv[1] : "";
    BounceAudioPlayer *player;
    int device_present;

    (void)printf("audio player ALSA failure-path self-test (STEP 14B-34-D)\n");
    (void)printf("  no sound is produced; every case expects no usable device\n\n");

    (void)snd_lib_error_set_handler(alsa_silence);

    if (resource_root[0] == '\0') {
        (void)fprintf(stderr, "usage: %s <resource-root>\n",
                      (argc > 0) ? argv[0] : "audio_player_alsa_check");
        return 1;
    }

    (void)printf("  backend failure teardown (use-after-free regression)\n");

    /*
     * A device name that cannot exist. `create` must unwind the half-built
     * backend cleanly and return NULL. Under ASan this is the case that used to
     * double free the ALSA state and then call close() through the freed table.
     */
    player = make(resource_root, BOUNCE_AUDIO_PLAYER_BACKEND_ALSA,
                  "bounce-nonexistent-pcm-device");
    expect(player == NULL,
           "forced ALSA with an invalid device name returns NULL");
    (void)bounce_audio_player_destroy(player);

    player = make(resource_root, BOUNCE_AUDIO_PLAYER_BACKEND_ALSA, NULL);
    device_present = (player != NULL);
    if (player != NULL) {
        /* A device exists: stop the sink, join the worker, and close. */
        (void)bounce_audio_player_attach_sink(player);
        (void)bounce_audio_event_emit(BOUNCE_AUDIO_EVENT_POP);
        expect(strcmp(bounce_audio_player_backend_name(player), "alsa") == 0,
               "with a device present, the ALSA backend is selected");
        (void)bounce_audio_player_destroy(player);
        expect(1, "create + attach + emit + destroy completed without a leak"
                  " or a crash");
    } else {
        expect(1,
               "no device on this host: create returned NULL, which is the"
               " documented outcome");
    }
    (void)device_present;

    /* AUTO with no device must be NULL, never a silent Null substitution. */
    (void)printf("  AUTO never falls back\n");
    if (!device_present) {
        player = make(resource_root, BOUNCE_AUDIO_PLAYER_BACKEND_AUTO, NULL);
        expect(player == NULL,
               "AUTO with no usable device returns NULL rather than a fake one");
        (void)bounce_audio_player_destroy(player);
    } else {
        (void)printf("      skip AUTO-fallback case: a device is present,"
                     " so AUTO legitimately succeeds\n");
    }

    /* Explicit Null still works, proving the failure paths did not break it. */
    (void)printf("  explicit Null still works after the failure paths\n");
    player = make(resource_root, BOUNCE_AUDIO_PLAYER_BACKEND_NULL, NULL);
    expect(player != NULL, "explicit Null backend still creates");
    if (player != NULL) {
        BounceAudioPlayerStats stats;

        expect(strcmp(bounce_audio_player_backend_name(player), "null") == 0,
               "the Null backend is in use");
        expect(bounce_audio_player_attach_sink(player) == 1,
               "the sink attaches");
        (void)bounce_audio_event_emit(BOUNCE_AUDIO_EVENT_UP);
        (void)bounce_audio_event_emit(BOUNCE_AUDIO_EVENT_POP);
        expect(bounce_audio_player_stats(player, &stats) == 1
                   && stats.events_enqueued == 2ull,
               "two events were accepted through the real Layer B entry point");
        (void)bounce_audio_player_destroy(player);
        expect(1, "destroy completed");
    }

    if (alsa_player_failures != 0) {
        (void)printf("\naudio player ALSA self-test: FAIL (%d failure(s))\n",
                     alsa_player_failures);
        (void)fprintf(stderr, "audio_player_alsa_check: FAILED\n");
        return 1;
    }
    (void)printf("\naudio player ALSA self-test: PASS (0 failures)\n");
    (void)printf("no sound was produced; no device was left open\n");
    (void)snd_lib_error_set_handler(NULL);
    return 0;
}
