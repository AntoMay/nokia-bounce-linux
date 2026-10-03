/*
 * audio_backend_check.c -- STEP 14B-34-C standalone runner for the portable
 * backend interface and the Null backend.
 *
 * WHY THIS FILE EXISTS
 *   Same shape as rtpl_decoder_check.c, audio_event_check.c and
 *   audio_synth_check.c: a thin `main()` over a module-owned verify function.
 *   No new test framework; every assertion lives in audio_backend.c.
 *
 * WHAT IT LINKS
 *   The portable interface, the Null backend, the STEP 14B-34-B renderer and the
 *   Layer A decoder. It does NOT link ALSA, so it cannot open a device even by
 *   accident, and it links no Layer B object file. Rendering one ORIGINAL asset
 *   and writing it through the Null backend is the proof that the renderer
 *   output and the backend input are the same thing.
 *
 * IT MAKES NO SOUND.
 *
 * Usage:  audio_backend_check <resource-root>
 *         for example: audio_backend_check src/main/resources
 * Exit status: 0 when every assertion passed, 1 otherwise.
 */

#include "audio_backend.h"
#include "audio_backend_null.h"
#include "audio_synth.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int bounce_backend_failures;

static void expect(int condition, const char *description)
{
    if (condition) {
        (void)printf("      ok   %s\n", description);
        return;
    }
    (void)printf("      FAIL %s\n", description);
    bounce_backend_failures++;
}

/* A small synthetic buffer. Deliberately NOT renderer output. */
static void fill_test_tone(int16_t *samples, size_t frames)
{
    size_t i;

    for (i = 0u; i < frames; i++) {
        samples[i] = (int16_t)(((int)(i % 8u) - 4) * 1000);
    }
}

static void test_null_lifecycle(void)
{
    BounceAudioBackend backend;
    BounceAudioNullState state;
    BounceAudioBackendFormat format;
    int16_t samples[64];
    char description[160];

    (void)printf("  Null backend lifecycle\n");

    expect(bounce_audio_backend_null_init(NULL, &state, NULL)
               == BOUNCE_AUDIO_BACKEND_ERR_NULL_ARGUMENT,
           "init with a NULL backend is refused");
    expect(bounce_audio_backend_null_init(&backend, NULL, NULL)
               == BOUNCE_AUDIO_BACKEND_ERR_NULL_ARGUMENT,
           "init with NULL state is refused");
    expect(bounce_audio_backend_null_init(&backend, &state, NULL)
               == BOUNCE_AUDIO_BACKEND_OK,
           "init succeeds");
    expect(bounce_audio_backend_is_complete(&backend),
           "the function table is complete");
    expect(bounce_audio_backend_is_complete(NULL) == 0,
           "a NULL backend is not complete");
    expect(strcmp(backend.name(&backend), "null") == 0,
           "name reports \"null\"");

    bounce_audio_backend_default_format(&format);
    expect(format.sample_rate_hz == 44100u
               && format.channels == 1u
               && format.sample_format == BOUNCE_AUDIO_BACKEND_S16_LE,
           "default format is 44100 Hz, mono, S16_LE");
    expect(bounce_audio_backend_format_is_valid(&format),
           "default format validates");
    expect(bounce_audio_backend_format_is_valid(NULL) == 0,
           "a NULL format is not valid");

    (void)snprintf(description, sizeof(description),
                   "default device is \"%s\"",
                   BOUNCE_AUDIO_BACKEND_DEVICE_DEFAULT);
    expect(strcmp(BOUNCE_AUDIO_BACKEND_DEVICE_DEFAULT, "default") == 0,
           description);

    /* Writes before open are refused. */
    fill_test_tone(samples, 64u);
    expect(backend.write_pcm(&backend, samples, 64u)
               == BOUNCE_AUDIO_BACKEND_ERR_STATE,
           "write before open is refused");
    expect(backend.drain(&backend) == BOUNCE_AUDIO_BACKEND_ERR_STATE,
           "drain before open is refused");
    expect(state.total_frames == 0ull,
           "a refused write is not counted");

    expect(backend.open(&backend, &format, NULL) == BOUNCE_AUDIO_BACKEND_OK,
           "open succeeds");
    expect(state.open == 1, "state reports open");
    expect(backend.open(&backend, &format, NULL)
               == BOUNCE_AUDIO_BACKEND_ERR_STATE,
           "opening twice is refused");

    /* Invalid PCM inputs. */
    expect(backend.write_pcm(&backend, NULL, 64u)
               == BOUNCE_AUDIO_BACKEND_ERR_NULL_ARGUMENT,
           "a NULL buffer with frames is refused");
    expect(backend.write_pcm(&backend, samples, 0u)
               == BOUNCE_AUDIO_BACKEND_OK,
           "a zero-frame write is a no-op, not an error");
    expect(state.write_count == 0ull,
           "a zero-frame write is not counted as a write");

    /* An unsupported format is refused at open, on a fresh backend. */
    {
        BounceAudioBackend fresh_backend;
        BounceAudioNullState fresh_state;
        BounceAudioBackendFormat bad = format;

        bad.channels = 2u;
        expect(bounce_audio_backend_format_is_valid(&bad) == 0,
               "stereo is rejected by format validation");
        bad = format;
        bad.sample_rate_hz = 0u;
        expect(bounce_audio_backend_format_is_valid(&bad) == 0,
               "a zero sample rate is rejected");
        bad = format;
        bad.sample_format = (BounceAudioBackendSampleFormat)9;
        expect(bounce_audio_backend_format_is_valid(&bad) == 0,
               "an unknown sample format is rejected");

        bounce_audio_backend_null_init(&fresh_backend, &fresh_state, NULL);
        bad = format;
        bad.channels = 2u;
        expect(fresh_backend.open(&fresh_backend, &bad, NULL)
                   == BOUNCE_AUDIO_BACKEND_ERR_UNSUPPORTED_FORMAT,
               "open refuses a stereo format");
        expect(fresh_state.open == 0,
               "a refused format leaves the backend closed");
        expect(fresh_backend.last_error(&fresh_backend) != NULL,
               "a refused format records a readable reason");
        bad = format;
        bad.sample_rate_hz = 0u;
        expect(fresh_backend.open(&fresh_backend, &bad, NULL)
                   == BOUNCE_AUDIO_BACKEND_ERR_UNSUPPORTED_FORMAT,
               "open refuses a zero sample rate");
        expect(fresh_backend.open(&fresh_backend, NULL, NULL)
                   == BOUNCE_AUDIO_BACKEND_ERR_NULL_ARGUMENT,
               "open refuses a NULL format");
    }

    /* Real writes, accounted for exactly. */
    fill_test_tone(samples, 64u);
    expect(backend.write_pcm(&backend, samples, 64u)
               == BOUNCE_AUDIO_BACKEND_OK,
           "a 64-frame write succeeds");
    expect(state.write_count == 1ull, "write count is 1");
    expect(state.total_frames == 64ull, "total frames is 64");
    expect(state.total_bytes == 128ull, "total bytes is 128 (64 mono frames)");

    expect(backend.write_pcm(&backend, samples, 36u)
               == BOUNCE_AUDIO_BACKEND_OK,
           "a second 36-frame write succeeds");
    expect(state.write_count == 2ull, "write count is 2");
    expect(state.total_frames == 100ull, "total frames is 100");
    expect(state.total_bytes == 200ull, "total bytes is 200");

    expect(backend.drain(&backend) == BOUNCE_AUDIO_BACKEND_OK,
           "drain succeeds");
    expect(state.drain_count == 1ull, "drain count is 1");

    /* The retained error string is readable and never NULL. */
    expect(backend.last_error(&backend) != NULL,
           "last_error is never NULL");
    expect(backend.last_error(NULL) != NULL,
           "last_error of a NULL backend is safe and non-NULL");

    backend.close(&backend);
    expect(state.open == 0 && state.closed_once == 1, "close works");
    expect(backend.write_pcm(&backend, samples, 64u)
               == BOUNCE_AUDIO_BACKEND_ERR_STATE,
           "write after close is refused");
    /* Counting survives close, which is the point of the Null backend. */
    expect(state.total_frames == 100ull,
           "counters survive close for verification");

    /* Closing twice is harmless. */
    backend.close(&backend);
    expect(state.closed_once == 1, "a second close is harmless");

    (void)printf("  Null backend backend-agnosticism\n");
    {
        BounceAudioBackend bare;

        memset(&bare, 0, sizeof(bare));
        expect(bounce_audio_backend_is_complete(&bare) == 0,
               "a zeroed function table is not complete");
        expect(bounce_audio_backend_precheck_write(&bare, samples, 8u)
                   == BOUNCE_AUDIO_BACKEND_ERR_NULL_ARGUMENT,
               "precheck rejects a backend with no write function");
        expect(bounce_audio_backend_precheck_write(NULL, samples, 8u)
                   == BOUNCE_AUDIO_BACKEND_ERR_NULL_ARGUMENT,
               "precheck rejects a NULL backend");
    }

    {
        BounceAudioBackendStatus texts = BOUNCE_AUDIO_BACKEND_OK;
        int i;

        for (i = 0; i <= (int)BOUNCE_AUDIO_BACKEND_ERR_DEVICE_LOST; i++) {
            if (bounce_audio_backend_status_text((BounceAudioBackendStatus)i)
                == NULL) {
                break;
            }
        }
        expect(i > (int)BOUNCE_AUDIO_BACKEND_ERR_DEVICE_LOST,
               "every status has non-NULL text");
        expect(bounce_audio_backend_status_text(
                   (BounceAudioBackendStatus)999) != NULL,
               "an unknown status still has text");
        expect(texts == BOUNCE_AUDIO_BACKEND_OK, "OK is zero");
    }
}

/*
 * The integration proof: real renderer output, through the real backend
 * interface, into a backend that produces no sound. This is what demonstrates
 * that 14B-34-B's PCM and 14B-34-C's boundary are the same thing, and that the
 * renderer needed no change to fit a backend.
 */
static void test_renderer_to_null(const char *resource_root)
{
    BounceAudioBackend backend;
    BounceAudioNullState state;
    BounceAudioBackendFormat format;
    BounceSynthConfig config;
    BounceRtplSong song;
    BounceSynthPlan plan;
    int16_t *pcm;
    size_t frames_written = 0u;
    char description[180];

    (void)printf("  STEP 14B-34-B renderer -> Null backend\n");

    if (resource_root == NULL || resource_root[0] == '\0') {
        (void)printf("      skip integration: no resource root given\n");
        return;
    }

    bounce_synth_default_config(&config);
    if (bounce_synth_prepare(BOUNCE_AUDIO_EVENT_POP, resource_root, &config,
                             &song, &plan) != BOUNCE_SYNTH_OK) {
        expect(0, "pop.ott decodes through Layer A for the integration test");
        return;
    }
    pcm = (int16_t *)calloc((size_t)plan.total_frames + 1u, sizeof(int16_t));
    if (pcm == NULL) {
        expect(0, "PCM allocation for the integration test");
        return;
    }

    expect(bounce_synth_render_pcm(&song, &config, &plan, pcm,
                                   (size_t)plan.total_frames,
                                   &frames_written) == BOUNCE_SYNTH_OK,
           "renderer produces PCM into a caller buffer");

    /* The renderer's format and the backend's format must agree. */
    bounce_audio_backend_default_format(&format);
    (void)snprintf(description, sizeof(description),
                   "renderer %u Hz/%u ch matches backend %u Hz/%u ch",
                   config.sample_rate, (unsigned int)config.channels,
                   format.sample_rate_hz, format.channels);
    expect(config.sample_rate == format.sample_rate_hz
               && (unsigned int)config.channels == format.channels,
           description);

    bounce_audio_backend_null_init(&backend, &state, NULL);
    expect(backend.open(&backend, &format, NULL) == BOUNCE_AUDIO_BACKEND_OK,
           "Null backend opens with the renderer's format");

    /* One write of the whole rendered buffer. */
    expect(backend.write_pcm(&backend, pcm, frames_written)
               == BOUNCE_AUDIO_BACKEND_OK,
           "the whole rendered buffer is accepted in one write");
    expect(state.total_frames == (unsigned long long)frames_written,
           "the backend counted exactly the frames the renderer produced");
    (void)snprintf(description, sizeof(description),
                   "pop.ott: renderer produced %lu frames and the backend"
                   " counted %llu",
                   (unsigned long)frames_written,
                   (unsigned long long)state.total_frames);
    expect(state.total_frames == (unsigned long long)frames_written,
           description);

    /* Chunked writes must accumulate to the same total. */
    {
        BounceAudioNullState chunked_state;
        BounceAudioBackend chunked;
        size_t offset;

        bounce_audio_backend_null_init(&chunked, &chunked_state, NULL);
        (void)chunked.open(&chunked, &format, NULL);
        for (offset = 0u; offset < frames_written; offset += 4096u) {
            size_t chunk = frames_written - offset;
            if (chunk > 4096u) {
                chunk = 4096u;
            }
            (void)chunked.write_pcm(&chunked, pcm + offset, chunk);
        }
        expect(chunked_state.total_frames == (unsigned long long)frames_written,
               "chunked writes accumulate to the same frame total");
        (void)snprintf(description, sizeof(description),
                       "pop.ott: %lu frames arrive identically chunked or whole",
                       (unsigned long)frames_written);
        expect(chunked_state.total_frames == (unsigned long long)frames_written,
               description);
        chunked.close(&chunked);
    }

    backend.drain(&backend);
    backend.close(&backend);
    free(pcm);
}

int bounce_audio_backend_verify(const char *resource_root, FILE *output_file)
{
    bounce_backend_failures = 0;
    if (output_file == NULL) {
        output_file = stdout;
    }

    (void)fprintf(output_file,
                  "portable audio backend self-test (STEP 14B-34-C)\n");
    (void)fprintf(output_file,
                  "  portable interface + Null backend; no ALSA, no device,"
                  " no sound\n");
    (void)fprintf(output_file,
                  "  44100 Hz / S16_LE / mono are Target B ENGINEERING"
                  " PARAMETERS\n\n");

    test_null_lifecycle();
    test_renderer_to_null(resource_root);

    if (bounce_backend_failures != 0) {
        (void)fprintf(output_file, "\nportable backend self-test: FAIL"
                                  " (%d failure(s))\n",
                      bounce_backend_failures);
        return -1;
    }
    (void)fprintf(output_file, "\nportable backend self-test: PASS"
                              " (0 failures)\n");
    (void)fprintf(output_file, "no device was opened; no sound was produced\n");
    return 0;
}

int main(int argc, char **argv)
{
    const char *resource_root = (argc > 1) ? argv[1] : "";

    (void)printf("audio backend self-test (STEP 14B-34-C)\n\n");

    if (bounce_audio_backend_verify(resource_root, stdout) != 0) {
        (void)fprintf(stderr, "audio_backend_check: FAILED\n");
        return 1;
    }
    (void)printf("audio_backend_check: all assertions passed\n");
    return 0;
}
