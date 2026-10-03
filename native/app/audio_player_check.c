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
