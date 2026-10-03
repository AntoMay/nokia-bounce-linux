/*
 * persistence.h -- STEP 37 PERSIST: native equivalent of the Java ME RecordStore.
 *
 * WHAT THIS IS
 *   The original keeps three records in a private store named "bounceRMS"
 *   (BounceGame.java:275, :410):
 *
 *     record 1  (1 byte)      MaxLevels
 *     record 2  (4 bytes)     HighScore
 *     record 3  (variable)   full mid-level resume snapshot
 *
 * THIS MODULE PERSISTS RECORDS 1 AND 2, AND -- since G-R3-P1 -- THE RECORD 3
 * *REPRESENTATION*. What it does NOT do is the gameplay half of Record 3: no
 * Continue UI, no availability flag, no menu handler, no snapshot trigger and
 * no resume path. Record 3 is serialised and deserialised here and nowhere
 * else; what a caller does with it is a later milestone.
 *
 * WHY THE BYTES ARE NOT RMS-COMPATIBLE
 *   The RMS container is a Java ME implementation detail, not observable
 *   behaviour. Nothing in the recovered game reads or writes a file, and no
 *   other tool reads it. What has to be reproduced is the *observable*
 *   persistence behaviour -- a fresh launch with no store behaves like a
 *   Java fresh install, and a value written at game over or level complete is
 *   still there on the next launch. The on-disk layout below is therefore a
 *   native Linux format, deliberately not a byte-for-byte copy of an RMS
 *   record.
 *
 * FILE LOCATION
 *   $XDG_DATA_HOME/bounce/records.bin, falling back to
 *   $HOME/.local/share/bounce/records.bin. XDG_DATA_HOME is the standard
 *   per-user data location on Linux and is the closest native analogue of the
 *   per-application private store Java ME gives RecordStore. The repository has
 *   no other writable-data convention -- its only path constant is
 *   BOUNCE_RESOURCE_ROOT ("src/main/resources", vertical_slice.c:32), which is
 *   read-only game data -- so a new location had to be chosen, and the standard
 *   one was chosen rather than a project-local path.
 *
 * FAILURE POLICY
 *   Java's LoadRecords() catches every exception and leaves J = 0
 *   (BounceGame.java:332-334), which makes the game behave as if nothing had
 *   ever been saved. This module does the same: a missing, unreadable, short,
 *   wrong-magic or wrong-version file is reported as "not loaded" and the
 *   caller keeps its documented defaults. Nothing here can abort the game.
 */

#ifndef BOUNCE_NATIVE_APP_PERSISTENCE_H
#define BOUNCE_NATIVE_APP_PERSISTENCE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The persisted set. Both members are int32_t rather than the Java byte/int
 * widths so that a malformed or truncated file can never produce a value the
 * caller would have to range-check; bounce_persistence_load() only ever stores
 * what it read after validating the container.
 */
typedef struct BouncePersistenceRecords {
    int32_t max_levels; /* Java MaxLevels, BounceGame.java:21 */
    int32_t high_score; /* Java HighScore, BounceGame.java:23 */
} BouncePersistenceRecords;

/*
 * G-R3-P1 -- record ids, mirroring the three indices Java addresses in
 * "bounceRMS" (BounceGame.java:281-283 reads, :411 writes).
 */
#define BOUNCE_PERSISTENCE_RECORD_MAX_LEVELS 1u
#define BOUNCE_PERSISTENCE_RECORD_HIGH_SCORE 2u
#define BOUNCE_PERSISTENCE_RECORD_SNAPSHOT 3u

/*
 * TUGAS 2b -- the selected color profile, as record 4 of the SAME container.
 *
 * THERE IS NO JAVA RECORD 4, and there cannot be: the color profiles are a
 * native feature (N-03, ui_shell.h's BOUNCE_UI_COLOR_PROFILE_COUNT), and the
 * recovered game has no Settings screen at all. So the PAYLOAD is native-only.
 * The MECHANISM is not -- Java's whole persistence surface is
 *
 *   BounceGame.java:275   RecordStore.createRecordStore("bounceRMS", 1)
 *   BounceGame.java:281-283  readRecord(1), readRecord(2), readRecord(3)
 *   BounceGame.java:410-428  setRecord(1), setRecord(2), setRecord(3)
 *
 * i.e. one private store addressed by integer record id, written whole. The
 * version-2 container this module already writes is the transcription of that:
 * a directory of (record id, payload length) pairs in ascending id order, where
 * "a record that is not in the directory does not exist". Adding id 4 therefore
 * uses the format as designed and invents nothing: no new file, no new magic,
 * no new version, no new framing.
 *
 * THE PAYLOAD IS A SINGLE u32, the color-profile index, which is the same shape
 * as records 1 and 2. The value is bounded by the caller against
 * BOUNCE_UI_COLOR_PROFILE_COUNT; persistence.c only refuses a negative one,
 * mirroring how it treats MaxLevels and HighScore, because it cannot know the
 * number of native profiles.
 *
 * ABSENCE IS NORMAL and means "the default profile", index 0, exactly as a fresh
 * install of the Java game means record 1 absent -> 0 (BounceGame.java:276-279).
 */
#define BOUNCE_PERSISTENCE_RECORD_THEME 4u

/*
 * NATIVE ADDITION -- the numeric-keypad setting, as record 5 of the SAME
 * container.
 *
 * THERE IS NO JAVA RECORD 5, and there cannot be: the keypad is a native feature
 * with no counterpart in the recovered game, which has no Settings screen and no
 * keypad input. So the PAYLOAD is native-only, exactly as record 4's is.
 *
 * THE MECHANISM IS THE SOURCE'S, unchanged, for the reason record 4's is: Java's
 * whole persistence surface is one private store addressed by integer record id
 * (BounceGame.java:275, :281-283, :410-428), which is what the version-2
 * container transcribes. So id 5 uses the format as designed and invents nothing:
 * no new file, no new magic, no new version, no new framing. The only ceiling
 * that moves is the record count, 4 -> 5.
 *
 * THE PAYLOAD IS A SINGLE u32, 0 for OFF and 1 for ON, the same shape as records
 * 1, 2 and 4. Anything non-zero reads as ON, because the reader cannot know how
 * many native settings exist.
 *
 * ABSENCE IS NORMAL and means OFF, the default, for the same reason record 4's
 * absence means profile 0.
 */
#define BOUNCE_PERSISTENCE_RECORD_T9_INPUT 5u

/*
 * G-R3-P1 -- the fixed prefix of Java's Record 3, in bytes.
 *
 * BounceGame.java:360-379 writes, in order: a long timestamp, five bytes
 * (b1, lives, HoopsScored, level, ballSize), then fourteen ints. Two of those
 * fourteen are literal zeros (:373-374). 8 + 5 + 14*4 = 69.
 */
#define BOUNCE_PERSISTENCE_RECORD3_PREFIX_SIZE 69u

/*
 * Java allocates the tile-delta array as new int[50][3] (BounceGame.java:380)
 * and indexes it with an unbounded byte (b2, :389), so a write can only ever
 * carry at most 50 deltas before Java itself would throw. A larger count on
 * read is therefore not representable and is rejected.
 */
#define BOUNCE_PERSISTENCE_RECORD3_MAX_TILE_DELTAS 50u

/*
 * Java reads DynThornsCount from a single byte of the level file
 * (b.java:248) and allocates four arrays of that length (b.java:293-298), so
 * the count it can round-trip through Record 3 is a signed byte. Anything
 * above that is rejected rather than allocated.
 */
#define BOUNCE_PERSISTENCE_RECORD3_MAX_DYN_THORNS 127u

/*
 * One tile delta, exactly as BounceGame.java:395-397 writes it and :315-317
 * reads it: short row, short col, byte tile id.
 */
typedef struct BouncePersistenceTileDelta {
    int16_t row;
    int16_t col;
    uint8_t tile_id;
} BouncePersistenceTileDelta;

/*
 * One dynamic-thorn entry, exactly as BounceGame.java:402-405 writes it and
 * :323-326 reads it: short w.x, short w.y, short ae.x, short ae.y. Both
 * Vec2S pairs are shorts, never ints.
 */
typedef struct BouncePersistenceDynThorn {
    int16_t w_x;
    int16_t w_y;
    int16_t ae_x;
    int16_t ae_y;
} BouncePersistenceDynThorn;

/*
 * Java's Record 3, reproduced structurally (G-R3-P1 decision F1).
 *
 * THE FOUR UNUSED FIELDS ARE DELIBERATE. Java writes timestamp, camera l,
 * camera k and two literal zero ints, and never reads any of them back on any
 * resume path -- BounceGame.java:299-300 and :305-306 assign them and nothing
 * in the tree references them again. They are reproduced anyway, under F1, so
 * that the native payload stays byte-comparable with Java's. THEY MUST NOT BE
 * TREATED AS RESTORE STATE: Java recalculates the camera from the level load
 * and the first tick (e.java:98, then the ordinary camera path) and never
 * resumes a persisted camera. Restoring camera l/k from this struct would be a
 * divergence, not a reproduction.
 */
typedef struct BouncePersistenceRecord3 {
    int64_t timestamp_millis;  /* :360  written, never consumed */
    uint8_t b1;                /* :361  K encoding; see the branch note below */
    /*
     * STEP DV3 -- SIGNED, because Java's is.
     *
     * BounceGame.java:31 declares `public byte RecordLives;`, and a Java byte is
     * signed. The value is written with DataOutputStream.writeByte()
     * (BounceGame.java:361), which emits only the low 8 bits, and read back with
     * DataInputStream.readByte() (:294), which returns a SIGNED byte. e.java:95
     * then assigns it to the int `lives`, so the assignment sign-extends.
     *
     * The round trip is therefore exactly the two's-complement interpretation of
     * the low byte: -1 -> 0xFF -> -1, and -128 -> 0x80 -> -128.
     *
     * WHY IT MATTERS HERE AND ONLY HERE. The one value that is ever negative in
     * play is -1, produced by the death path (vertical_slice.c:2031) and consumed
     * as the game-over test. Read as unsigned it became 255, so a snapshot taken
     * at game over restored a 255-life run. Nothing else about the field changes:
     * BOUNCE_APP_ENTRY_INITIAL_LIVES is 3 and the only other writer is the 1-up,
     * so every positive value is unaffected.
     *
     * Deliberately NOT clamped, and NOT validated on decode. Java does not clamp
     * either -- it round-trips the truncated bits and sign-extends on read -- so
     * clamping here would diverge from the recovered source. The existing
     * `lives < 0` guard in bounce_app_flow_begin_resume_entry() (app_flow.c:974)
     * is what refuses such a resume, and it stays meaningful precisely because
     * this field is signed.
     */
    int8_t lives;              /* :362  SIGNED; Java byte semantics */
    uint8_t hoops_scored;      /* :363 */
    uint8_t level;             /* :364 */
    uint8_t ball_size;         /* :365 */
    int32_t score;             /* :366 */
    int32_t l;                 /* :367  camera l; written, never consumed */
    int32_t k;                 /* :368  camera k; written, never consumed */
    int32_t unk_x;             /* :369  aq.TODO_unkX */
    int32_t unk_y;             /* :370  aq.TODO_unkY */
    int32_t aq_l;              /* :371  aq.l, horizontal velocity */
    int32_t aq_o;              /* :372  aq.o, vertical velocity */
    int32_t zero_slot_0;       /* :373  literal 0 in Java; written, never consumed */
    int32_t zero_slot_1;       /* :374  literal 0 in Java; written, never consumed */
    int32_t tile_x;            /* :375  aq.d */
    int32_t tile_y;            /* :376  aq.c */
    int32_t power_up_1;        /* :377 */
    int32_t power_up_gravity;  /* :378 */
    int32_t power_up_3;        /* :379 */
    uint32_t tile_delta_count; /* :393  Java's b2 */
    uint32_t dyn_thorn_count;  /* :400 */
    BouncePersistenceTileDelta tile_deltas[BOUNCE_PERSISTENCE_RECORD3_MAX_TILE_DELTAS];
    BouncePersistenceDynThorn dyn_thorns[BOUNCE_PERSISTENCE_RECORD3_MAX_DYN_THORNS];
} BouncePersistenceRecord3;

/*
 * b1 is not gameplay state. BounceGame.java:354-359 encodes K (K==1 -> 1,
 * K==5 -> 2, otherwise 0) and BounceGame.java:227-231 branches on it to pick
 * the resume path. It is a resume-path selector, nothing more.
 */
#define BOUNCE_PERSISTENCE_RECORD3_B1_NONE 0u
#define BOUNCE_PERSISTENCE_RECORD3_B1_IN_LEVEL 1u
#define BOUNCE_PERSISTENCE_RECORD3_B1_LEVEL_COMPLETE 2u

/*
 * Read the records.
 *
 * Returns 0 when a well-formed store was read and *out was filled, and -1 when
 * there is no usable store. Safe with out == NULL (returns -1). On -1 the
 * contents of *out are set to the fresh-install defaults, so a caller can use
 * the struct either way.
 */
int bounce_persistence_load(BouncePersistenceRecords *out);

/*
 * Write the records, creating the directory when needed.
 *
 * Returns 0 on success and -1 on failure. A failure here is never fatal: the
 * caller keeps playing and simply loses the update, which is what Java's
 * WriteToStore() does with its empty catch block (BounceGame.java:413-414).
 * Writing is whole-file: the two records live in one container, so there is no
 * partial-record case to reconcile.
 */
int bounce_persistence_save(const BouncePersistenceRecords *in);

/*
 * G-R3-P1 -- the optional Record 3.
 *
 * Record 3 ABSENCE IS NORMAL, not an error. A fresh install and a legacy
 * version-1 save both mean "no snapshot", exactly like Java's
 * getNumRecords() != 3 branch (BounceGame.java:276-279) which creates three
 * empty records and reads nothing. It is never a fatal load, never a
 * gameplay error, and never an implicit Continue or New Game -- there is no
 * Continue wiring in this step at all.
 *
 * *present is set to false whenever no usable Record 3 exists, including when
 * the container itself is missing or malformed, so a caller can never mistake
 * "absent" for "loaded". Returns 0 whenever the outcome is well-defined,
 * whether or not the record was there; -1 only for a NULL argument.
 */
int bounce_persistence_load_record3(BouncePersistenceRecord3 *out, bool *present);

/*
 * Write Record 3, replacing any previous copy (Java's setRecord, :411).
 *
 * An existing Record 3 that the caller does not supply is NOT preserved: the
 * contract is that this function owns record 3, exactly as Java's
 * WriteToStore(3) replaces the record outright. Callers that must leave it
 * alone should use bounce_persistence_save(), which never touches it.
 */
int bounce_persistence_save_record3(const BouncePersistenceRecord3 *in);

/*
 * Write records 1 and 2 while LEAVING Record 3 untouched.
 *
 * This is what Java's WriteToStore() does (BounceGame.java:417-428): the
 * no-argument form writes only MaxLevels and HighScore, which is why
 * e.java:269 and e.java:319 -- the game-over arm and the `e` completion block
 * -- never produce a snapshot. The only Record 3 writer in the Java tree is
 * Bounce.destroyApp (Bounce.java:25).
 *
 * A legacy version-1 file, or a new-format file with no Record 3, is rewritten
 * in the new format with records 1 and 2 only, which keeps the "absent" state
 * representable. A Record 3 that IS present is read back and rewritten, so it
 * survives; one that is absent or unreadable stays absent.
 */
int bounce_persistence_save_records_12_preserving_record3(
    const BouncePersistenceRecords *in);

/*
 * STEP 38-RESET -- delete Record 3, the Continue snapshot.
 *
 * Record 3's ABSENCE is a normal, already-handled state: the loader reports
 * present == false and the container validates without it, because a fresh install
 * and a legacy version-1 save both have none. This is how a caller reaches that state
 * deliberately. Writing a zeroed record instead would NOT do, because the record
 * would still be present and every reader would have to treat an empty payload as
 * absent.
 *
 * RECORDS 1, 2, 4 AND 5 ARE PRESERVED. This function removes exactly one record and
 * touches nothing else, like every other writer in this module, so it composes with
 * a caller that has just written new values for them.
 *
 * Returns 0 on success and -1 on failure, and a failure is never fatal: the record
 * may survive, which leaves a stale Continue rather than a broken game.
 */
int bounce_persistence_delete_record3(void);

/*
 * TUGAS 2b -- read the persisted color-profile index.
 *
 * Returns 0 whenever the answer is well-defined, which includes "there is no
 * such record": *out_index is then 0, the default profile, so a caller never has
 * to distinguish absent from failed. Returns -1 only for a NULL argument. This
 * mirrors bounce_persistence_load_record3()'s "absence is normal, not an error"
 * contract rather than bounce_persistence_load()'s stricter one, because a
 * missing theme is not a missing store.
 *
 * A negative stored value is refused and reported as 0, the same defence
 * bounce_persistence_load() applies to MaxLevels and HighScore.
 */
int bounce_persistence_load_theme_index(int32_t *out_index);

/*
 * TUGAS 2b -- write the color-profile index as record 4, preserving records 1,
 * 2 and 3.
 *
 * Records 1 and 2 are read back and rewritten unchanged, and so is Record 3,
 * exactly as bounce_persistence_save_records_12_preserving_record3() already
 * preserves the snapshot (Java's WriteToStore() at BounceGame.java:417-428 writes
 * only the two scalars and leaves record 3 alone, because only
 * Bounce.destroyApp at Bounce.java:25 writes it). A partial update must never
 * cost the player a snapshot.
 *
 * A failure here is never fatal, exactly as for every other write in this
 * module: Java's WriteToStore() swallows its IOException
 * (BounceGame.java:413-414) and the game carries on.
 */
int bounce_persistence_save_theme_index(int32_t index);

/*
 * NATIVE ADDITION -- read the numeric-keypad setting. Returns 0 whenever the
 * answer is well-defined, including "there is no such record", which means OFF.
 * Returns -1 only for a NULL argument.
 */
int bounce_persistence_load_t9_enabled(int32_t *out_enabled);

/*
 * NATIVE ADDITION -- write the numeric-keypad setting as record 5, preserving
 * records 1, 2, 3 AND 4. Every writer in this module owns one record and must
 * leave the others alone, so a theme change cannot reset the keypad setting and a
 * keypad change cannot reset the color profile.
 */
int bounce_persistence_save_t9_enabled(int32_t enabled);

/*
 * The serialised Record 3 length for the given counts, which Java fixes at
 * 79 + 5*tile_delta_count + 8*dyn_thorn_count bytes. Returns 0 when either
 * count exceeds what Java can represent, so a caller never allocates from an
 * unchecked value.
 */
uint32_t bounce_persistence_record3_payload_size(uint32_t tile_delta_count,
    uint32_t dyn_thorn_count);

/*
 * The resolved absolute path, or NULL when neither XDG_DATA_HOME nor HOME is
 * set. Exposed for diagnostics and for the verification routine; not used by
 * the gameplay path.
 */
const char *bounce_persistence_path(void);

/*
 * Format check without touching the filesystem: returns 1 when the buffer has
 * the expected magic, version and length. Used by the self-test and by the
 * loader itself.
 */
int bounce_persistence_is_well_formed(const unsigned char *bytes, uint32_t size);

/* Self-test. Returns 0 on success, -1 on failure. Never writes outside tmp. */
int bounce_persistence_verify(void);

#ifdef __cplusplus
}
#endif

#endif /* BOUNCE_NATIVE_APP_PERSISTENCE_H */
