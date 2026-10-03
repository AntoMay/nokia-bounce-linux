/*
 * audio_runtime_check.c -- STEP 14B-34-F runtime / audibility verification.
 *
 * WHAT THIS FILE IS FOR
 *   14B-34-E proved a real ALSA device opens and that a real snd_pcm_writei()
 *   succeeds, but it wrote SILENCE written directly by the backend self-test.
 *   That proves the BACKEND reaches the device. It does not prove the game's
 *   own pipeline -- Layer B event, sink, FIFO, worker thread, RTPL->PCM
 *   renderer, backend -- delivers the three actual game sounds to a real device.
 *
 *   This harness does that. It drives the IDENTICAL production code path the
 *   game uses, from bounce_audio_event_emit() through to snd_pcm_writei(), and
 *   reports the three evidence levels separately:
 *
 *       LEVEL A  backend initialized
 *       LEVEL B  PCM submitted to the real ALSA device
 *       LEVEL C  human-confirmed audible output
 *
 *   LEVEL C CANNOT BE ESTABLISHED BY ANY CODE IN THIS FILE. It is reported
 *   NOT PROVEN unconditionally. Only the project owner, by listening, can
 *   change it. A successful snd_pcm_writei() is NOT audibility and is never
 *   reported as such.
 *
 * NO PRODUCTION CODE IS CHANGED OR ADDED BY THIS FILE
 *   It is a test binary. It calls the same public entry points
 *   (bounce_audio_event_emit, bounce_audio_player_create,
 *   bounce_audio_player_attach_sink, bounce_audio_player_stats) that
 *   vertical_slice.c calls, and the same frozen renderer the worker calls.
 *   Nothing else in the native app tree learns that this file exists.
 *
 * WHY THE HARNESS DRIVES THE PIPELINE RATHER THAN THE GAME BINARY
 *   The game reaches its audio player only through
 *   BounceAudioPlayerConfig.device_name, and vertical_slice.c leaves that field
 *   NULL, meaning "default". On this host "default" does not open. There is no
 *   runtime override, because adding one is a production change AND a
 *   device-policy decision that STEP 14B-34-F explicitly defers (PHASE 8). So
 *   the pipeline is driven here, unchanged, with the device named explicitly --
 *   which is exactly what BounceAudioPlayerConfig.device_name was already
 *   designed to accept. This is reported as a gap, not worked around silently.
 *
 * SILENCE CONTRACT
 *   Default run (no BOUNCE_AUDIO_ALSA_SMOKE): opens devices to report their
 *   availability, renders PCM into memory for numerical analysis, and drives
 *   the full pipeline against the Null backend. It writes NO samples to ANY
 *   device and makes NO sound.
 *   Opt-in run (BOUNCE_AUDIO_ALSA_SMOKE=1): additionally plays the three real
 *   game sounds on the real device. That produces sound. It is opt-in only.
 */

#include "audio_backend.h"
#include "audio_backend_alsa.h"
#include "audio_event.h"
#include "audio_player.h"
#include "audio_synth.h"
#include "rtpl_decoder.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Reuse the ONE opt-in switch already defined by STEP 14B-34-C/D/E. */
#define RUNTIME_SMOKE_VAR "BOUNCE_AUDIO_ALSA_SMOKE"

/*
 * The two device names STEP 14B-34-F requires compared. "default" is what the
 * production config resolves to; "plughw:0,3" is the real device 14B-34-E
 * proved openable. Neither is hard-coded into production: they are arguments to
 * a test binary.
 */
#define RUNTIME_DEVICE_PRODUCTION_DEFAULT "default"
#define RUNTIME_DEVICE_KNOWN_GOOD "plughw:0,3"

static int failures;
static const char *g_resource_root = "src/main/resources";

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

/* -------------------------------------------------------------------------
 * PHASE 7 -- numerical quality analysis of the rendered PCM
 *
 * These are MEASUREMENTS of the Target B reconstruction, not claims about the
 * original. A number here says what this renderer emits; it says nothing about
 * what a Nokia handset produced. The waveform and the Natural rest magnitude
 * remain UNKNOWN -- EVIDENCE EXHAUSTED, and this file does not reopen them.
 * ------------------------------------------------------------------------- */

static void analyse_event(BounceAudioEvent event,
                          const char *resource_root,
                          unsigned int sample_rate)
{
    BounceSynthConfig config;
    static BounceRtplSong song;
    BounceSynthPlan plan;
    int16_t *pcm;
    size_t written = 0u;
    unsigned long frames;
    unsigned long i;
    long lead = 0;
    long trail = 0;
    int peak = 0;
    int max_step = 0;
    int in_run = 0;
    unsigned int runs = 0u;
    unsigned long clipped = 0uL;
    double sum_sq = 0.0;
    double sum = 0.0;
    BounceSynthStatus st;

    bounce_synth_default_config(&config);
    st = bounce_synth_prepare(event, resource_root, &config, &song, &plan);
    expect(st == BOUNCE_SYNTH_OK, "render plan builds for the event");
    if (st != BOUNCE_SYNTH_OK) {
        (void)printf("        prepare = %s\n", bounce_synth_status_text(st));
        return;
    }

    /* +1 frame of headroom, so a renderer overrun is visible rather than fatal. */
    pcm = (int16_t *)calloc((size_t)plan.total_frames + 1u, sizeof(int16_t));
    if (pcm == NULL) {
        expect(0, "allocation for the rendered PCM");
        return;
    }

    st = bounce_synth_render_pcm(&song, &config, &plan, pcm,
                                 (size_t)plan.total_frames + 1u, &written);
    expect(st == BOUNCE_SYNTH_OK, "PCM renders for the event");
    if (st != BOUNCE_SYNTH_OK) {
        (void)printf("        render_pcm = %s\n",
                     bounce_synth_status_text(st));
        free(pcm);
        return;
    }
    frames = (unsigned long)written;
    expect(frames == plan.total_frames,
           "the renderer produced exactly the planned frame count");

    for (i = 0uL; i < frames; i++) {
        int v = (int)pcm[i];
        int a = (v < 0) ? -v : v;

        if (a > peak) {
            peak = a;
        }
        if (a >= 32767) {
            clipped++;
        }
        sum_sq += (double)v * (double)v;
        sum += (double)v;
        if (i > 0uL) {
            int step = (int)pcm[i] - (int)pcm[i - 1uL];

            if (step < 0) {
                step = -step;
            }
            if (step > max_step) {
                max_step = step;
            }
        }
        if (v != 0 && !in_run) {
            in_run = 1;
            runs++;
        } else if (v == 0) {
            in_run = 0;
        }
    }

    while (lead < (long)frames && pcm[lead] == 0) {
        lead++;
    }
    while (trail < (long)frames - lead
           && pcm[(long)frames - 1L - trail] == 0) {
        trail++;
    }

    (void)printf("      %-6s frames=%-6lu dur=%lums  peak=%-5d  rms=%7.1f/1000"
                 "  dc=%+.5f\n",
                 bounce_audio_event_name(event), frames,
                 (frames * 1000uL) / (sample_rate ? sample_rate : 44100u),
                 peak,
                 (frames > 0uL)
                     ? (((sum_sq / (double)frames) / (32768.0 * 32768.0))
                        * 1000.0)
                     : 0.0,
                 (frames > 0uL) ? ((sum / (double)frames) / 32768.0) : 0.0);
    (void)printf("             clipped=%lu  lead_sil=%ld  trail_sil=%ld"
                 "  sound_runs=%u  max_step=%d  notes=%u rests=%u\n",
                 clipped, lead, trail, runs, max_step, plan.note_count,
                 plan.rest_count);

    /* Objective quality facts, stated without interpreting audibility. */
    expect(clipped == 0uL, "no sample is clipped at full scale");
    expect(max_step < 20000, "no inter-sample jump large enough to be a click");
    expect(peak > 0, "the event is not entirely silent");
    expect(runs > 0u, "the event contains at least one sounding run");

    free(pcm);
}

/* -------------------------------------------------------------------------
 * Real-device trial (PHASE 3, 4, 5)
 *
 * One trial per event, so UP, PICKUP and POP each get their OWN Level B
 * evidence rather than one aggregate. Shutdown is DRAIN, never DISCARD:
 * DISCARD would drop the still-queued tone, the device would never see it, and
 * the harness would report a false negative for a working device.
 * ------------------------------------------------------------------------- */

typedef struct TrialResult {
    int level_a;
    int level_b;
    unsigned long long events_played;
    unsigned long long frames_played;
    unsigned long long backend_errors;
    unsigned long long render_errors;
} TrialResult;

/*
 * Silent availability probe: opens the device and closes it again, writing
 * NOTHING. The default (non-smoke) run uses this, so it can report LEVEL A
 * without ever producing a sound.
 */
static int probe_device(const char *device_label)
{
    int ok = bounce_audio_backend_alsa_device_available(device_label);

    (void)printf("    device \"%s\": availability probe (opens, writes"
                 " nothing)\n", device_label);
    (void)printf("      LEVEL A (backend initialized) = %s\n",
                 ok ? "PROVEN (openable)" : "NOT PROVEN (not openable)");
    return ok;
}

static void trial_event(const char *device_label,
                        const char *device_name,
                        BounceAudioEvent event,
                        int smoke_enabled,
                        TrialResult *out)
{
    BounceAudioPlayerConfig config;
    BounceAudioPlayer *player;
    BounceAudioPlayerStats stats;
    unsigned int spins;

    memset(out, 0, sizeof(*out));
    memset(&stats, 0, sizeof(stats));

    if (!smoke_enabled) {
        /*
         * NOT a shortcut for speed. Creating an ALSA-backed player starts the
         * worker, which renders the tone and WRITES it to the device, which
         * makes a sound. An earlier revision of this file gated only the
         * REPORTING on the smoke flag and still played; that was a real
         * violation of the silence contract and is fixed here. The default run
         * never constructs an ALSA-backed player.
         */
        return;
    }

    (void)printf("    event %-6s on device \"%s\"\n",
                 bounce_audio_event_name(event), device_label);

    bounce_audio_player_default_config(&config);
    config.resource_root = g_resource_root;
    config.backend = BOUNCE_AUDIO_PLAYER_BACKEND_ALSA;
    config.device_name = device_name;
    config.shutdown = BOUNCE_AUDIO_PLAYER_SHUTDOWN_DRAIN;
    config.drain_timeout_ms = 3000u;

    player = bounce_audio_player_create(&config);
    if (player == NULL) {
        (void)printf("      ..   create() returned NULL: this device is NOT"
                     " usable in this session\n");
        (void)printf("      LEVEL A (backend initialized) = NOT PROVEN\n");
        return;
    }
    out->level_a = 1;
    (void)printf("      LEVEL A (backend initialized) = PROVEN\n");

    if (!bounce_audio_player_attach_sink(player)) {
        (void)printf("      FAIL could not attach the Layer B sink\n");
        failures++;
        bounce_audio_player_destroy(player);
        return;
    }

    /* ONE real gameplay event, emitted exactly as vertical_slice.c emits it. */
    if (bounce_audio_event_emit(event) < 0) {
        (void)printf("      FAIL emit(%s) was refused\n",
                     bounce_audio_event_name(event));
        failures++;
    }

    /* Bounded wait: the worker must consume it with no busy-wait deadlock. */
    for (spins = 0u; spins < 4000u; spins++) {
        (void)bounce_audio_player_stats(player, &stats);
        if (stats.events_played >= 1ull || stats.backend_errors > 0ull) {
            break;
        }
        nap_ms(2u);
    }
    (void)bounce_audio_player_stats(player, &stats);

    bounce_audio_player_destroy(player);

    (void)printf("      ..   enqueued=%llu played=%llu frames=%llu"
                 " render_err=%llu backend_err=%llu discarded=%llu\n",
                 stats.events_enqueued, stats.events_played,
                 stats.frames_played, stats.render_errors,
                 stats.backend_errors, stats.discarded_on_shutdown);

    out->events_played = stats.events_played;
    out->frames_played = stats.frames_played;
    out->backend_errors = stats.backend_errors;
    out->render_errors = stats.render_errors;

    if (!smoke_enabled) {
        (void)printf("      LEVEL B (PCM submitted to device) = NOT PROVEN"
                     " (smoke not enabled; nothing was written)\n");
        return;
    }

    if (stats.events_played >= 1ull && stats.frames_played > 0ull
        && stats.backend_errors == 0ull) {
        out->level_b = 1;
        (void)printf("      LEVEL B (PCM submitted to device) = PROVEN"
                     " (%llu frames accepted by the device)\n",
                     stats.frames_played);
    } else {
        (void)printf("      LEVEL B (PCM submitted to device) = NOT PROVEN\n");
    }
}

/* -------------------------------------------------------------------------
 * PHASE 6 -- failure isolation: no usable device must not break anything
 * ------------------------------------------------------------------------- */

static void test_failure_isolation(void)
{
    BounceAudioPlayerConfig config;
    BounceAudioPlayer *player;
    BounceAudioPlayerStats stats;
    int i;

    (void)printf("    a device name that cannot open\n");

    bounce_audio_player_default_config(&config);
    config.resource_root = g_resource_root;
    config.backend = BOUNCE_AUDIO_PLAYER_BACKEND_ALSA;
    config.device_name = "plughw:99,99"; /* deliberately absent */

    player = bounce_audio_player_create(&config);
    expect(player == NULL,
           "create() returns NULL for an unopenable device (no crash, no abort)");

    if (player != NULL) {
        bounce_audio_player_destroy(player);
    }

    (void)printf("    gameplay is unaffected with no player installed\n");
    expect(!bounce_audio_event_has_sink(),
           "no sink is installed after a failed create");
    expect(bounce_audio_event_emit(BOUNCE_AUDIO_EVENT_UP) == 0,
           "emitting with no sink is a safe no-op, not a crash");
    expect(bounce_audio_event_emit(BOUNCE_AUDIO_EVENT_POP) == 0,
           "emitting again with no sink is still a safe no-op");

    (void)printf("    the Null backend is reached only by explicit request\n");
    bounce_audio_player_default_config(&config);
    config.resource_root = g_resource_root;
    config.backend = BOUNCE_AUDIO_PLAYER_BACKEND_NULL;
    config.shutdown = BOUNCE_AUDIO_PLAYER_SHUTDOWN_DRAIN;
    config.drain_timeout_ms = 3000u;
    player = bounce_audio_player_create(&config);
    expect(player != NULL, "an explicit Null request succeeds");
    if (player != NULL) {
        unsigned int spins;

        expect(bounce_audio_player_attach_sink(player) == 1,
               "the Layer B sink attaches to the Null-backed player");
        for (i = 0; i < 3; i++) {
            (void)bounce_audio_event_emit(BOUNCE_AUDIO_EVENT_PICKUP);
        }
        (void)bounce_audio_player_stats(player, &stats);
        expect(stats.events_enqueued == 3ull,
               "all three events were queued through the real sink");

        /*
         * Bounded wait, so the whole silent pipeline is genuinely exercised:
         * Layer B emit -> sink -> FIFO -> worker thread -> renderer -> backend.
         * The Null backend discards the samples, so this proves the chain
         * COMPLETES without any device and without any sound.
         */
        for (spins = 0u; spins < 4000u; spins++) {
            (void)bounce_audio_player_stats(player, &stats);
            if (stats.events_played >= 3ull) {
                break;
            }
            nap_ms(2u);
        }
        (void)bounce_audio_player_stats(player, &stats);
        (void)printf("      ..   played=%llu frames=%llu render_err=%llu"
                     " backend_err=%llu\n",
                     stats.events_played, stats.frames_played,
                     stats.render_errors, stats.backend_errors);
        expect(stats.events_played == 3ull,
               "the worker thread consumed all three queued events");
        expect(stats.frames_played == 3ull * 33075ull,
               "the renderer produced the full frame count for each event");
        expect(stats.render_errors == 0ull && stats.backend_errors == 0ull,
               "no render or backend error occurred without a device");

        bounce_audio_player_destroy(player);
    }
    expect(!bounce_audio_event_has_sink(),
           "the sink is removed when the player is destroyed");
}

/* -------------------------------------------------------------------------
 * PHASE 5 -- the device comparison, with the levels kept separate
 * ------------------------------------------------------------------------- */

static void test_device_comparison(const char *known_good, int smoke_enabled)
{
    static const BounceAudioEvent order[3] = {
        BOUNCE_AUDIO_EVENT_UP,
        BOUNCE_AUDIO_EVENT_PICKUP,
        BOUNCE_AUDIO_EVENT_POP
    };
    TrialResult prod[3];
    TrialResult good[3];
    int i;
    int prod_a = 1;
    int good_a = 1;
    int prod_b = 1;
    int good_b = 1;

    memset(&prod, 0, sizeof(prod));
    memset(&good, 0, sizeof(good));

    (void)printf("\n  --- device \"%s\" (what the production config resolves"
                 " to) ---\n", RUNTIME_DEVICE_PRODUCTION_DEFAULT);
    if (!smoke_enabled) {
        prod_a = probe_device(RUNTIME_DEVICE_PRODUCTION_DEFAULT);
        for (i = 0; i < 3; i++) {
            prod[i].level_a = prod_a;
        }
    } else {
        for (i = 0; i < 3; i++) {
            trial_event(RUNTIME_DEVICE_PRODUCTION_DEFAULT,
                        RUNTIME_DEVICE_PRODUCTION_DEFAULT, order[i],
                        smoke_enabled, &prod[i]);
        }
    }

    (void)printf("\n  --- device \"%s\" (the real device target) ---\n",
                 known_good);
    if (!smoke_enabled) {
        good_a = probe_device(known_good);
        for (i = 0; i < 3; i++) {
            good[i].level_a = good_a;
        }
    } else {
        for (i = 0; i < 3; i++) {
            trial_event(known_good, known_good, order[i], smoke_enabled,
                        &good[i]);
        }
    }

    (void)printf("\n  --- LEVEL A / LEVEL B per event, devices compared ---\n");
    (void)printf("      %-7s | %-28s | %-28s\n", "event",
                 RUNTIME_DEVICE_PRODUCTION_DEFAULT, known_good);
    for (i = 0; i < 3; i++) {
        (void)printf("      %-7s | A=%-3s B=%-3s            | A=%-3s B=%-3s\n",
                     bounce_audio_event_name(order[i]),
                     prod[i].level_a ? "PROVEN" : "NOTPR",
                     prod[i].level_b ? "PROVEN" : "NOTPR",
                     good[i].level_a ? "PROVEN" : "NOTPR",
                     good[i].level_b ? "PROVEN" : "NOTPR");
        if (!prod[i].level_a) {
            prod_a = 0;
        }
        if (!good[i].level_a) {
            good_a = 0;
        }
        if (!prod[i].level_b) {
            prod_b = 0;
        }
        if (!good[i].level_b) {
            good_b = 0;
        }
    }

    if (!prod_a) {
        (void)printf("\n      FACT: the production default device \"%s\" is NOT"
                     " openable in this session.\n",
                     RUNTIME_DEVICE_PRODUCTION_DEFAULT);
        (void)printf("      FACT: create() returned NULL, so the GAME installs"
                     " no audio sink and produces no sound here.\n");
    } else {
        (void)printf("\n      OBSERVATION: the production default device is"
                     " openable in this session.\n");
    }
    if (!good_a) {
        (void)printf("      FACT: the requested real device \"%s\" is NOT"
                     " openable in this session.\n", known_good);
        (void)printf("      FACT: that is an ENVIRONMENTAL condition, not a"
                     " defect. A sound daemon holding the device\n");
        (void)printf("      FACT: exclusively is the usual cause, and this"
                     " harness does not modify system audio state.\n");
    }

    (void)printf("\n      LEVEL A (backend initialized): default=%s  %s=%s\n",
                 prod_a ? "PROVEN" : "NOT PROVEN", known_good,
                 good_a ? "PROVEN" : "NOT PROVEN");
    (void)printf("      LEVEL B (PCM to device):       default=%s  %s=%s\n",
                 prod_b ? "PROVEN" : "NOT PROVEN", known_good,
                 good_b ? "PROVEN" : "NOT PROVEN");
    (void)printf("      LEVEL C (human-confirmed audible) = NOT PROVEN"
                 " -- no listening occurred in this harness\n");

    /*
     * Device AVAILABILITY is environmental and is deliberately NOT an assertion:
     * STEP 14B-34-E established that a test may pass when the failure to open a
     * device is a valid property of the host. What IS asserted, for every trial
     * that DID create a player, is a property of the code and not of the host.
     */
    for (i = 0; i < 3; i++) {
        if (good[i].level_a) {
            if (smoke_enabled) {
                expect(good[i].events_played == 1ull
                           && good[i].render_errors == 0ull
                           && good[i].backend_errors == 0ull,
                       "an initialized device plays the event with no render"
                       " or backend error");
            } else {
                (void)printf("      ..   not played: smoke not enabled, so"
                             " no code invariant is claimed here\n");
            }
        }
    }
    if (smoke_enabled && good_a) {
        expect(good_b,
               "all three game sounds were submitted to the real device");
    }
}

/* ------------------------------------------------------------------------- */

int main(int argc, char **argv)
{
    const char *smoke;
    const char *known_good;
    int smoke_enabled;
    unsigned int sample_rate;

    if (argc > 1 && argv[1][0] != '\0') {
        g_resource_root = argv[1];
    }
    known_good = (argc > 2 && argv[2][0] != '\0') ? argv[2]
                                                  : RUNTIME_DEVICE_KNOWN_GOOD;

    smoke = getenv(RUNTIME_SMOKE_VAR);
    smoke_enabled = (smoke != NULL && smoke[0] == '1' && smoke[1] == '\0');

    {
        BounceSynthConfig cfg;

        bounce_synth_default_config(&cfg);
        sample_rate = cfg.sample_rate;
    }

    (void)printf("audio runtime / audibility verification (STEP 14B-34-F)\n");
    (void)printf("  resource root : %s\n", g_resource_root);
    (void)printf("  known-good dev: %s\n", known_good);
    (void)printf("  PCM format    : S16LE mono @ %u Hz (Target B reconstruction)\n",
                 sample_rate);
    if (smoke_enabled) {
        (void)printf("  %s=1  THE OPT-IN SMOKE TEST IS ENABLED AND WILL MAKE"
                     " SOUND\n\n", RUNTIME_SMOKE_VAR);
    } else {
        (void)printf("  no sound is produced (set %s=1 for the opt-in runtime"
                     " test)\n\n", RUNTIME_SMOKE_VAR);
    }
    (void)printf("  NOTE: the waveform and the Natural rest magnitude remain"
                 " UNKNOWN.\n");
    (void)printf("  What is measured below is the Target B RECONSTRUCTION, not"
                 " any Nokia handset.\n\n");

    (void)printf("  1. rendered PCM, measured in memory (never played)\n");
    (void)printf("  --------------------------------------------------\n");
    analyse_event(BOUNCE_AUDIO_EVENT_UP, g_resource_root, sample_rate);
    analyse_event(BOUNCE_AUDIO_EVENT_PICKUP, g_resource_root, sample_rate);
    analyse_event(BOUNCE_AUDIO_EVENT_POP, g_resource_root, sample_rate);

    (void)printf("\n  2. failure isolation\n");
    (void)printf("  ---------------------\n");
    test_failure_isolation();

    (void)printf("\n  3. device comparison, per event\n");
    (void)printf("  --------------------------------\n");
    test_device_comparison(known_good, smoke_enabled);

    (void)printf("\n  4. evidence summary\n");
    (void)printf("  --------------------\n");
    (void)printf("      LEVEL A  backend initialized          see section 3\n");
    (void)printf("      LEVEL B  PCM submitted to real ALSA   see section 3\n");
    (void)printf("      LEVEL C  human-confirmed audible      NOT PROVEN\n");
    (void)printf("      AUDIBILITY = NOT PROVEN -- this harness cannot"
                 " listen.\n");

    if (failures != 0) {
        (void)printf("\naudio runtime verification: FAIL (%d failure(s))\n",
                     failures);
        (void)fprintf(stderr, "audio_runtime_check: FAILED\n");
        return 1;
    }
    (void)printf("\naudio runtime verification: PASS (0 failures)\n");
    if (!smoke_enabled) {
        (void)printf("no sample was written to any device; no sound was"
                     " produced\n");
    }
    return 0;
}
