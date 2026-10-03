/*
 * STEP 14B-27 -- LAYER B, GAMEPLAY AUDIO EVENT IDENTITY.
 *
 * See audio_event.h for the full boundary statement, the source evidence for
 * the three identities, and why there is no playback argument, no queue and no
 * decoder dependency.
 *
 * This translation unit is deliberately tiny and deliberately inert: it holds
 * one function pointer, forwards one enum to it, and self-tests the ordering
 * property. It contains no playback, no PCM, no waveform, no backend, and no
 * reference to the RTPL decoder.
 */

#include "audio_event.h"

#include <string.h>

/*
 * The installed consumer, or NULL when none is. Module-private so that no
 * gameplay module can mutate it: installation goes through
 * bounce_audio_event_set_sink(), and emission goes through
 * bounce_audio_event_emit().
 */
static BounceAudioEventSink audio_event_sink = NULL;
static void *audio_event_context = NULL;

const char *bounce_audio_event_name(BounceAudioEvent event)
{
    switch (event) {
    case BOUNCE_AUDIO_EVENT_NONE:   return "NONE";
    case BOUNCE_AUDIO_EVENT_UP:     return "UP";
    case BOUNCE_AUDIO_EVENT_PICKUP: return "PICKUP";
    case BOUNCE_AUDIO_EVENT_POP:    return "POP";
    default:                        return "RESERVED";
    }
}

int bounce_audio_event_set_sink(BounceAudioEventSink sink, void *context)
{
    if (sink == NULL || context == NULL) {
        /* Removing the consumer is a legitimate operation, not an error. */
        audio_event_sink = NULL;
        audio_event_context = NULL;
        return 0;
    }
    audio_event_sink = sink;
    audio_event_context = context;
    return 0;
}

bool bounce_audio_event_has_sink(void)
{
    return audio_event_sink != NULL;
}

int bounce_audio_event_emit(BounceAudioEvent event)
{
    BounceAudioEventSink sink;
    void *context;

    if (event == BOUNCE_AUDIO_EVENT_NONE) {
        return 1;
    }
    if (event != BOUNCE_AUDIO_EVENT_UP
        && event != BOUNCE_AUDIO_EVENT_PICKUP
        && event != BOUNCE_AUDIO_EVENT_POP) {
        return -1;
    }
    /*
     * Read both module fields once, so a consumer that re-enters emission
     * (for example by triggering a cascading event) cannot observe a half
     * updated pair.
     */
    sink = audio_event_sink;
    context = audio_event_context;
    if (sink == NULL) {
        return 0;
    }
    /* Synchronous, immediate, in program order. Nothing is buffered. */
    sink(context, event);
    return 0;
}

/* ------------------------------------------------------------------------ */
/* Self-test                                                                  */
/* ------------------------------------------------------------------------ */

enum {
    /* Capacity for the ordering test; comfortably above the events it emits. */
    AUDIO_EVENT_TEST_CAPACITY = 8
};

typedef struct AudioEventTestLog {
    BounceAudioEvent events[AUDIO_EVENT_TEST_CAPACITY];
    unsigned int count;
    unsigned int overflowed;
} AudioEventTestLog;

static void audio_event_test_sink(void *context, BounceAudioEvent event)
{
    AudioEventTestLog *log = (AudioEventTestLog *)context;

    if (log == NULL) {
        return;
    }
    if (log->count >= AUDIO_EVENT_TEST_CAPACITY) {
        log->overflowed = 1u;
        return;
    }
    log->events[log->count] = event;
    log->count += 1u;
}

int bounce_audio_event_verify(FILE *output_file)
{
    AudioEventTestLog log;
    static const BounceAudioEvent expected[3] = {
        BOUNCE_AUDIO_EVENT_UP,
        BOUNCE_AUDIO_EVENT_PICKUP,
        BOUNCE_AUDIO_EVENT_POP
    };
    int failures = 0;
    unsigned int index;

    if (output_file != NULL) {
        (void)fprintf(output_file,
            "audio event identity self-test\n"
            "  (no playback, no PCM, no backend, no RTPL decode)\n");
    }

    /* 1. With no sink installed, emission is a silent no-op that still
     *    reports success, so gameplay is unaffected before audio exists. */
    (void)bounce_audio_event_set_sink(NULL, NULL);
    if (bounce_audio_event_has_sink()) {
        ++failures;
    }
    if (bounce_audio_event_emit(BOUNCE_AUDIO_EVENT_UP) != 0) {
        ++failures;
    }
    if (bounce_audio_event_emit(BOUNCE_AUDIO_EVENT_POP) != 0) {
        ++failures;
    }
    if (output_file != NULL) {
        (void)fprintf(output_file,
            "  %s no sink installed: emission is a silent no-op\n",
            failures == 0 ? "ok  " : "FAIL");
    }

    /* 2. With a sink installed, sequential emissions arrive in ORDER and each
     *    one arrives BEFORE the next is emitted. This is the property a
     *    "pending sound" scalar could not provide: with a scalar, only the
     *    last of these three events could survive to be consumed. */
    memset(&log, 0, sizeof(log));
    (void)bounce_audio_event_set_sink(audio_event_test_sink, &log);
    if (!bounce_audio_event_has_sink()) {
        ++failures;
    }
    for (index = 0u; index < 3u; ++index) {
        if (bounce_audio_event_emit(expected[index]) != 0) {
            ++failures;
        }
        /*
         * Assert the consumer has ALREADY seen this event, before the next
         * emission. This is what distinguishes synchronous emission from
         * buffering, and it is the whole point of the design.
         */
        if (log.count != index + 1u) {
            ++failures;
        }
    }
    if (log.overflowed != 0u || log.count != 3u) {
        ++failures;
    }
    for (index = 0u; index < 3u && index < log.count; ++index) {
        if (log.events[index] != expected[index]) {
            ++failures;
        }
    }
    if (output_file != NULL) {
        (void)fprintf(output_file,
            "  %s 3 sequential emissions preserved in order:",
            (log.count == 3u && log.overflowed == 0u) ? "ok  " : "FAIL");
        for (index = 0u; index < log.count; ++index) {
            (void)fprintf(output_file, " %s", bounce_audio_event_name(log.events[index]));
        }
        (void)fprintf(output_file, "\n");
    }

    /* 3. Interleaving repeats proves no collapse: UP, PICKUP, UP, POP keeps all
     *    four, which is what a multi-tile walk touching two hoops must do. */
    memset(&log, 0, sizeof(log));
    (void)bounce_audio_event_set_sink(audio_event_test_sink, &log);
    (void)bounce_audio_event_emit(BOUNCE_AUDIO_EVENT_UP);
    (void)bounce_audio_event_emit(BOUNCE_AUDIO_EVENT_PICKUP);
    (void)bounce_audio_event_emit(BOUNCE_AUDIO_EVENT_UP);
    (void)bounce_audio_event_emit(BOUNCE_AUDIO_EVENT_POP);
    if (log.count != 4u
        || log.overflowed != 0u
        || log.events[0] != BOUNCE_AUDIO_EVENT_UP
        || log.events[1] != BOUNCE_AUDIO_EVENT_PICKUP
        || log.events[2] != BOUNCE_AUDIO_EVENT_UP
        || log.events[3] != BOUNCE_AUDIO_EVENT_POP) {
        ++failures;
    }
    if (output_file != NULL) {
        (void)fprintf(output_file,
            "  %s repeated + interleaved emissions not collapsed (%u of 4 kept)\n",
            log.count == 4u ? "ok  " : "FAIL", log.count);
    }

    /* 4. The sentinel is never delivered to a consumer. */
    memset(&log, 0, sizeof(log));
    (void)bounce_audio_event_set_sink(audio_event_test_sink, &log);
    if (bounce_audio_event_emit(BOUNCE_AUDIO_EVENT_NONE) != 1) {
        ++failures;
    }
    if (log.count != 0u) {
        ++failures;
    }
    if (output_file != NULL) {
        (void)fprintf(output_file,
            "  %s NONE is a sentinel, never emitted (%u delivered)\n",
            log.count == 0u ? "ok  " : "FAIL", log.count);
    }

    /* 5. The identity vocabulary is exactly the three original resources, and
     *    a reserved value is rejected rather than forwarded. */
    if (bounce_audio_event_name(BOUNCE_AUDIO_EVENT_UP)[0] != 'U'
        || bounce_audio_event_name(BOUNCE_AUDIO_EVENT_PICKUP)[0] != 'P'
        || bounce_audio_event_name(BOUNCE_AUDIO_EVENT_POP)[0] != 'P') {
        ++failures;
    }
    if (bounce_audio_event_emit((BounceAudioEvent)4) != -1) {
        ++failures;
    }
    if (log.count != 0u) {
        ++failures;
    }
    if (output_file != NULL) {
        (void)fprintf(output_file,
            "  %s reserved identity 4 rejected, not forwarded\n",
            log.count == 0u ? "ok  " : "FAIL");
    }

    /* Always leave no consumer installed, so gameplay runs sink-free. */
    (void)bounce_audio_event_set_sink(NULL, NULL);
    if (bounce_audio_event_has_sink()) {
        ++failures;
    }

    if (output_file != NULL) {
        (void)fprintf(output_file,
            "audio event identity self-test: %s (%d failures)\n",
            failures == 0 ? "PASS" : "FAIL", failures);
    }
    return failures == 0 ? 0 : -1;
}
