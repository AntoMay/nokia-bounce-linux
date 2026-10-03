/*
 * audio_backend_null.c -- STEP 14B-34-C Null audio backend implementation.
 *
 * No ALSA include. No platform header. No engine header. No allocation. No
 * thread. No queue. Produces no sound.
 *
 * Every write is accounted for and then discarded, which is the whole point: a
 * caller can verify that the right number of frames reached the output boundary
 * without a device existing.
 */

#include "audio_backend_null.h"

#include <stdio.h>
#include <string.h>

/* Copy a bounded string, always NUL terminating. */
static void bounce_audio_null_copy(char *dst, size_t capacity,
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

static void bounce_audio_null_set_error(BounceAudioNullState *state,
                                        const char *message)
{
    if (state == NULL) {
        return;
    }
    bounce_audio_null_copy(state->error, sizeof(state->error), message);
}

static const char *bounce_audio_null_last_error(const BounceAudioBackend *self)
{
    const BounceAudioNullState *state;

    if (self == NULL || self->state == NULL) {
        return "";
    }
    state = (const BounceAudioNullState *)self->state;
    return state->error;
}

static const char *bounce_audio_null_name(const BounceAudioBackend *self)
{
    (void)self;
    return "null";
}

/*
 * "Open" records the requested format and succeeds. It contacts no device, so it
 * can never fail for lack of hardware -- that is the entire point of a Null
 * backend, and it is why it must never be substituted automatically for a real
 * backend that failed.
 */
static BounceAudioBackendStatus bounce_audio_null_open(
    BounceAudioBackend *self,
    const BounceAudioBackendFormat *format,
    const char *device_name)
{
    BounceAudioNullState *state;

    if (self == NULL || self->state == NULL || format == NULL) {
        return BOUNCE_AUDIO_BACKEND_ERR_NULL_ARGUMENT;
    }
    if (!bounce_audio_backend_format_is_valid(format)) {
        bounce_audio_null_set_error((BounceAudioNullState *)self->state,
                                    "unsupported PCM format requested");
        return BOUNCE_AUDIO_BACKEND_ERR_UNSUPPORTED_FORMAT;
    }
    state = (BounceAudioNullState *)self->state;
    if (state->open) {
        bounce_audio_null_set_error(state, "backend is already open");
        return BOUNCE_AUDIO_BACKEND_ERR_STATE;
    }

    state->sample_rate_hz = format->sample_rate_hz;
    state->channels = format->channels;
    bounce_audio_null_copy(state->device, sizeof(state->device),
                           (device_name != NULL) ? device_name
                                                 : BOUNCE_AUDIO_BACKEND_DEVICE_DEFAULT);
    bounce_audio_null_copy(state->error, sizeof(state->error), "");
    state->open = 1;
    state->closed_once = 0;
    return BOUNCE_AUDIO_BACKEND_OK;
}

static BounceAudioBackendStatus bounce_audio_null_write(
    BounceAudioBackend *self,
    const int16_t *samples,
    size_t frames)
{
    BounceAudioNullState *state;
    BounceAudioBackendStatus precheck;

    precheck = bounce_audio_backend_precheck_write(self, samples, frames);
    if (precheck != BOUNCE_AUDIO_BACKEND_OK) {
        return precheck;
    }
    state = (BounceAudioNullState *)self->state;
    if (!state->open) {
        bounce_audio_null_set_error(state, "write before open");
        return BOUNCE_AUDIO_BACKEND_ERR_STATE;
    }
    if (frames == 0u) {
        return BOUNCE_AUDIO_BACKEND_OK; /* Documented no-op, not an error. */
    }

    /*
     * Counted, then discarded. The PCM is NOT retained: retaining it would be a
     * queue, which belongs to a later step and not here.
     */
    state->total_frames += (unsigned long long)frames;
    state->total_bytes += (unsigned long long)frames
                        * (unsigned long long)sizeof(int16_t)
                        * (unsigned long long)state->channels;
    state->write_count++;
    return BOUNCE_AUDIO_BACKEND_OK;
}

static BounceAudioBackendStatus bounce_audio_null_drain(
    BounceAudioBackend *self)
{
    BounceAudioNullState *state;

    if (self == NULL || self->state == NULL) {
        return BOUNCE_AUDIO_BACKEND_ERR_NULL_ARGUMENT;
    }
    state = (BounceAudioNullState *)self->state;
    if (!state->open) {
        bounce_audio_null_set_error(state, "drain before open");
        return BOUNCE_AUDIO_BACKEND_ERR_STATE;
    }
    state->drain_count++;
    return BOUNCE_AUDIO_BACKEND_OK;
}

static void bounce_audio_null_close(BounceAudioBackend *self)
{
    BounceAudioNullState *state;

    /* Closing is always safe, even from a half-initialised backend. */
    if (self == NULL || self->state == NULL) {
        return;
    }
    state = (BounceAudioNullState *)self->state;
    state->open = 0;
    state->closed_once = 1;
}

void bounce_audio_backend_null_reset(BounceAudioNullState *state)
{
    if (state == NULL) {
        return;
    }
    state->open = 0;
    state->closed_once = 0;
    state->sample_rate_hz = 0u;
    state->channels = 0u;
    state->total_frames = 0ull;
    state->total_bytes = 0ull;
    state->write_count = 0ull;
    state->drain_count = 0ull;
    state->device[0] = '\0';
    state->error[0] = '\0';
}

BounceAudioBackendStatus bounce_audio_backend_null_init(
    BounceAudioBackend *backend,
    BounceAudioNullState *state,
    const char *device_name)
{
    if (backend == NULL || state == NULL) {
        return BOUNCE_AUDIO_BACKEND_ERR_NULL_ARGUMENT;
    }

    bounce_audio_backend_null_reset(state);
    bounce_audio_null_copy(state->device, sizeof(state->device),
                           (device_name != NULL) ? device_name
                                                 : BOUNCE_AUDIO_BACKEND_DEVICE_DEFAULT);

    backend->open = bounce_audio_null_open;
    backend->write_pcm = bounce_audio_null_write;
    backend->drain = bounce_audio_null_drain;
    backend->close = bounce_audio_null_close;
    backend->name = bounce_audio_null_name;
    backend->last_error = bounce_audio_null_last_error;
    backend->state = state;
    return BOUNCE_AUDIO_BACKEND_OK;
}
