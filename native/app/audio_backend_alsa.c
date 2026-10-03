/*
 * audio_backend_alsa.c -- STEP 14B-34-C ALSA audio backend implementation.
 *
 * The ONLY file in the audio path that includes <alsa/asoundlib.h>. Nothing
 * portable may include it, and this file includes nothing from the engine, so
 * the dependency runs one way only: ALSA backend -> portable interface.
 *
 * Target B PCM, all ENGINEERING PARAMETERS and not recovered Nokia values:
 *
 *     44100 Hz   S16_LE   mono (1 channel)   interleaved
 *
 * No waveform, frequency, style, or rest decision appears here. This code is
 * handed finished int16 frames and does not know what they mean.
 *
 * ERROR POLICY
 *   Every ALSA return code is translated into a BounceAudioBackendStatus and,
 *   where useful, into a human-readable string via ALSA's own snd_strerror().
 *   A device that cannot be opened is reported as ERR_OPEN or ERR_NOT_AVAILABLE.
 *   It is NEVER silently redirected to the Null backend, never substituted with
 *   another device, and never compensated for with a synthesised tone. The
 *   caller decides what to do about a failure; this file only reports it.
 *
 * THREADING
 *   write_pcm may block, which is expected for a real device. No thread is
 *   created here.
 */

#include "audio_backend_alsa.h"

/* Platform header: confined to this file by design. */
#include <alsa/asoundlib.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* Default period when the caller does not choose one. A Target B choice. */
#define BOUNCE_AUDIO_ALSA_DEFAULT_PERIOD_FRAMES 1024u

static void bounce_audio_alsa_copy(char *dst, size_t capacity,
                                  const char *src)
{
    size_t i;

    if (dst == NULL || capacity == 0u) {
        return;
    }
    if (src == NULL) {
        src = "";
    }
    for (i = 0u; i + 1u < capacity && src[i] != '\0'; i++) {
        dst[i] = src[i];
    }
    dst[i] = '\0';
}

/* Record a message, appending ALSA's own explanation when one exists. */
static void bounce_audio_alsa_error(BounceAudioAlsaState *state,
                                   const char *context, int alsa_code)
{
    if (state == NULL) {
        return;
    }
    if (alsa_code < 0) {
        (void)snprintf(state->error, sizeof(state->error), "%s: %s",
                       context, snd_strerror(alsa_code));
    } else if (alsa_code > 0) {
        (void)snprintf(state->error, sizeof(state->error), "%s: %s",
                       context, snd_strerror(alsa_code));
    } else {
        bounce_audio_alsa_copy(state->error, sizeof(state->error), context);
    }
}

static void bounce_audio_alsa_clear_error(BounceAudioAlsaState *state)
{
    if (state != NULL) {
        state->error[0] = '\0';
    }
}

static const char *bounce_audio_alsa_last_error(const BounceAudioBackend *self)
{
    const BounceAudioAlsaState *state;

    if (self == NULL || self->state == NULL) {
        return "";
    }
    state = (const BounceAudioAlsaState *)self->state;
    return state->error;
}

static const char *bounce_audio_alsa_name(const BounceAudioBackend *self)
{
    (void)self;
    return "alsa";
}

/*
 * Open and configure. Sequence: open, then set parameters, then prepare. Any
 * failure closes whatever was opened so no handle is leaked, and returns a
 * status. "default" is used when no device was named; no hw: name is baked in.
 */
static BounceAudioBackendStatus bounce_audio_alsa_open(
    BounceAudioBackend *self,
    const BounceAudioBackendFormat *format,
    const char *device_name)
{
    BounceAudioAlsaState *state;
    const char *device;
    unsigned int period;
    int err;

    if (self == NULL || self->state == NULL || format == NULL) {
        return BOUNCE_AUDIO_BACKEND_ERR_NULL_ARGUMENT;
    }
    state = (BounceAudioAlsaState *)self->state;
    if (state->open) {
        bounce_audio_alsa_error(state, "backend is already open", 0);
        return BOUNCE_AUDIO_BACKEND_ERR_STATE;
    }
    if (!bounce_audio_backend_format_is_valid(format)) {
        bounce_audio_alsa_error(state, "unsupported PCM format requested", 0);
        return BOUNCE_AUDIO_BACKEND_ERR_UNSUPPORTED_FORMAT;
    }

    device = (device_name != NULL && device_name[0] != '\0')
                 ? device_name
                 : BOUNCE_AUDIO_BACKEND_DEVICE_DEFAULT;
    bounce_audio_alsa_copy(state->device, sizeof(state->device), device);

    state->pcm = NULL;
    state->disconnected = 0;
    state->sample_rate_hz = format->sample_rate_hz;
    state->channels = format->channels;
    period = (format->period_frames > 0u)
                 ? (unsigned int)format->period_frames
                 : BOUNCE_AUDIO_ALSA_DEFAULT_PERIOD_FRAMES;
    state->period_frames = (size_t)period;

    /* Blocking mode: this call may wait, which is why write_pcm must not be
     * called from the gameplay thread. See the threading note above. */
    err = snd_pcm_open(&state->pcm, device, SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) {
        state->pcm = NULL;
        bounce_audio_alsa_error(state, "snd_pcm_open failed", err);
        /* A missing or unusable device is reported, never worked around. */
        return (err == -ENODEV || err == -ENOENT || err == -EACCES)
                   ? BOUNCE_AUDIO_BACKEND_ERR_NOT_AVAILABLE
                   : BOUNCE_AUDIO_BACKEND_ERR_OPEN;
    }

    /*
     * S16_LE, RW_INTERLEAVED, the requested rate and channel count, no soft
     * resampling (the renderer already emits the exact rate), latency sized to
     * a few periods.
     */
    err = snd_pcm_set_params(state->pcm, SND_PCM_FORMAT_S16_LE,
                             SND_PCM_ACCESS_RW_INTERLEAVED,
                             (unsigned int)state->channels,
                             (unsigned int)state->sample_rate_hz, 0,
                             (unsigned int)(period * 4u));
    if (err < 0) {
        bounce_audio_alsa_error(state, "snd_pcm_set_params failed", err);
        (void)snd_pcm_close(state->pcm);
        state->pcm = NULL;
        return BOUNCE_AUDIO_BACKEND_ERR_CONFIGURE;
    }

    err = snd_pcm_prepare(state->pcm);
    if (err < 0) {
        bounce_audio_alsa_error(state, "snd_pcm_prepare failed", err);
        (void)snd_pcm_close(state->pcm);
        state->pcm = NULL;
        return BOUNCE_AUDIO_BACKEND_ERR_CONFIGURE;
    }

    state->open = 1;
    bounce_audio_alsa_clear_error(state);
    return BOUNCE_AUDIO_BACKEND_OK;
}

/* True once ALSA reports the handle as disconnected. */
static int bounce_audio_alsa_is_disconnected(BounceAudioAlsaState *state)
{
    return (snd_pcm_state(state->pcm) == SND_PCM_STATE_DISCONNECTED) ? 1 : 0;
}

/*
 * Write frames. May block. A short write is reported as a failure rather than
 * silently accepted, because a caller rendering a fixed-length buffer has no
 * way to resume a partial write.
 */
static BounceAudioBackendStatus bounce_audio_alsa_write(
    BounceAudioBackend *self,
    const int16_t *samples,
    size_t frames)
{
    BounceAudioAlsaState *state;
    BounceAudioBackendStatus precheck;
    snd_pcm_sframes_t written;
    int err;

    precheck = bounce_audio_backend_precheck_write(self, samples, frames);
    if (precheck != BOUNCE_AUDIO_BACKEND_OK) {
        return precheck;
    }
    state = (BounceAudioAlsaState *)self->state;
    if (!state->open || state->pcm == NULL) {
        bounce_audio_alsa_error(state, "write before open", 0);
        return BOUNCE_AUDIO_BACKEND_ERR_STATE;
    }
    if (frames == 0u) {
        return BOUNCE_AUDIO_BACKEND_OK; /* Documented no-op. */
    }

    if (bounce_audio_alsa_is_disconnected(state)) {
        state->disconnected = 1;
        bounce_audio_alsa_error(state, "output device is gone", 0);
        return BOUNCE_AUDIO_BACKEND_ERR_DEVICE_LOST;
    }

    written = snd_pcm_writei(state->pcm, samples,
                             (snd_pcm_uframes_t)frames);
    if (written < 0) {
        err = (int)written;
        if (err == -EPIPE) {
            /*
             * An underrun. One documented recovery attempt, then report it. This
             * is not a silent retry loop and not a fallback to another path.
             */
            err = snd_pcm_recover(state->pcm, err, 1);
            if (err >= 0) {
                bounce_audio_alsa_clear_error(state);
                return BOUNCE_AUDIO_BACKEND_ERR_WRITE;
            }
        }
        if (bounce_audio_alsa_is_disconnected(state)) {
            state->disconnected = 1;
            bounce_audio_alsa_error(state, "output device was lost", err);
            return BOUNCE_AUDIO_BACKEND_ERR_DEVICE_LOST;
        }
        bounce_audio_alsa_error(state, "snd_pcm_writei failed", err);
        return BOUNCE_AUDIO_BACKEND_ERR_WRITE;
    }
    if ((size_t)written != frames) {
        (void)snprintf(state->error, sizeof(state->error),
                       "short write: %ld of %lu frames",
                       (long)written, (unsigned long)frames);
        return BOUNCE_AUDIO_BACKEND_ERR_WRITE;
    }

    bounce_audio_alsa_clear_error(state);
    return BOUNCE_AUDIO_BACKEND_OK;
}

static BounceAudioBackendStatus bounce_audio_alsa_drain(
    BounceAudioBackend *self)
{
    BounceAudioAlsaState *state;
    int err;

    if (self == NULL || self->state == NULL) {
        return BOUNCE_AUDIO_BACKEND_ERR_NULL_ARGUMENT;
    }
    state = (BounceAudioAlsaState *)self->state;
    if (!state->open || state->pcm == NULL) {
        bounce_audio_alsa_error(state, "drain before open", 0);
        return BOUNCE_AUDIO_BACKEND_ERR_STATE;
    }
    err = snd_pcm_drain(state->pcm);
    if (err < 0) {
        bounce_audio_alsa_error(state, "snd_pcm_drain failed", err);
        return BOUNCE_AUDIO_BACKEND_ERR_WRITE;
    }
    bounce_audio_alsa_clear_error(state);
    return BOUNCE_AUDIO_BACKEND_OK;
}

static void bounce_audio_alsa_close(BounceAudioBackend *self)
{
    BounceAudioAlsaState *state;

    if (self == NULL || self->state == NULL) {
        return;
    }
    state = (BounceAudioAlsaState *)self->state;
    if (state->pcm != NULL) {
        (void)snd_pcm_close(state->pcm);
        state->pcm = NULL;
    }
    state->open = 0;
}

void bounce_audio_backend_alsa_reset(BounceAudioAlsaState *state)
{
    if (state == NULL) {
        return;
    }
    state->pcm = NULL;
    state->sample_rate_hz = 0u;
    state->channels = 0u;
    state->period_frames = 0u;
    state->open = 0;
    state->disconnected = 0;
    state->device[0] = '\0';
    state->error[0] = '\0';
}

int bounce_audio_backend_alsa_device_available(const char *device_name)
{
    snd_pcm_t *pcm = NULL;
    const char *device;
    int err;

    device = (device_name != NULL && device_name[0] != '\0')
                 ? device_name
                 : BOUNCE_AUDIO_BACKEND_DEVICE_DEFAULT;
    err = snd_pcm_open(&pcm, device, SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) {
        return 0;
    }
    (void)snd_pcm_close(pcm);
    return 1;
}

BounceAudioBackendStatus bounce_audio_backend_alsa_init(
    BounceAudioBackend *backend,
    BounceAudioAlsaState *state,
    const char *device_name)
{
    if (backend == NULL || state == NULL) {
        return BOUNCE_AUDIO_BACKEND_ERR_NULL_ARGUMENT;
    }

    bounce_audio_backend_alsa_reset(state);
    bounce_audio_alsa_copy(state->device, sizeof(state->device),
                           (device_name != NULL) ? device_name
                                                 : BOUNCE_AUDIO_BACKEND_DEVICE_DEFAULT);

    backend->open = bounce_audio_alsa_open;
    backend->write_pcm = bounce_audio_alsa_write;
    backend->drain = bounce_audio_alsa_drain;
    backend->close = bounce_audio_alsa_close;
    backend->name = bounce_audio_alsa_name;
    backend->last_error = bounce_audio_alsa_last_error;
    backend->state = state;
    return BOUNCE_AUDIO_BACKEND_OK;
}
