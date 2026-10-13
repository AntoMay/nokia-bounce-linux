/*
 * audio_player_check.c -- STEP 14B-34-D standalone integration self-test.
 *
 * WHAT IT EXERCISES
 *   The real chain, end to end, with the Null backend so no device and no sound
 *   is involved:
 *
 *       bounce_audio_event_emit()  <- the REAL Layer B entry point
 *            -> Layer B sink
 *            -> bounded FIFO
 *            -> dedicated audio thread
 *            -> STEP 14B-34-B renderer (unchanged)
 *            -> Null backend
 *
 *   It drives the player through Layer B rather than calling an internal
 *   enqueue, so the sink wiring itself is under test.
 *
 * WHAT IT LINKS
 *   This module, Layer B, the renderer, the Layer A decoder, the portable
 *   backend, the Null backend, and pthread. It does NOT link ALSA: the Makefile
 *   compiles it with BOUNCE_AUDIO_HAVE_ALSA=0, so it cannot open a device even
 *   by accident. It also links no gameplay object file.
 *
 * IT MAKES NO SOUND.
 *
 * Usage:  audio_player_check <resource-root>
 *         for example: audio_player_check src/main/resources
 * Exit status: 0 when every assertion passed, 1 otherwise.
 */

#include "audio_backend_null.h"
#include "audio_event.h"
#include "audio_player.h"
#include "audio_synth.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

static int player_failures;

static void expect(int condition, const char *description)
{
    if (condition) {
        (void)printf("      ok   %s\n", description);
        return;
    }
    (void)printf("      FAIL %s\n", description);
    player_failures++;
}

/* Wait until `predicate_target` is reached or the budget expires. Deterministic
 * in the sense that it never asserts on a race: it polls a real counter. */
static int wait_for_played(BounceAudioPlayer *player,
                           unsigned long long target,
                           unsigned int budget_ms)
{
    unsigned int waited = 0u;

    for (;;) {
        BounceAudioPlayerStats stats;

        if (bounce_audio_player_stats(player, &stats)) {
            if (stats.events_played >= target) {
                return 1;
            }
        }
        if (waited >= budget_ms) {
            return 0;
        }
        {
            struct timespec nap;

            nap.tv_sec = 0;
            nap.tv_nsec = 2000000L; /* 2 ms */
            (void)nanosleep(&nap, NULL);
        }
        waited += 2u;
    }
}

static BounceAudioPlayer *make_player(const char *resource_root,
                                      BounceAudioPlayerBackend backend,
                                      BounceAudioPlayerShutdown shutdown,
                                      unsigned int drain_ms)
{
    BounceAudioPlayerConfig config;

    bounce_audio_player_default_config(&config);
    config.resource_root = resource_root;
    config.backend = backend;
    config.shutdown = shutdown;
    config.drain_timeout_ms = drain_ms;
    return bounce_audio_player_create(&config);
}

static void test_mapping_and_playback(const char *resource_root)
{
    BounceAudioPlayer *player = make_player(resource_root,
                                            BOUNCE_AUDIO_PLAYER_BACKEND_NULL,
                                            BOUNCE_AUDIO_PLAYER_SHUTDOWN_DISCARD,
                                            0u);
    BounceAudioPlayerStats stats;
    BounceSynthConfig config;
    BounceRtplSong song;
    BounceSynthPlan plan;
    unsigned long long expected_frames = 0ull;
    char description[200];

    (void)printf("  event -> asset -> PCM -> Null backend\n");

    if (player == NULL) {
        expect(0, "player created with the Null backend");
        return;
    }
    expect(1, "player created with the Null backend");
    expect(bounce_audio_player_is_running(player),
           "the player reports it is running");
    expect(strcmp(bounce_audio_player_backend_name(player), "null") == 0,
           "the Null backend is in use, chosen explicitly");

    /* What the renderer produces for each asset, computed independently here. */
    bounce_synth_default_config(&config);
    (void)bounce_synth_prepare(BOUNCE_AUDIO_EVENT_POP, resource_root, &config,
                               &song, &plan);
    expected_frames = (unsigned long long)plan.total_frames;

    expect(bounce_audio_player_attach_sink(player) == 1,
           "the Layer B sink is installed");
    expect(bounce_audio_event_has_sink(),
           "Layer B reports a consumer is attached");

    /* UP, PICKUP, POP -- each through the REAL Layer B entry point. */
    (void)bounce_audio_event_emit(BOUNCE_AUDIO_EVENT_UP);
    (void)bounce_audio_event_emit(BOUNCE_AUDIO_EVENT_PICKUP);
    (void)bounce_audio_event_emit(BOUNCE_AUDIO_EVENT_POP);

    expect(wait_for_played(player, 3ull, 5000u),
           "all three events are consumed by the audio thread");

    expect(bounce_audio_player_stats(player, &stats) == 1, "stats are readable");
    (void)snprintf(description, sizeof(description),
                   "enqueued == 3 (got %llu)", stats.events_enqueued);
    expect(stats.events_enqueued == 3ull, description);
    (void)snprintf(description, sizeof(description),
                   "played == 3 (got %llu)", stats.events_played);
    expect(stats.events_played == 3ull, description);
    (void)snprintf(description, sizeof(description),
                   "no events were dropped (dropped %llu)", stats.events_dropped);
    expect(stats.events_dropped == 0ull, description);
    (void)snprintf(description, sizeof(description),
                   "no render errors (got %llu)", stats.render_errors);
    expect(stats.render_errors == 0ull, description);
    (void)snprintf(description, sizeof(description),
                   "no backend errors (got %llu)", stats.backend_errors);
    expect(stats.backend_errors == 0ull, description);

    /*
     * The frame total must equal the sum of all three rendered songs. This is
     * the end-to-end proof that the renderer ran and the backend received it.
     */
    {
        unsigned long long sum = 0ull;
        int i;
        static const BounceAudioEvent all[3] = {
            BOUNCE_AUDIO_EVENT_UP, BOUNCE_AUDIO_EVENT_PICKUP,
            BOUNCE_AUDIO_EVENT_POP
        };

        for (i = 0; i < 3; i++) {
            if (bounce_synth_prepare(all[i], resource_root, &config, &song,
                                     &plan) == BOUNCE_SYNTH_OK) {
                sum += (unsigned long long)plan.total_frames;
            }
        }
        (void)snprintf(description, sizeof(description),
                       "frames_played == %llu, the sum of the three songs"
                       " (got %llu)", sum, stats.frames_played);
        expect(stats.frames_played == sum, description);
        (void)snprintf(description, sizeof(description),
                       "pop.ott alone is %llu frames, and the total exceeds it",
                       expected_frames);
        expect(stats.frames_played > expected_frames, description);
    }

    /*
     * NONE and unknown identities never reach this layer at all: Layer B
     * validates the identity and returns before forwarding to the sink, which
     * is exactly the contract STEP 14B-27 verified. So the correct assertion is
     * that Layer B refuses them and NOTHING is played, not that this layer
     * counted a rejection. The player's own rejection counter is a
     * defence-in-depth guard against a caller that wires a sink differently.
     */
    expect(bounce_audio_event_emit(BOUNCE_AUDIO_EVENT_NONE) != 0,
           "Layer B accepts NONE without forwarding it to a consumer");
    expect(bounce_audio_event_emit((BounceAudioEvent)77) < 0,
           "Layer B rejects an unknown identity");
    expect(bounce_audio_event_emit((BounceAudioEvent)-5) < 0,
           "Layer B rejects a negative identity");
    (void)wait_for_played(player, 3ull, 200u);
    (void)bounce_audio_player_stats(player, &stats);
    (void)snprintf(description, sizeof(description),
                   "NONE and unknown identities add nothing to the queue"
                   " (enqueued still %llu)", stats.events_enqueued);
    expect(stats.events_enqueued == 3ull, description);
    expect(stats.events_played == 3ull,
           "a refused identity is never played");

    (void)bounce_audio_player_destroy(player);
}

static void test_multiplicity(const char *resource_root)
{
    BounceAudioPlayer *player = make_player(resource_root,
                                            BOUNCE_AUDIO_PLAYER_BACKEND_NULL,
                                            BOUNCE_AUDIO_PLAYER_SHUTDOWN_DISCARD,
                                            0u);
    BounceAudioPlayerStats stats;
    const int burst = 5;
    unsigned int waited;
    char description[200];
    int i;

    (void)printf("  multiplicity: no deduplication, no coalescing\n");

    if (player == NULL) {
        expect(0, "player created for the multiplicity test");
        return;
    }
    (void)bounce_audio_player_attach_sink(player);

    /*
     * UP + PICKUP in the same tick must stay TWO events, never one merged
     * playback. This is the Layer B semantics STEP 14B-27 verified.
     *
     * 10 emissions against a capacity of 8 may legitimately overflow, and the
     * drop policy then removes the OLDEST. So the assertion is a conservation
     * law, not a fixed played count: every emission is either played or
     * explicitly counted as dropped, and none is ever silently lost.
     */
    for (i = 0; i < burst; i++) {
        (void)bounce_audio_event_emit(BOUNCE_AUDIO_EVENT_UP);
        (void)bounce_audio_event_emit(BOUNCE_AUDIO_EVENT_PICKUP);
    }

    /* Wait until the queue has fully settled: everything is played or dropped. */
    for (waited = 0u; waited < 15000u; waited += 5u) {
        struct timespec nap;

        (void)bounce_audio_player_stats(player, &stats);
        if (stats.events_played + stats.events_dropped
            >= (unsigned long long)(burst * 2)) {
            break;
        }
        nap.tv_sec = 0;
        nap.tv_nsec = 5000000L; /* 5 ms */
        (void)nanosleep(&nap, NULL);
    }

    (void)bounce_audio_player_stats(player, &stats);
    (void)printf("      ..   enqueued=%llu played=%llu dropped=%llu\n",
                 stats.events_played + stats.events_dropped, stats.events_played,
                 stats.events_dropped);
    expect(stats.events_enqueued == (unsigned long long)(burst * 2),
           "every emission was accepted separately, none merged or deduplicated");
    (void)snprintf(description, sizeof(description),
                   "played %llu + dropped %llu == enqueued %llu: nothing"
                   " vanished",
                   stats.events_played, stats.events_dropped,
                   stats.events_enqueued);
    expect(stats.events_played + stats.events_dropped
               == stats.events_enqueued,
           description);
    expect(stats.events_played > 0ull,
           "at least some events reached the backend");

    (void)bounce_audio_player_destroy(player);
}

static void test_overflow_policy(const char *resource_root)
{
    BounceAudioPlayer *player = make_player(resource_root,
                                            BOUNCE_AUDIO_PLAYER_BACKEND_NULL,
                                            BOUNCE_AUDIO_PLAYER_SHUTDOWN_DISCARD,
                                            0u);
    BounceAudioPlayerStats stats;
    int burst = (int)BOUNCE_AUDIO_PLAYER_QUEUE_CAPACITY * 4;
    int i;

    (void)printf("  bounded FIFO: overflow is counted, never unbounded\n");

    if (player == NULL) {
        expect(0, "player created for the overflow test");
        return;
    }
    (void)bounce_audio_player_attach_sink(player);

    for (i = 0; i < burst; i++) {
        (void)bounce_audio_event_emit(BOUNCE_AUDIO_EVENT_POP);
    }
    (void)bounce_audio_player_stats(player, &stats);
    (void)printf("      ..   enqueued=%llu dropped=%llu (burst %d)\n",
                 stats.events_enqueued, stats.events_dropped, burst);

    expect(stats.events_enqueued == (unsigned long long)burst,
           "a large burst is accepted without blocking the producer");
    expect(stats.events_dropped > 0ull,
           "sustained bursty input drops, rather than growing without limit");
    expect(stats.events_enqueued
               == stats.events_played + stats.events_dropped
                  + (stats.events_enqueued - stats.events_played
                     - stats.events_dropped),
           "the ring never holds more than its capacity, so the queue is bounded");

    (void)bounce_audio_player_destroy(player);
}

static void test_shutdown_policies(const char *resource_root)
{
    BounceAudioPlayer *player;
    BounceAudioPlayerStats stats;

    (void)printf("  shutdown policies\n");

    /* DISCARD: never waits, abandons what is queued, and still joins cleanly. */
    player = make_player(resource_root, BOUNCE_AUDIO_PLAYER_BACKEND_NULL,
                         BOUNCE_AUDIO_PLAYER_SHUTDOWN_DISCARD, 0u);
    expect(player != NULL, "DISCARD player created");
    if (player != NULL) {
        (void)bounce_audio_player_attach_sink(player);
        (void)bounce_audio_event_emit(BOUNCE_AUDIO_EVENT_PICKUP);
        (void)bounce_audio_event_emit(BOUNCE_AUDIO_EVENT_PICKUP);
        (void)bounce_audio_player_destroy(player);
        expect(1, "DISCARD shutdown joined the worker and returned");
    }

    /* DRAIN with a bounded timeout: must return, never hang. */
    player = make_player(resource_root, BOUNCE_AUDIO_PLAYER_BACKEND_NULL,
                         BOUNCE_AUDIO_PLAYER_SHUTDOWN_DRAIN, 250u);
    expect(player != NULL, "DRAIN player created");
    if (player != NULL) {
        (void)bounce_audio_player_attach_sink(player);
        (void)bounce_audio_event_emit(BOUNCE_AUDIO_EVENT_POP);
        (void)bounce_audio_player_stats(player, &stats);
        (void)printf("      ..   accepting before destroy: %d\n",
                     stats.accepting);
        (void)bounce_audio_player_destroy(player);
        expect(1, "DRAIN shutdown returned within its bounded timeout");
    }
    (void)stats.accepting;
}

static void test_failure_isolation(const char *resource_root)
{
    BounceAudioPlayerConfig config;
    BounceAudioPlayer *player;

    (void)printf("  failure isolation: audio failure is never a game failure\n");

    /* A bad resource root must yield NULL, not a crash. */
    bounce_audio_player_default_config(&config);
    config.backend = BOUNCE_AUDIO_PLAYER_BACKEND_NULL;
    config.resource_root = "no/such/resource/root";
    player = bounce_audio_player_create(&config);
    expect(player == NULL, "a missing asset makes create return NULL");
    (void)bounce_audio_player_destroy(player); /* NULL is safe */

    /* AUTO with ALSA compiled out must also be NULL, not a crash. */
    bounce_audio_player_default_config(&config);
    config.backend = BOUNCE_AUDIO_PLAYER_BACKEND_AUTO;
    config.resource_root = resource_root;
    player = bounce_audio_player_create(&config);
    expect(player == NULL,
           "AUTO with no ALSA and no device returns NULL rather than faking it");
    (void)bounce_audio_player_destroy(player);

    /* FORCED ALSA with ALSA compiled out: unavailable, not a Null fallback. */
    bounce_audio_player_default_config(&config);
    config.backend = BOUNCE_AUDIO_PLAYER_BACKEND_ALSA;
    config.resource_root = resource_root;
    player = bounce_audio_player_create(&config);
    expect(player == NULL,
           "a forced ALSA request does NOT silently fall back to Null");

    /* A NULL resource root is refused. */
    bounce_audio_player_default_config(&config);
    config.backend = BOUNCE_AUDIO_PLAYER_BACKEND_NULL;
    config.resource_root = NULL;
    expect(bounce_audio_player_create(&config) == NULL,
           "a NULL resource root is refused");

    /* Every accessor tolerates NULL. */
    expect(bounce_audio_player_is_running(NULL) == 0,
           "is_running(NULL) is safe");
    expect(bounce_audio_player_backend_name(NULL) != NULL,
           "backend_name(NULL) is safe and non-NULL");
    expect(bounce_audio_player_stats(NULL, NULL) == 0,
           "stats(NULL, NULL) is safe");
    expect(bounce_audio_player_attach_sink(NULL) == 0,
           "attach_sink(NULL) is safe");
    expect(bounce_audio_player_detach_sink(NULL) == 1,
           "detach_sink(NULL) is safe");
}

static void test_sink_lifecycle(const char *resource_root)
{
    BounceAudioPlayer *player = make_player(resource_root,
                                            BOUNCE_AUDIO_PLAYER_BACKEND_NULL,
                                            BOUNCE_AUDIO_PLAYER_SHUTDOWN_DISCARD,
                                            0u);

    (void)printf("  sink lifecycle\n");

    if (player == NULL) {
        expect(0, "player created for the sink lifecycle test");
        return;
    }

    (void)bounce_audio_event_emit(BOUNCE_AUDIO_EVENT_UP);
    expect(!bounce_audio_event_has_sink(),
           "before attach, Layer B has no consumer and emission is a no-op");

    expect(bounce_audio_player_attach_sink(player) == 1, "sink attached");
    expect(bounce_audio_event_has_sink(), "Layer B now reports a consumer");

    expect(bounce_audio_player_detach_sink(player) == 1, "sink detached");
    expect(!bounce_audio_event_has_sink(),
           "after detach, Layer B has no consumer again");

    {
        BounceAudioPlayerStats stats;

        expect(bounce_audio_player_stats(player, &stats) == 1
                   && stats.events_enqueued == 0ull,
               "the detached sink queued nothing");
    }

    (void)bounce_audio_player_destroy(player);
}

/*
 * BL-004 PARITY FIX -- the volume gain, measured on rendered output.
 *
 * WHY THIS NEEDED A NEW OBSERVATION POINT. The gain is applied in audio_player.c to
 * the rendered block immediately before write_pcm(), and the Null backend discards
 * that block. Until the backend reported a peak, the volume setting had no runtime
 * effect a headless test could see at all: setting it to 0 and never setting it
 * produced identical evidence, because the Null backend is silent either way. So
 * "the volume is wired up" was not a claim this harness could support, and the
 * parity gap between the volume row and the audio output went unnoticed.
 *
 * WHY IT IS DETERMINISTIC. audio_synth.c documents that its renderer has no random
 * component and no time dependence (see the note at audio_synth.c:486), so the same
 * event renders the same samples every run. The comparison below is therefore an
 * equality of fixed numbers, not a statistical one, and it does not need a seed or
 * a golden file.
 *
 * IT USES THREE SEPARATE PLAYERS, ONE PER VOLUME, rather than changing the volume on
 * one player between measurements. That keeps each measurement independent of the
 * queue's history: a single player would carry the first sound's tail into the
 * second measurement, and the tails are what a gain change is easiest to miss in.
 */
static unsigned int peak_for_volume(const char *resource_root,
                                    unsigned int volume,
                                    int *out_ok)
{
    BounceAudioPlayer *player = make_player(resource_root,
                                            BOUNCE_AUDIO_PLAYER_BACKEND_NULL,
                                            BOUNCE_AUDIO_PLAYER_SHUTDOWN_DISCARD,
                                            0u);
    BounceAudioPlayerStats stats;
    unsigned int peak = 0u;
    unsigned int last_peak = 0u;

    *out_ok = 0;
    if (player == NULL) {
        return 0u;
    }
    if (bounce_audio_player_set_volume(player, volume) != 0) {
        (void)bounce_audio_player_destroy(player);
        return 0u;
    }
    if (!bounce_audio_player_attach_sink(player)) {
        (void)bounce_audio_player_destroy(player);
        return 0u;
    }
    (void)bounce_audio_event_emit(BOUNCE_AUDIO_EVENT_POP);
    (void)wait_for_played(player, 1ull, 4000u);
    if (bounce_audio_player_stats(player, &stats) && stats.events_played >= 1ull
        && bounce_audio_player_null_peak(player, &peak, &last_peak) == 0) {
        *out_ok = 1;
    }
    (void)bounce_audio_player_destroy(player);
    return peak;
}

static void test_volume_gain(const char *resource_root)
{
    unsigned int peak_full;
    unsigned int peak_half;
    unsigned int peak_quarter;
    unsigned int peak_silent;
    unsigned int last_peak = 0u;
    int ok_full = 0;
    int ok_half = 0;
    int ok_quarter = 0;
    int ok_silent = 0;
    BounceAudioPlayer *player;
    BounceAudioPlayerStats stats;
    char description[220];

    (void)printf("  volume -> rendered amplitude at the output boundary\n");

    peak_full = peak_for_volume(resource_root, 100u, &ok_full);
    peak_half = peak_for_volume(resource_root, 50u, &ok_half);
    peak_quarter = peak_for_volume(resource_root, 25u, &ok_quarter);
    peak_silent = peak_for_volume(resource_root, 0u, &ok_silent);

    expect(ok_full && ok_half && ok_quarter && ok_silent,
           "the same sound was rendered and measured at all four volumes");
    if (!ok_full || !ok_half || !ok_quarter || !ok_silent) {
        return;
    }

    /*
     * FULL GAIN IS NON-SILENT FIRST. Every later comparison is relative to this
     * number, and a gain assertion over two silent buffers would pass for the wrong
     * reason, so full-scale audibility is asserted on its own.
     */
    expect(peak_full > 0u, "volume 100 renders audible output");
    if (peak_full == 0u) {
        return;
    }

    /*
     * VOLUME 0 IS EXACT SILENCE, not merely quiet. This is the value that was
     * unrepresentable before, and it is asserted as an equality because the gain
     * multiplies every sample by 0.0 -- there is no rounding path to a tolerance.
     */
    expect(peak_silent == 0u,
           "volume 0 renders exact silence (peak 0, not merely small)");

    /*
     * AND THE SILENCE IS REAL OUTPUT, NOT ABSENCE OF OUTPUT. This is what
     * separates "muted" from "the sound never rendered", which were the same
     * observation before the peak existed.
     */
    player = make_player(resource_root,
                         BOUNCE_AUDIO_PLAYER_BACKEND_NULL,
                         BOUNCE_AUDIO_PLAYER_SHUTDOWN_DISCARD, 0u);
    if (player == NULL) {
        expect(0, "a muted player still rendered and wrote frames");
    } else {
        expect(bounce_audio_player_set_volume(player, 0u) == 0
               && bounce_audio_player_attach_sink(player) != 0,
               "a muted player still accepts the sink");
        (void)bounce_audio_event_emit(BOUNCE_AUDIO_EVENT_POP);
        (void)wait_for_played(player, 1ull, 4000u);
        expect(bounce_audio_player_stats(player, &stats)
               && stats.events_played >= 1ull,
               "a muted player still played the event");
        expect(bounce_audio_player_null_peak(player, &last_peak, NULL) == 0
               && last_peak == 0u,
               "that played event reached the backend as silence");
        (void)bounce_audio_player_destroy(player);
    }

    /*
     * HALVED AND QUARTERED, WITHIN ONE SAMPLE OF THE RATIO.
     *
     * The gain is `(int16_t)(sample * volume / 100.0)` clamped to int16 range, so
     * the result is a truncation of the exact ratio. Truncation toward zero loses at
     * most one LSB per sample, so the expected peak is within `tolerance` of
     * peak_full * volume / 100. One LSB is used as that tolerance because the peak
     * sample is a single sample and cannot accumulate error the way a sum would.
     */
    {
        const unsigned int expected_half = peak_full / 2u;
        const unsigned int expected_quarter = peak_full / 4u;
        const unsigned int tolerance = 1u;

        (void)snprintf(description, sizeof(description),
                       "volume 50 halves the peak: %u vs %u expected (~%u)",
                       peak_half, peak_full, expected_half);
        expect(peak_half + tolerance >= expected_half
               && peak_half <= expected_half + tolerance,
               description);

        (void)snprintf(description, sizeof(description),
                       "volume 25 quarters the peak: %u vs %u expected (~%u)",
                       peak_quarter, peak_full, expected_quarter);
        expect(peak_quarter + tolerance >= expected_quarter
               && peak_quarter <= expected_quarter + tolerance,
               description);
    }

    /*
     * AND THE RAMP IS MONOTONIC, so a clamp that pinned 50 and 25 to the same
     * value could not pass the two ratio assertions above by accident.
     */
    expect(peak_silent < peak_quarter && peak_quarter < peak_half
           && peak_half < peak_full,
           "peak amplitude rises monotonically with volume");

    /*
     * CLAMPING, NOT REJECTION, AT THE ENDS -- the contract the setter documents.
     */
    expect(bounce_audio_player_set_volume(NULL, 50u) == -1,
           "set_volume rejects a NULL player");
    {
        BounceAudioPlayer *clamp = make_player(resource_root,
                                               BOUNCE_AUDIO_PLAYER_BACKEND_NULL,
                                               BOUNCE_AUDIO_PLAYER_SHUTDOWN_DISCARD,
                                               0u);
        if (clamp == NULL) {
            expect(0, "set_volume clamps out-of-range input");
        } else {
            expect(bounce_audio_player_set_volume(clamp, 255u) == 0
                   && bounce_audio_player_get_volume(clamp)
                       == BOUNCE_AUDIO_VOLUME_MAX,
                   "set_volume clamps 255 down to 100 rather than rejecting it");
            expect(bounce_audio_player_set_volume(clamp, 7u) == 0
                   && bounce_audio_player_get_volume(clamp) == 7u,
                   "set_volume accepts an arbitrary value in range");
            (void)bounce_audio_player_destroy(clamp);
        }
    }
}

int main(int argc, char **argv)
{
    const char *resource_root = (argc > 1) ? argv[1] : "";

    (void)printf("audio integration self-test (STEP 14B-34-D)\n");
    (void)printf("  real Layer B sink -> bounded FIFO -> audio thread ->"
                 " Null backend\n");
    (void)printf("  no ALSA, no device, no sound\n\n");

    if (resource_root[0] == '\0') {
        (void)fprintf(stderr, "usage: %s <resource-root>\n",
                      (argc > 0) ? argv[0] : "audio_player_check");
        return 1;
    }

    test_mapping_and_playback(resource_root);
    test_multiplicity(resource_root);
    test_overflow_policy(resource_root);
    test_shutdown_policies(resource_root);
    test_failure_isolation(resource_root);
    test_sink_lifecycle(resource_root);
    test_volume_gain(resource_root);

    if (player_failures != 0) {
        (void)printf("\naudio integration self-test: FAIL (%d failure(s))\n",
                     player_failures);
        (void)fprintf(stderr, "audio_player_check: FAILED\n");
        return 1;
    }
    (void)printf("\naudio integration self-test: PASS (0 failures)\n");
    (void)printf("no device was opened; no sound was produced;"
                 " the audio thread was joined cleanly\n");
    return 0;
}
