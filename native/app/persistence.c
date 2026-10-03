/*
 * persistence.c -- STEP 37 PERSIST. See persistence.h for the contract.
 *
 * CONTAINER FORMAT (native, not RMS-compatible)
 *
 *   offset  size  field
 *   0       8     magic  "BNCPRST1"
 *   8       4     version (little-endian, 1)
 *   12      4     max_levels  (little-endian)
 *   16      4     high_score  (little-endian)
 *   ---- 20 bytes total
 *
 * The magic makes a text file, a truncated write, or an unrelated file in the
 * same directory a clean "not loaded" rather than a garbage value, which is the
 * behaviour Java gets for free from a typed container.
 *
 * WHY THE WRITE IS NOT ATOMIC BY TEMP-FILE-AND-RENAME
 *   The original has exactly the same exposure: WriteToStore() writes record 1
 *   and record 2 as two independent setRecord() calls (BounceGame.java:420,
 *   :425), so a crash between them leaves the store holding a new MaxLevels and
 *   an old HighScore. A single whole-file write is strictly better than that and
 *   is no less faithful to the observable contract.
 */

#include "persistence.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define BOUNCE_PERSISTENCE_MAGIC "BNCPRST1"
#define BOUNCE_PERSISTENCE_MAGIC_SIZE 8u
#define BOUNCE_PERSISTENCE_VERSION 1u
#define BOUNCE_PERSISTENCE_SIZE 20u

/*
 * G-R3-P1 -- the multi-record container, approved as decision P1.
 *
 * The magic is DELIBERATELY UNCHANGED. The authoritative version is the u32 at
 * offset 8, so version 2 reuses the version mechanism that already exists
 * rather than inventing a second one, and a version-1 file still passes the
 * magic check that bounce_persistence_is_well_formed() already performs. The
 * "1" inside the magic string is a product tag, not the version; changing it
 * would reject every existing save for no gain.
 *
 * Version 1 (unchanged, still readable):
 *
 *   [0..7]    "BNCPRST1"
 *   [8..11]   u32 LE version = 1
 *   [12..15]  u32 LE MaxLevels
 *   [16..19]  u32 LE HighScore
 *
 * Version 2 adds explicit record identity and per-record length, in one file:
 *
 *   [0..7]    "BNCPRST1"
 *   [8..11]   u32 LE version = 2
 *   [12..15]  u32 LE record count N
 *   directory, N entries, ascending record id (the deterministic order):
 *       u32 LE record id
 *       u32 LE payload length
 *   payloads, in the same order as the directory
 *
 * A record that is not in the directory does not exist. That is how an absent
 * Record 3 is represented; there is no sentinel payload and no flag byte.
 *
 * TUGAS 2b adds record 4, the selected color profile, in the same directory and
 * by the same rule. It is a NATIVE record -- the color profiles are native-only
 * (N-03) and the recovered game has no Settings screen -- but the mechanism is
 * the source's: Java's whole persistence surface is one private store addressed
 * by integer record id (BounceGame.java:275, :281-283, :410-428), which is what
 * this directory transcribes. A version-1 or version-2 file written before this
 * change has no record 4 and therefore means "the default profile", index 0,
 * with no upgrade step and no migration: absence is already representable.
 */
#define BOUNCE_PERSISTENCE_VERSION2 2u
#define BOUNCE_PERSISTENCE_V2_HEADER_SIZE 16u
#define BOUNCE_PERSISTENCE_V2_DIR_ENTRY_SIZE 8u
/*
 * TUGAS 2b -- raised from 3 to 4. The ceiling exists so a malformed directory
 * cannot make the reader walk past the buffer; record 4 is the color profile,
 * described at persistence.h. Three is still the number of JAVA records, and
 * the two counts are unrelated: this ceiling bounds the container, not the
 * source.
 */
/*
 * NATIVE ADDITION -- 5, was 4. Record 5 is the numeric-keypad setting. The
 * container's format is a directory of (id, length) pairs and the reader keys on
 * the ids it wants, so the ceiling is the only thing that has to move; nothing
 * about the framing, the ordering or the reader changes.
 */
#define BOUNCE_PERSISTENCE_V2_MAX_RECORDS 5u

/* Largest container this module will write or accept. */
#define BOUNCE_PERSISTENCE_MAX_FILE_SIZE (1u << 20)

/* Java's Record 3 trailer, BounceGame.java:11. */
#define BOUNCE_PERSISTENCE_RECORD3_MAGIC (-559038737LL)

#define BOUNCE_PERSISTENCE_DIR "bounce"
#define BOUNCE_PERSISTENCE_FILE "records.bin"

/* Longest path this module will build: <root>/bounce/records.bin. */
#define BOUNCE_PERSISTENCE_PATH_MAX 1024u

static void put_u32(unsigned char *bytes, uint32_t value)
{
    bytes[0] = (unsigned char)(value & 0xFFu);
    bytes[1] = (unsigned char)((value >> 8) & 0xFFu);
    bytes[2] = (unsigned char)((value >> 16) & 0xFFu);
    bytes[3] = (unsigned char)((value >> 24) & 0xFFu);
}

static uint32_t get_u32(const unsigned char *bytes)
{
    return (uint32_t)bytes[0]
        | ((uint32_t)bytes[1] << 8)
        | ((uint32_t)bytes[2] << 16)
        | ((uint32_t)bytes[3] << 24);
}

/*
 * G-R3-P1 -- Java DataOutputStream primitives, for the Record 3 payload ONLY.
 *
 * Java's DataOutputStream writes every multi-byte primitive big-endian
 * (JSR 37 DataOutput.writeShort/writeInt/writeLong), and the whole Record 3
 * payload is produced that way (BounceGame.java:360-407). The native container
 * metadata above is little-endian because it is native-only. The two encodings
 * are deliberately different: converting the payload to little-endian "for
 * consistency" would make it stop being Java-compatible, which is the one
 * property this payload is required to keep.
 */
static void put_be16(unsigned char *bytes, int16_t value)
{
    uint16_t raw = (uint16_t)value;
    bytes[0] = (unsigned char)((raw >> 8) & 0xFFu);
    bytes[1] = (unsigned char)(raw & 0xFFu);
}

static int16_t get_be16(const unsigned char *bytes)
{
    return (int16_t)(((uint16_t)bytes[0] << 8) | (uint16_t)bytes[1]);
}

static void put_be32(unsigned char *bytes, int32_t value)
{
    uint32_t raw = (uint32_t)value;
    bytes[0] = (unsigned char)((raw >> 24) & 0xFFu);
    bytes[1] = (unsigned char)((raw >> 16) & 0xFFu);
    bytes[2] = (unsigned char)((raw >> 8) & 0xFFu);
    bytes[3] = (unsigned char)(raw & 0xFFu);
}

static int32_t get_be32(const unsigned char *bytes)
{
    return (int32_t)(((uint32_t)bytes[0] << 24) | ((uint32_t)bytes[1] << 16)
        | ((uint32_t)bytes[2] << 8) | (uint32_t)bytes[3]);
}

static void put_be64(unsigned char *bytes, int64_t value)
{
    uint64_t raw = (uint64_t)value;
    int i;
    for (i = 0; i < 8; ++i)
        bytes[i] = (unsigned char)((raw >> (56 - 8 * i)) & 0xFFu);
}

static int64_t get_be64(const unsigned char *bytes)
{
    uint64_t raw = 0u;
    int i;
    for (i = 0; i < 8; ++i)
        raw = (raw << 8) | (uint64_t)bytes[i];
    return (int64_t)raw;
}

uint32_t bounce_persistence_record3_payload_size(uint32_t tile_delta_count,
    uint32_t dyn_thorn_count)
{
    /*
     * Both counts are bounds-checked BEFORE any arithmetic, so the largest value
     * that can reach the addition is 5*50 + 8*127, which cannot overflow a
     * uint32_t. A malformed count is rejected rather than clamped.
     */
    if (tile_delta_count > BOUNCE_PERSISTENCE_RECORD3_MAX_TILE_DELTAS
        || dyn_thorn_count > BOUNCE_PERSISTENCE_RECORD3_MAX_DYN_THORNS)
        return 0u;
    /* 69 fixed prefix + 1 count byte + 1 count byte + 8 magic = 79. */
    return BOUNCE_PERSISTENCE_RECORD3_PREFIX_SIZE + 1u + 1u + 8u
        + 5u * tile_delta_count + 8u * dyn_thorn_count;
}

static void record3_zero(BouncePersistenceRecord3 *out)
{
    memset(out, 0, sizeof *out);
}

/*
 * Serialise Record 3 into payload, which must have room for
 * bounce_persistence_record3_payload_size(counts) bytes. Returns the byte count
 * written, or 0 on any rejection -- including a count Java could not produce.
 */
static uint32_t record3_encode(const BouncePersistenceRecord3 *in,
    unsigned char *payload, uint32_t capacity)
{
    uint32_t needed;
    uint32_t offset;
    uint32_t i;

    if (in == NULL || payload == NULL)
        return 0u;
    if (in->tile_delta_count > BOUNCE_PERSISTENCE_RECORD3_MAX_TILE_DELTAS
        || in->dyn_thorn_count > BOUNCE_PERSISTENCE_RECORD3_MAX_DYN_THORNS)
        return 0u;
    needed = bounce_persistence_record3_payload_size(in->tile_delta_count,
        in->dyn_thorn_count);
    if (needed == 0u || capacity < needed)
        return 0u;

    offset = 0u;
    put_be64(payload + offset, in->timestamp_millis);
    offset += 8u;
    payload[offset++] = in->b1;
    payload[offset++] = in->lives;
    payload[offset++] = in->hoops_scored;
    payload[offset++] = in->level;
    payload[offset++] = in->ball_size;
    put_be32(payload + offset, in->score);
    offset += 4u;
    put_be32(payload + offset, in->l);
    offset += 4u;
    put_be32(payload + offset, in->k);
    offset += 4u;
    put_be32(payload + offset, in->unk_x);
    offset += 4u;
    put_be32(payload + offset, in->unk_y);
    offset += 4u;
    put_be32(payload + offset, in->aq_l);
    offset += 4u;
    put_be32(payload + offset, in->aq_o);
    offset += 4u;
    put_be32(payload + offset, in->zero_slot_0);
    offset += 4u;
    put_be32(payload + offset, in->zero_slot_1);
    offset += 4u;
    put_be32(payload + offset, in->tile_x);
    offset += 4u;
    put_be32(payload + offset, in->tile_y);
    offset += 4u;
    put_be32(payload + offset, in->power_up_1);
    offset += 4u;
    put_be32(payload + offset, in->power_up_gravity);
    offset += 4u;
    put_be32(payload + offset, in->power_up_3);
    offset += 4u;

    payload[offset++] = (unsigned char)in->tile_delta_count;
    for (i = 0u; i < in->tile_delta_count; ++i) {
        put_be16(payload + offset, in->tile_deltas[i].row);
        offset += 2u;
        put_be16(payload + offset, in->tile_deltas[i].col);
        offset += 2u;
        payload[offset++] = in->tile_deltas[i].tile_id;
    }

    payload[offset++] = (unsigned char)in->dyn_thorn_count;
    for (i = 0u; i < in->dyn_thorn_count; ++i) {
        put_be16(payload + offset, in->dyn_thorns[i].w_x);
        offset += 2u;
        put_be16(payload + offset, in->dyn_thorns[i].w_y);
        offset += 2u;
        put_be16(payload + offset, in->dyn_thorns[i].ae_x);
        offset += 2u;
        put_be16(payload + offset, in->dyn_thorns[i].ae_y);
        offset += 2u;
    }

    put_be64(payload + offset, BOUNCE_PERSISTENCE_RECORD3_MAGIC);
    offset += 8u;
    return offset;
}

/*
 * Parse Record 3. Returns 1 on success, 0 for any rejection: a truncated
 * payload, a count Java could not produce, a length that is not exactly
 * 79 + 5*b2 + 8*n, a trailing or missing byte, or a wrong Java magic. Nothing
 * is allocated from an unchecked count, and nothing is clamped.
 */
static int record3_decode(const unsigned char *payload, uint32_t size,
    BouncePersistenceRecord3 *out)
{
    uint32_t offset = 0u;
    uint32_t tile_count;
    uint32_t thorn_count;
    uint32_t expected;
    uint32_t i;

    record3_zero(out);

    if (size < BOUNCE_PERSISTENCE_RECORD3_PREFIX_SIZE + 2u + 8u)
        return 0;

    out->timestamp_millis = get_be64(payload + offset);
    offset += 8u;
    out->b1 = payload[offset++];
    out->lives = payload[offset++];
    out->hoops_scored = payload[offset++];
    out->level = payload[offset++];
    out->ball_size = payload[offset++];
    out->score = get_be32(payload + offset);
    offset += 4u;
    out->l = get_be32(payload + offset);
    offset += 4u;
    out->k = get_be32(payload + offset);
    offset += 4u;
    out->unk_x = get_be32(payload + offset);
    offset += 4u;
    out->unk_y = get_be32(payload + offset);
    offset += 4u;
    out->aq_l = get_be32(payload + offset);
    offset += 4u;
    out->aq_o = get_be32(payload + offset);
    offset += 4u;
    out->zero_slot_0 = get_be32(payload + offset);
    offset += 4u;
    out->zero_slot_1 = get_be32(payload + offset);
    offset += 4u;
    out->tile_x = get_be32(payload + offset);
    offset += 4u;
    out->tile_y = get_be32(payload + offset);
    offset += 4u;
    out->power_up_1 = get_be32(payload + offset);
    offset += 4u;
    out->power_up_gravity = get_be32(payload + offset);
    offset += 4u;
    out->power_up_3 = get_be32(payload + offset);
    offset += 4u;

    tile_count = payload[offset++];
    if (tile_count > BOUNCE_PERSISTENCE_RECORD3_MAX_TILE_DELTAS)
        return 0; /* Java's write path cannot produce this. */
    for (i = 0u; i < tile_count; ++i) {
        out->tile_deltas[i].row = get_be16(payload + offset);
        offset += 2u;
        out->tile_deltas[i].col = get_be16(payload + offset);
        offset += 2u;
        out->tile_deltas[i].tile_id = payload[offset++];
    }
    out->tile_delta_count = tile_count;

    thorn_count = payload[offset++];
    if (thorn_count > BOUNCE_PERSISTENCE_RECORD3_MAX_DYN_THORNS)
        return 0;
    for (i = 0u; i < thorn_count; ++i) {
        out->dyn_thorns[i].w_x = get_be16(payload + offset);
        offset += 2u;
        out->dyn_thorns[i].w_y = get_be16(payload + offset);
        offset += 2u;
        out->dyn_thorns[i].ae_x = get_be16(payload + offset);
        offset += 2u;
        out->dyn_thorns[i].ae_y = get_be16(payload + offset);
        offset += 2u;
    }
    out->dyn_thorn_count = thorn_count;

    /* Exact length: no trailing byte and no missing byte is tolerated. */
    expected = bounce_persistence_record3_payload_size(tile_count, thorn_count);
    if (expected == 0u || size != expected || offset + 8u != size)
        return 0;
    if (get_be64(payload + offset) != BOUNCE_PERSISTENCE_RECORD3_MAGIC)
        return 0;
    return 1;
}

/*
 * STEP SAVE-DIR -- Resolve the store, in this order:
 *
 *     $BOUNCE_SAVE_DIR/bounce/records.bin      an explicit override, if set
 *     $XDG_DATA_HOME/bounce/records.bin        the XDG standard
 *     $HOME/.local/share/bounce/records.bin    the XDG default
 *
 * WHY THE OVERRIDE IS A ROOT AND NOT A FILE PATH. It occupies exactly the position
 * XDG_DATA_HOME already has, so the layout below it is unchanged and every existing
 * test that repoints the store keeps working by setting either variable. Someone
 * wanting the save beside a checkout writes BOUNCE_SAVE_DIR=/path/to/checkout and
 * gets /path/to/checkout/bounce/records.bin -- the same shape XDG_DATA_HOME produces,
 * which is one less thing to remember and one less way to be wrong.
 *
 * It is NOT a file name, deliberately. Allowing that would mean two variables with
 * different shapes, and a caller who set the wrong one would silently get a file
 * somewhere they did not look.
 *
 * main()'s --save=DIR flag sets this variable before anything reads the store, so the
 * two are the same switch by two routes: a flag for a person at a terminal, an
 * environment variable for a launcher, a desktop file or an AppRun. --save wins over a
 * set BOUNCE_SAVE_DIR because main() overwrites it.
 *
 * WHY IT IS NOT THE DEFAULT. XDG_DATA_HOME is where a desktop user expects their data,
 * and a save that wanders off it is a save that gets wiped by a package uninstall or
 * never found after a move. The override exists for the cases the standard does not
 * cover -- a portable checkout, a throwaway instance for testing a build, a shared
 * install -- not to replace the rule.
 *
 * Returns 0 and fills out, or -1 when no variable is usable. out must have room for
 * BOUNCE_PERSISTENCE_PATH_MAX bytes.
 */
static int resolve_path(char *out, size_t out_size)
{
    const char *root;
    size_t written;

    if (out == NULL || out_size == 0u)
        return -1;

    root = getenv("BOUNCE_SAVE_DIR");
    if (root == NULL || root[0] == '\0')
        root = getenv("XDG_DATA_HOME");
    if (root != NULL && root[0] != '\0')
        written = (size_t)snprintf(
            out, out_size, "%s/%s/%s", root, BOUNCE_PERSISTENCE_DIR,
            BOUNCE_PERSISTENCE_FILE
        );
    else {
        root = getenv("HOME");
        if (root == NULL || root[0] == '\0')
            return -1;
        written = (size_t)snprintf(
            out, out_size, "%s/.local/share/%s/%s", root,
            BOUNCE_PERSISTENCE_DIR, BOUNCE_PERSISTENCE_FILE
        );
    }

    /* snprintf returns the length it wanted; a short result means truncation. */
    if (written == 0u || written >= out_size) {
        out[0] = '\0';
        return -1;
    }
    return 0;
}

const char *bounce_persistence_path(void)
{
    static char cached[BOUNCE_PERSISTENCE_PATH_MAX];
    static int resolved;
    static int failed;

    if (!resolved) {
        resolved = 1;
        failed = (resolve_path(cached, sizeof cached) != 0);
    }
    if (failed)
        return NULL;
    return cached;
}

int bounce_persistence_is_well_formed(const unsigned char *bytes, uint32_t size)
{
    if (bytes == NULL || size != BOUNCE_PERSISTENCE_SIZE)
        return 0;
    if (memcmp(bytes, BOUNCE_PERSISTENCE_MAGIC, BOUNCE_PERSISTENCE_MAGIC_SIZE)
        != 0)
        return 0;
    if (get_u32(bytes + 8u) != BOUNCE_PERSISTENCE_VERSION
        && get_u32(bytes + 8u) != BOUNCE_PERSISTENCE_VERSION2)
        return 0;
    return 1;
}

int bounce_persistence_load(BouncePersistenceRecords *out)
{
    unsigned char bytes[BOUNCE_PERSISTENCE_MAX_FILE_SIZE];
    char path[BOUNCE_PERSISTENCE_PATH_MAX];
    FILE *file;
    size_t read_count;
    uint32_t version;

    if (out == NULL)
        return -1;

    /* Fresh-install defaults, applied on every failure path. */
    out->max_levels = 0;
    out->high_score = 0;

    if (resolve_path(path, sizeof path) != 0)
        return -1;

    file = fopen(path, "rb");
    if (file == NULL)
        return -1; /* No store yet. Java's fresh-install branch, :276-279. */

    read_count = fread(bytes, 1u, sizeof bytes, file);
    (void)fclose(file);

    if (read_count < BOUNCE_PERSISTENCE_V2_HEADER_SIZE
        || memcmp(bytes, BOUNCE_PERSISTENCE_MAGIC, BOUNCE_PERSISTENCE_MAGIC_SIZE)
            != 0)
        return -1; /* Short or wrong magic: treat as absent. */

    version = get_u32(bytes + 8u);

    if (version == BOUNCE_PERSISTENCE_VERSION) {
        /*
         * The legacy 20-byte layout, still read exactly as before. A version-1
         * file can never carry a Record 3, so one is correctly absent.
         */
        if (read_count != BOUNCE_PERSISTENCE_SIZE)
            return -1;
        out->max_levels = (int32_t)get_u32(bytes + 12u);
        out->high_score = (int32_t)get_u32(bytes + 16u);
    } else if (version == BOUNCE_PERSISTENCE_VERSION2) {
        uint32_t count = get_u32(bytes + 12u);
        uint32_t dir_at = BOUNCE_PERSISTENCE_V2_HEADER_SIZE;
        uint32_t payload_at;
        uint32_t i;
        bool found_levels = false;
        bool found_score = false;

        if (count > BOUNCE_PERSISTENCE_V2_MAX_RECORDS)
            return -1;
        payload_at = dir_at
            + count * BOUNCE_PERSISTENCE_V2_DIR_ENTRY_SIZE;
        if (read_count < payload_at)
            return -1;
        for (i = 0u; i < count; ++i) {
            uint32_t id = get_u32(bytes + dir_at + i * 8u);
            uint32_t len = get_u32(bytes + dir_at + i * 8u + 4u);
            uint32_t at = payload_at;
            uint32_t j;
            for (j = 0u; j < i; ++j)
                at += get_u32(bytes + dir_at + j * 8u + 4u);
            if (len > read_count || (uint32_t)read_count - at < len)
                return -1; /* A record that does not fit is a bad container. */
            if (id == BOUNCE_PERSISTENCE_RECORD_MAX_LEVELS && len == 4u) {
                out->max_levels = (int32_t)get_u32(bytes + at);
                found_levels = true;
            } else if (id == BOUNCE_PERSISTENCE_RECORD_HIGH_SCORE && len == 4u) {
                out->high_score = (int32_t)get_u32(bytes + at);
                found_score = true;
            }
        }
        if (!found_levels || !found_score)
            return -1;
    } else {
        return -1; /* Forward rejection: an unknown version is never guessed at. */
    }

    /* A negative value cannot come from the Java types being mirrored. */
    if (out->max_levels < 0 || out->high_score < 0) {
        out->max_levels = 0;
        out->high_score = 0;
        return -1;
    }
    return 0;
}

/*
 * Read the whole container, reporting its version and contents. Shared by the
 * Record 3 reader and the save paths so that a version-1 file is understood by
 * all of them in one place.
 */
static int read_container(unsigned char *bytes, uint32_t capacity,
    uint32_t *size_out, uint32_t *version_out)
{
    char path[BOUNCE_PERSISTENCE_PATH_MAX];
    FILE *file;
    size_t read_count;

    if (bytes == NULL || size_out == NULL || version_out == NULL)
        return -1;
    if (resolve_path(path, sizeof path) != 0)
        return -1;
    file = fopen(path, "rb");
    if (file == NULL)
        return -1;
    read_count = fread(bytes, 1u, capacity, file);
    (void)fclose(file);
    if (read_count < BOUNCE_PERSISTENCE_V2_HEADER_SIZE
        || memcmp(bytes, BOUNCE_PERSISTENCE_MAGIC, BOUNCE_PERSISTENCE_MAGIC_SIZE)
            != 0)
        return -1;
    *size_out = (uint32_t)read_count;
    *version_out = get_u32(bytes + 8u);
    return 0;
}

/*
 * Locate a record payload in a version-2 container. Returns 1 when found.
 *
 * TUGAS 2b -- `wanted_id` replaces the record-3 comparison, because the theme
 * record needs the identical walk and duplicating it would give the two records
 * two chances to disagree about where a payload starts. Both callers are the
 * same shape as before: absence is a 0, not an error.
 */
static int container_find_record(const unsigned char *bytes, uint32_t size,
    uint32_t wanted_id, const unsigned char **payload, uint32_t *payload_size)
{
    uint32_t count;
    uint32_t dir_at = BOUNCE_PERSISTENCE_V2_HEADER_SIZE;
    uint32_t payload_at;
    uint32_t i;

    if (size < BOUNCE_PERSISTENCE_V2_HEADER_SIZE)
        return 0;
    count = get_u32(bytes + 12u);
    if (count > BOUNCE_PERSISTENCE_V2_MAX_RECORDS)
        return 0;
    payload_at = dir_at + count * BOUNCE_PERSISTENCE_V2_DIR_ENTRY_SIZE;
    if (size < payload_at)
        return 0;
    for (i = 0u; i < count; ++i) {
        uint32_t id = get_u32(bytes + dir_at + i * 8u);
        uint32_t len = get_u32(bytes + dir_at + i * 8u + 4u);
        uint32_t at = payload_at;
        uint32_t j;
        for (j = 0u; j < i; ++j)
            at += get_u32(bytes + dir_at + j * 8u + 4u);
        if (len > size || size - at < len)
            return 0;
        if (id == wanted_id) {
            *payload = bytes + at;
            *payload_size = len;
            return 1;
        }
    }
    return 0;
}

int bounce_persistence_load_record3(BouncePersistenceRecord3 *out, bool *present)
{
    static unsigned char bytes[BOUNCE_PERSISTENCE_MAX_FILE_SIZE];
    const unsigned char *payload = NULL;
    uint32_t size = 0u;
    uint32_t version = 0u;

    if (out == NULL || present == NULL)
        return -1;

    /*
     * Absence is the normal answer, so *present is cleared before anything can
     * fail. A missing store, a legacy version-1 file, an unknown version, a
     * missing record, and a malformed record are all "not present" rather than
     * an error the caller has to distinguish.
     */
    *present = false;
    record3_zero(out);

    if (read_container(bytes, (uint32_t)sizeof bytes, &size, &version) != 0)
        return 0;
    if (version == BOUNCE_PERSISTENCE_VERSION)
        return 0; /* A version-1 file has no Record 3, correctly absent. */
    if (version != BOUNCE_PERSISTENCE_VERSION2)
        return 0; /* Forward rejection. */
    if (!container_find_record(
            bytes,
            size,
            BOUNCE_PERSISTENCE_RECORD_SNAPSHOT,
            &payload,
            &size))
        return 0;

    *present = record3_decode(payload, size, out) == 1;
    if (!*present)
        record3_zero(out);
    return 0;
}

/*
 * Ensure the parent directory exists. Shared by both save paths.
 *
 * EVERY LEVEL, NOT THE LAST TWO. mkdir() does not create its parents, so making just
 * the final directory and the one above it only ever worked for the two shapes the
 * default location has: $HOME/.local/share (with $HOME present) and a root given as
 * $XDG_DATA_HOME (with its parent present). It broke the moment a caller named a path
 * whose whole chain was new -- --save=DIR pointing at a directory that does not exist
 * yet wrote nothing at all and reported only "could not write the save probe to ...",
 * which names the file and not the reason.
 *
 * WHY IT STILL DOES NOT REPORT MKDIR FAILURES. The contract is unchanged from the
 * version this replaces: a directory that cannot be created is not an error here, it is
 * an error at open() time, and the caller already treats a failed write as "keep the
 * documented defaults" (see FAILURE POLICY in persistence.h). Turning this into a real
 * return code would change which errors are fatal and which are survivable, and none of
 * that is what this change is about.
 *
 * The walk starts one byte in so the leading '/' is never passed to mkdir as an empty
 * string. It stops at the end of the buffer, so a truncated copy cannot walk off it.
 */
static int ensure_directory(const char *path)
{
    char dir[BOUNCE_PERSISTENCE_PATH_MAX];
    char *cursor;

    if (path == NULL
        || snprintf(dir, sizeof dir, "%s", path) >= (int)sizeof dir)
        return -1;
    cursor = strrchr(dir, '/');
    if (cursor == NULL)
        return -1;
    *cursor = '\0';
    for (cursor = dir + 1; *cursor != '\0'; cursor++) {
        if (*cursor != '/')
            continue;
        *cursor = '\0';
        (void)mkdir(dir, 0777);
        *cursor = '/';
    }
    (void)mkdir(dir, 0777);
    return 0;
}

/*
 * Write a version-2 container holding the supplied records in ascending record
 * id. Any record passed as absent is simply left out of the directory, which is
 * how an absent Record 3 -- and, since TUGAS 2b, an absent color profile -- is
 * represented.
 *
 * TUGAS 2b -- `theme` / `have_theme` are the color-profile record, written LAST
 * because id 4 is the highest and the directory is kept in ascending id order.
 *
 * THE PAYLOAD PACKING IS REBUILT rather than extended. The previous version
 * computed each body pointer as `payload + (have_record3 ? record3_len : 0) + k`,
 * which only works for a fixed set of records at fixed offsets; with one more
 * optional record the arithmetic had four interacting cases and a missing
 * bounds check on the small payload at the end. It is now a single running
 * offset with one bounds check per append, which is the property the format
 * actually needs: nothing may be written past the buffer.
 */
static int write_container(const BouncePersistenceRecords *records12,
    const BouncePersistenceRecord3 *record3, bool have_record3,
    int32_t theme, bool have_theme,
    int32_t t9, bool have_t9)
{
    static unsigned char payload[BOUNCE_PERSISTENCE_MAX_FILE_SIZE];
    static unsigned char container[BOUNCE_PERSISTENCE_MAX_FILE_SIZE];
    uint32_t ids[BOUNCE_PERSISTENCE_V2_MAX_RECORDS];
    uint32_t offsets[BOUNCE_PERSISTENCE_V2_MAX_RECORDS];
    uint32_t lengths[BOUNCE_PERSISTENCE_V2_MAX_RECORDS];
    uint32_t count = 0u;
    uint32_t used = 0u;
    uint32_t dir_at;
    uint32_t payload_at;
    uint32_t total;
    uint32_t i;
    char path[BOUNCE_PERSISTENCE_PATH_MAX];
    FILE *file;
    size_t written;

    if (records12 == NULL)
        return -1;

    /*
     * STAGING ORDER IS FREE; DIRECTORY ORDER IS NOT. The format specifies a
     * directory in ascending record id, so the staging buffer below is filled in
     * whatever order is convenient and the DIRECTORY is sorted before it is
     * written. Record 3 is staged first so its big-endian encoding lands at
     * payload offset 0, which is where the self-test reads it from -- exactly
     * where the previous layout put it.
     */
    if (have_record3) {
        uint32_t len = record3_encode(
            record3, payload + used, (uint32_t)sizeof payload - used);
        if (len == 0u)
            return -1;
        ids[count] = BOUNCE_PERSISTENCE_RECORD_SNAPSHOT;
        offsets[count] = used;
        lengths[count] = len;
        ++count;
        used += len;
    }
    if (used + 4u > (uint32_t)sizeof payload)
        return -1;
    put_u32(payload + used, (uint32_t)records12->max_levels);
    ids[count] = BOUNCE_PERSISTENCE_RECORD_MAX_LEVELS;
    offsets[count] = used;
    lengths[count] = 4u;
    ++count;
    used += 4u;

    if (used + 4u > (uint32_t)sizeof payload)
        return -1;
    put_u32(payload + used, (uint32_t)records12->high_score);
    ids[count] = BOUNCE_PERSISTENCE_RECORD_HIGH_SCORE;
    offsets[count] = used;
    lengths[count] = 4u;
    ++count;
    used += 4u;

    if (have_theme) {
        /*
         * TUGAS 2b. A negative index is refused here rather than written, for the
         * same reason bounce_persistence_load() refuses a negative MaxLevels: a
         * malformed or stale value must never reach the file as a valid uint32
         * that a later reader would have to range-check. persistence.c cannot
         * know BOUNCE_UI_COLOR_PROFILE_COUNT, so the upper bound is the
         * caller's responsibility and is documented at persistence.h.
         */
        if (theme < 0)
            return -1;
        if (used + 4u > (uint32_t)sizeof payload)
            return -1;
        put_u32(payload + used, (uint32_t)theme);
        ids[count] = BOUNCE_PERSISTENCE_RECORD_THEME;
        offsets[count] = used;
        lengths[count] = 4u;
        ++count;
        used += 4u;
    }

    if (have_t9) {
        /*
         * NATIVE ADDITION -- record 5, the numeric-keypad setting. One u32, the
         * same shape as records 1, 2 and 4, because persistence.c cannot know how
         * many native settings exist and only has to refuse an impossible value.
         */
        if (t9 < 0)
            return -1;
        if (used + 4u > (uint32_t)sizeof payload)
            return -1;
        put_u32(payload + used, (uint32_t)t9);
        ids[count] = BOUNCE_PERSISTENCE_RECORD_T9_INPUT;
        offsets[count] = used;
        lengths[count] = 4u;
        ++count;
        used += 4u;
    }

    /*
     * Insertion sort the directory by ascending record id. At most five entries,
     * so this is not a performance question; it is a guarantee that the on-disk
     * order matches the format's own wording no matter what order the records
     * above happened to be staged in. Duplicated ids cannot occur -- each record
     * is staged once -- and are not specially handled, because the reader keys
     * off the first match and a duplicate would mean a bug above rather than an
     * input.
     */
    for (i = 1u; i < count; ++i) {
        uint32_t key_id = ids[i];
        uint32_t key_offset = offsets[i];
        uint32_t key_length = lengths[i];
        uint32_t j = i;
        while (j > 0u && ids[j - 1u] > key_id) {
            ids[j] = ids[j - 1u];
            offsets[j] = offsets[j - 1u];
            lengths[j] = lengths[j - 1u];
            --j;
        }
        ids[j] = key_id;
        offsets[j] = key_offset;
        lengths[j] = key_length;
    }

    dir_at = BOUNCE_PERSISTENCE_V2_HEADER_SIZE;
    payload_at = dir_at + count * BOUNCE_PERSISTENCE_V2_DIR_ENTRY_SIZE;
    total = payload_at;
    for (i = 0u; i < count; ++i)
        total += lengths[i];
    if (total > (uint32_t)sizeof container)
        return -1;

    memset(container, 0, total);
    memcpy(container, BOUNCE_PERSISTENCE_MAGIC, BOUNCE_PERSISTENCE_MAGIC_SIZE);
    put_u32(container + 8u, BOUNCE_PERSISTENCE_VERSION2);
    put_u32(container + 12u, count);

    {
        uint32_t at = payload_at;
        for (i = 0u; i < count; ++i) {
            put_u32(container + dir_at + i * 8u, ids[i]);
            put_u32(container + dir_at + i * 8u + 4u, lengths[i]);
            if (lengths[i] != 0u) {
                memcpy(container + at, payload + offsets[i], lengths[i]);
                at += lengths[i];
            }
        }
    }

    if (resolve_path(path, sizeof path) != 0)
        return -1;
    if (ensure_directory(path) != 0)
        return -1;
    file = fopen(path, "wb");
    if (file == NULL)
        return -1;
    written = fwrite(container, 1u, total, file);
    if (fclose(file) != 0)
        return -1;
    if (written != total)
        return -1;
    return 0;
}

/*
 * TUGAS 2b -- read the persisted color-profile index out of the container.
 *
 * Returns 0 whenever the answer is well-defined, which includes "there is no
 * such record": *out_index is then 0, the default profile. That is the same
 * contract bounce_persistence_load_record3() uses, and for the same reason: a
 * missing theme is not a missing store, so a caller must never have to tell the
 * two apart. A version-1 file cannot carry record 4 and correctly reports the
 * default, exactly as it correctly reports an absent Record 3.
 *
 * -1 is returned only for a NULL argument. A malformed container, an unknown
 * version and a wrong payload length all report the default, because none of
 * them is a reason to refuse a setting the game can perfectly well run without.
 */
int bounce_persistence_load_theme_index(int32_t *out_index)
{
    static unsigned char bytes[BOUNCE_PERSISTENCE_MAX_FILE_SIZE];
    const unsigned char *payload = NULL;
    uint32_t payload_size = 0u;
    uint32_t size = 0u;
    uint32_t version = 0u;

    if (out_index == NULL)
        return -1;

    /* The default, applied before anything can fail. */
    *out_index = 0;

    if (read_container(bytes, (uint32_t)sizeof bytes, &size, &version) != 0)
        return 0;
    if (version == BOUNCE_PERSISTENCE_VERSION)
        return 0; /* A version-1 file has no record 4, correctly absent. */
    if (version != BOUNCE_PERSISTENCE_VERSION2)
        return 0; /* Forward rejection. */
    if (!container_find_record(
            bytes,
            size,
            BOUNCE_PERSISTENCE_RECORD_THEME,
            &payload,
            &payload_size))
        return 0;
    /* Four bytes, like every other scalar record. */
    if (payload_size != 4u)
        return 0;

    /* A negative index cannot come from the value this module writes. */
    if (get_u32(payload) > (uint32_t)INT32_MAX)
        return 0;
    *out_index = (int32_t)get_u32(payload);
    return 0;
}

/*
 * NATIVE ADDITION -- read the numeric-keypad setting.
 *
 * Returns 0 whenever the answer is well-defined, which includes "there is no
 * such record": *out_enabled is then 0, the default, so a caller never has to
 * distinguish absent from failed. Same contract as the theme loader, for the same
 * reason -- a missing setting is not a missing store.
 */
int bounce_persistence_load_t9_enabled(int32_t *out_enabled)
{
    static unsigned char bytes[BOUNCE_PERSISTENCE_MAX_FILE_SIZE];
    const unsigned char *payload = NULL;
    uint32_t payload_size = 0u;
    uint32_t size = 0u;
    uint32_t version = 0u;

    if (out_enabled == NULL)
        return -1;

    /* The default, applied before anything can fail. OFF. */
    *out_enabled = 0;

    if (read_container(bytes, (uint32_t)sizeof bytes, &size, &version) != 0)
        return 0;
    if (version == BOUNCE_PERSISTENCE_VERSION)
        return 0; /* A version-1 file has no record 5, correctly absent. */
    if (version != BOUNCE_PERSISTENCE_VERSION2)
        return 0; /* Forward rejection. */
    if (!container_find_record(
            bytes,
            size,
            BOUNCE_PERSISTENCE_RECORD_T9_INPUT,
            &payload,
            &payload_size))
        return 0;
    if (payload_size != 4u)
        return 0;
    if (get_u32(payload) > (uint32_t)INT32_MAX)
        return 0;
    /* Anything non-zero is ON: persistence.c cannot know how many native
     * settings exist, so the caller narrows the value, not this reader. */
    *out_enabled = (get_u32(payload) != 0u) ? 1 : 0;
    return 0;
}

/*
 * NATIVE ADDITION -- write the numeric-keypad setting as record 5, preserving
 * records 1, 2, 3 and 4.
 *
 * RECORD 4 IS READ BACK AND REWRITTEN, and that is the whole point of doing this
 * as a read-modify-write rather than a partial update. Every writer in this module
 * owns exactly one record and must leave the others alone: a menu Exit snapshots
 * the session, a level completion updates the two scalars, a theme change
 * replaces the profile, and a keypad change replaces the flag. If any of them
 * rewrote the container from only the records it knows, every one of the other
 * three would silently reset. A partial update must never cost the player a
 * snapshot, a profile or a setting.
 *
 * A failure here is never fatal, exactly as for every other write in this module:
 * Java's WriteToStore() swallows its IOException (BounceGame.java:413-414).
 */
int bounce_persistence_save_t9_enabled(int32_t enabled)
{
    BouncePersistenceRecords records12;
    BouncePersistenceRecord3 record3;
    bool have_record3 = false;
    int32_t theme = 0;
    int32_t ok_value = (enabled != 0) ? 1 : 0;
    bool ok;

    if (bounce_persistence_load(&records12) != 0) {
        records12.max_levels = 0;
        records12.high_score = 0;
    }
    (void)bounce_persistence_load_record3(&record3, &have_record3);
    if (bounce_persistence_load_theme_index(&theme) != 0)
        theme = 0;
    ok = write_container(
        &records12, &record3, have_record3, theme, true, ok_value, true) == 0;
    return ok ? 0 : -1;
}

int bounce_persistence_save_theme_index(int32_t index)
{
    BouncePersistenceRecords records12;
    BouncePersistenceRecord3 record3;
    bool have_record3 = false;
    int32_t t9 = 0;
    bool ok;

    /*
     * Read-modify-write of the WHOLE container, so a theme change can never cost
     * the player a snapshot. This is the same obligation the other two writers
     * already carry: Java's WriteToStore(3) leaves records 1 and 2 alone
     * (BounceGame.java:410-416) and the no-argument WriteToStore() leaves record 3
     * alone (:417-428), because Bounce.destroyApp at Bounce.java:25 is the only
     * writer of the snapshot. Record 4 is the third member of that contract.
     */
    if (bounce_persistence_load(&records12) != 0) {
        records12.max_levels = 0;
        records12.high_score = 0;
    }
    (void)bounce_persistence_load_record3(&record3, &have_record3);
    /*
     * NATIVE ADDITION -- record 5 is read back and rewritten. Without this, a
     * theme change would erase the keypad setting, because write_container()
     * stages only the records it is handed. That is the mutual-erasure the task
     * this record exists to prevent, and it is why the load is here rather than
     * defaulted to OFF.
     */
    if (bounce_persistence_load_t9_enabled(&t9) != 0)
        t9 = 0;
    ok = write_container(
        &records12, &record3, have_record3, index, true, t9, true) == 0;
    return ok ? 0 : -1;
}

int bounce_persistence_save_record3(const BouncePersistenceRecord3 *in)
{
    BouncePersistenceRecords records12;
    int32_t theme = 0;
    int32_t t9 = 0;
    bool have_t9;

    if (in == NULL)
        return -1;

    /*
     * Records 1 and 2 are carried over unchanged. Java's WriteToStore(3) does
     * the same: it writes the snapshot without touching the other two.
     */
    if (bounce_persistence_load(&records12) != 0) {
        records12.max_levels = 0;
        records12.high_score = 0;
    }
    /*
     * TUGAS 2b -- and so is record 4, for the reason Java's two writers never
     * disturb each other. A menu Exit snapshots the session and must not reset
     * the player's color profile.
     */
    if (bounce_persistence_load_theme_index(&theme) != 0)
        theme = 0;
    /*
     * NATIVE ADDITION -- and so is record 5, read back for the same reason. A
     * snapshot written on menu Exit must not reset the player's keypad choice.
     *
     * Presence is preserved as presence for the same reason record 4's is: OFF is
     * the default, so absent and 0 mean the same thing, and a writer that merely
     * rewrites the container must not start manufacturing records.
     */
    have_t9 = bounce_persistence_load_t9_enabled(&t9) == 0 && t9 != 0;
    if (!have_t9)
        t9 = 0;
    return write_container(
        &records12, in, true, theme, true, t9, have_t9);
}

/*
 * STEP 38-RESET -- remove Record 3, the Continue snapshot, from the store.
 *
 * WHY A DELETE EXISTS AT ALL. Every writer in this module replaces one record and
 * preserves the rest, so until now there was no way to express "this record should
 * not exist". Record 3's absence is already a defined, normal state --
 * bounce_persistence_load_record3() reports present == false for it and
 * bounce_persistence_is_well_formed() accepts the container without it -- because a
 * fresh install and a legacy version-1 save both lack it. Reset to Default needs to
 * reach that state deliberately, and overwriting Record 3 with zeroes would NOT be
 * the same thing: the record would still be present, and the resume path would have
 * to treat a zeroed payload as absent. Removing it makes the store say what it means.
 *
 * RECORDS 1, 2, 4 AND 5 ARE PRESERVED, exactly as every other writer here preserves
 * them. That is deliberate and is what makes this composable with the reset: the
 * shell writes the new records 1 and 2, the default theme and the default keypad,
 * and calls this to drop the snapshot. Nothing in this function may overwrite a value
 * the reset has just written, so a caller that resets the store must not expect this
 * to clear anything else.
 *
 * FAILURE IS NOT FATAL, for the same reason as every other write here: Java's
 * WriteToStore() swallows its IOException (BounceGame.java:413-414) and the game
 * carries on. A snapshot that survives a failed delete is a stale Continue, which is
 * the defect this stage is partly about -- so the caller is expected to re-derive
 * availability from what it reads back, which bounce_app_flow's menu refresh already
 * does on every arrival.
 */
int bounce_persistence_delete_record3(void)
{
    BouncePersistenceRecords records12;
    int32_t theme = 0;
    int32_t t9 = 0;
    bool have_t9;

    if (bounce_persistence_load(&records12) != 0) {
        records12.max_levels = 0;
        records12.high_score = 0;
    }
    if (bounce_persistence_load_theme_index(&theme) != 0)
        theme = 0;
    have_t9 = bounce_persistence_load_t9_enabled(&t9) == 0 && t9 != 0;
    if (!have_t9)
        t9 = 0;
    /* have_record3 == false: the record is omitted from the directory entirely. */
    return write_container(
        &records12, NULL, false, theme, true, t9, have_t9);
}

int bounce_persistence_save_records_12_preserving_record3(
    const BouncePersistenceRecords *in)
{
    BouncePersistenceRecord3 existing;
    bool present = false;
    int32_t theme = 0;
    bool have_theme;
    int32_t t9 = 0;
    bool have_t9;

    if (in == NULL)
        return -1;
    /*
     * Java's WriteToStore() (BounceGame.java:417-428) writes only MaxLevels and
     * HighScore and never touches record 3, so a snapshot survives a level
     * completion or a game over. Record 3 is therefore read back and rewritten,
     * not dropped. A version-1 file, or a new-format file whose record 3 was
     * absent or unreadable, simply stays absent -- absence is the normal state
     * and is never an error here.
     */
    (void)bounce_persistence_load_record3(&existing, &present);
    /*
     * TUGAS 2b -- and record 4 is carried the same way, so a game over or a
     * level completion cannot silently reset the color profile. Absence stays
     * absence: this writer must not CREATE the record just because it is
     * rewriting the container, or every first save would pin profile 0 in the
     * file and the "no record means default" rule would stop being observable.
     */
    have_theme = bounce_persistence_load_theme_index(&theme) == 0
        && theme != 0;
    if (!have_theme)
        theme = 0;
    /*
     * NATIVE ADDITION -- record 5 rides along too, so a game over or a level
     * completion cannot silently switch the keypad off.
     *
     * ITS ABSENCE IS PRESERVED AS ABSENCE, exactly as record 4's is above, and
     * for the same reason: OFF is the default, so a file with no record 5 and a
     * file with record 5 == 0 mean the same thing, and this writer must not
     * CREATE the record just because it is rewriting the container.
     */
    have_t9 = bounce_persistence_load_t9_enabled(&t9) == 0 && t9 != 0;
    if (!have_t9)
        t9 = 0;
    return write_container(
        in, &existing, present, theme, have_theme, t9, have_t9);
}

int bounce_persistence_save(const BouncePersistenceRecords *in)
{
    if (in == NULL)
        return -1;
    /*
     * The signature and the meaning are unchanged for the one production caller
     * (vertical_slice.c:2166, app_persist_records). Only the on-disk version
     * moves: records 1 and 2 are now written into the version-2 container, and
     * any existing Record 3 is left exactly as it was. A legacy version-1 file
     * is upgraded in place, so no existing save is invalidated and no save is
     * lost by the change.
     */
    return bounce_persistence_save_records_12_preserving_record3(in);
}

/*
 * Self-test. Everything happens inside a temporary directory supplied through
 * XDG_DATA_HOME, so a run can never touch a real store. The caller's
 * environment is restored before returning.
 */
int bounce_persistence_verify(void)
{
    static const char template[] = "/tmp/bounce-persist-XXXXXX";
    char root[sizeof template];
    char saved[BOUNCE_PERSISTENCE_PATH_MAX];
    int saved_ok;
    BouncePersistenceRecords in;
    BouncePersistenceRecords out;
    FILE *file;
    unsigned char bytes[BOUNCE_PERSISTENCE_SIZE];
    int ok = -1;

    memcpy(root, template, sizeof template);
    if (mkdtemp(root) == NULL)
        return -1;

    saved_ok = (getenv("XDG_DATA_HOME") != NULL);
    if (saved_ok)
        (void)snprintf(saved, sizeof saved, "%s", getenv("XDG_DATA_HOME"));

    if (setenv("XDG_DATA_HOME", root, 1) != 0)
        goto done;

    /* 1. A missing store loads as "not loaded" with fresh-install defaults. */
    in.max_levels = 0;
    in.high_score = 0;
    if (bounce_persistence_load(&out) != -1)
        goto done;
    if (out.max_levels != 0 || out.high_score != 0)
        goto done;

    /* 2. A round trip preserves both records. */
    in.max_levels = 7;
    in.high_score = 12345;
    if (bounce_persistence_save(&in) != 0)
        goto done;
    if (bounce_persistence_load(&out) != 0)
        goto done;
    if (out.max_levels != 7 || out.high_score != 12345)
        goto done;

    /* 3. A second round trip overwrites rather than appending. */
    in.max_levels = 11;
    in.high_score = 0;
    if (bounce_persistence_save(&in) != 0)
        goto done;
    if (bounce_persistence_load(&out) != 0)
        goto done;
    if (out.max_levels != 11 || out.high_score != 0)
        goto done;

    /* 4. Wrong magic is rejected without reading any value. */
    {
        char path[BOUNCE_PERSISTENCE_PATH_MAX];
        if (resolve_path(path, sizeof path) != 0)
            goto done;
        file = fopen(path, "wb");
        if (file == NULL)
            goto done;
        memset(bytes, 'X', sizeof bytes);
        if (fwrite(bytes, 1u, sizeof bytes, file) != sizeof bytes)
            goto done;
        if (fclose(file) != 0)
            goto done;
    }
    in.max_levels = 3;
    in.high_score = 9;
    if (bounce_persistence_load(&out) != -1)
        goto done;
    if (out.max_levels != 0 || out.high_score != 0)
        goto done;

    /* 5. A truncated file is rejected. */
    {
        char path[BOUNCE_PERSISTENCE_PATH_MAX];
        if (resolve_path(path, sizeof path) != 0)
            goto done;
        file = fopen(path, "wb");
        if (file == NULL)
            goto done;
        if (fwrite(bytes, 1u, 7u, file) != 7u)
            goto done;
        if (fclose(file) != 0)
            goto done;
    }
    if (bounce_persistence_load(&out) != -1)
        goto done;

    /* 6. A directory instead of a file is rejected, not a crash. */
    {
        char path[BOUNCE_PERSISTENCE_PATH_MAX];
        if (resolve_path(path, sizeof path) != 0)
            goto done;
        (void)mkdir(path, 0777);
        if (bounce_persistence_load(&out) != -1)
            goto done;
    }

    /* 7. Wrong version is rejected. */
    {
        char path[BOUNCE_PERSISTENCE_PATH_MAX];
        BouncePersistenceRecords tmp;
        tmp.max_levels = 5;
        tmp.high_score = 5;
        if (bounce_persistence_save(&tmp) != 0)
            goto done;
        if (resolve_path(path, sizeof path) != 0)
            goto done;
        file = fopen(path, "r+b");
        if (file == NULL)
            goto done;
        memcpy(bytes, BOUNCE_PERSISTENCE_MAGIC, BOUNCE_PERSISTENCE_MAGIC_SIZE);
        put_u32(bytes + 8u, 99u);
        put_u32(bytes + 12u, 5u);
        put_u32(bytes + 16u, 5u);
        if (fwrite(bytes, 1u, sizeof bytes, file) != sizeof bytes)
            goto done;
        if (fclose(file) != 0)
            goto done;
    }
    if (bounce_persistence_load(&out) != -1)
        goto done;

    /* 8. NULL argument is safe. */
    if (bounce_persistence_load(NULL) != -1)
        goto done;
    if (bounce_persistence_save(NULL) != -1)
        goto done;

    /*
     * G-R3-P1 -- Record 3 and the multi-record container.
     *
     * V1  legacy version-1 file: still loads, values identical, Record 3 absent
     * V2  new container with records 1 and 2 only: loads, Record 3 absent
     * V3  full Record 3 round trip: every field compared
     * V4  payload length is exactly 79 + 5*b2 + 8*n
     * V5  representative fields are big-endian, as Java's DataOutputStream writes
     * V6  explicit record3_present == false for the absent case
     * V7  a truncated Record 3 payload is rejected
     * V8  a wrong Java magic is rejected
     * V9  an impossible count is rejected without an oversized allocation
     */
    {
        static BouncePersistenceRecord3 r3;
        static BouncePersistenceRecord3 back;
        static unsigned char probe[BOUNCE_PERSISTENCE_MAX_FILE_SIZE];
        char path[BOUNCE_PERSISTENCE_PATH_MAX];
        bool present = false;
        uint32_t want;
        uint32_t got;
        uint32_t k;
        const unsigned char *pl = NULL;
        uint32_t pl_size = 0u;
        FILE *rf;

        /* --- V1: hand-written legacy version-1 container. --- */
        if (resolve_path(path, sizeof path) != 0)
            goto done;
        memset(probe, 0, sizeof probe);
        memcpy(probe, BOUNCE_PERSISTENCE_MAGIC, BOUNCE_PERSISTENCE_MAGIC_SIZE);
        put_u32(probe + 8u, BOUNCE_PERSISTENCE_VERSION);
        put_u32(probe + 12u, 6u);
        put_u32(probe + 16u, 4242u);
        rf = fopen(path, "wb");
        if (rf == NULL)
            goto done;
        if (fwrite(probe, 1u, BOUNCE_PERSISTENCE_SIZE, rf)
            != BOUNCE_PERSISTENCE_SIZE) {
            (void)fclose(rf);
            goto done;
        }
        if (fclose(rf) != 0)
            goto done;
        in.max_levels = 0;
        in.high_score = 0;
        if (bounce_persistence_load(&out) != 0)
            goto done;
        if (out.max_levels != 6 || out.high_score != 4242)
            goto done; /* The four legacy fields must be unchanged. */
        if (bounce_persistence_load_record3(&back, &present) != 0)
            goto done;
        if (present)
            goto done; /* A version-1 file can never carry a Record 3. */

        /* --- V2: write records 1 and 2 into the new container. --- */
        in.max_levels = 11;
        in.high_score = 987654;
        if (bounce_persistence_save(&in) != 0)
            goto done;
        if (bounce_persistence_load(&out) != 0)
            goto done;
        if (out.max_levels != 11 || out.high_score != 987654)
            goto done;
        if (bounce_persistence_load_record3(&back, &present) != 0 || present)
            goto done; /* --- V6: absent is normal and reported as such. --- */

        /* --- V3/V4/V5: deterministic Record 3 fixture, round trip. --- */
        memset(&r3, 0, sizeof r3);
        r3.timestamp_millis = INT64_C(0x0102030405060708);
        r3.b1 = BOUNCE_PERSISTENCE_RECORD3_B1_IN_LEVEL;
        r3.lives = 2u;
        r3.hoops_scored = 5u;
        r3.level = 7u;
        r3.ball_size = 16u;
        r3.score = 314159;
        r3.l = 111;
        r3.k = 222;
        r3.unk_x = 678;
        r3.unk_y = 135;
        r3.aq_l = -7;
        r3.aq_o = 9;
        r3.zero_slot_0 = 0; /* Java writes a literal 0, :373. */
        r3.zero_slot_1 = 0; /* Java writes a literal 0, :374. */
        r3.tile_x = 4;
        r3.tile_y = 3;
        r3.power_up_1 = 300;
        r3.power_up_gravity = -300;
        r3.power_up_3 = 42;
        r3.tile_delta_count = 3u;
        r3.tile_deltas[0].row = 1; r3.tile_deltas[0].col = 2;
        r3.tile_deltas[0].tile_id = 22u;
        r3.tile_deltas[1].row = -3; r3.tile_deltas[1].col = 40;
        r3.tile_deltas[1].tile_id = 7u;
        r3.tile_deltas[2].row = 7; r3.tile_deltas[2].col = 111;
        r3.tile_deltas[2].tile_id = 29u;
        r3.dyn_thorn_count = 2u;
        r3.dyn_thorns[0].w_x = 10; r3.dyn_thorns[0].w_y = -20;
        r3.dyn_thorns[0].ae_x = 30; r3.dyn_thorns[0].ae_y = 40;
        r3.dyn_thorns[1].w_x = -1; r3.dyn_thorns[1].w_y = 2;
        r3.dyn_thorns[1].ae_x = -3; r3.dyn_thorns[1].ae_y = 4;

        want = bounce_persistence_record3_payload_size(3u, 2u);
        if (want != 79u + 5u * 3u + 8u * 2u)
            goto done; /* --- V4: the formula itself. --- */

        if (bounce_persistence_save_record3(&r3) != 0)
            goto done;
        if (bounce_persistence_load_record3(&back, &present) != 0)
            goto done;
        if (!present)
            goto done;
        if (back.timestamp_millis != r3.timestamp_millis
            || back.b1 != r3.b1
            || back.lives != r3.lives
            || back.hoops_scored != r3.hoops_scored
            || back.level != r3.level
            || back.ball_size != r3.ball_size
            || back.score != r3.score
            || back.l != r3.l
            || back.k != r3.k
            || back.unk_x != r3.unk_x
            || back.unk_y != r3.unk_y
            || back.aq_l != r3.aq_l
            || back.aq_o != r3.aq_o
            || back.zero_slot_0 != r3.zero_slot_0
            || back.zero_slot_1 != r3.zero_slot_1
            || back.tile_x != r3.tile_x
            || back.tile_y != r3.tile_y
            || back.power_up_1 != r3.power_up_1
            || back.power_up_gravity != r3.power_up_gravity
            || back.power_up_3 != r3.power_up_3
            || back.tile_delta_count != r3.tile_delta_count
            || back.dyn_thorn_count != r3.dyn_thorn_count)
            goto done;
        for (k = 0u; k < r3.tile_delta_count; ++k) {
            if (back.tile_deltas[k].row != r3.tile_deltas[k].row
                || back.tile_deltas[k].col != r3.tile_deltas[k].col
                || back.tile_deltas[k].tile_id != r3.tile_deltas[k].tile_id)
                goto done;
        }
        for (k = 0u; k < r3.dyn_thorn_count; ++k) {
            if (back.dyn_thorns[k].w_x != r3.dyn_thorns[k].w_x
                || back.dyn_thorns[k].w_y != r3.dyn_thorns[k].w_y
                || back.dyn_thorns[k].ae_x != r3.dyn_thorns[k].ae_x
                || back.dyn_thorns[k].ae_y != r3.dyn_thorns[k].ae_y)
                goto done;
        }
        /* Records 1 and 2 survived the Record 3 write. */
        if (bounce_persistence_load(&out) != 0
            || out.max_levels != 11 || out.high_score != 987654)
            goto done;

        /* --- V5: read the payload back out of the file and check the bytes. --- */
        if (resolve_path(path, sizeof path) != 0)
            goto done;
        rf = fopen(path, "rb");
        if (rf == NULL)
            goto done;
        got = (uint32_t)fread(probe, 1u, sizeof probe, rf);
        (void)fclose(rf);
        {
            if (!container_find_record(
                    probe,
                    got,
                    BOUNCE_PERSISTENCE_RECORD_SNAPSHOT,
                    &pl,
                    &pl_size))
                goto done;
            if (pl_size != want)
                goto done;
            /*
             * Java writeLong is big-endian. The fixture timestamp is
             * 0x0102030405060708, so the first eight payload bytes must be
             * exactly that, most significant byte first.
             */
            if (pl[0] != 0x01u || pl[1] != 0x02u || pl[2] != 0x03u
                || pl[3] != 0x04u || pl[4] != 0x05u || pl[5] != 0x06u
                || pl[6] != 0x07u || pl[7] != 0x08u)
                goto done;
            /* Java writeInt(score == 314159 == 0x0004CB2F) is big-endian. */
            if (pl[13] != 0x00u || pl[14] != 0x04u || pl[15] != 0xCBu
                || pl[16] != 0x2Fu)
                goto done;
            /*
             * Payload offset 69 is the tile-delta count, so the first delta
             * starts at 70: Java writeShort(row == 1) is 0x00 0x01 there.
             */
            if (pl[69] != 0x03u || pl[70] != 0x00u || pl[71] != 0x01u)
                goto done;
            /* Java writeInt(-559038737) == 0xDEADBEEF, big-endian trailer. */
            if (pl[pl_size - 4u] != 0xDEu || pl[pl_size - 3u] != 0xADu
                || pl[pl_size - 2u] != 0xBEu || pl[pl_size - 1u] != 0xEFu)
                goto done;
        }

        /*
         * --- V5b: STEP DV3 GUARD H. RecordLives is a SIGNED byte. ---
         *
         * WHY THIS NEEDS ITS OWN BLOCK. Every other Record 3 fixture in this file
         * uses lives 0, 2 or 3, and those three values are bit-identical whether
         * the field is read as signed or unsigned. A struct-only comparison
         * therefore cannot tell the two interpretations apart, which is exactly
         * why the regression went unnoticed; only the SERIALIZED BYTE and a
         * negative value distinguish them.
         *
         * THE CONTRACT (BounceGame.java). writeByte(this.v.lives) at :361 emits
         * the low 8 bits; readByte() at :294 returns a SIGNED byte; e.java:95
         * sign-extends it into the int `lives`. So the round trip is the
         * two's-complement interpretation of the low byte, and these four cases
         * are the ones that pin it down: 127 is the largest positive that
         * survives, 128 wraps to -128, and -1 and -128 are the negatives a
         * game-over snapshot can carry.
         *
         * Payload layout: [0..7] timestamp, [8] b1, [9] lives. Byte 9 is the one
         * asserted here, read back out of the written file rather than from the
         * struct, so the encoder is genuinely under test.
         */
        {
            static const int lives_cases[4] = { 127, 128, -1, -128 };
            unsigned int case_index;

            for (case_index = 0u; case_index < 4u; ++case_index) {
                const int in_lives = lives_cases[case_index];
                const unsigned char expect_byte
                    = (unsigned char)(in_lives & 0xFF);
                const int expect_back = (int)(int8_t) expect_byte;

                memset(&r3, 0, sizeof r3);
                r3.timestamp_millis = INT64_C(0x0102030405060708);
                r3.b1 = BOUNCE_PERSISTENCE_RECORD3_B1_NONE;
                r3.lives = (int8_t) in_lives;
                r3.level = 7u;
                r3.ball_size = 12u;
                if (bounce_persistence_record3_payload_size(0u, 0u) != 79u)
                    goto done;
                if (bounce_persistence_save_record3(&r3) != 0)
                    goto done;
                if (bounce_persistence_load_record3(&back, &present) != 0
                    || !present)
                    goto done;

                /* The decoded value must equal the SIGNED reading of the byte. */
                if ((int) back.lives != expect_back)
                    goto done;

                /* And the byte on disk must be the low 8 bits, unchanged. */
                if (resolve_path(path, sizeof path) != 0)
                    goto done;
                rf = fopen(path, "rb");
                if (rf == NULL)
                    goto done;
                got = (uint32_t) fread(probe, 1u, sizeof probe, rf);
                (void) fclose(rf);
                if (!container_find_record(
                        probe,
                        got,
                        BOUNCE_PERSISTENCE_RECORD_SNAPSHOT,
                        &pl,
                        &pl_size))
                    goto done;
                if (pl_size < 10u || pl[9] != expect_byte)
                    goto done;
            }
        }

        /* --- V7: truncate the Record 3 payload by one byte. --- */
        {
            /*
             * TUGAS 2b -- the offsets are now DERIVED, not written down.
             *
             * This block used to compute
             *
             *     dir_at + 3 * DIR_ENTRY_SIZE + 8
             *
             * for record 3's payload, and its sibling rewrote
             * `dir_at + 2 * 8 + 4` for record 3's length field. Both were true
             * only while the container held exactly three records: the first
             * needs a directory of 3 to put payload_at in the right place, and
             * the second needs record 3 to remain the LAST payload. Adding
             * record 4 made both wrong, and they failed loudly rather than
             * silently, which is the only reason this is worth recording.
             *
             * The intent of both checks -- corrupt the stored snapshot in place,
             * at whatever offset it actually occupies -- is preserved by asking
             * the module's own reader where record 3 is. The directory stays
             * sorted by id, so record 3 is still entry index 2, but nothing here
             * has to know that, and the next record added will not break it.
             */
            uint32_t dir_at = BOUNCE_PERSISTENCE_V2_HEADER_SIZE;
            uint32_t at;

            if (resolve_path(path, sizeof path) != 0)
                goto done;
            rf = fopen(path, "rb");
            if (rf == NULL)
                goto done;
            got = (uint32_t) fread(probe, 1u, sizeof probe, rf);
            (void) fclose(rf);
            if (!container_find_record(
                    probe,
                    got,
                    BOUNCE_PERSISTENCE_RECORD_SNAPSHOT,
                    &pl,
                    &pl_size))
                goto done;
            at = (uint32_t)(pl - probe);
            rf = fopen(path, "r+b");
            if (rf == NULL)
                goto done;
            if (fseek(rf, (long)(at + pl_size - 1u), SEEK_SET) != 0
                || fputc(0, rf) == EOF) {
                (void)fclose(rf);
                goto done;
            }
            {
                /*
                 * The directory entry for record 3 is found by walking the
                 * directory, not by index, for the same reason: the entry order
                 * is ascending by id by specification, but nothing forces a
                 * record to occupy a fixed slot once more of them exist.
                 */
                uint32_t entries = get_u32(probe + 12u);
                uint32_t slot;
                int found = 0;
                uint32_t len_at = 0u;

                if (entries > BOUNCE_PERSISTENCE_V2_MAX_RECORDS)
                    goto done;
                for (slot = 0u; slot < entries; ++slot) {
                    if (get_u32(probe + dir_at + slot * 8u)
                        == BOUNCE_PERSISTENCE_RECORD_SNAPSHOT) {
                        len_at = dir_at + slot * 8u + 4u;
                        found = 1;
                        break;
                    }
                }
                if (!found)
                    goto done;
                (void)fseek(rf, (long)len_at, SEEK_SET);
            }
            {
                unsigned char lenbuf[4];
                put_u32(lenbuf, pl_size - 1u);
                if (fwrite(lenbuf, 1u, 4u, rf) != 4u) {
                    (void)fclose(rf);
                    goto done;
                }
            }
            if (fclose(rf) != 0)
                goto done;
        }
        if (bounce_persistence_load_record3(&back, &present) != 0)
            goto done;
        if (present)
            goto done; /* Truncated Record 3 is rejected, i.e. absent. */

        /* --- V8: corrupt the Java magic trailer. --- */
        if (bounce_persistence_save_record3(&r3) != 0)
            goto done;
        if (resolve_path(path, sizeof path) != 0)
            goto done;
        rf = fopen(path, "r+b");
        if (rf == NULL)
            goto done;
        {
            /* TUGAS 2b -- derived from the file, for the reason V7 documents. */
            uint32_t at;

            if (fclose(rf) != 0)
                goto done;
            rf = fopen(path, "rb");
            if (rf == NULL)
                goto done;
            got = (uint32_t) fread(probe, 1u, sizeof probe, rf);
            (void) fclose(rf);
            if (!container_find_record(
                    probe,
                    got,
                    BOUNCE_PERSISTENCE_RECORD_SNAPSHOT,
                    &pl,
                    &pl_size))
                goto done;
            at = (uint32_t)(pl - probe);
            rf = fopen(path, "r+b");
            if (rf == NULL)
                goto done;
            if (fseek(rf, (long)(at + pl_size - 1u), SEEK_SET) != 0
                || fputc(0x00, rf) == EOF) {
                (void)fclose(rf);
                goto done;
            }
        }
        if (fclose(rf) != 0)
            goto done;
        if (bounce_persistence_load_record3(&back, &present) != 0)
            goto done;
        if (present)
            goto done;

        /* --- V9: an impossible tile-delta count is rejected, not allocated. --- */
        {
            BouncePersistenceRecord3 bad = r3;
            /*
             * Put a good record back first: V8 deliberately corrupted the
             * stored one, so "the refused write left the store alone" can only
             * be observed against a valid record.
             */
            if (bounce_persistence_save_record3(&r3) != 0)
                goto done;
            bad.tile_delta_count =
                (uint32_t)BOUNCE_PERSISTENCE_RECORD3_MAX_TILE_DELTAS + 1u;
            if (bounce_persistence_record3_payload_size(bad.tile_delta_count, 0u)
                != 0u)
                goto done;
            /* The encode is refused, so the save fails and nothing is written. */
            if (bounce_persistence_save_record3(&bad) != -1)
                goto done;
            bad.tile_delta_count = r3.tile_delta_count;
            bad.dyn_thorn_count =
                (uint32_t)BOUNCE_PERSISTENCE_RECORD3_MAX_DYN_THORNS + 1u;
            if (bounce_persistence_record3_payload_size(0u, bad.dyn_thorn_count)
                != 0u)
                goto done;
            if (bounce_persistence_save_record3(&bad) != -1)
                goto done;
        }
        /* The refused writes left the previous good record in place. */
        if (bounce_persistence_load_record3(&back, &present) != 0 || !present)
            goto done;
        if (back.timestamp_millis != r3.timestamp_millis
            || back.tile_delta_count != r3.tile_delta_count
            || back.dyn_thorn_count != r3.dyn_thorn_count)
            goto done;

        /*
         * Java's WriteToStore() writes records 1 and 2 and never touches record
         * 3, so a snapshot must survive the records-1/2 save that
         * app_persist_records performs at level completion and game over.
         */
        {
            BouncePersistenceRecords only12;
            only12.max_levels = 3;
            only12.high_score = 99;
            if (bounce_persistence_save_records_12_preserving_record3(&only12)
                != 0)
                goto done;
            if (bounce_persistence_load(&out) != 0
                || out.max_levels != 3 || out.high_score != 99)
                goto done;
            if (bounce_persistence_load_record3(&back, &present) != 0
                || !present)
                goto done;
            if (back.timestamp_millis != r3.timestamp_millis
                || back.tile_delta_count != r3.tile_delta_count
                || back.dyn_thorn_count != r3.dyn_thorn_count)
                goto done;
        }

        /*
         * --- T4: the color profile (TUGAS 2b) ---
         *
         * The requirement is that the theme SURVIVES, so the interesting cases
         * are not the round trip but the three ways the other records could be
         * lost by adding a fourth: a theme write must not drop the snapshot, a
         * snapshot write must not drop the theme, and a records-1/2 write must
         * not drop either.
         */
        {
            int32_t theme = -1;

            /* Absent on a store that has never had one: the default profile. */
            if (bounce_persistence_load_theme_index(&theme) != 0
                || theme != 0)
                goto done;

            /* A plain round trip. */
            if (bounce_persistence_save_theme_index(5) != 0)
                goto done;
            if (bounce_persistence_load_theme_index(&theme) != 0
                || theme != 5)
                goto done;

            /* A theme write must not cost the snapshot. */
            if (bounce_persistence_load_record3(&back, &present) != 0
                || !present
                || back.timestamp_millis != r3.timestamp_millis)
                goto done;
            if (bounce_persistence_load(&out) != 0
                || out.max_levels != 3 || out.high_score != 99)
                goto done;

            /* And it must not cost records 1 and 2 either. */
            if (bounce_persistence_save_theme_index(0) != 0
                || bounce_persistence_save_theme_index(12) != 0)
                goto done;
            if (bounce_persistence_load_theme_index(&theme) != 0
                || theme != 12)
                goto done;
            if (bounce_persistence_load(&out) != 0
                || out.max_levels != 3 || out.high_score != 99)
                goto done;
            if (bounce_persistence_load_record3(&back, &present) != 0
                || !present
                || back.timestamp_millis != r3.timestamp_millis)
                goto done;

            /* A snapshot write must not drop the theme. */
            if (bounce_persistence_save_record3(&r3) != 0)
                goto done;
            if (bounce_persistence_load_theme_index(&theme) != 0
                || theme != 12)
                goto done;

            /*
             * A records-1/2 write must not drop the theme, and -- the subtle
             * half -- must not CREATE one either. If it pinned 0 into the file,
             * "no record means the default profile" would stop being observable
             * and a player who had chosen a profile would silently lose it at
             * their next level completion.
             */
            {
                BouncePersistenceRecords only12;
                only12.max_levels = 11;
                only12.high_score = 5;
                if (bounce_persistence_save_records_12_preserving_record3(
                        &only12) != 0)
                    goto done;
            }
            if (bounce_persistence_load_theme_index(&theme) != 0
                || theme != 12)
                goto done;

            /* A negative index is refused, and the stored value is untouched. */
            if (bounce_persistence_save_theme_index(-1) != -1)
                goto done;
            if (bounce_persistence_load_theme_index(&theme) != 0
                || theme != 12)
                goto done;

            /* NULL is safe. */
            if (bounce_persistence_load_theme_index(NULL) != -1)
                goto done;

            /*
             * The record is physically present with a four-byte payload under
             * id 4, which is what "added to the existing container" has to mean.
             * Reading it through the container's own directory walker is the
             * check: an implementation that kept the theme somewhere else in the
             * file would not be found here.
             */
            if (resolve_path(path, sizeof path) != 0)
                goto done;
            rf = fopen(path, "rb");
            if (rf == NULL)
                goto done;
            got = (uint32_t) fread(probe, 1u, sizeof probe, rf);
            (void) fclose(rf);
            if (!container_find_record(
                    probe,
                    got,
                    BOUNCE_PERSISTENCE_RECORD_THEME,
                    &pl,
                    &pl_size))
                goto done;
            if (pl_size != 4u || get_u32(pl) != 12u)
                goto done;
        }

        /* NULL is safe on the new API too. */
        if (bounce_persistence_load_record3(NULL, &present) != -1)
            goto done;
        if (bounce_persistence_load_record3(&back, NULL) != -1)
            goto done;
        if (bounce_persistence_save_record3(NULL) != -1)
            goto done;
        if (bounce_persistence_save_records_12_preserving_record3(NULL) != -1)
            goto done;
    }

    ok = 0;

done:
    if (saved_ok)
        (void)setenv("XDG_DATA_HOME", saved, 1);
    else
        (void)unsetenv("XDG_DATA_HOME");
    /* The temporary tree is deliberately left for the OS tmp cleaner; nothing
       under $HOME or the real XDG location was ever touched. */
    (void)root;
    return ok;
}
