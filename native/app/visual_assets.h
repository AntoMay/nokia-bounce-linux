#ifndef BOUNCE_NATIVE_APP_VISUAL_ASSETS_H
#define BOUNCE_NATIVE_APP_VISUAL_ASSETS_H

#include <stdint.h>

#include "../renderer/renderer.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * App-local owner for the verified original PNG resources. The objects atlas
 * is decoded once and exposed only through the small rendering operations
 * below; no source-level map or asset representation is duplicated.
 */
typedef struct BounceVisualAssets BounceVisualAssets;

BounceVisualAssets *bounce_visual_assets_create(const char *resource_root);
void bounce_visual_assets_destroy(BounceVisualAssets *assets);

/* Deterministic decoder/atlas transparency and representative draw checks. */
int bounce_visual_assets_verify(const BounceVisualAssets *assets);

/* Render one LevelTiles value at a top-left logical destination. */
int bounce_visual_assets_draw_tile(
    BounceRenderer *renderer,
    const BounceVisualAssets *assets,
    uint16_t tile_value,
    int destination_x,
    int destination_y
);

/*
 * Original player draw, e.java:209-219. spriteCurrentBall is a pure
 * function of ballSize (f.java:169-171, f.java:210-212) and the destination
 * offset is the live p (e.java:217), so both are supplied by the caller and
 * no sprite mode is duplicated in native state. ball_size 16 selects the
 * 16x16 big-ball sprite; anything else selects the regular 12x12 sprite.
 *
 * STEP 14B-11 adds death_state, the caller's f.z (f.java:218). e.java:214-215
 * selects SpriteIDs.POPPED_BALL and a fixed -6 offset while z == 2, so death
 * is a rendering decision made by the caller, not a stored sprite mode.
 */
int bounce_visual_assets_draw_player_ball(
    BounceRenderer *renderer,
    const BounceVisualAssets *assets,
    int ball_size,
    int half_size,
    int death_state,
    int screen_x,
    int screen_y
);

/* Original regular-ball sprite through the existing logical renderer. */
int bounce_visual_assets_draw_regular_ball(
    BounceRenderer *renderer,
    const BounceVisualAssets *assets,
    int screen_x,
    int screen_y
);

/* Original HUD icon crops used by e.q(). */
int bounce_visual_assets_draw_hud_ball(
    BounceRenderer *renderer,
    const BounceVisualAssets *assets,
    int destination_x,
    int destination_y
);
int bounce_visual_assets_draw_hud_hoop(
    BounceRenderer *renderer,
    const BounceVisualAssets *assets,
    int destination_x,
    int destination_y
);

/*
 * The original in-game SCORE, e.java:183.
 *
 * `drawString(PadZeroes(this.score), 64, 100, Graphics.TOP | Graphics.LEFT)`, preceded by
 * `setColor(0xfffffe)` at `e.java:181`, drawn between the life row (`:177-178`) and the hoop
 * row (`:179-180`) inside the `0x0853aa` band filled at `:175-176`.
 *
 * STEP 12X-HUD-SCORE-IMPL-B. The value is formatted with the source's own `PadZeroes`
 * (`e.java:436-454`) semantics, which is NOT conventional seven-digit padding: it selects a
 * fixed zero prefix by magnitude and concatenates the signed decimal. The recovered outputs
 * are therefore 8 characters for 1-digit and 3-to-8-digit values, 9 for 2-digit values
 * (`10` -> `000000010`), 8 zeros for `0` -> `00000000`, and `prefix + "-N"` for negatives.
 * That ladder is reproduced verbatim; see the implementation for the full note.
 *
 * The digits are code-resident 5x7 shapes rather than atlas crops, so this seam takes NO
 * BounceVisualAssets: it is a shape, not a sprite, and the other two HUD helpers need the
 * assets only because they blit. Deliberately NOT routed through the UI Theme System -- the
 * source colour is a fixed literal, and the gameplay view is outside the palette by design.
 *
 * The destination is an explicit TOP|LEFT origin, matching the MIDP anchor, and is never
 * adjusted for the formatted length: the score is left-anchored, not centred.
 */
int bounce_visual_assets_draw_hud_score(
    BounceRenderer *renderer,
    int destination_x,
    int destination_y,
    int32_t score,
    uint32_t color
);

/* Original 128x128 splash image, drawn in the locked logical surface. */
int bounce_visual_assets_draw_splash(
    BounceRenderer *renderer,
    const BounceVisualAssets *assets
);

/*
 * STEP 12E — Native Exit Door Rendering.
 *
 * Draws the 24x48 exit door sprite at the specified screen position.
 * Uses the existing transparent blit mechanism and clipping architecture.
 * The sprite is drawn as a single image, not as individual tiles.
 */
int bounce_visual_assets_draw_exit_door(
    BounceRenderer *renderer,
    const BounceVisualAssets *assets,
    int screen_x,
    int screen_y
);

/*
 * STEP 12R — Exact Exit Sprite Window.
 *
 * Draws a 24x24 source region from the 24x48 exit door sprite at the
 * specified screen position. The source Y offset is the door animation
 * offset (door_image_offset), reproducing the Java aa window behavior.
 */
int bounce_visual_assets_draw_exit_door_window(
    BounceRenderer *renderer,
    const BounceVisualAssets *assets,
    int screen_x,
    int screen_y,
    int source_y
);

/*
 * STEP 12V — Dynamic Thorn Rendering.
 *
 * Draws the 24x24 Dynamic Thorn composite at the specified screen position.
 * Uses the existing transparent blit mechanism and clipping architecture.
 */
int bounce_visual_assets_draw_dyn_thorn(
    BounceRenderer *renderer,
    const BounceVisualAssets *assets,
    int screen_x,
    int screen_y
);

#ifdef __cplusplus
}
#endif

#endif /* BOUNCE_NATIVE_APP_VISUAL_ASSETS_H */
