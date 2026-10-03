#include "dyn_thorns.h"

#include <stdlib.h>
#include <string.h>

enum {
    BOUNCE_DYN_THORN_RECORD_BYTES = 8
};

/* Java short arithmetic promoted to int, then narrowed back by the caller. */
static int32_t java_int16_widen(int16_t value)
{
    return (int32_t)value;
}

static int32_t java_int32_add(int32_t left, int32_t right)
{
    uint32_t bits = (uint32_t)left + (uint32_t)right;
    int32_t result;

    (void)memcpy(&result, &bits, sizeof result);
    return result;
}

static int32_t java_int32_subtract(int32_t left, int32_t right)
{
    uint32_t bits = (uint32_t)left - (uint32_t)right;
    int32_t result;

    (void)memcpy(&result, &bits, sizeof result);
    return result;
}

static int32_t java_int32_multiply(int32_t left, int32_t right)
{
    uint32_t bits = (uint32_t)left * (uint32_t)right;
    int32_t result;

    (void)memcpy(&result, &bits, sizeof result);
    return result;
}

static int32_t java_int32_negate(int32_t value)
{
    uint32_t bits = 0u - (uint32_t)value;
    int32_t result;

    (void)memcpy(&result, &bits, sizeof result);
    return result;
}

/* (short) someInt: keep the low 16 bits, sign-extended. */
static int16_t java_narrow_short(int32_t value)
{
    uint16_t low = (uint16_t)((uint32_t)value & 0xffffu);

    if (low <= 0x7fffu)
        return (int16_t)low;
    return (int16_t)(int32_t)low - 0x10000;
}

int32_t bounce_dyn_thorns_test_point_inside(
    const BounceDynThornsSource *source,
    int32_t tile_x,
    int32_t tile_y
)
{
    uint32_t index;

    if (source == NULL || source->read_record == NULL)
        return -1;

    for (index = 0u; index < source->count; ++index) {
        BounceDynThornRecord record;

        if (!source->read_record(source->context, index, &record))
            return -1;

        /*
         * b.java:391-392. TopRight is exclusive and BottomLeft inclusive; the
         * shorts are promoted to int for the comparisons, as in Java.
         */
        if (tile_x >= java_int16_widen(record.bottom_left.x)
            && tile_x < java_int16_widen(record.top_right.x)
            && tile_y >= java_int16_widen(record.bottom_left.y)
            && tile_y < java_int16_widen(record.top_right.y))
            return (int32_t)index;
    }
    return -1;
}

int bounce_level_dyn_thorn_record(
    const BounceLevel *level,
    uint32_t index,
    BounceDynThornRecord *record_out
)
{
    const uint8_t *payload = NULL;
    size_t length = 0;
    size_t offset;
    const uint8_t *bytes;

    if (level == NULL || record_out == NULL)
        return -1;
    if (bounce_level_get_thorn_payload(level, &payload, &length) != 0)
        return -1;
    if (payload == NULL || index >= bounce_level_thorn_count(level))
        return -1;
    if ((size_t)index > length / (size_t)BOUNCE_DYN_THORN_RECORD_BYTES)
        return -1;

    offset = (size_t)index * (size_t)BOUNCE_DYN_THORN_RECORD_BYTES;
    if (length - offset < (size_t)BOUNCE_DYN_THORN_RECORD_BYTES)
        return -1;

    bytes = payload + offset;

    /*
     * Recovered 8-byte layout; see dyn_thorns.h. Each field is a Java
     * (short) levelDIS.read(), i.e. the signed byte value.
     */
    record_out->bottom_left.x = (int16_t)(int8_t)bytes[0];
    record_out->bottom_left.y = (int16_t)(int8_t)bytes[1];
    record_out->top_right.x = (int16_t)(int8_t)bytes[2];
    record_out->top_right.y = (int16_t)(int8_t)bytes[3];
    record_out->ae.x = (int16_t)(int8_t)bytes[4];
    record_out->ae.y = (int16_t)(int8_t)bytes[5];
    record_out->w.x = (int16_t)(int8_t)bytes[6];
    record_out->w.y = (int16_t)(int8_t)bytes[7];
    return 0;
}

static bool level_read_record(
    const void *context,
    uint32_t index,
    BounceDynThornRecord *record_out
)
{
    return bounce_level_dyn_thorn_record(
        (const BounceLevel *)context,
        index,
        record_out
    ) == 0;
}

int bounce_level_dyn_thorns_source(
    const BounceLevel *level,
    BounceDynThornsSource *source_out
)
{
    if (level == NULL || source_out == NULL)
        return -1;

    source_out->read_record = level_read_record;
    source_out->context = level;
    source_out->count = bounce_level_thorn_count(level);
    return 0;
}

int bounce_dyn_thorns_init_from_level(
    BounceDynThorns *thorns,
    const BounceLevel *level
)
{
    uint32_t count;
    uint32_t index;
    BounceDynThornRecord *records;

    if (thorns == NULL || level == NULL)
        return -1;

    /*
     * The caller supplies fresh storage, as elsewhere in this project. The
     * struct is zeroed rather than released, because releasing would free a
     * pointer the caller may never have initialized. A struct that already owns
     * records must be passed to bounce_dyn_thorns_reset() first.
     */
    memset(thorns, 0, sizeof *thorns);

    count = bounce_level_thorn_count(level);
    if (count == 0u)
        return 0;

    records = (BounceDynThornRecord *)calloc(count, sizeof *records);
    if (records == NULL)
        return -1;

    for (index = 0u; index < count; ++index) {
        if (bounce_level_dyn_thorn_record(level, index, &records[index]) != 0) {
            free(records);
            return -1;
        }
    }
    thorns->records = records;
    thorns->count = count;
    return 0;
}

void bounce_dyn_thorns_reset(BounceDynThorns *thorns)
{
    if (thorns == NULL)
        return;

    free(thorns->records);
    thorns->records = NULL;
    thorns->count = 0u;
}

static bool owned_read_record(
    const void *context,
    uint32_t index,
    BounceDynThornRecord *record_out
)
{
    const BounceDynThorns *thorns = (const BounceDynThorns *)context;

    if (thorns == NULL || record_out == NULL || index >= thorns->count)
        return false;
    *record_out = thorns->records[index];
    return true;
}

int bounce_dyn_thorns_source(
    const BounceDynThorns *thorns,
    BounceDynThornsSource *source_out
)
{
    if (source_out == NULL)
        return -1;

    source_out->read_record = owned_read_record;
    source_out->context = thorns;
    source_out->count = thorns == NULL ? 0u : thorns->count;
    if (source_out->count != 0u && thorns->records == NULL)
        return -1;
    return 0;
}

int bounce_dyn_thorns_update(BounceDynThorns *thorns)
{
    uint32_t index;

    if (thorns == NULL)
        return -1;

    for (index = 0u; index < thorns->count; ++index) {
        BounceDynThornRecord *record = &thorns->records[index];
        int32_t s1 = java_int16_widen(record->bottom_left.x);
        int32_t s2 = java_int16_widen(record->bottom_left.y);
        int32_t bound_x;
        int32_t bound_y;
        int32_t w_x = java_int16_widen(record->w.x);
        int32_t w_y = java_int16_widen(record->w.y);
        int32_t ae_x = java_int16_widen(record->ae.x);
        int32_t ae_y = java_int16_widen(record->ae.y);

        /* b.java:346-348: integrate x, then compute both bounds. */
        w_x = java_int32_add(w_x, ae_x);
        bound_x = java_int32_multiply(
            java_int32_subtract(
                java_int32_subtract(
                    java_int16_widen(record->top_right.x),
                    s1
                ),
                2
            ),
            12
        );
        bound_y = java_int32_multiply(
            java_int32_subtract(
                java_int32_subtract(
                    java_int16_widen(record->top_right.y),
                    s2
                ),
                2
            ),
            12
        );

        /* b.java:349-355 */
        if (w_x < 0)
            w_x = 0;
        else if (w_x > bound_x)
            w_x = bound_x;
        record->w.x = java_narrow_short(w_x);
        if (record->w.x == 0 || record->w.x == bound_x)
            record->ae.x = java_narrow_short(java_int32_negate(ae_x));

        /* b.java:356-363 */
        w_y = java_int32_add(w_y, ae_y);
        if (w_y < 0)
            w_y = 0;
        else if (w_y > bound_y)
            w_y = bound_y;
        record->w.y = java_narrow_short(w_y);
        if (record->w.y == 0 || record->w.y == bound_y)
            record->ae.y = java_narrow_short(java_int32_negate(ae_y));
    }
    return 0;
}

int bounce_dyn_thorns_restore_offsets(
    BounceDynThorns *thorns,
    uint32_t count,
    const BounceVec2S *w_values,
    const BounceVec2S *ae_values
)
{
    uint32_t index;

    if (thorns == NULL)
        return -1;
    if (count == 0u)
        return 0;
    if (w_values == NULL || ae_values == NULL)
        return -1;
    if (thorns->records == NULL)
        return -1;
    /*
     * e.java:483 bounds the loop with game.r and indexes ae[]/w[], which
     * LoadLevelDynThorns sized from the LEVEL's DynThornsCount (b.java:293-296).
     * A larger game.r therefore indexes past the live arrays and Java throws
     * ArrayIndexOutOfBoundsException. Refusing here is the native equivalent of
     * that failure: it is reported, not silently truncated to a partial state.
     */
    if (count > thorns->count)
        return -1;

    /*
     * e.java:484-487, in the source's order: ae takes game.l (the saved ae pair)
     * and w takes game.D (the saved w pair). The Record 3 field names are
     * w_x, w_y, ae_x, ae_y in that serialization order (BounceGame.java:402-405).
     *
     * Values are assigned as-is. There is deliberately no integration, no
     * clamping, and no negation: AddScore() does none of those, and the first
     * UpdateDynThorns() from the tick owns all three.
     */
    for (index = 0u; index < count; ++index) {
        thorns->records[index].ae.x = ae_values[index].x;
        thorns->records[index].ae.y = ae_values[index].y;
        thorns->records[index].w.x = w_values[index].x;
        thorns->records[index].w.y = w_values[index].y;
    }
    return 0;
}
