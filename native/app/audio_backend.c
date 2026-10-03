/*
 * audio_backend.c -- STEP 14B-34-C portable interface helpers.
 *
 * Platform-free by construction: this file includes no audio library, no
 * platform header, and no engine header. It holds the shared policy that every
 * backend must obey, so the Null, ALSA, and any future WASAPI backend reject
 * the same inputs identically instead of each inventing its own rules.
 *
 * It allocates nothing, opens nothing, and plays nothing.
 */

#include "audio_backend.h"

/*
 * Target B PCM format, declared by STEP 14B-33 section 13 and produced by the
 * STEP 14B-34-B renderer. These are ENGINEERING PARAMETERS. No original evidence
 * describes any PCM layout: the original game passed raw `.ott` bytes to the
 * platform's native `play0` boundary and never exposed samples.
 */
void bounce_audio_backend_default_format(BounceAudioBackendFormat *out)
{
    if (out == NULL) {
        return;
    }
    out->sample_rate_hz = 44100u;
    out->channels = 1u;
    out->sample_format = BOUNCE_AUDIO_BACKEND_S16_LE;
    out->period_frames = 0u;
}

int bounce_audio_backend_format_is_valid(const BounceAudioBackendFormat *format)
{
    if (format == NULL) {
        return 0;
    }
    if (format->sample_format != BOUNCE_AUDIO_BACKEND_S16_LE) {
        return 0;
    }
    /* Wide enough to accept any real rate, narrow enough to reject nonsense. */
    if (format->sample_rate_hz < 8000u || format->sample_rate_hz > 192000u) {
        return 0;
    }
    /* Target B is mono. Anything else is refused rather than silently mixed. */
    if (format->channels != 1u) {
        return 0;
    }
    return 1;
}

const char *bounce_audio_backend_status_text(BounceAudioBackendStatus status)
{
    switch (status) {
    case BOUNCE_AUDIO_BACKEND_OK:
        return "ok";
    case BOUNCE_AUDIO_BACKEND_ERR_NULL_ARGUMENT:
        return "null argument";
    case BOUNCE_AUDIO_BACKEND_ERR_STATE:
        return "operation is not valid in the backend's current state";
    case BOUNCE_AUDIO_BACKEND_ERR_UNSUPPORTED_FORMAT:
        return "unsupported PCM format";
    case BOUNCE_AUDIO_BACKEND_ERR_NOT_AVAILABLE:
        return "no usable output device on this host";
    case BOUNCE_AUDIO_BACKEND_ERR_OPEN:
        return "output device could not be opened";
    case BOUNCE_AUDIO_BACKEND_ERR_CONFIGURE:
        return "output device could not be configured";
    case BOUNCE_AUDIO_BACKEND_ERR_WRITE:
        return "write to the output device failed";
    case BOUNCE_AUDIO_BACKEND_ERR_DEVICE_LOST:
        return "output device was lost";
    default:
        break;
    }
    return "unknown status";
}

int bounce_audio_backend_is_complete(const BounceAudioBackend *backend)
{
    if (backend == NULL) {
        return 0;
    }
    if (backend->open == NULL || backend->write_pcm == NULL
        || backend->drain == NULL || backend->close == NULL
        || backend->name == NULL || backend->last_error == NULL) {
        return 0;
    }
    return 1;
}

BounceAudioBackendStatus bounce_audio_backend_precheck_write(
    const BounceAudioBackend *backend,
    const int16_t *samples,
    size_t frames)
{
    if (backend == NULL || backend->write_pcm == NULL) {
        return BOUNCE_AUDIO_BACKEND_ERR_NULL_ARGUMENT;
    }
    /* An empty buffer is a no-op, not a caller error. */
    if (frames == 0u) {
        return BOUNCE_AUDIO_BACKEND_OK;
    }
    if (samples == NULL) {
        return BOUNCE_AUDIO_BACKEND_ERR_NULL_ARGUMENT;
    }
    if (backend->state == NULL) {
        /* Never initialised, so there is nothing to write to. */
        return BOUNCE_AUDIO_BACKEND_ERR_STATE;
    }
    return BOUNCE_AUDIO_BACKEND_OK;
}
