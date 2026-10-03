/*
 * Standalone runner for the STEP 14B-34-A Layer C skeleton self-test.
 *
 * WHY THIS FILE EXISTS
 *   The project's main self-test entry point is `bounce_vertical_slice --check`,
 *   which lives in vertical_slice.c. This runner keeps the same shape as
 *   rtpl_decoder_check.c and audio_event_check.c: a thin `main()` that calls the
 *   module-owned verify function. It introduces no new test framework -- every
 *   assertion lives in audio_synth.c.
 *
 * IT LINKS ONLY ITS OWN MODULE, THE LAYER A DECODER, AND LIBC. No X11, no
 * libpng, no audio library, and no Layer B object file. It therefore cannot
 * open a device, produce PCM, or play anything: the module under test has no
 * playback code at all in STEP 14B-34-A. Note that audio_synth.h includes
 * audio_event.h, but only for the identity enum, whose values are compile-time
 * constants -- so no Layer B symbol is referenced and none is linked.
 *
 * It needs the original resources, exactly as check-rtpl does, so it is run
 * from the repository root with the resource directory as its argument.
 *
 * Usage:  audio_synth_check <resource-root>
 *         for example: audio_synth_check src/main/resources
 * Exit status: 0 when every assertion passed, 1 otherwise.
 */

#include "audio_synth.h"

#include <stdio.h>

int main(int argc, char **argv)
{
    const char *resource_root = NULL;

    if (argc > 1) {
        resource_root = argv[1];
    }

    (void)printf("Layer C skeleton self-test (STEP 14B-34-A, Target B)\n");
    (void)printf("no device, no backend, no PCM, no thread, no queue\n\n");

    if (resource_root == NULL) {
        (void)fprintf(stderr,
                      "usage: %s <resource-root>   "
                      "(for example: %s src/main/resources)\n",
                      (argc > 0) ? argv[0] : "audio_synth_check",
                      (argc > 0) ? argv[0] : "audio_synth_check");
        return 1;
    }

    if (bounce_synth_verify(resource_root, stdout) != 0) {
        (void)fprintf(stderr, "audio_synth_check: FAILED\n");
        return 1;
    }
    (void)printf("\n");
    if (bounce_synth_verify_pcm(resource_root, stdout) != 0) {
        (void)fprintf(stderr, "audio_synth_check: FAILED\n");
        return 1;
    }
    (void)printf("audio_synth_check: all assertions passed\n");
    return 0;
}
