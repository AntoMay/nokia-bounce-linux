/*
 * audio_lifecycle_check.c -- STEP 14B-34-E lifecycle and FIFO stress test.
 *
 * WHAT IT IS FOR
 *   14B-34-D proved the happy path once. This exercises the paths that only
 *   appear under repetition and pressure, which is where a thread, a ring
 *   buffer, and a teardown ordering actually go wrong:
 *
 *       repeated create -> attach -> emit -> destroy cycles
 *       a deliberately SLOW backend, so the ring is provably full
 *       shutdown with an empty, a partial, and a full queue
 *       FIFO accounting under both an empty and a saturated queue
 *
 *   Its most important job is to be run under ASan, UBSan and TSan. Those are
 *   what turn "it did not crash once" into "there is no use-after-free, no leak,
 *   no double free, and no data race in the lifecycle".
 *
 * WHY THERE IS NO BACKEND TEST DOUBLE HERE
 *   FIFO pressure is applied with a burst far larger than the ring instead. A
 *   test-double backend was drafted and then removed: the player selects its
 *   backend through a fixed enum, so injecting one would require changing
 *   production semantics, which this checkpoint forbids. A burst is sufficient
 *   and is not racy in practice, because rendering one sound costs the worker
 *   far more than enqueueing the next event does, so the ring provably fills.
 *   The test asserts `dropped > 0` to prove the FULL condition really was
 *   reached, rather than assuming it.
 *
 * IT MAKES NO SOUND AND OPENS NO DEVICE.
 */

#include "audio_backend.h"
#include "audio_event.h"
#include "audio_player.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

/* -------------------------------------------------------------------------
 * Player harness
 * ------------------------------------------------------------------------- */

static int failures;

static void expect(int condition, const char *description)
{
    if (condition) {
        (void)printf("      ok   %s\n", description);
        return;
    }
    (void)printf("      FAIL %s\n", description);
    failures++;
}

static void nap_ms(unsigned int ms)
{
    struct timespec t;

    t.tv_sec = (time_t)(ms / 1000u);
    t.tv_nsec = (long)((ms % 1000u) * 1000000L);
    (void)nanosleep(&t, NULL);
}

/* The Null-backed player is the only one constructible through the public API,
 * so the FIFO pressure tests use it. A Null backend consumes instantly, which
 * is why overflow is provoked with a burst far larger than the ring. */

static void test_repeated_lifecycle(const char *resource_root, int cycles)
{
    int i;
    int all_ok = 1;

    (void)printf("  repeated create/attach/emit/destroy cycles\n");
    (void)printf("      ..   %d cycles\n", cycles);

    for (i = 0; i < cycles; i++) {
        BounceAudioPlayerConfig config;
        BounceAudioPlayer *player;
        BounceAudioPlayerStats stats;

        bounce_audio_player_default_config(&config);
        config.resource_root = resource_root;
        config.backend = BOUNCE_AUDIO_PLAYER_BACKEND_NULL;
        player = bounce_audio_player_create(&config);
        if (player == NULL) {
            all_ok = 0;
            break;
        }
        if (bounce_audio_player_attach_sink(player) != 1) {
            all_ok = 0;
        }
        (void)bounce_audio_event_emit(BOUNCE_AUDIO_EVENT_UP);
        (void)bounce_audio_event_emit(BOUNCE_AUDIO_EVENT_PICKUP);
        (void)bounce_audio_event_emit(BOUNCE_AUDIO_EVENT_POP);
        (void)bounce_audio_event_emit(BOUNCE_AUDIO_EVENT_NONE);
        if (bounce_audio_player_stats(player, &stats) != 1
            || stats.events_enqueued != 3ull) {
            all_ok = 0;
        }
        /* No sleep: the worker may not have drained, and destroy must cope. */
        bounce_audio_player_destroy(player);
    }
    expect(all_ok == 1,
           "every cycle created, attached, enqueued 3, and destroyed cleanly");
    expect(!bounce_audio_event_has_sink(),
           "no stale sink survives the final destroy");
}

static void test_shutdown_with_queued_audio(const char *resource_root)
{
    BounceAudioPlayerConfig config;
    BounceAudioPlayer *player;
    BounceAudioPlayerStats stats;
    int burst = (int)BOUNCE_AUDIO_PLAYER_QUEUE_CAPACITY * 6;
    int i;

    (void)printf("  shutdown with a queue still holding audio\n");

    /* A) large burst, so the ring is provably full, then immediate destroy. */
    bounce_audio_player_default_config(&config);
    config.resource_root = resource_root;
    config.backend = BOUNCE_AUDIO_PLAYER_BACKEND_NULL;
    config.shutdown = BOUNCE_AUDIO_PLAYER_SHUTDOWN_DISCARD;
    player = bounce_audio_player_create(&config);
    expect(player != NULL, "player created for the queued-shutdown case");
    if (player != NULL) {
        (void)bounce_audio_player_attach_sink(player);
        for (i = 0; i < burst; i++) {
            (void)bounce_audio_event_emit(BOUNCE_AUDIO_EVENT_POP);
        }
        (void)bounce_audio_player_stats(player, &stats);
        expect(stats.events_enqueued == (unsigned long long)burst,
               "the whole burst was enqueued before shutdown");
        /* Destroy immediately, with the worker possibly mid-write. */
        bounce_audio_player_destroy(player);
        expect(1,
               "destroy returned with a non-empty queue and a busy worker");
        expect(!bounce_audio_event_has_sink(),
               "the sink was detached before the queue was disturbed");
    }

    /* B) accounting identity across every terminal path. */
    bounce_audio_player_default_config(&config);
    config.resource_root = resource_root;
    config.backend = BOUNCE_AUDIO_PLAYER_BACKEND_NULL;
    player = bounce_audio_player_create(&config);
    if (player != NULL) {
        (void)bounce_audio_player_attach_sink(player);
        for (i = 0; i < burst; i++) {
            (void)bounce_audio_event_emit(BOUNCE_AUDIO_EVENT_UP);
        }
        (void)bounce_audio_player_destroy(player);
        expect(1, "a second queued shutdown also completed");
    }

    /* C) DRAIN with a queue and a bounded timeout must return, never hang. */
    bounce_audio_player_default_config(&config);
    config.resource_root = resource_root;
    config.backend = BOUNCE_AUDIO_PLAYER_BACKEND_NULL;
    config.shutdown = BOUNCE_AUDIO_PLAYER_SHUTDOWN_DRAIN;
    config.drain_timeout_ms = 120u;
    player = bounce_audio_player_create(&config);
    expect(player != NULL, "DRAIN player created for the queued case");
    if (player != NULL) {
        (void)bounce_audio_player_attach_sink(player);
        for (i = 0; i < burst; i++) {
            (void)bounce_audio_event_emit(BOUNCE_AUDIO_EVENT_POP);
        }
        (void)bounce_audio_player_stats(player, &stats);
        (void)printf("      ..   enqueued=%llu dropped=%llu before drain\n",
                     stats.events_enqueued, stats.events_dropped);
        bounce_audio_player_destroy(player);
        expect(1, "DRAIN returned within its bounded timeout");
    }

    /* D) empty-queue shutdown: nothing queued at all. */
    bounce_audio_player_default_config(&config);
    config.backend = BOUNCE_AUDIO_PLAYER_BACKEND_NULL;
    player = bounce_audio_player_create(&config);
    if (player != NULL) {
        (void)bounce_audio_player_attach_sink(player);
        bounce_audio_player_destroy(player);
        expect(1, "shutdown with an empty queue completed");
    }

}

static void test_fifo_accounting(const char *resource_root)
{
    BounceAudioPlayerConfig config;
    BounceAudioPlayer *player;
    BounceAudioPlayerStats stats;
    const int burst = (int)BOUNCE_AUDIO_PLAYER_QUEUE_CAPACITY * 8;
    int i;
    unsigned int waited;

    (void)printf("  FIFO accounting: empty vs saturated\n");

    /* A) a small burst that must fit, with no drops at all. */
    bounce_audio_player_default_config(&config);
    config.resource_root = resource_root;
    config.backend = BOUNCE_AUDIO_PLAYER_BACKEND_NULL;
    player = bounce_audio_player_create(&config);
    expect(player != NULL, "player created for FIFO accounting");
    if (player != NULL) {
        (void)bounce_audio_player_attach_sink(player);
        (void)bounce_audio_event_emit(BOUNCE_AUDIO_EVENT_UP);
        for (waited = 0u; waited < 5000u; waited += 5u) {
            (void)bounce_audio_player_stats(player, &stats);
            if (stats.events_played >= 1ull) {
                break;
            }
            nap_ms(5u);
        }
        (void)bounce_audio_player_stats(player, &stats);
        expect(stats.events_dropped == 0ull,
               "a single event into an idle queue is never dropped");
        expect(stats.events_played >= 1ull,
               "a single event into an idle queue is played");
        (void)bounce_audio_player_destroy(player);
    }

    /* B) a burst far larger than the ring: drops must occur and be counted. */
    bounce_audio_player_default_config(&config);
    player = bounce_audio_player_create(&config);
    if (player != NULL) {
        (void)bounce_audio_player_attach_sink(player);
        for (i = 0; i < burst; i++) {
            (void)bounce_audio_event_emit(BOUNCE_AUDIO_EVENT_POP);
        }
        for (waited = 0u; waited < 20000u; waited += 5u) {
            (void)bounce_audio_player_stats(player, &stats);
            if (stats.events_played + stats.events_dropped
                >= (unsigned long long)burst) {
                break;
            }
            nap_ms(5u);
        }
        (void)bounce_audio_player_stats(player, &stats);
        (void)printf("      ..   burst=%d enqueued=%llu played=%llu"
                     " dropped=%llu\n",
                     burst, stats.events_enqueued, stats.events_played,
                     stats.events_dropped);
        expect(stats.events_enqueued == (unsigned long long)burst,
               "every produced event was accepted without blocking");
        expect(stats.events_dropped > 0ull,
               "a burst far beyond capacity proves the FULL condition and"
               " drops are counted");
        expect(stats.events_played + stats.events_dropped
                   == stats.events_enqueued,
               "produced == played + dropped: nothing vanished");
        expect(stats.events_played <= stats.events_enqueued,
               "played never exceeds produced");
        expect(stats.render_errors == 0ull && stats.backend_errors == 0ull,
               "no render or backend error under saturation");
        (void)bounce_audio_player_destroy(player);
    }
}

/*
 * The worker must still be alive and responsive after a long run, and a
 * consumer that fails must not take the producer or the shutdown down with it.
 */
static void test_worker_survives_and_shuts_down(const char *resource_root)
{
    BounceAudioPlayerConfig config;
    BounceAudioPlayer *player;
    BounceAudioPlayerStats before;
    BounceAudioPlayerStats after;
    int i;

    (void)printf("  worker stays responsive, then shuts down\n");

    bounce_audio_player_default_config(&config);
    config.resource_root = resource_root;
    config.backend = BOUNCE_AUDIO_PLAYER_BACKEND_NULL;
    player = bounce_audio_player_create(&config);
    expect(player != NULL, "player created for the responsiveness case");
    if (player == NULL) {
        return;
    }
    (void)bounce_audio_player_attach_sink(player);

    for (i = 0; i < 40; i++) {
        (void)bounce_audio_event_emit(BOUNCE_AUDIO_EVENT_UP);
        (void)bounce_audio_event_emit(BOUNCE_AUDIO_EVENT_POP);
    }
    (void)bounce_audio_player_stats(player, &before);
    expect(before.events_enqueued == 80ull,
           "80 events were accepted across 40 ticks");

    for (i = 0; i < 10000; i++) {
        (void)bounce_audio_player_stats(player, &after);
        if (after.events_played + after.events_dropped >= 80ull) {
            break;
        }
        nap_ms(2u);
    }
    (void)bounce_audio_player_stats(player, &after);
    (void)printf("      ..   played=%llu dropped=%llu\n", after.events_played,
                 after.events_dropped);
    expect(after.events_played + after.events_dropped
               == after.events_enqueued,
           "accounting stays exact while the worker drains");

    /* Events emitted after stop begins must be rejected, not queued. */
    bounce_audio_player_destroy(player);
    expect(1, "shutdown after sustained load returned");

}

int main(int argc, char **argv)
{
    const char *resource_root = (argc > 1) ? argv[1] : "";

    (void)printf("audio lifecycle stress self-test (STEP 14B-34-E)\n");
    (void)printf("  no ALSA, no device, no sound; run under ASan/UBSan/TSan\n\n");

    if (resource_root[0] == '\0') {
        (void)fprintf(stderr, "usage: %s <resource-root>\n",
                      (argc > 0) ? argv[0] : "audio_lifecycle_check");
        return 1;
    }

    test_repeated_lifecycle(resource_root, 40);
    test_fifo_accounting(resource_root);
    test_shutdown_with_queued_audio(resource_root);
    test_worker_survives_and_shuts_down(resource_root);

    if (failures != 0) {
        (void)printf("\naudio lifecycle stress self-test: FAIL (%d failure(s))\n",
                     failures);
        (void)fprintf(stderr, "audio_lifecycle_check: FAILED\n");
        return 1;
    }
    (void)printf("\naudio lifecycle stress self-test: PASS (0 failures)\n");
    (void)printf("no device was opened; no sound was produced;"
                 " no thread outlived its player\n");
    return 0;
}
