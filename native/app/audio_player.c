/*
 * audio_player.c -- STEP 14B-34-D audio integration layer.
 *
 * Layout of the state this file owns:
 *
 *   cached songs   the three ORIGINAL assets, decoded ONCE at create time, so
 *                  neither the producer nor the worker ever touches the disk
 *                  after start-up
 *   slots          a fixed ring of pre-allocated PCM buffers
 *   mutex + cond   one of each, guarding the ring, the stop flag, the counters
 *   worker         one pthread, created at start-up and joined at shutdown
 *
 * pthread appears HERE and nowhere else. Layer B, the RTPL decoder, the
 * renderer, and the portable backend interface do not include a threading
 * header and do not know a thread exists.
 *
 * Every failure path returns or reports. There is no abort(), no exit(), and no
 * assert() on a runtime path; a failed audio layer always leaves the game
 * running with audio off.
 *
 * FIFO DISCIPLINE
 *   The ring is scanned strictly oldest-first, so slot `head` is always the
 *   oldest pending event. Between popping a slot and finishing its device write,
 *   the worker owns that slot exclusively: `count` has already been decremented,
 *   so the producer's next insertion index cannot be the slot in flight. No
 *   borrowed pointer ever crosses the thread boundary.
 */

#include "audio_player.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "audio_backend_null.h"
#include "audio_synth.h"

#if BOUNCE_AUDIO_HAVE_ALSA
#include "audio_backend_alsa.h"
#endif

/* One ring entry. Its PCM buffer is owned for the player's whole life. */
typedef struct BounceAudioSlot {
    BounceAudioEvent event;
    size_t frames;
    int16_t *pcm;
} BounceAudioSlot;

struct BounceAudioPlayer {
    BounceAudioBackendFormat format;
    BounceSynthConfig synth;
    BounceAudioPlayerShutdown shutdown;
    unsigned int drain_timeout_ms;
    BounceAudioPlayerBackend requested;

    BounceAudioBackend backend;
    BounceAudioNullState *null_state; /* owned, NULL unless Null */
#if BOUNCE_AUDIO_HAVE_ALSA
    BounceAudioAlsaState *alsa_state; /* owned, NULL unless ALSA */
#endif
    char backend_name[16];

    /* Pre-decoded ORIGINAL assets, indexed by bounce_audio_player_index(). */
    BounceRtplSong songs[3];
    BounceSynthPlan plans[3];

    BounceAudioSlot slots[BOUNCE_AUDIO_PLAYER_QUEUE_CAPACITY];
    size_t slot_frames;
    unsigned int head;  /* oldest pending entry */
    unsigned int count; /* entries in the ring */

    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int mutex_ready;
    int cond_ready;
    pthread_t worker;
    int thread_started;
    int stop;
    int sink_installed;
    int running;

    /* Counters, all guarded by `mutex`. */
    BounceAudioPlayerStats stats;
};

/* Identity -> cached song index. Mirrors audio_synth's mapping exactly. */
static int bounce_audio_player_index(BounceAudioEvent event)
{
    switch (event) {
    case BOUNCE_AUDIO_EVENT_UP:
        return 0;
    case BOUNCE_AUDIO_EVENT_PICKUP:
        return 1;
    case BOUNCE_AUDIO_EVENT_POP:
        return 2;
    case BOUNCE_AUDIO_EVENT_NONE:
    default:
        break;
    }
    return -1;
}

void bounce_audio_player_default_config(BounceAudioPlayerConfig *out)
{
    if (out == NULL) {
        return;
    }
    out->resource_root = "src/main/resources";
    out->backend = BOUNCE_AUDIO_PLAYER_BACKEND_AUTO;
    out->device_name = NULL;
    out->shutdown = BOUNCE_AUDIO_PLAYER_SHUTDOWN_DISCARD;
    out->drain_timeout_ms = 0u;
}

/* -------------------------------------------------------------------------
 * Backend selection. AUTO never falls back; a failure means "no audio".
 * ------------------------------------------------------------------------- */

static int bounce_audio_player_open_backend(BounceAudioPlayer *player,
                                            const char *device_name)
{
#if BOUNCE_AUDIO_HAVE_ALSA
    int want_alsa = (player->requested == BOUNCE_AUDIO_PLAYER_BACKEND_ALSA)
                 || (player->requested == BOUNCE_AUDIO_PLAYER_BACKEND_AUTO);
#else
    int want_alsa = 0;
#endif

    if (want_alsa) {
#if BOUNCE_AUDIO_HAVE_ALSA
        player->alsa_state =
            (BounceAudioAlsaState *)calloc(1u, sizeof(*player->alsa_state));
        if (player->alsa_state == NULL) {
            return 0;
        }
        if (bounce_audio_backend_alsa_init(&player->backend, player->alsa_state,
                                           device_name)
            != BOUNCE_AUDIO_BACKEND_OK) {
            /* The state is still owned by the player; the caller's teardown
             * frees it. Freeing it here as well would double free. */
            return 0;
        }
        if (player->backend.open(&player->backend, &player->format, device_name)
            != BOUNCE_AUDIO_BACKEND_OK) {
            /* Reported, never worked around: no silent substitution. Teardown
             * closes whatever the failed open may have left, then frees. */
            return 0;
        }
        (void)snprintf(player->backend_name, sizeof(player->backend_name),
                       "alsa");
        return 1;
#else
        /*
         * Built without ALSA. AUTO and ALSA are BOTH reported as unavailable
         * here, and neither is quietly turned into a Null backend: doing that
         * would hide a real absence behind a fake device. The caller gets NULL
         * and carries on with audio off.
         */
        (void)device_name;
        return 0;
#endif
    }

    if (player->requested != BOUNCE_AUDIO_PLAYER_BACKEND_NULL) {
        /* Only an explicit NULL request may produce the Null backend. */
        return 0;
    }

    /* Explicitly requested Null. */
    player->null_state =
        (BounceAudioNullState *)calloc(1u, sizeof(*player->null_state));
    if (player->null_state == NULL) {
        return 0;
    }
    if (bounce_audio_backend_null_init(&player->backend, player->null_state,
                                       device_name)
        != BOUNCE_AUDIO_BACKEND_OK) {
        return 0; /* Teardown frees the state. */
    }
    if (player->backend.open(&player->backend, &player->format, device_name)
        != BOUNCE_AUDIO_BACKEND_OK) {
        return 0; /* Teardown closes and frees. */
    }
    (void)snprintf(player->backend_name, sizeof(player->backend_name), "null");
    return 1;
}

/*
 * Close and release the backend exactly once, then disarm the table.
 *
 * The disarm matters: a failed `open` leaves the function pointers populated but
 * the state owned by this player, so a teardown that ran twice would call
 * `close` through a table whose state has already been freed. Clearing the table
 * here makes a second call a no-op instead of a use-after-free.
 */
static void bounce_audio_player_close_backend(BounceAudioPlayer *player)
{
    if (player->backend.state != NULL
        && bounce_audio_backend_is_complete(&player->backend)) {
        player->backend.close(&player->backend);
    }
    memset(&player->backend, 0, sizeof(player->backend));
    player->backend_name[0] = '\0';
    free(player->null_state);
    player->null_state = NULL;
#if BOUNCE_AUDIO_HAVE_ALSA
    free(player->alsa_state);
    player->alsa_state = NULL;
#endif
}

/* -------------------------------------------------------------------------
 * The producer. O(1): a table lookup, a lock, one struct copy, a signal.
 * ------------------------------------------------------------------------- */

static int bounce_audio_player_enqueue(BounceAudioPlayer *player,
                                       BounceAudioEvent event)
{
    unsigned int insert_at;
    int index;

    if (player == NULL) {
        return 0;
    }
    index = bounce_audio_player_index(event);
    if (index < 0) {
        /*
         * NONE, or an identity this layer does not know. Rejected without
         * queueing, without touching the disk, and without blocking.
         */
        (void)pthread_mutex_lock(&player->mutex);
        player->stats.events_rejected++;
        (void)pthread_mutex_unlock(&player->mutex);
        return 0;
    }

    (void)pthread_mutex_lock(&player->mutex);

    if (player->stop) {
        player->stats.events_rejected++;
        (void)pthread_mutex_unlock(&player->mutex);
        return 0;
    }

    if (player->count == BOUNCE_AUDIO_PLAYER_QUEUE_CAPACITY) {
        /*
         * IMPLEMENTATION POLICY -- DROP-OLDEST, chosen over drop-newest so the
         * most recent gameplay event survives, and over blocking so the gameplay
         * thread is never delayed by a slow device. The original policy is
         * UNKNOWN, so this is declared rather than inferred.
         */
        player->head = (player->head + 1u) % BOUNCE_AUDIO_PLAYER_QUEUE_CAPACITY;
        player->count--;
        player->stats.events_dropped++;
    }

    insert_at = (player->head + player->count) % BOUNCE_AUDIO_PLAYER_QUEUE_CAPACITY;
    player->slots[insert_at].event = event;
    player->slots[insert_at].frames = 0u;
    player->count++;
    player->stats.events_enqueued++;

    (void)pthread_cond_signal(&player->cond);
    (void)pthread_mutex_unlock(&player->mutex);
    return 1;
}

/* The Layer B sink. Returns immediately; never blocks, never allocates. */
static void bounce_audio_player_sink(void *context, BounceAudioEvent event)
{
    (void)bounce_audio_player_enqueue((BounceAudioPlayer *)context, event);
}

/* -------------------------------------------------------------------------
 * The consumer. Everything that can block lives here.
 * ------------------------------------------------------------------------- */

static void *bounce_audio_player_worker(void *argument)
{
    BounceAudioPlayer *player = (BounceAudioPlayer *)argument;

    for (;;) {
        BounceAudioEvent event;
        unsigned int slot_index;
        size_t frames = 0u;
        int rendered;
        int written = 0;

        /* Claim the oldest entry, if any. */
        (void)pthread_mutex_lock(&player->mutex);
        while (!player->stop && player->count == 0u) {
            (void)pthread_cond_wait(&player->cond, &player->mutex);
        }
        if (player->count == 0u) {
            /* stop == 1 and the queue is empty: the worker is finished. */
            (void)pthread_mutex_unlock(&player->mutex);
            break;
        }
        event = player->slots[player->head].event;
        slot_index = player->head;
        player->head = (player->head + 1u) % BOUNCE_AUDIO_PLAYER_QUEUE_CAPACITY;
        player->count--;
        (void)pthread_mutex_unlock(&player->mutex);

        /*
         * From here the slot is exclusively ours: `count` was decremented, so
         * the producer cannot choose this index. Render and write with the
         * mutex released, so a slow device never delays the gameplay thread.
         */
        {
            BounceAudioSlot *slot = &player->slots[slot_index];
            int index = bounce_audio_player_index(event);

            rendered = (index >= 0)
                    && (bounce_synth_render_pcm(&player->songs[index],
                                                &player->synth,
                                                &player->plans[index], slot->pcm,
                                                player->slot_frames, &frames)
                        == BOUNCE_SYNTH_OK);
            if (rendered) {
                /* MAY BLOCK. This is the only place a device is touched. */
                written = (player->backend.write_pcm(&player->backend, slot->pcm,
                                                     frames)
                           == BOUNCE_AUDIO_BACKEND_OK);
            }
        }

        (void)pthread_mutex_lock(&player->mutex);
        if (!rendered) {
            player->stats.render_errors++;
        } else if (!written) {
            player->stats.backend_errors++;
        } else {
            player->stats.events_played++;
            player->stats.frames_played += (unsigned long long)frames;
        }
        player->slots[slot_index].frames = frames;
        /* Wake a draining destroyer that is waiting for the ring to empty. */
        (void)pthread_cond_broadcast(&player->cond);
        (void)pthread_mutex_unlock(&player->mutex);
    }
    return NULL;
}

/* -------------------------------------------------------------------------
 * Lifecycle
 * ------------------------------------------------------------------------- */

BounceAudioPlayer *bounce_audio_player_create(
    const BounceAudioPlayerConfig *config)
{
    BounceAudioPlayerConfig defaults;
    BounceAudioPlayer *player;
    size_t max_plan_frames = 0u;
    unsigned int i;
    int index;

    if (config == NULL) {
        bounce_audio_player_default_config(&defaults);
        config = &defaults;
    }
    if (config->resource_root == NULL) {
        return NULL;
    }

    player = (BounceAudioPlayer *)calloc(1u, sizeof(*player));
    if (player == NULL) {
        return NULL;
    }

    player->requested = config->backend;
    player->shutdown = config->shutdown;
    player->drain_timeout_ms = config->drain_timeout_ms;
    bounce_synth_default_config(&player->synth);
    bounce_audio_backend_default_format(&player->format);

    /* The backend format must be the one the renderer already produces. */
    if (player->format.sample_rate_hz != player->synth.sample_rate
        || player->format.channels != (unsigned int)player->synth.channels) {
        free(player);
        return NULL;
    }

    if (pthread_mutex_init(&player->mutex, NULL) != 0) {
        free(player);
        return NULL;
    }
    player->mutex_ready = 1;
    if (pthread_cond_init(&player->cond, NULL) != 0) {
        (void)pthread_mutex_destroy(&player->mutex);
        free(player);
        return NULL;
    }
    player->cond_ready = 1;

    /*
     * Pre-decode the three ORIGINAL assets ONCE. After this no thread touches
     * the filesystem, so there is no disk latency anywhere in the audio path.
     */
    for (index = 0; index < 3; index++) {
        BounceAudioEvent event = (index == 0)   ? BOUNCE_AUDIO_EVENT_UP
                                 : (index == 1) ? BOUNCE_AUDIO_EVENT_PICKUP
                                                : BOUNCE_AUDIO_EVENT_POP;

        if (bounce_synth_prepare(event, config->resource_root, &player->synth,
                                 &player->songs[index], &player->plans[index])
            != BOUNCE_SYNTH_OK) {
            goto fail;
        }
        if (player->plans[index].total_frames > max_plan_frames) {
            max_plan_frames = player->plans[index].total_frames;
        }
    }

    /* Derive the slot size from the data, with declared headroom. */
    player->slot_frames = max_plan_frames
                        + (max_plan_frames * BOUNCE_AUDIO_PLAYER_HEADROOM_PERCENT
                           / 100u);
    if (player->slot_frames == 0u) {
        goto fail;
    }

    for (i = 0u; i < BOUNCE_AUDIO_PLAYER_QUEUE_CAPACITY; i++) {
        player->slots[i].pcm =
            (int16_t *)calloc(player->slot_frames, sizeof(int16_t));
        if (player->slots[i].pcm == NULL) {
            goto fail;
        }
    }

    if (!bounce_audio_player_open_backend(player, config->device_name)) {
        /* No usable device. A NORMAL outcome: the game runs without audio
         * rather than falling back to a fake backend. */
        goto fail;
    }

    if (pthread_create(&player->worker, NULL, bounce_audio_player_worker,
                       player) != 0) {
        goto fail;
    }
    player->thread_started = 1;

    (void)pthread_mutex_lock(&player->mutex);
    player->stats.accepting = 1;
    (void)pthread_mutex_unlock(&player->mutex);
    player->running = 1;
    return player;

fail:
    bounce_audio_player_close_backend(player);
    for (i = 0u; i < BOUNCE_AUDIO_PLAYER_QUEUE_CAPACITY; i++) {
        free(player->slots[i].pcm);
        player->slots[i].pcm = NULL;
    }
    if (player->cond_ready) {
        (void)pthread_cond_destroy(&player->cond);
    }
    if (player->mutex_ready) {
        (void)pthread_mutex_destroy(&player->mutex);
    }
    free(player);
    return NULL;
}

int bounce_audio_player_attach_sink(BounceAudioPlayer *player)
{
    if (player == NULL) {
        return 0;
    }
    if (bounce_audio_event_set_sink(bounce_audio_player_sink, player) != 0) {
        return 0;
    }
    player->sink_installed = 1;
    return 1;
}

int bounce_audio_player_detach_sink(BounceAudioPlayer *player)
{
    if (player == NULL || !player->sink_installed) {
        return 1;
    }
    (void)bounce_audio_event_set_sink(NULL, NULL);
    player->sink_installed = 0;
    return 1;
}

int bounce_audio_player_is_running(const BounceAudioPlayer *player)
{
    return (player != NULL && player->running) ? 1 : 0;
}

const char *bounce_audio_player_backend_name(const BounceAudioPlayer *player)
{
    if (player == NULL) {
        return "none";
    }
    return player->backend_name;
}

int bounce_audio_player_stats(BounceAudioPlayer *player,
                              BounceAudioPlayerStats *out)
{
    if (player == NULL || out == NULL) {
        return 0;
    }
    (void)pthread_mutex_lock(&player->mutex);
    *out = player->stats;
    (void)pthread_mutex_unlock(&player->mutex);
    return 1;
}

/* Absolute deadline `timeout_ms` from now, for the drain wait. */
static void bounce_audio_player_deadline(struct timespec *when,
                                         unsigned int timeout_ms)
{
    struct timespec now;

    (void)clock_gettime(CLOCK_REALTIME, &now);
    when->tv_sec = now.tv_sec + (time_t)(timeout_ms / 1000u);
    when->tv_nsec = now.tv_nsec
                  + (long)((timeout_ms % 1000u) * 1000000u);
    if (when->tv_nsec >= 1000000000L) {
        when->tv_sec += 1;
        when->tv_nsec -= 1000000000L;
    }
}

void bounce_audio_player_destroy(BounceAudioPlayer *player)
{
    unsigned int i;

    if (player == NULL) {
        return;
    }

    /* 1. Stop accepting events before anything else. */
    (void)bounce_audio_player_detach_sink(player);

    (void)pthread_mutex_lock(&player->mutex);
    player->stop = 1;
    player->stats.accepting = 0;

    if (player->shutdown == BOUNCE_AUDIO_PLAYER_SHUTDOWN_DISCARD) {
        /* IMPLEMENTATION POLICY: abandon queued audio so shutdown never waits. */
        for (i = 0u; i < BOUNCE_AUDIO_PLAYER_QUEUE_CAPACITY; i++) {
            if (i < player->count) {
                player->stats.discarded_on_shutdown++;
            }
        }
        player->count = 0u;
    } else if (player->drain_timeout_ms > 0u) {
        /* DRAIN: let the worker finish what is queued, up to the declared
         * timeout, so shutdown cannot hang on a stuck device. */
        struct timespec deadline;

        bounce_audio_player_deadline(&deadline, player->drain_timeout_ms);
        while (player->count > 0u) {
            if (pthread_cond_timedwait(&player->cond, &player->mutex, &deadline)
                != 0) {
                break; /* Timed out: stop waiting, then discard. */
            }
        }
        for (i = 0u; i < player->count; i++) {
            player->stats.discarded_on_shutdown++;
        }
        player->count = 0u;
    }
    (void)pthread_mutex_unlock(&player->mutex);

    /* 2. Wake the worker. It exits once the ring is empty. */
    (void)pthread_cond_broadcast(&player->cond);

    /* 3. Join, so any in-flight device write finishes before the backend closes.
     *    This ordering is exactly what makes the close safe. */
    if (player->thread_started) {
        (void)pthread_join(player->worker, NULL);
    }

    /* 4. The backend is now guaranteed idle. */
    bounce_audio_player_close_backend(player);
    player->running = 0;

    for (i = 0u; i < BOUNCE_AUDIO_PLAYER_QUEUE_CAPACITY; i++) {
        free(player->slots[i].pcm);
        player->slots[i].pcm = NULL;
    }
    if (player->cond_ready) {
        (void)pthread_cond_destroy(&player->cond);
    }
    if (player->mutex_ready) {
        (void)pthread_mutex_destroy(&player->mutex);
    }
    free(player);
}
