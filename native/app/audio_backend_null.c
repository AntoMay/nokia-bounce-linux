/*
 * audio_backend_null.c -- STEP 14B-34-C Null audio backend implementation.
 *
 * No ALSA include. No platform header. No engine header. No allocation. No
 * thread. No queue. Produces no sound.
 *
 * "No thread" means it does not CREATE one. It is still called FROM a thread --
 * the audio player's worker -- and since BL-004 PARITY FIX the two amplitude
 * statistics it keeps are _Atomic, because that worker writes them with the
 * player's mutex released while a reader may run on the gameplay thread. See the
 * note on peak_sample in the header for the full argument; the assertion below is
 * the half of it that can be checked by the compiler.
 *
 * Every write is accounted for and then discarded, which is the whole point: a
 * caller can verify that the right number of frames reached the output boundary
 * without a device existing.
 */

#include "audio_backend_null.h"

#include <stdio.h>
#include <string.h>

/*
 * THE PEAK STATISTICS MUST BE LOCK-FREE, AND THIS SAYS SO AT BUILD TIME.
 *
 * Everything above argues that atomics are cheaper than a mutex HERE. That argument
 * is only true while they really are a plain word access: if a target were to
 * implement atomic_uint with a hidden lock, the audio worker would acquire it once
 * per rendered block, which is precisely the cost the design set out to avoid and
 * it would be invisible in review. ATOMIC_INT_LOCK_FREE == 2 is C11's "always
 * lock-free" value, so this build fails rather than silently paying for a lock.
 *
 * The other legitimate value is 1, "sometimes lock-free, never a lock-based
 * fallback", which is only acceptable when the compiler has told us it will use a
 * lock-free path on the target we are building for. Both are fine here; 0 and
 * anything else is not, which is what the assertion rejects.
 */
#if defined(ATOMIC_INT_LOCK_FREE) && ATOMIC_INT_LOCK_FREE
_Static_assert(ATOMIC_INT_LOCK_FREE != 0,
               "the peak statistics require a lock-free unsigned int;"
               " a lock-based fallback would put a lock on the audio hot path");
#endif

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
     *
     * The peak is the one thing taken from the buffer before it is dropped, and it
     * is two integers rather than the samples. See the note on peak_sample in the
     * header for why the volume gain is otherwise unobservable here.
     */
    {
        unsigned int peak = 0u;
        unsigned int observed;
        size_t i;

        for (i = 0u; i < frames; i++) {
            /*
             * Negated in unsigned arithmetic so INT16_MIN -- -32768, whose
             * magnitude has no int16 counterpart -- cannot overflow into a small
             * positive number and read as a quiet sample.
             */
            const unsigned int magnitude
                = (samples[i] < 0)
                      ? (unsigned int)(-(int)samples[i])
                      : (unsigned int)samples[i];

            if (magnitude > peak)
                peak = magnitude;
        }
        /*
         * THE PEAK UPDATE IS ATOMIC BECAUSE THE AUDIO WORKER IS ITS WRITER AND IT IS
         * NOT HOLDING A LOCK HERE. The worker drops player->mutex across render and
         * write on purpose, so a reader on the gameplay thread can race these two
         * words. Relaxed ordering is the right strength and no less: there is no other
         * memory this has to be ordered against, only two counters that must never be
         * seen half-written.
         *
         * THE MAXIMUM IS TAKEN WITH A COMPARE-EXCHANGE LOOP rather than a
         * load-compare-store. With one writer the simpler form would be fine, but this
         * is the same code path a second writer would use and the loop is what makes
         * "highest peak so far" actually mean that when two writers overlap: a plain
         * load-compare-store can read a stale maximum and write back a smaller one,
         * silently losing a peak. The exchange refreshes `observed` on failure, which
         * is what the loop re-tests.
         */
        atomic_store_explicit(
            &state->last_write_peak, peak, memory_order_relaxed);
        observed = atomic_load_explicit(&state->peak_sample, memory_order_relaxed);
        while (peak > observed
            && !atomic_compare_exchange_weak_explicit(
                   &state->peak_sample,
                   &observed,
                   peak,
                   memory_order_relaxed,
                   memory_order_relaxed)) {
            /* `observed` was refreshed by the failed exchange; re-test it. */
        }
    }
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
    /*
     * The amplitude summaries reset with the counters, atomically like every other
     * access. atomic_store rather than a plain assignment: this function is public
     * and is not restricted to the single-threaded start-up case by its signature,
     * so a caller that reset a live player's backend must not introduce a data race
     * with the audio worker. Relaxed is sufficient -- a reset is a value, not an
     * event, and there is nothing for it to be ordered against.
     */
    atomic_store_explicit(&state->peak_sample, 0u, memory_order_relaxed);
    atomic_store_explicit(&state->last_write_peak, 0u, memory_order_relaxed);
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
