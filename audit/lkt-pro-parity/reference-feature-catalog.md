# Reference Feature Catalog — LKT Pro Mod

**Package:** `com.lktpro.boun` | **Total:** 4,263 lines across 9 Java files  
**Provenance:** Decompiled with CFR 0.152 from `Bounce_mod_LKT_Pro_final.jar`

---

## 1. Startup / Initialization / Shutdown

### 1.1 MIDlet Lifecycle

| Feature | Evidence | Details |
|---|---|---|
| MIDlet entry point | `Bounce.java:14-38` | `class Bounce extends MIDlet` |
| Constructor | `Bounce.java:19-23` | Creates `BounceUI(this)` |
| `startApp()` | `Bounce.java:25-26` | Empty — startup handled by `BounceUI` constructor |
| `pauseApp()` | `Bounce.java:28-29` | Empty |
| `destroyApp(boolean)` | `Bounce.java:31-38` | Saves game data, stops timer, sets display to null, nulls UI |

### 1.2 UI Initialization

| Feature | Evidence | Details |
|---|---|---|
| Settings load | `BounceUI.java:101-116` | Loads `bounceSettingsRMS` (keyboard, fps, volume, language) |
| Language set | `BounceUI.java:105` | `Local.setLanguage(this.mLanguage)` |
| Game data load | `BounceUI.java:106` | `loadGameData()` — reads `bounceRMS` |
| Canvas creation | `BounceUI.java:107-108` | `new BounceCanvas(this, 1)` + `start()` |
| Display setup | `BounceUI.java:109-110` | `Display.getDisplay(this)` + `setCurrent(canvas)` |
| Menu items | `BounceUI.java:111-115` | 5 items: Continue, New Game, High Score, Instructions, Settings |

### 1.3 Canvas Initialization

| Feature | Evidence | Details |
|---|---|---|
| Sound loading | `BounceCanvas.java:65-67` | 3 sounds: up.wav, pickup.wav, pop.wav |
| Full-screen buffer | `BounceCanvas.java:69` | 128x128 image |
| Splash image | `BounceCanvas.java:72-78` | Loads `bouncesplash.png` |
| Timer start | `BounceCanvas.java:79` | `start()` — begins game loop |

### 1.4 Settings

| Feature | Evidence | Details |
|---|---|---|
| Settings RMS | `BounceUI.java:82` | `bounceSettingsRMS` |
| Keyboard type | `BounceUI.java:83` | 0=T9, 1=QWERTY |
| FPS | `BounceUI.java:89` | Default 25, range 10-60 |
| Volume | `BounceUI.java:90` | Default 100, range 0-100 |
| Language | `BounceUI.java:91` | 0=Auto, 1=Vietnamese |
| Settings load | `BounceUI.java:567-601` | Reads 4-byte record |
| Settings save | `BounceUI.java:603-618` | Writes 4-byte record |

---

## 2. Game State Machine

### 2.1 States

| State | Constant | Value | Evidence |
|---|---|---|---|
| `STATE_INIT` | `BounceConst.java:212` | 0 | Defined, unused |
| `STATE_READY` | `BounceConst.java:213` | 1 | Defined, unused |
| `STATE_START` | `BounceConst.java:255` | 2 | Initial `mState` |
| `STATE_GAME_OVER` | `BounceConst.java:256` | 3 | Set on game over |
| `STATE_GAME_STARTING` | `BounceConst.java:257` | 4 | Set on new game when `mBestLevel <= 1` |
| `STATE_GAME_PLAYING` | `BounceConst.java:254` | 1 | Set on game start |

### 2.2 Transitions

| Transition | Trigger | Evidence |
|---|---|---|
| START → GAME_PLAYING | `displayGame(true, level)` | `BounceUI.java:161-168` |
| GAME_PLAYING → GAME_STARTING | New game when `mBestLevel <= 1` | `BounceUI.java:260-262` |
| GAME_PLAYING → GAME_OVER (win) | `mLevelNum > 11` | `BounceCanvas.java:454-455` → `BounceUI.java:539-544` |
| GAME_PLAYING → GAME_OVER (lose) | `numLives < 0` | `BounceCanvas.java:368-371` → `BounceUI.java:539-544` |
| Any → GAME_PLAYING | Continue / level complete | `BounceUI.java:240-251, 284-285` |

### 2.3 Main Loop

`BounceCanvas.java:320-463` — `run()`:

1. If `mLoadLevelFlag` → `createNewLevel()` + repaint
2. Else if splash active → advance splash timer (30 frames per splash, 2 splashes)
3. Else → gameplay frame:
   - Decrement `mLevelDisCntr` (level number display, 120 frames)
   - If ball Y in `[0,96]` → `cleanBuffer(true)` + `mBall.update()` + `testScroll()`
   - Else → `screenFlip()`
   - If `ballState==1` (dead) → recreate buffer at respawn point
   - `updateMovingSpikeObj()` if moving objects exist
   - Check all rings collected → `mOpenExitFlag = true`
   - Check exit tile overlap → `openExit()` (animated opening, 4px/frame)
   - Update `bonusCntrValue` (max of speed/grav/jump bonuses)
   - `scrollBuffer()` + `paint2Buffer()` + `repaint()`
   - If `mLeaveGame` → `mLevelNum++`, `add2Score(5000)`, check win, `displayLevelComplete()`

---

## 3. Player (Ball)

### 3.1 Physics

| Feature | Evidence | Details |
|---|---|---|
| Position | `Ball.java:19-20` | `xPos, yPos` (buffer coords), `globalBallX/Y` (tile coords) |
| Speed | `Ball.java:24-25` | `xSpeed, ySpeed` — clamped to ±150 |
| Direction mask | `Ball.java:26` | Bitmask: 8=UP, 4=DOWN, 2=RIGHT, 1=LEFT |
| Horizontal accel | `Ball.java:1171-1179` | ±6 accel, ±4 friction, max 50 (100 with speed bonus) |
| Gravity | `Ball.java:1031-1075` | Normal: acc=4, max=80; Large: acc=3, max=38; Water normal: acc=6, max=42; Water large: acc=-2, max=-30 |
| Gravity flip | `Ball.java:1064-1074` | Inverts gravity, sets `isUpsideDown` |
| Jump | `Ball.java:1183-1186` | `ySpeed = -67 + jumpOffset` when grounded + UP direction |
| Jump offset | `Ball.java:1077-1081, 1180-1182` | -80 with jump bonus, ±10 rubber bounce, ±5 large ball decay |
| Ramp movement | `Ball.java:1113-1124, 1196-1209` | Diagonal slide when `mCDRampFlag` |
| Rubber bounce | `Ball.java:1128-1131` | `jumpOffset += ±10` on rubber tile + UP |
| Screen wrap | `Ball.java:1213-1224` | Wrap at `x > 156+size` or `x-size < 0` |

### 3.2 Ball States

| State | Constant | Value | Evidence |
|---|---|---|---|
| `BALL_STATE_NORMAL` | `BounceConst.java:23` | 0 | Normal play |
| `BALL_STATE_DEAD` | `BounceConst.java:24` | 1 | Waiting respawn |
| `BALL_STATE_POPPED` | `BounceConst.java:25` | 2 | Pop animation |

- Pop: `Ball.java:515-527` — 5-frame pop animation, decrements `numLives`, clears all bonuses, plays pop sound (unless invincible)
- Respawn: `BounceCanvas.java:367-381` — recreates buffer at respawn point

### 3.3 Ball Sizes

| Size | Constant | Image | Evidence |
|---|---|---|---|
| Normal (12px) | `BounceConst.java:17` | `mTileImages[47]` | `Ball.java:47` |
| Enlarged (16px) | `BounceConst.java:21` | `mTileImages[49]` | `Ball.java:48` |
| Popped (12px) | `BounceConst.java:19` | `mTileImages[48]` | `Ball.java:46` |

- Enlarge: `Ball.java:462-501` — pushes ball out of walls in 8 directions
- Shrink: `Ball.java:503-513` — simple push down/up

### 3.4 Power-ups

| Bonus | Tile IDs | Duration | Counter | Effect | Evidence |
|---|---|---|---|---|---|
| Speed | 38 | 300 | `speedBonusCntr` | Max horz speed 50→100 | `Ball.java:642,800,802` |
| Gravity | 47-50 | 300 | `gravBonusCntr` | Upside-down gravity | `Ball.java:1064-1074` |
| Jump | 51-54 | 300 | `jumpBonusCntr` | Jump strength -67→-80 | `Ball.java:1180-1182` |
| Life | 29 | — | — | +1 life (max 5), +1000 pts | `Ball.java:933-934` |

### 3.5 Rendering

- `Ball.java:1228-1241` — `paint()`: draws ball image with wrap-around
- `Ball.java:1243-1276` — `dirtyTiles()`: marks tiles with `0x80` flag for redraw

### 3.6 Static Mask Data

- `TRI_TILE_DATA` (`Ball.java:41-142`): 12x12 triangle masks (8 directions)
- `SMALL_BALL_DATA` (`Ball.java:42-228`): 12x12 ball pixel mask
- `LARGE_BALL_DATA` (`Ball.java:43-368`): 16x16 ball pixel mask

---

## 4. Collision System

### 4.1 Collision Methods

| Method | Evidence | Purpose |
|---|---|---|
| `collisionDetection(nextX, nextY)` | `Ball.java:424-460` | Main entry — computes tile range, handles wrap/divisor |
| `squareCollide(row, col)` | `Ball.java:580-623` | Pixel-perfect square tile collision using ball mask |
| `triangleCollide(row, col, type)` | `Ball.java:625-693` | Triangle tile collision + `redirectBall()` |
| `thinCollide(row, col, type)` | `Ball.java:695-731` | Thin tile (4px inner) collision |
| `edgeCollide(row, col, type)` | `Ball.java:733-773` | Edge/ramp tile collision |
| `testTile(row, col, state)` | `Ball.java:775-1016` | Master tile handler — dispatches by tile ID |
| `redirectBall(type)` | `Ball.java:535-578` | Triangle bounce redirect (8 types) |

### 4.2 Tile Interaction Rules

| Tile ID(s) | Behavior | Evidence |
|---|---|---|
| 1 | Solid brick — square collide, sets `mCDRampFlag` | `Ball.java:437-444` |
| 2 | Rubber brick — square collide, sets `mCDRubberFlag` + `mCDRampFlag` | `Ball.java:448-454` |
| 3-6 | Spikes — thin collide → `popBall()` | `Ball.java:474-478` |
| 7 | Respawn gem — +200 pts, sets respawn point, plays pickup | `Ball.java:810-816` |
| 9 | Exit tile — if open (`mOpenFlag`), sets `mLeaveGame`; else blocks | `Ball.java:601-602` |
| 10 | Moving spike — rect collide with spike position → `popBall()` | `Ball.java:463-473` |
| 13-16 | Hoops (active) — thin collide, `addRing()`, animates to inactive | `Ball.java:525-534` |
| 17-20 | Hoops (inactive large) — thin collide + edge collide, blocks | `Ball.java:535-578` |
| 21-24 | Hoops (large active) — same as 13-16 | `Ball.java:525-534` |
| 25, 27, 28 | Large hoop edges — edge collide only | `Ball.java:535-578` |
| 29 | Extra life gem — +1000 pts, +1 life (max 5) | `Ball.java:933-934` |
| 30-33 | Triangles (normal) — triangle collide, ramp | `Ball.java:625-693` |
| 34-37 | Rubber triangles — triangle collide, rubber + ramp | `Ball.java:625-693` |
| 38 | Speed boost — sets `speedBonusCntr = 300` | `Ball.java:642` |
| 39-42 | Deflators — shrink ball (if large), blocks | `Ball.java:503-513` |
| 43-46 | Inflators — enlarge ball (if small), blocks | `Ball.java:462-501` |
| 47-50 | Gravity flip — sets `gravBonusCntr = 300` | `Ball.java:1064-1074` |
| 51-54 | Jump boost — sets `jumpBonusCntr = 300` | `Ball.java:1180-1182` |

### 4.3 Flags

- `0x80` (`TILE_DIRTY`): tile needs redraw
- `0x40` (`ID_WATER_FLAG`): water tile
- `0x10` (bit 4): marks hoop inactive state

---

## 5. Enemies / Hazards

### 5.1 Static Spikes (Tiles 3-6)

- 4 orientations: floor, left wall, ceiling, right wall
- `thinCollide()` with 4px inner rect
- Instant death (`popBall()`)

### 5.2 Moving Spikes (Tile 10)

| Feature | Evidence | Details |
|---|---|---|
| Object creation | `TileCanvas.java:209-234` | `createMovingObj(DataInputStream)` |
| Per-object data | `TileCanvas.java:67-71` | `mMOTopLeft`, `mMOBotRight`, `mMODirection`, `mMOOffset` |
| Update logic | `TileCanvas.java:259-315` | `updateMovingSpikeObj()` — linear patrol |
| Rendering | `TileCanvas.java:404-421` | 24x24 spike image, clipped to tile |
| Collision | `Ball.java:829-835` | Rect collide with spike's current position |
| Dirty tracking | `TileCanvas.java:300-312` | Marks old + new tiles dirty |
| Save/restore | `BounceCanvas.java:691-703` | `resetSpikes()` |

### 5.3 Spike Image Composition

`TileCanvas.java:228-233`: 24x24 image composed of 4 transformed copies of `mTileImages[46]`

---

## 6. Levels

### 6.1 Level Loading

| Feature | Evidence | Details |
|---|---|---|
| File format | Binary: `/levels/J2MElvl.NNN` | 3-digit zero-padded |
| Loader | `TileCanvas.java:122-170` | `loadLevel(int levelIndex)` |
| Header fields | `TileCanvas.java:138-147` | `mStartCol, mStartRow, ballSizeFlag, mExitPosX, mExitPosY, mTotalNumRings, mTileMapWidth, mTileMapHeight` |
| Tile data | `TileCanvas.java:148-157` | Raw bytes → `short[mTileMapHeight][mTileMapWidth]` |
| Moving objects | `TileCanvas.java:158-161` | `mNumMoveObj` + `createMovingObj()` if > 0 |
| Max levels | `BounceConst.java:202` | `MAX_LEVEL = 11` |

### 6.2 Level Files

`J2MElvl.001` through `J2MElvl.011` — all 11 levels present

### 6.3 Level Transitions

| Feature | Evidence | Details |
|---|---|---|
| Level complete | `BounceCanvas.java:447-461` | `mLeaveGame=true`, `mLevelNum++`, +5000 pts |
| Win (all 11 levels) | `BounceCanvas.java:454-455` | `mUI.gameOver(true)` |
| Level display timer | `BounceCanvas.java:119` | `mLevelDisCntr = 120` frames |
| Level name shown | `BounceCanvas.java:312-317` | `TEXT_FONT_LARGE` centered |

### 6.4 Checkpoints

- Respawn gems (tile 7): `Ball.java:810-816` — updates respawn point, +200 pts
- Save on exit: `Bounce.java:33` — `saveGameData(3)` in `destroyApp()`
- Full state save: `BounceUI.java:442-503` — position, speed, bonuses, tiles, spikes

### 6.5 Exit Tile

| Feature | Evidence | Details |
|---|---|---|
| Creation | `TileCanvas.java:964-972` | `createExitTileObject(col, row, image)` |
| Image | `TileCanvas.java:813-829` | `createExitImage()`: 24x48, 4 colored strips + 4 tile transforms |
| Open animation | `TileCanvas.java:979-986` | `openExit()`: `mImageOffset += 4`, `mOpenFlag = true` at 24 |
| Exit check | `BounceCanvas.java:388-428` | Wrap-aware exit position comparison |

---

## 7. Scoring / Lives / Progression

### 7.1 Scoring

| Event | Points | Evidence |
|---|---|---|
| Ring collected | 500 | `Ball.java:530` |
| Respawn gem | 200 | `Ball.java:811` |
| Extra life gem | 1000 | `Ball.java:933` |
| Level exit | 5000 | `BounceCanvas.java:452` |

### 7.2 Lives

| Feature | Evidence | Details |
|---|---|---|
| Initial | 3 | `BounceConst.java:261` |
| Max | 5 | `Ball.java:934` |
| Decrement | `Ball.java:520` | `--numLives` on pop |
| Game over | `BounceCanvas.java:368` | `if (numLives < 0)` |
| UI display | `BounceCanvas.java:238-241` | Life icons at `y=99` |

### 7.3 High Scores

| Feature | Evidence | Details |
|---|---|---|
| Best score | `BounceUI.java:48` | `mBestScore` |
| Best level | `BounceUI.java:47` | `mBestLevel` |
| New high score flag | `BounceUI.java:49` | `mNewBestScore` |
| Check & save | `BounceUI.java:526-537` | `checkData()` |
| Display | `BounceUI.java:170-176` | `displayHighScore()` |

### 7.4 Rings

- `numRings` / `mTotalNumRings` tracked in `BounceCanvas` / `TileCanvas`
- UI: ring icons at `y=112` (`BounceCanvas.java:242-246`)
- All collected → `mOpenExitFlag = true` (`BounceCanvas.java:385-387`)

---

## 8. Input

### 8.1 Key Mappings

| Game Action | Key Code | Direction | Evidence |
|---|---|---|---|
| UP (1) | — | `setDirection(8)` | `BounceCanvas.java:574-579` |
| LEFT (2) | — | `setDirection(1)` | `BounceCanvas.java:574-579` |
| RIGHT (5) | — | `setDirection(2)` | `BounceCanvas.java:574-579` |
| DOWN/FIRE (6) | — | `setDirection(4)` | `BounceCanvas.java:574-579` |
| GAME_A (8) | — | — | `BounceCanvas.java:574-579` |

### 8.2 QWERTY Keyboard Mode (`mKeyboardType == 1`)

| Key | Mapped To | Evidence |
|---|---|---|
| `r/R (114/82)` | `49` (keypad 1) | `BounceCanvas.java:472-473` |
| `t/T (116/84)` | `50` (keypad 2) + `setDirection(8)` UP | `BounceCanvas.java:474-475, 497-499` |
| `y/Y (121/89)` | `51` (keypad 3) | `BounceCanvas.java:476-477` |
| `f/F (102/70)` | `52` (keypad 4) + `setDirection(1)` LEFT | `BounceCanvas.java:478-479, 500-502` |
| `g/G (103/71)` | `53` (keypad 5) | `BounceCanvas.java:480-481` |
| `h/H (104/72)` | `54` (keypad 6) + `setDirection(2)` RIGHT | `BounceCanvas.java:482-483, 503-505` |
| `v/V (118/86)` | `55` (keypad 7) | `BounceCanvas.java:484-485` |
| `b/B (98/66)` | `56` (keypad 8) + `setDirection(4)` DOWN | `BounceCanvas.java:486-487, 506-508` |
| `n/N (110/78)` | `57` (keypad 9) | `BounceCanvas.java:488-489` |
| `u/U (117/85)` | `42` (*) | `BounceCanvas.java:490-491` |
| `j/J (106/74)` | `35` (#) | `BounceCanvas.java:492-493` |
| `m/M (109/77)` | `48` (0) | `BounceCanvas.java:494-495` |

### 8.3 Edge Cases

| Feature | Evidence | Details |
|---|---|---|
| Splash skip | `BounceCanvas.java:466-467` | Any key sets `mSplashTimer = 31` |
| Incoming call | `BounceCanvas.java:681-689` | `hideNotify()` resets directions, shows menu |

---

## 9. UI / Menus

### 9.1 Menus

| Menu | Method | Evidence |
|---|---|---|
| Main menu | `displayMainMenu()` | `BounceUI.java:118-144` |
| New game (level select) | `displayNewGameMenu()` | `BounceUI.java:146-159` |
| High score | `displayHighScore()` | `BounceUI.java:170-176` |
| Instructions | `displayInstructions()` | `BounceUI.java:178-190` |
| Settings | `displaySettingsMenu()` | `BounceUI.java:546-565` |
| Game over | `displayGameOver(isWin)` | `BounceUI.java:192-213` |
| Level complete | `displayLevelComplete()` | `BounceUI.java:215-229` |

### 9.2 Main Menu Items

| Index | Label Key | Action | Evidence |
|---|---|---|---|
| 0 | `QTJ_BOUN_CONTINUE` (8) | Continue game / resume | `BounceUI.java:111, 123-124` |
| 1 | `QTJ_BOUN_NEW_GAME` (16) | New game / level select | `BounceUI.java:112` |
| 2 | `QTJ_BOUN_HIGH_SCORES` (12) | Show high score | `BounceUI.java:113` |
| 3 | `QTJ_BOUN_INSTRUCTIONS` (13) | Show instructions | `BounceUI.java:114` |
| 4 | `QTJ_BOUN_SETTINGS` (21) | Open settings | `BounceUI.java:115` |

### 9.3 Settings Menu

| Setting | Widget | Evidence |
|---|---|---|
| Language | `ChoiceGroup` (Auto, VI) | `BounceUI.java:548-549` |
| Keyboard | `ChoiceGroup` (T9, QWERTY) | `BounceUI.java:550-551` |
| FPS | `TextField` (10-60, default 25) | `BounceUI.java:552` |
| Volume | `Gauge` (0-100, default 100) | `BounceUI.java:553` |

### 9.4 HUD

- Background bar: color `545706`, rect `(0, 97, 128, 32)` — `BounceCanvas.java:234-237`
- Life icons: `mUILife` at `(5 + i*width, 99)` — `BounceCanvas.java:238-241`
- Ring icons: `mUIRing` at `(5 + j*(width-4), 112)` — `BounceCanvas.java:242-246`
- Score: `TEXT_FONT` at `(64, 100)` — `BounceCanvas.java:247-250`
- Bonus bar: color `16750611`, width = `bonusCntrValue/30` — `BounceCanvas.java:251-255`

### 9.5 Splash Screens

| Index | Image | Duration | Evidence |
|---|---|---|---|
| 0 | `/icons/nokiagames.png` | 30 frames | `BounceCanvas.java:56, 72-78` |
| 1 | `/icons/bouncesplash.png` | 30 frames | `BounceCanvas.java:56, 72-78` |

---

## 10. Audio

### 10.1 Sound Effects

| Sound | File | Usage | Evidence |
|---|---|---|---|
| `up.wav` | `/sounds/up.wav` | Hoop collected | `BounceCanvas.java:65` |
| `pickup.wav` | `/sounds/pickup.wav` | Gem/power-up collected | `BounceCanvas.java:66` |
| `pop.wav` | `/sounds/pop.wav` | Ball popped (death) | `BounceCanvas.java:67` |

- `loadSound()`: `BounceCanvas.java:650-662` — creates `Manager.createPlayer()`, `realize()`, `prefetch()`
- `playAudio()`: `BounceCanvas.java:664-679` — sets `VolumeControl` level, stops if playing, resets to 0, starts
- Volume: `mUI.mVolume` (0-100), set in settings

### 10.2 Music

No background music — only 3 SFX files exist in `/sounds/`

---

## 11. Persistence

### 11.1 RMS Record Stores

| Store | Name | Records | Evidence |
|---|---|---|---|
| Game data | `bounceRMS` | 3 (level, score, save) | `BounceUI.java:75` |
| Settings | `bounceSettingsRMS` | 1 | `BounceUI.java:82` |

### 11.2 Game Data Records

| Record ID | Content | Length | Evidence |
|---|---|---|---|
| 1 | `mBestLevel` (byte) | 1 | `BounceUI.java:342` |
| 2 | `mBestScore` (int) | 4 | `BounceUI.java:343` |
| 3 | Full game state | 255 | `BounceUI.java:344` |

### 11.3 Save Game Format (Record 3)

`BounceUI.java:442-503` — `writeSaveGame()`:

| Field | Type | Bytes |
|---|---|---|
| `mSavedValid` | byte | 1 (-6=valid, -16=invalid, -70=next level) |
| `mSavedLives` | byte | 1 |
| `mSavedRings` | byte | 1 |
| `mSavedLevel` | byte | 1 |
| `mSavedSize` | byte | 1 |
| `mSavedScore` | int | 4 |
| `mSavedTileX` | int | 4 |
| `mSavedTileY` | int | 4 |
| `mSavedGlobalBallX` | int | 4 |
| `mSavedGlobalBallY` | int | 4 |
| `mSavedXSpeed` | int | 4 |
| `mSavedYSpeed` | int | 4 |
| `mSavedXPos` | int | 4 |
| `mSavedYPos` | int | 4 |
| `mSavedRespawnX` | int | 4 |
| `mSavedRespawnY` | int | 4 |
| `mSavedSpeedBonus` | int | 4 |
| `mSavedGravBonus` | int | 4 |
| `mSavedJumpBonus` | int | 4 |
| Special tile count | byte | 1 |
| Special tiles | `count * 5` | variable |
| `mSavedSpikeCount` | byte | 1 |
| Spike data | `count * 8` | variable |

### 11.4 Settings Record

`BounceUI.java:607` — 4 bytes: `[keyboardType, fps, volume, language]`

### 11.5 Save Triggers

| Trigger | Evidence |
|---|---|
| App destroy | `Bounce.java:33` — `saveGameData(3)` |
| Level complete | `BounceCanvas.java:453` — `checkData()` → `saveGameData(1/2)` |
| Settings change | `BounceUI.java:309` — `saveSettingsData()` |
| Game over | `BounceUI.java:541` — `mSavedValid = -16` (clears save) |

---

## 12. Rendering

### 12.1 Buffer System

| Buffer | Size | Evidence |
|---|---|---|
| Game buffer | 156x96 | `TileCanvas.java:96` |
| Full-screen buffer | 128x128 | `BounceCanvas.java:69` |
| Tile image | 12x12 | `TileCanvas.java:97` |
| Exit tile | 24x24 | `TileCanvas.java:968` |
| Large ball | 16x16 | `TileCanvas.java:791` |
| Exit image | 24x48 | `TileCanvas.java:814` |

### 12.2 Scaling

- `scaleImageNearest()`: `BounceCanvas.java:258-290` — nearest-neighbor scaling with cached pixel arrays

### 12.3 Scrolling

| Function | Evidence | Description |
|---|---|---|
| `createBufferFocused()` | `BounceCanvas.java:126-158` | Initial buffer + scroll setup |
| `screenFlip()` | `BounceCanvas.java:160-201` | Vertical/horizontal screen transition |
| `scrollBuffer()` | `BounceCanvas.java:696-771` | Incremental horizontal scroll (12px steps) |
| `testScroll()` | `BounceCanvas.java:773-788` | Scroll trigger when ball near screen edge |
| `cleanBuffer()` | `BounceCanvas.java:671-694` | Redraw dirty tiles (0x80 flag) |

### 12.4 Dirty Tile System

- `Ball.dirtyTiles()` (`Ball.java:1243-1276`): marks tiles with `0x80`
- `TileCanvas.cleanBuffer()` (`TileCanvas.java:671-694`): clears `0x80`, redraws
- `TileCanvas.drawTile()` (`TileCanvas.java:328-651`): clears `0x80` before drawing

### 12.5 Sprite Loading

| Method | Evidence | Description |
|---|---|---|
| `loadTileImages()` | `TileCanvas.java:853-928` | Tries individual PNGs first, falls back to sprite sheet |
| `loadSpriteTiles()` | `TileCanvas.java:831-851` | Loads 67 Q*.png + A.png (life) + D.png (ring) |
| Fallback | `TileCanvas.java:857-927` | Extracts from `/icons/objects_nm.png` sprite sheet |
| `manipulateImage()` | `TileCanvas.java:172-207` | 6 transform modes |

### 12.6 Game Loop Timing

- `GameTimer.java:19-24`: delay = `1000 / mFps` ms (default 25fps = 40ms)

---

## 13. Debug / Cheat Features

### 13.1 Debug Flags (All Disabled)

| Flag | File | Line | Default |
|---|---|---|---|
| `Bounce.DEBUG` | `Bounce.java` | 17 | `false` |
| `BounceConst.CD_DEBUG` | `BounceConst.java` | 7 | `false` |
| `BounceConst.KJM_DEBUG` | `BounceConst.java` | 8 | `false` |
| `BounceConst.KJM2_DEBUG` | `BounceConst.java` | 9 | `false` |
| `BounceConst.SCR_DEBUG` | `BounceConst.java` | 56 | `false` |
| `BounceConst.MEM_FREE_DISPLAY` | `BounceConst.java` | 214 | `false` |
| `BounceConst.FPS_DISPLAY` | `BounceConst.java` | 215 | `false` |
| `Profile.PROFILING` | `Profile.java` | 7 | `false` |
| `Ball.mDebugCD` | `Ball.java` | 18 | `false` |

### 13.2 Cheat Code System

**Activation sequence** (key codes): `7` → `8` → `9` → `8` → `9` (or `7` → `8` → `*` → `#` → `8` → `9`)

| Step | Key | `mCheatSeq` | Action | Evidence |
|---|---|---|---|---|
| 1 | 7 (55) | 0→1 | — | `BounceCanvas.java:537-543` |
| 2 | 8 (56) | 1→2 | — | `BounceCanvas.java:537-543` |
| 3 | 9 (57) | 2→3 | — | `BounceCanvas.java:559-563` |
| 4 | 8 (56) | 3→4 | — | `BounceCanvas.java:559-563` |
| 5 | 9 (57) | 4→5 | `mCheat = true`, plays pop sound | `BounceCanvas.java:564-568` |

### 13.3 Cheat Effects (Only when `mCheat == true`)

| Key | Effect | Evidence |
|---|---|---|
| `#` (35) | `gravBonusCntr = 300` (gravity flip) | `BounceCanvas.java:519-521` |
| `1` (49) | Previous level (`--mLevelNum`, wrap to 11) | `BounceCanvas.java:523-528` |
| `3` (51) | Next level (`++mLevelNum`, wrap to 1) | `BounceCanvas.java:530-535` |
| `0` (48) | Skip level (`mLeaveGame = true`) | `BounceCanvas.java:595-596` |

### 13.4 Invincibility Cheat

- `mInvincible` flag (`BounceCanvas.java:54`)
- Activated by: `7` → `8` → `*` (42) → `#` (35) → `8` → `*` (42)
- Effect: `popBall()` checks `mInvincible` (`Ball.java:516`) — no death

### 13.5 Profiling

- 10 static timing fields for: `cleanBuffer`, `drawTile`, `paint2Buffer`, `run`, `paint`, `testTile`, `update`, `collisionDetection`
- `PROFILING = false` — all instrumentation disabled

---

## 14. Mod-Specific Features (LKT Pro vs Standard Bounce)

### 14.1 Package & Naming

- Package renamed: `com.lktpro.boun` (mod) vs `com.nokia.mid.appl.boun` (original)
- All classes renamed to human-readable: `Ball`, `Bounce`, `BounceCanvas`, `TileCanvas`, etc.

### 14.2 Language System

- Vietnamese support (`lang.vi-VN`) — added by mod
- `Local.java`: full localization framework with 30 strings
- Language selection in settings (Auto/VI)

### 14.3 QWERTY Keyboard Support

- `mKeyboardType` setting: 0=T9 (standard), 1=QWERTY
- Full QWERTY→keypad mapping (`BounceCanvas.java:471-508, 606-619`)

### 14.4 Settings Menu

- Language selection
- Keyboard type selection
- FPS adjustment (10-60, default 25)
- Volume control (0-100)
- Persisted in separate `bounceSettingsRMS`

### 14.5 Sprite System

- 67 individual `Q*.png` sprite files + `A.png` (life) + `D.png` (ring) + `L.png`
- Fallback to `/icons/objects_nm.png` sprite sheet if individual files missing

### 14.6 Save System

- Full state save/restore (position, speed, bonuses, tile states, spike positions)
- 3-slot RMS: best level, best score, game state
- Save on app destroy
- Continue from exact position

### 14.7 Rendering

- Full-screen scaling (nearest-neighbor) for modern screens
- 128x128 internal buffer scaled to device resolution
- Dark theme (water) rendering for underwater tiles

---

## 15. Resources Summary

| Directory | Contents |
|---|---|
| `resources/sprites/` | 67 Q*.png (12x12 tiles), A.png (life), D.png (ring), L.png |
| `resources/sounds/` | up.wav, pickup.wav, pop.wav |
| `resources/levels/` | J2MElvl.001 – J2MElvl.011 (11 levels) |
| `resources/icons/` | icon.png, nokiagames.png, bouncesplash.png |
| `resources/META-INF/` | MANIFEST.MF |
| `resources/` | lang.xx (default), lang.vi-VN (Vietnamese) |

---

## 16. Localization Keys (30 strings)

| ID | Constant | Purpose |
|---|---|---|
| 0-5 | `QHJ_BOUN_INSTRUCTIONS_PART_1-6` | Instructions paragraphs |
| 6 | `QTJ_BOUN_BACK` | Back button |
| 7 | `QTJ_BOUN_CONGRATULATIONS` | Win message |
| 8 | `QTJ_BOUN_CONTINUE` | Continue menu item |
| 9 | `QTJ_BOUN_EXIT` | Exit |
| 10 | `QTJ_BOUN_GAME_NAME` | "Bounce" title |
| 11 | `QTJ_BOUN_GAME_OVER` | Game over text |
| 12 | `QTJ_BOUN_HIGH_SCORES` | High score menu |
| 13 | `QTJ_BOUN_INSTRUCTIONS` | Instructions menu |
| 14 | `QTJ_BOUN_LEVEL` | Level name (parameterized) |
| 15 | `QTJ_BOUN_LEVEL_COMPLETED` | Level complete (parameterized) |
| 16 | `QTJ_BOUN_NEW_GAME` | New game menu |
| 17 | `QTJ_BOUN_NEW_HIGH_SCORE` | New high score message |
| 18 | `QTJ_BOUN_NEXT` | Next (unused in code) |
| 19 | `QTJ_BOUN_OK` | OK button |
| 20 | `QTJ_BOUN_PAUSE` | Pause (unused in code) |
| 21 | `QTJ_BOUN_SETTINGS` | Settings menu |
| 22 | `QTJ_BOUN_KEYBOARD_TITLE` | Keyboard settings title |
| 23 | `QTJ_BOUN_KEYBOARD_T9` | T9 option |
| 24 | `QTJ_BOUN_KEYBOARD_QWERTY` | QWERTY option |
| 25 | `QTJ_BOUN_FPS` | FPS setting |
| 26 | `QTJ_BOUN_VOLUME` | Volume setting |
| 27 | `QTJ_BOUN_LANGUAGE` | Language setting title |
| 28 | `QTJ_BOUN_LANG_AUTO` | Auto language option |
| 29 | `QTJ_BOUN_LANG_VI` | Vietnamese language option |
