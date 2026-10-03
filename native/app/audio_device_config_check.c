/*
 * audio_device_config_check.c -- STEP 14B-34-H explicit device configuration.
 *
 * WHAT THIS TEST IS FOR
 *   The owner selected Option B of STEP 14B-34-G: a caller may name the ALSA
 *   device explicitly, and nothing else about the audio architecture changes.
 *   Three properties have to hold, and each is asserted here against the real
 *   production code rather than against a description of it:
 *
 *     1. DEFAULT UNCHANGED. With no device supplied, the backend still resolves
 *        to BOUNCE_AUDIO_BACKEND_DEVICE_DEFAULT. "default" is not quietly
 *        repointed at some other device.
 *     2. EXPLICIT IS EXACT. A supplied name reaches the backend verbatim and is
 *        recorded verbatim. It is never trimmed, normalised, or replaced.
 *     3. NO SILENT FALLBACK. A name that cannot be opened fails, cleanly and
 *        observably. No second device is attempted, and the failure never
 *        degrades into the Null backend.
 *
 *   The device string is asserted where it becomes observable, which is the
 *   ALSA backend's own recorded state. That is the last point before
 *   snd_pcm_open(), so a match there is a match at the boundary.
 *
 * THE ONE HARDWARE CAVEAT, STATED PLAINLY
 *   A successful open depends on the host. This test therefore NEVER asserts
 *   that any particular device opens. Where a device happens to be openable it
 *   is used and the result is reported as an OBSERVATION, not as a requirement.
 *   The assertions that must hold on every host are the failure-path ones, and
 *   those are the ones carrying weight here.
 *
 * IT MAKES NO SOUND. Every case either fails to open, or opens and closes again
 * without writing a sample. The audible opt-in remains the single existing
 * BOUNCE_AUDIO_ALSA_SMOKE mechanism, which this file does not add to.
 *
 * Usage:  audio_device_config_check <resource-root> [openable-device]
 * Exit status: 0 when every assertion passed, 1 otherwise.
 */

#include "audio_backend.h"
#include "audio_backend_alsa.h"
#include "audio_event.h"
#include "audio_player.h"

#include <alsa/asoundlib.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static int failures;

/* Every case here deliberately tries to open devices that may not exist. The
 * reason is already asserted through the returned status, so ALSA's own stderr
 * chatter is suppressed rather than left to look like test noise. */
static void alsa_silence(const char *file, int line, const char *function,
                         int err, const char *fmt, ...)
{
    (void)file;
    (void)line;
    (void)function;
    (void)err;
    (void)fmt;
}

static void expect(int condition, const char *description)
{
    if (condition) {
        (void)printf("      ok   %s\n", description);
        return;
    }
    (void)printf("      FAIL %s\n", description);
    failures++;
}

static void describe(int condition, const char *description, const char *actual)
{
    char text[256];

    (void)snprintf(text, sizeof(text), "%s (recorded \"%s\")", description,
                   actual != NULL ? actual : "(null)");
    expect(condition, text);
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

static void nap_ms(unsigned int ms)
{
    struct timespec t;

    t.tv_sec = (time_t)(ms / 1000u);
    t.tv_nsec = (long)((ms % 1000u) * 1000000L);
    (void)nanosleep(&t, NULL);
}

/* -------------------------------------------------------------------------
 * 1. DEFAULT UNCHANGED -- the load-bearing requirement for backward behaviour.
 * ------------------------------------------------------------------------- */

static void test_default_unchanged(void)
{
    BounceAudioBackend backend;
    BounceAudioAlsaState state;

    (void)printf("  1. no device supplied -> the backend's own default\n");

    memset(&state, 0, sizeof(state));
    expect(bounce_audio_backend_alsa_init(&backend, &state, NULL)
               == BOUNCE_AUDIO_BACKEND_OK,
           "the ALSA backend initialises with a NULL device name");
    describe(strcmp(state.device, BOUNCE_AUDIO_BACKEND_DEVICE_DEFAULT) == 0,
             "the effective device is the declared default", state.device);
    expect(strcmp(BOUNCE_AUDIO_BACKEND_DEVICE_DEFAULT, "default") == 0,
           "the declared default is still the string \"default\"");

    /*
     * An empty string must end up on the default device, and the resolution
     * point is open(), not init(). init() records the REQUESTED name -- NULL
     * becomes the default, while "" is carried through verbatim -- and open()
     * is where a NULL-or-empty request is resolved to the effective device.
     * So the assertion belongs after the open attempt. The open is expected to
     * fail wherever "default" is unusable, and it is the recorded name that is
     * being asserted, not the open outcome.
     */
    memset(&state, 0, sizeof(state));
    expect(bounce_audio_backend_alsa_init(&backend, &state, "") == BOUNCE_AUDIO_BACKEND_OK,
           "the ALSA backend initialises with an empty device name");
    {
        BounceAudioBackendFormat format;

        bounce_audio_backend_default_format(&format);
        (void)backend.open(&backend, &format, "");
        describe(strcmp(state.device, BOUNCE_AUDIO_BACKEND_DEVICE_DEFAULT) == 0,
                 "an empty name resolves to the default at open, not to a"
                 " blank device", state.device);
        backend.close(&backend);
    }
}

/* -------------------------------------------------------------------------
 * 2. EXPLICIT IS EXACT -- the name reaches the backend verbatim.
 * ------------------------------------------------------------------------- */

static void test_explicit_is_exact(const char *device)
{
    BounceAudioBackend backend;
    BounceAudioAlsaState state;

    (void)printf("  2. an explicit name is recorded verbatim\n");

    memset(&state, 0, sizeof(state));
    expect(bounce_audio_backend_alsa_init(&backend, &state, device)
               == BOUNCE_AUDIO_BACKEND_OK,
           "the ALSA backend initialises with the requested name");
    describe(strcmp(state.device, device) == 0,
             "the requested name is recorded exactly, not resolved or replaced",
             state.device);

    /* A name that is certainly not a device: it must be RECORDED even though it
     * cannot be opened, which is what proves the string is passed through
     * instead of being validated away at configuration time. */
    memset(&state, 0, sizeof(state));
    expect(bounce_audio_backend_alsa_init(&backend, &state, "bounce-no-such-pcm")
               == BOUNCE_AUDIO_BACKEND_OK,
           "an unknown name is still accepted at configuration time");
    describe(strcmp(state.device, "bounce-no-such-pcm") == 0,
             "an unknown name is recorded, not rejected and not rewritten",
             state.device);
}

/* -------------------------------------------------------------------------
 * 3. NO SILENT FALLBACK -- failure is explicit, and never becomes Null.
 * ------------------------------------------------------------------------- */

static void test_no_silent_fallback(const char *resource_root)
{
    static const char *const bogus[] = {
        "bounce-no-such-pcm", "plughw:99,99", "hw:99,99", "nonexistent-device"
    };
    size_t i;
    BounceAudioPlayer *player;
    BounceAudioBackend backend;
    BounceAudioAlsaState state;
    BounceAudioBackendFormat format;

    (void)printf("  3. an unusable name fails explicitly and never falls back\n");

    for (i = 0u; i < sizeof(bogus) / sizeof(bogus[0]); i++) {
        BounceAudioBackendStatus st;

        memset(&state, 0, sizeof(state));
        if (bounce_audio_backend_alsa_init(&backend, &state, bogus[i])
            != BOUNCE_AUDIO_BACKEND_OK) {
            expect(0, "backend init accepts the name for the open attempt");
            continue;
        }
        bounce_audio_backend_default_format(&format);
        st = backend.open(&backend, &format, bogus[i]);
        expect(st != BOUNCE_AUDIO_BACKEND_OK,
               "opening an unusable name reports failure");
        describe(strcmp(state.device, bogus[i]) == 0,
                 "the failing name is still what was recorded -- no second"
                 " device was substituted", state.device);
        backend.close(&backend);
    }

    (void)printf("  4. the player reports failure rather than degrading\n");

    player = make(resource_root, BOUNCE_AUDIO_PLAYER_BACKEND_ALSA,
                  "bounce-no-such-pcm");
    expect(player == NULL,
           "a player on an unusable device is not created");
    expect(!bounce_audio_event_has_sink(),
           "no Layer B sink is installed after a failed device open");
    expect(strcmp(bounce_audio_player_backend_name(player), "none") == 0,
           "a failed create reports no backend at all, not a substitute one");
    (void)bounce_audio_player_destroy(player);

    player = make(resource_root, BOUNCE_AUDIO_PLAYER_BACKEND_AUTO,
                  "bounce-no-such-pcm");
    expect(player == NULL,
           "AUTO on an unusable device is NULL -- it does NOT fall back to Null");
    expect(!bounce_audio_event_has_sink(),
           "AUTO failure installs no sink either");
    (void)bounce_audio_player_destroy(player);

    (void)printf("  5. gameplay is unaffected while audio is unavailable\n");
    expect(bounce_audio_event_emit(BOUNCE_AUDIO_EVENT_UP) == 0,
           "emitting UP with no sink is a safe no-op");
    expect(bounce_audio_event_emit(BOUNCE_AUDIO_EVENT_POP) == 0,
           "emitting POP with no sink is a safe no-op");
}

/* -------------------------------------------------------------------------
 * 6. NULL IS EXPLICIT ONLY.
 * ------------------------------------------------------------------------- */

static void test_null_is_explicit(const char *resource_root)
{
    BounceAudioPlayer *player;
    BounceAudioPlayerStats stats;
    unsigned int spins;
    int attached;

    (void)printf("  6. the Null backend is reachable only on explicit request\n");

    player = make(resource_root, BOUNCE_AUDIO_PLAYER_BACKEND_NULL, NULL);
    expect(player != NULL, "an explicit Null request creates a player");
    if (player == NULL) {
        return;
    }
    expect(strcmp(bounce_audio_player_backend_name(player), "null") == 0,
           "the backend is reported as null");

    attached = bounce_audio_player_attach_sink(player);
    expect(attached == 1, "the sink attaches to the Null-backed player");
    (void)bounce_audio_event_emit(BOUNCE_AUDIO_EVENT_POP);
    for (spins = 0u; spins < 4000u; spins++) {
        (void)bounce_audio_player_stats(player, &stats);
        if (stats.events_played >= 1ull) {
            break;
        }
        nap_ms(2u);
    }
    (void)bounce_audio_player_stats(player, &stats);
    expect(stats.events_played == 1ull && stats.frames_played == 13231ull,
           "the event rendered to exactly 13231 frames");

    bounce_audio_player_destroy(player);
    expect(!bounce_audio_event_has_sink(),
           "the sink is removed when an explicit Null player is destroyed");

    (void)printf("  7. a device name alone never selects Null\n");
    /*
     * Supplying a device name must not be a way to reach Null. It is passed to
     * the ALSA backend, and if that open fails the whole create fails.
     */
    player = make(resource_root, BOUNCE_AUDIO_PLAYER_BACKEND_AUTO,
                  "bounce-no-such-pcm");
    if (player != NULL) {
        expect(strcmp(bounce_audio_player_backend_name(player), "null") != 0,
               "a player built with a device name never reports the Null"
               " backend");
        bounce_audio_player_destroy(player);
    } else {
        expect(1, "a player built with a device name is NULL rather than Null");
    }
}

/* -------------------------------------------------------------------------
 * 8. CONFIGURATION ISOLATION -- the device setting touches nothing else.
 * ------------------------------------------------------------------------- */

static void test_isolation(const char *resource_root)
{
    BounceAudioPlayer *player;
    BounceAudioPlayer *player_named;
    BounceAudioPlayerStats stats;
    BounceAudioPlayerStats named_stats;
    unsigned int spins;

    (void)printf("  8. configuring a device changes nothing else\n");

    /* Layer B identity is untouched by any of this. */
    expect(BOUNCE_AUDIO_EVENT_NONE == 0 && BOUNCE_AUDIO_EVENT_UP == 1
               && BOUNCE_AUDIO_EVENT_PICKUP == 2 && BOUNCE_AUDIO_EVENT_POP == 3,
           "Layer B identity values are unchanged");
    expect(strcmp(bounce_audio_event_name(BOUNCE_AUDIO_EVENT_UP), "UP") == 0,
           "Layer B still names the UP identity");

    /* FIFO capacity is untouched. */
    expect(BOUNCE_AUDIO_PLAYER_QUEUE_CAPACITY == 8u,
           "FIFO capacity is still 8");

    /* The renderer is untouched: a device name must not perturb a single frame.
     * Null is used for the comparison because it is the one backend that is
     * openable on every host, so the frame counts are comparable anywhere.
     *
     * The two runs are SEQUENTIAL on purpose. Layer B installs exactly ONE global
     * sink, so attaching a second player replaces the first; running them at the
     * same time would measure the second player twice and the first not at all.
     * That was a real mistake in an earlier revision of this test, and it is
     * worth stating rather than quietly fixing. */
    player = make(resource_root, BOUNCE_AUDIO_PLAYER_BACKEND_NULL, NULL);
    expect(player != NULL, "a Null-backed player with no device name is created");
    if (player != NULL) {
        (void)bounce_audio_player_attach_sink(player);
        (void)bounce_audio_event_emit(BOUNCE_AUDIO_EVENT_UP);
        for (spins = 0u; spins < 4000u; spins++) {
            (void)bounce_audio_player_stats(player, &stats);
            if (stats.events_played >= 1ull) {
                break;
            }
            nap_ms(2u);
        }
        (void)bounce_audio_player_stats(player, &stats);
        expect(stats.frames_played == 26460ull,
               "without a device name the renderer still produces 26460 frames");
        bounce_audio_player_destroy(player);
    }
    expect(!bounce_audio_event_has_sink(),
           "no sink survives the first teardown");

    player_named = make(resource_root, BOUNCE_AUDIO_PLAYER_BACKEND_NULL,
                        "bounce-no-such-pcm");
    expect(player_named != NULL,
           "a Null-backed player WITH a device name is created");
    if (player_named != NULL) {
        (void)bounce_audio_player_attach_sink(player_named);
        (void)bounce_audio_event_emit(BOUNCE_AUDIO_EVENT_UP);
        for (spins = 0u; spins < 4000u; spins++) {
            (void)bounce_audio_player_stats(player_named, &named_stats);
            if (named_stats.events_played >= 1ull) {
                break;
            }
            nap_ms(2u);
        }
        (void)bounce_audio_player_stats(player_named, &named_stats);
        expect(named_stats.frames_played == 26460ull,
               "with a device name the renderer produces the identical 26460"
               " frames");
        bounce_audio_player_destroy(player_named);
    }
    expect(!bounce_audio_event_has_sink(),
           "no sink survives either teardown");
}

/* -------------------------------------------------------------------------
 * Host observation. Deliberately NOT an assertion.
 * ------------------------------------------------------------------------- */

static void report_host(const char *device)
{
    BounceAudioBackend backend;
    BounceAudioAlsaState state;
    BounceAudioBackendFormat format;
    BounceAudioBackendStatus st;
    const char *name;

    (void)printf("  9. host observation (NOT an assertion -- hardware varies)\n");

    name = BOUNCE_AUDIO_BACKEND_DEVICE_DEFAULT;
    memset(&state, 0, sizeof(state));
    if (bounce_audio_backend_alsa_init(&backend, &state, name)
        == BOUNCE_AUDIO_BACKEND_OK) {
        bounce_audio_backend_default_format(&format);
        st = backend.open(&backend, &format, name);
        (void)printf("      ..   \"%s\" -> %s\n", name,
                     bounce_audio_backend_status_text(st));
        backend.close(&backend);
    }

    if (device == NULL || device[0] == '\0') {
        return;
    }
    memset(&state, 0, sizeof(state));
    if (bounce_audio_backend_alsa_init(&backend, &state, device)
        == BOUNCE_AUDIO_BACKEND_OK) {
        bounce_audio_backend_default_format(&format);
        st = backend.open(&backend, &format, device);
        (void)printf("      ..   \"%s\" -> %s (%u Hz, %u ch)\n", device,
                     bounce_audio_backend_status_text(st),
                     state.sample_rate_hz, state.channels);
        backend.close(&backend);
    }
    (void)printf("      ..   an openable device here is a HOST FACT. It is"
                 " not asserted, and it is not\n");
    (void)printf("           evidence that any sound is audible.\n");
}

int main(int argc, char **argv)
{
    const char *resource_root = (argc > 1) ? argv[1] : "";
    const char *device = (argc > 2) ? argv[2] : "";

    (void)printf("audio device configuration self-test (STEP 14B-34-H)\n");
    (void)printf("  owner decision: Option B -- explicit device configuration\n");
    (void)printf("  no probing, no fallback chain, no sound is produced\n\n");

    (void)snd_lib_error_set_handler(alsa_silence);

    if (resource_root[0] == '\0') {
        (void)fprintf(stderr, "usage: %s <resource-root> [openable-device]\n",
                      (argc > 0) ? argv[0] : "audio_device_config_check");
        return 1;
    }

    test_default_unchanged();
    test_explicit_is_exact(device[0] != '\0' ? device : "plughw:1,0");
    test_no_silent_fallback(resource_root);
    test_null_is_explicit(resource_root);
    test_isolation(resource_root);
    report_host(device);

    if (failures != 0) {
        (void)printf("\naudio device configuration self-test: FAIL (%d"
                     " failure(s))\n", failures);
        (void)fprintf(stderr, "audio_device_config_check: FAILED\n");
        return 1;
    }
    (void)printf("\naudio device configuration self-test: PASS (0 failures)\n");
    (void)printf("no probing occurred; no device was substituted; no sample"
                 " was written\n");
    return 0;
}
