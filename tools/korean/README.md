# Local compatibility checks

These tools require your own ROMs. They do not download or upload game data.
Temporary binaries and extracted input files are kept outside tracked source.

## Korean ARM/C comparison

Install `unicorn` into a Python virtual environment, then run:

```sh
python tools/korean/verify_text.py /path/to/your/Korean.gba
```

The tool verifies the exact supported SHA-256 and compares production C text
functions against the original ARM routines in that ROM. It executes the
original decoder, font lookup, stylized lookup and width calculation; only
the GBA DMA copy hardware effect is supplied by the test harness.

Expected result: **18,825 ARM differential comparisons** and **3 explicit
Korean choice compatibility cases**, plus **4 popup width cases** (18,832
total). The choice correction
intentionally differs from the translation's retained, incompatible ARM
sentinel handling. This does not exercise the complete game or 3DS hardware.

## European text regression

```sh
python tools/korean/verify_eu_regression.py /path/to/your/European.gba
```

A ZIP containing one GBA ROM is also accepted. This compares **62,756 cases**
against `src/text.c` from upstream commit `c8fc571`. That Git object must be
available locally, as in a normal full clone. This is an upstream C behavior
comparison, not an independent ARM or whole-game validation.

## Synthetic profile and pointer bounds

```sh
cc -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined \
  -Iport tools/korean/profile_test.c -o /tmp/tmc-korean-profile-test
/tmp/tmc-korean-profile-test
```

This test contains no game data. CI runs the same assertions with its native
compiler. Local sanitizer runs check the short-input and ROM-boundary cases.

## Optional 3DS runtime probe

Normal builds exclude the runtime-probe source. Set the manual build workflow's
`runtime_probe` input to true, or set `TMC3DS_RUNTIME_PROBE=ON` for a local 3DS
build, to enable it. The artifact is named `tmc-3ds-runtime-probe`.

The probe saves `probe-top.bmp`, `probe-bottom.bmp` and `probe-state.txt` in the
application SD directory every 180 engine frames. For scripted input, write
`<3DS key bitmask> <frame count>` into `probe-keys.txt` in that directory. The
probe consumes that file. Captures and saves remain local and must not be
committed. Use a separate virtual SD for tests; preserve the player's live
session and saves.

Gameplay and interaction checks:

```sh
python3 tools/korean/run_host_tests.py --sanitize
python3 tools/korean/verify_gameplay.py /path/to/KOREAN.gba /path/to/EU.gba-or-zip
```

The host suite needs a native C compiler and no ROM. The local gameplay audit
requires the supported ROM revisions and writes no extracted assets. See
[coverage and limitations](../../docs/KOREAN_GAMEPLAY_AUDIT.md).
Opt-in diagnostic builds also accept one local `probe-touch.txt` containing
`x y` in native 320×240 coordinates, and log the actual touch handler's state
changes/rejections. Production packages exclude all probe hooks.

K0.6 diagnostic builds additionally accept `probe-dialog.txt` containing a
hex text ID, or `probe-room.txt` containing hex area/room IDs and decimal
room-relative x/y coordinates. These commands run only during ordinary
gameplay. Room commands are development fixtures and must only be used with
an isolated copy of the SD/save, never a player's live progress. Extended
state observations include dialogue status and native save/VRAM addresses for
local emulator checks. Normal CIA/3DSX builds contain none of these commands.

The host suite now runs 21 programs, including production dialogue/choice
decoding, Crenel menu-return graphics, real animated save results, and the
upstream 4,096-scene PPU parity suite. See [K0.6 details](../../docs/KOREAN_K06_FIXES.md).
