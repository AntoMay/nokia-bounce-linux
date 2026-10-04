⚠️ **Work in Progress**

> This project is an ongoing native Linux reconstruction of Nokia Bounce.
> It is **not yet complete** and still requires further implementation,
> verification, bug fixing, and refinement. The current build represents
> the present state of the reconstruction and should not be considered a
> final or complete release.

## Screenshots

![Title Screen](screenshots/title-screen.png)

![Main Menu](screenshots/main-menu.png)

![Level Select](screenshots/level-select.png)

![Gameplay on Level 1](screenshots/gameplay-level-1.png)

![Instructions](screenshots/instructions.png)

![High Score](screenshots/high-score.png)

![Settings Menu](screenshots/settings-menu.png)

![Language Selection](screenshots/language-selection.png)

![About](screenshots/about.png)

# Nokia Bounce Decomp

## Provenance

`src/main/java/com/nokia/mid/appl/boun/` contains **recovered, decompiled Java
reference code**. It is *not* original Nokia source. It was produced by
decompiling a recovered game artifact with JD-Core 1.1.3, so class and member
names are partly obfuscated and the recovered text is a reconstruction rather
than authored source.

The reconstructed native Linux implementation is the code under `native/`. The
Java tree is retained as the reference basis for that reconstruction; it is not
compiled, run, or shipped as part of the native program.

Provenance for every source consulted — the base repository this project derives
from, the external references, and the evidence hierarchy that ranks them — is
recorded in [`reverse/EXTERNAL-SOURCES-REGISTRY.md`](reverse/EXTERNAL-SOURCES-REGISTRY.md).

Attribution: the base repository and recovered Java/resources originate from
[rndtrash/nokia-bounce-decomp](https://github.com/rndtrash/nokia-bounce-decomp).

## Building

It builds with `gradlew build` but there is no point in that at the moment since I didn't implement any wrappers for MIDlet and Nokia APIs.

The native Linux slice builds with the project Makefile, from the repository root:

```
make -C native/app            # produces native/app/bounce_vertical_slice
make -C native/app check      # self-tests; rc=0 and an empty stderr
make -C native/app run        # runs the game
```

The binary must be run from the repository root, because it resolves level and
asset paths relative to it:

```
./native/app/bounce_vertical_slice
```

## Where the save lives

The store is one file, `records.bin`, resolved in this order:

| order | path | when |
| --- | --- | --- |
| 1 | `$BOUNCE_SAVE_DIR/bounce/records.bin` | the override is set |
| 2 | `$XDG_DATA_HOME/bounce/records.bin` | the standard |
| 3 | `$HOME/.local/share/bounce/records.bin` | the standard's default |

`--save=DIR` sets the same variable, before anything reads the store:

```
./native/app/bounce_vertical_slice --save=/tmp/bounce-scratch
./native/app/bounce_vertical_slice --save=~/my-bounce-checkout
```

`BOUNCE_SAVE_DIR` takes the place of `XDG_DATA_HOME`, not the place of the
file name, so the layout below it is the same either way. It exists for a
checkout that should carry its own progress and for a throwaway instance that
must not read the real save. It is not the default: a save outside
`XDG_DATA_HOME` is one a package uninstall removes without asking.

`--save=DIR` beats `BOUNCE_SAVE_DIR` when both are given, because a flag typed
at a terminal is the more specific of the two. `DIR` does not have to exist:
`DIR/bounce/` is created, including any missing parent directories.

`--save-path` prints where the store resolved to and exits. If no store is
there yet it writes one throwaway record first, so `--check` can confirm which
file a run would really write; if a store **is** already there it writes
nothing and only prints. Running it therefore cannot cost you your save.

`--save-plant` force-writes a known store. It exists for `--check`, which uses
it to plant a store containing a live session so it can prove the point above,
and it will overwrite whatever is at the resolved path.

```
./native/app/bounce_vertical_slice --save-path
./native/app/bounce_vertical_slice --save=/tmp/b --save-path
```

```
make -C native/app run                     # the normal location
XDG_DATA_HOME=/tmp/bounce-fresh   make -C native/app run                   # a clean install
```

## Debug tracker (native, optional)

`NBB_DEBUG` turns on the terminal debug tracker. It writes to **stderr** and
changes no gameplay state; the variable is read once at start-up and is ignored
by `--check`.

| `NBB_DEBUG` | level | output |
| --- | --- | --- |
| unset or `0` | disabled | nothing |
| `1` | events | new game, level start/complete, game over, game end, death, respawn, checkpoint, flow transitions, tile events |
| `2` | + state | score, lives, ball size, and the per-tick death `z`/`q` ladder |
| `3` | + verbose | one `[NBB-PLAYER]` state line per simulation tick (~25 lines/s) |

Any other value is treated as disabled. There is no log file and no extra
dependency; only the C runtime `FILE *` is used.

```
NBB_DEBUG=1 ./native/app/bounce_vertical_slice
NBB_DEBUG=2 ./native/app/bounce_vertical_slice
NBB_DEBUG=3 ./native/app/bounce_vertical_slice
```

Each line is prefixed so terminal output stays greppable, e.g.

```
[NBB-EVENT] DEATH tick=1057 lives=2
[NBB-DEATH] tick=1057 z=2 q=7
[NBB-EVENT] RESPAWN tick=1064 level=1 lives=2
```

`tick=N` is the existing `BounceGameTimer.tick_entry_count`, i.e. the real
simulation tick, not a separate counter. Every line is flushed as it is written,
so a crash still leaves the most recent events on the terminal. Level 3 is a
per-tick text stream and is materially more expensive than 0-2; keep it opt-in.

Implementation notes: `native/app/debug_tracker.h`.

## Tools

### extract_locales

`extract_locales.py` -- extracts locales from `lang.***` files.

#### Requirements

 * Python 3 with PIP

#### Usage

```
pip install -r requirements.txt
python extract_locales.py
```

Replace `pip` and `python` with `pip3` and `python3` if your system has Python 2 installed.