/*
 * audio_backend_alsa.h -- STEP 14B-34-C ALSA audio backend.
 *
 * WHAT IT IS
 *   The Linux output backend. It opens an ALSA PCM device, negotiates the
 *   Target B PCM format, and writes rendered frames to it.
 *
 * PLATFORM SEPARATION
 *   This header contains NO ALSA include. The only ALSA type visible here is a
 *   forward declaration of `struct _snd_pcm`, which is exactly what ALSA's own
 *   `typedef struct _snd_pcm snd_pcm_t;` expands to. Everything else about ALSA
 *   lives in audio_backend_alsa.c, so a build without ALSA can still compile
 *   this header, the portable interface, and the Null backend.
 *
 * DEVICE SELECTION
 *   Nothing is hard-coded. No `/dev/snd/hw:*`, no card number, no device number.
 *   When the caller passes NULL the backend uses ALSA's own "default", which
 *   defers to the host's configuration. A caller may pass any ALSA plug name,
 *   including "default", "hw:0,0", or "plughw:1,0".
 *
 * THREADING
 *   `write_pcm` may BLOCK on a real device, so a later step must call it from a
 *   dedicated audio thread, never from the gameplay thread. This step creates no
 *   thread and starts no loop; see the note in audio_backend.h.
 *
 * ALLOCATION
 *   None. The caller supplies the state, so the backend fits in fixed storage.
 */

#ifndef BOUNCE_NATIVE_APP_AUDIO_BACKEND_ALSA_H
#define BOUNCE_NATIVE_APP_AUDIO_BACKEND_ALSA_H

#include <stddef.h>
#include <stdint.h>

#include "audio_backend.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration only. This mirrors ALSA's own snd_pcm_t definition and
 * deliberately avoids including <alsa/asoundlib.h> in a portable header. */
struct _snd_pcm;

#define BOUNCE_AUDIO_ALSA_DEVICE_CAPACITY 96u
#define BOUNCE_AUDIO_ALSA_ERROR_CAPACITY  256u

/*
 * Caller-owned state. Sized generously because a caller may keep one of these on
 * the stack; it is not a hot path.
 */
typedef struct BounceAudioAlsaState {
    struct _snd_pcm *pcm;   /* NULL until a successful open */
    unsigned int sample_rate_hz;
    unsigned int channels;
    size_t period_frames;
    int open;
    int disconnected;       /* 1 once the device has been reported lost */
    char device[BOUNCE_AUDIO_ALSA_DEVICE_CAPACITY];
    char error[BOUNCE_AUDIO_ALSA_ERROR_CAPACITY];
} BounceAudioAlsaState;

/*
 * Populate `backend` so it dispatches to the ALSA implementation, using `state`
 * as its private storage. This performs no allocation and opens no device; it
 * only wires up the function table. `device_name` may be NULL, meaning ALSA's
 * "default". The name is remembered and used by the later `open` call.
 *
 * Returns BOUNCE_AUDIO_BACKEND_ERR_NULL_ARGUMENT if either pointer is NULL,
 * having written nothing.
 */
BounceAudioBackendStatus bounce_audio_backend_alsa_init(
    BounceAudioBackend *backend,
    BounceAudioAlsaState *state,
    const char *device_name);

/*
 * Non-zero when a usable ALSA PCM device could be opened with the Target B
 * format. It opens and closes a device to find out, and never leaves one open.
 * Intended for start-up probing and for tests; it produces no sound.
 */
int bounce_audio_backend_alsa_device_available(const char *device_name);

/* Zero the state, closing nothing. Safe with NULL. */
void bounce_audio_backend_alsa_reset(BounceAudioAlsaState *state);

#ifdef __cplusplus
}
#endif

#endif /* BOUNCE_NATIVE_APP_AUDIO_BACKEND_ALSA_H */
