/*
 * Standalone runner for the RTPL decoder self-test.
 *
 * WHY THIS FILE EXISTS
 *   The project's main self-test entry point is `bounce_vertical_slice --check`,
 *   which lives in vertical_slice.c. That file currently carries uncommitted
 *   STEP 13G-X user work, so this milestone deliberately does not touch it in
 *   order to keep that work byte-for-byte intact. This runner is therefore a
 *   thin `main()` that calls the SAME module-owned verify function the
 *   production build uses. It introduces no new test framework: the
 *   assertions, the expected values and the negative tests all live in
 *   rtpl_decoder.c.
 *
 * WHAT IT DOES NOT DO
 *   It opens no audio device, generates no PCM, and plays no sound. It reads
 *   three small files and decodes them.
 *
 * Usage:  rtpl_decoder_check [resource-root]
 * Default resource root: src/main/resources
 *
 * Exit status: 0 when every assertion passed, 1 otherwise.
 */

#include "rtpl_decoder.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
    const char *resource_root = "src/main/resources";

    if (argc > 2) {
        (void)fprintf(stderr, "usage: %s [resource-root]\n", argv[0]);
        return 1;
    }
    if (argc == 2) {
        resource_root = argv[1];
    }
    /* Run from the repository root; the caller in the Makefile does that. */
    if (strstr(resource_root, "src/main/resources") == NULL
        && strstr(resource_root, "/") == NULL) {
        (void)fprintf(stderr,
            "rtpl_decoder_check: resource root '%s' does not look like a path\n",
            resource_root);
        return 1;
    }

    (void)printf("RTPL decoder self-test\n");
    (void)printf("resource root: %s\n", resource_root);
    (void)printf("decoded events only: no audio backend, no PCM, no waveform\n\n");

    if (bounce_rtpl_verify_original_assets(resource_root, stdout) != 0) {
        (void)fprintf(stderr, "rtpl_decoder_check: FAILED\n");
        return 1;
    }
    (void)printf("rtpl_decoder_check: all assertions passed\n");
    return 0;
}
