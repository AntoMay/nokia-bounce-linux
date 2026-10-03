/*
 * Standalone runner for the STEP 14B-27 audio event identity self-test.
 *
 * WHY THIS FILE EXISTS
 *   The project's main self-test entry point is `bounce_vertical_slice --check`,
 *   which lives in vertical_slice.c. This runner keeps the same shape as
 *   rtpl_decoder_check.c: a thin `main()` that calls the module-owned verify
 *   function. It introduces no new test framework -- every assertion lives in
 *   audio_event.c.
 *
 * IT LINKS ONLY ITS OWN MODULE AND LIBC. No X11, no libpng, no audio library,
 * no RTPL decoder. It cannot open a device, produce PCM, or play anything: the
 * module under test has no playback code at all.
 *
 * Usage:  audio_event_check
 * Exit status: 0 when every assertion passed, 1 otherwise.
 */

#include "audio_event.h"

#include <stdio.h>

int main(void)
{
    (void)printf("audio event identity self-test (standalone)\n");
    (void)printf(
        "no playback, no PCM, no backend, no RTPL decode,"
        " no queue\n\n");

    if (bounce_audio_event_verify(stdout) != 0) {
        (void)fprintf(stderr, "audio_event_check: FAILED\n");
        return 1;
    }
    (void)printf("audio_event_check: all assertions passed\n");
    return 0;
}
