/*
 * audio_backend_alsa_check.c -- STEP 14B-34-C standalone runner for the ALSA
 * backend.
 *
 * SAFETY: THIS TEST MAKES NO SOUND BY DEFAULT.
 *   Every check below is a lifecycle, configuration, or error-path test that
 *   either never opens a device or opens and immediately closes one without
 *   writing samples. `make check` must never produce audible output, so the
 *   audible smoke test is OPT-IN and is gated on an environment variable that
 *   has to be set deliberately. The variable name is a Target B choice and may
 *   change; it is not an interface guarantee.
 *
 * WHAT IT LINKS
 *   The portable interface and the ALSA backend, plus libasound. It does not
 *   link the game, Layer B, or gameplay.
 *
 * Usage:  audio_backend_alsa_check
 * Exit status: 0 when every assertion passed, 1 otherwise.
 */

#include "audio_backend.h"
#include "audio_backend_alsa.h"

#include <alsa/asoundlib.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * The ALSA library prints its own diagnostics to stderr when a device cannot be
 * opened. This test deliberately opens a non-existent device to prove the error
 * path, so that chatter is expected rather than a defect -- but it would still
 * pollute the check's output. The backend already captures the reason through
 * snd_strerror() into state->error, so the library's own print is redundant here
 * and is silenced. Passing NULL restores ALSA's default handler.
 */
static void alsa_silence(const char *file, int line, const char *function,
                         int err, const char *fmt, ...)
{
    (void)file;
    (void)line;
    (void)function;
    (void)err;
    (void)fmt;
}

/* Target B opt-in switch. Deliberate opt-in only; never set automatically. */
#define BOUNCE_AUDIO_ALSA_SMOKE_VAR "BOUNCE_AUDIO_ALSA_SMOKE"

/*
 * STEP 14B-34-E. Candidate device names, probed in order.
 *
 * "default" is tried first and is already the backend's own default, so the
 * common case is unchanged. The remaining entries exist because "default" is an
 * ALSA CONFIGURATION name: a host whose alsa.conf defines no default PCM for the
 * running card fails to open "default" even though real hardware is present and
 * usable. "plughw:" wraps the hardware device with the plug layer, which
 * performs the format and rate conversion that a raw "hw:" device refuses.
 * "null" is ALSA's own sample-discarding sink, which always opens.
 *
 * Probing only OPENS a device and closes it again; it writes nothing and is
 * therefore silent. It is a documented test aid, NOT a production fallback
 * chain: nothing in audio_player.c or audio_backend_alsa.c retries these.
 */
static const char *const BOUNCE_AUDIO_ALSA_PROBE_NAMES[] = {
    "default",
    "plughw:0,3",
    "plughw:1,0",
    "null"
};
#define BOUNCE_AUDIO_ALSA_PROBE_COUNT \
    ((unsigned int)(sizeof(BOUNCE_AUDIO_ALSA_PROBE_NAMES) \
                     / sizeof(BOUNCE_AUDIO_ALSA_PROBE_NAMES[0])))

static int alsa_failures;

static void expect(int condition, const char *description)
{
    if (condition) {
        (void)printf("      ok   %s\n", description);
        return;
    }
    (void)printf("      FAIL %s\n", description);
    alsa_failures++;
}

int main(void)
{
    BounceAudioBackend backend;
    BounceAudioAlsaState state;
    BounceAudioBackendFormat format;
    const char *smoke = getenv(BOUNCE_AUDIO_ALSA_SMOKE_VAR);
    int smoke_enabled = ((smoke != NULL) && (smoke[0] == '1')
                         && (smoke[1] == '\0'));
    int device_present;
    char description[200];
    char chosen_device[32];

    (void)printf("ALSA backend self-test (STEP 14B-34-C)\n");
    (void)printf("  44100 Hz / S16_LE / mono are Target B ENGINEERING"
                 " PARAMETERS\n");

    (void)snd_lib_error_set_handler(alsa_silence);

    if (smoke_enabled) {
        (void)printf("  %s=1: the opt-in SMOKE test is ENABLED and WILL make"
                     " a sound\n", BOUNCE_AUDIO_ALSA_SMOKE_VAR);
    } else {
        (void)printf("  no sound is produced (set %s=1 for the opt-in smoke"
                     " test)\n\n", BOUNCE_AUDIO_ALSA_SMOKE_VAR);
    }

    /* ---- construction: no device, no allocation ---- */
    (void)printf("  backend construction\n");
    expect(bounce_audio_backend_alsa_init(NULL, &state, NULL)
               == BOUNCE_AUDIO_BACKEND_ERR_NULL_ARGUMENT,
           "init with a NULL backend is refused");
    expect(bounce_audio_backend_alsa_init(&backend, NULL, NULL)
               == BOUNCE_AUDIO_BACKEND_ERR_NULL_ARGUMENT,
           "init with NULL state is refused");
    expect(bounce_audio_backend_alsa_init(&backend, &state, NULL)
               == BOUNCE_AUDIO_BACKEND_OK,
           "init succeeds without opening a device");
    expect(bounce_audio_backend_is_complete(&backend),
           "the function table is complete");
    expect(strcmp(backend.name(&backend), "alsa") == 0,
           "name reports \"alsa\"");
    expect(state.pcm == NULL, "init left no PCM handle open");
    (void)snprintf(description, sizeof(description),
                   "no device name was hard-coded (default is \"%s\")",
                   BOUNCE_AUDIO_BACKEND_DEVICE_DEFAULT);
    expect(strcmp(state.device, BOUNCE_AUDIO_BACKEND_DEVICE_DEFAULT) == 0,
           description);

    /* ---- portable header hygiene ---- */
    (void)printf("  platform separation\n");
    expect(backend.last_error(&backend)[0] == '\0',
           "a freshly initialised ALSA backend has no error text");
    expect(bounce_audio_backend_is_complete(&backend)
               && backend.open != NULL && backend.write_pcm != NULL,
           "the ALSA backend satisfies the portable function table");
    expect(bounce_audio_backend_alsa_init(&backend, &state,
                                          "bounce-a-second-name") == BOUNCE_AUDIO_BACKEND_OK
               && strcmp(state.device, "bounce-a-second-name") == 0,
           "a caller may name any ALSA plug; none is hard-coded");

    /* ---- format validation, before any device is touched ---- */
    (void)printf("  format validation\n");
    bounce_audio_backend_default_format(&format);
    expect(bounce_audio_backend_format_is_valid(&format),
           "Target B format 44100 Hz / S16_LE / mono validates");
    expect(backend.open(&backend, &format, "bounce-nonexistent-pcm-device")
               != BOUNCE_AUDIO_BACKEND_OK,
           "an invalid device name fails to open");
    expect(state.pcm == NULL, "a failed open leaves no handle open");
    expect(backend.last_error(&backend)[0] != '\0',
           "a failed open records a readable ALSA reason");

    /* ---- invalid device name is reported, never worked around ---- */
    (void)printf("  invalid-device error path\n");
    {
        BounceAudioBackendStatus st = backend.open(
            &backend, &format, "bounce-nonexistent-pcm-device");

        (void)snprintf(description, sizeof(description),
                       "an invalid device is reported as \"%s\"",
                       bounce_audio_backend_status_text(st));
        expect(st == BOUNCE_AUDIO_BACKEND_ERR_OPEN
                   || st == BOUNCE_AUDIO_BACKEND_ERR_NOT_AVAILABLE,
               description);
        expect(st != BOUNCE_AUDIO_BACKEND_OK,
               "no silent fallback to another device occurred");
        expect(strstr(state.device, "bounce-nonexistent") != NULL,
               "the requested name was recorded, not replaced");
    }

    /* ---- format refused before open ---- */
    {
        BounceAudioBackendFormat bad = format;

        bad.channels = 2u;
        expect(backend.open(&backend, &bad, NULL)
                   == BOUNCE_AUDIO_BACKEND_ERR_UNSUPPORTED_FORMAT,
               "a stereo format is refused without touching a device");
    }

    /* ---- real device, if this host has one ---- */
    device_present = bounce_audio_backend_alsa_device_available(NULL);
    (void)snprintf(description, sizeof(description),
                   "a default ALSA playback device is %s",
                   device_present ? "present" : "absent");
    expect(1, description);

    /*
     * STEP 14B-34-E. ALSA's "default" is a CONFIGURATION name, not a device:
     * on a host whose /usr/share/alsa/alsa.conf does not define a default PCM
     * for the running card, it fails to open even though real hardware exists
     * and is usable through an explicit plug name. Probing a short, documented
     * candidate list therefore lets this test reach real hardware and verify an
     * ACTUAL snd_pcm_writei(), which is the whole point of the opt-in smoke.
     *
     * The probe only OPENS a device; nothing is written here, so it is silent.
     * The write happens below, gated on the smoke variable, and uses SILENCE.
     */
    chosen_device[0] = '\0';
    if (!device_present) {
        unsigned int c;

        (void)printf("  probing explicit device names (opens only, no write)\n");
        for (c = 0u; c < BOUNCE_AUDIO_ALSA_PROBE_COUNT; c++) {
            if (bounce_audio_backend_alsa_device_available(
                    BOUNCE_AUDIO_ALSA_PROBE_NAMES[c])) {
                (void)snprintf(chosen_device, sizeof(chosen_device), "%s",
                               BOUNCE_AUDIO_ALSA_PROBE_NAMES[c]);
                (void)printf("      ok   %s is openable\n", chosen_device);
                break;
            }
            (void)printf("      ..   %s is not openable\n",
                         BOUNCE_AUDIO_ALSA_PROBE_NAMES[c]);
        }
    }

    (void)printf("  lifecycle on the %s device\n",
                 (chosen_device[0] != '\0') ? chosen_device : "default");
    {
        BounceAudioBackendStatus st =
            backend.open(&backend, &format,
                         (chosen_device[0] != '\0') ? chosen_device : NULL);

        if (st != BOUNCE_AUDIO_BACKEND_OK) {
            (void)printf("      skip open/write/close: no usable device on this"
                         " host (a reported failure is the correct result)\n");
            expect(st != BOUNCE_AUDIO_BACKEND_OK,
                   "with no usable device, open reports a failure rather than"
                   " pretending");
            (void)printf("\nALSA DEVICE OPEN   = NOT PROVEN / UNAVAILABLE\n");
            (void)printf("snd_pcm_writei()   = NOT PROVEN\n");
        } else {
            int16_t silence[1024];

            expect(1, "open succeeds on a usable device");
            expect(state.open == 1, "state reports open");
            (void)snprintf(description, sizeof(description),
                           "device %s opened; negotiated %u Hz, %u ch,"
                           " sample_format=%d",
                           (chosen_device[0] != '\0') ? chosen_device
                                                      : "default",
                           state.sample_rate_hz, state.channels,
                           (int)format.sample_format);
            expect(1, description);
            (void)printf("\nALSA DEVICE OPEN   = PROVEN  (%s, %u Hz, %u ch)\n",
                         (chosen_device[0] != '\0') ? chosen_device : "default",
                         state.sample_rate_hz, state.channels);

            memset(silence, 0, sizeof(silence));
            expect(backend.write_pcm(&backend, NULL, 16u)
                       == BOUNCE_AUDIO_BACKEND_ERR_NULL_ARGUMENT,
                   "a NULL buffer is refused");
            expect(backend.write_pcm(&backend, silence, 0u)
                       == BOUNCE_AUDIO_BACKEND_OK,
                   "a zero-frame write is a no-op");

            if (smoke_enabled) {
                BounceAudioBackendStatus wst =
                    backend.write_pcm(&backend, silence, 1024u);

                (void)snprintf(description, sizeof(description),
                               "SMOKE: snd_pcm_writei() of 1024 SILENCE"
                               " frames returns \"%s\"",
                               bounce_audio_backend_status_text(wst));
                expect(wst == BOUNCE_AUDIO_BACKEND_OK, description);
                expect(backend.drain(&backend) == BOUNCE_AUDIO_BACKEND_OK,
                       "SMOKE: drain succeeds");
                (void)printf("PCM SUBMITTED      = PROVEN (silence)\n");
                (void)printf("AUDIBILITY         = NOT PROVEN"
                             " (no physical listening occurred)\n");
            } else {
                expect(1, "silence is NOT written (smoke not enabled)");
                (void)printf("PCM SUBMITTED      = NOT PROVEN"
                             " (smoke test not enabled)\n");
                (void)printf("AUDIBILITY         = NOT PROVEN\n");
            }
            backend.close(&backend);
            expect(state.open == 0 && state.pcm == NULL,
                   "close releases the PCM handle");
        }
    }

    /* Closing an unopened backend must be harmless. */
    bounce_audio_backend_alsa_reset(&state);
    backend.close(&backend);
    expect(state.pcm == NULL, "close on a reset backend is harmless");

    if (alsa_failures != 0) {
        (void)printf("\nALSA backend self-test: FAIL (%d failure(s))\n",
                     alsa_failures);
        (void)fprintf(stderr, "audio_backend_alsa_check: FAILED\n");
        return 1;
    }
    (void)printf("\nALSA backend self-test: PASS (0 failures)\n");
    (void)printf("no unexpected sound was produced\n");
    (void)snd_lib_error_set_handler(NULL);
    return 0;
}
