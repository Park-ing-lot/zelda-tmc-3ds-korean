# K0.6: dialogue choices, text overflow and menu-return graphics

## K0.7 follow-up: fixed Save/Continue option panels

Runtime input testing exposed a separate renderer: `SetPopupState` draws
the Save/Continue options directly into a fixed stone panel. Its original
224px canvas began at x=88; the Korean negative Save label extended past the
panel even after scrolling dialogue had been fixed. The player correctly
identified this remaining overflow while watching the emulator.

The supported ROM's complete Save and Continue option blocks measure 120px
and 107px, including the cursor substitution. K0.7 measures those lines and
centers each block within the panel's x=56..184 content area. Both choices
retain a common left/cursor column; the canvas ends within the panel and
contains every character. USA/EU and the separate Sleep layout are retained.
The production popup caller is tested for both choices and regions, and the
local ROM checks assert all four actual option-width measurements.

This is a compatibility update to EstebanPdN's native port. It does not create
or redistribute the game or Francine's Korean translation. Players supply
their own ROM. No save files or game assets are included in the tests.

## Reported behavior

The player reported these problems on a physical Korean 3DS: Korean dialogue
lost text at its right edge; returning from SELECT on Mount Crenel corrupted
graphics while gameplay continued; NPC choices displayed a saving failure
inside the dialogue even though ordinary menu saves worked.

## NPC choices: a text-routing error, not a failed EEPROM operation

The supplied translation uses `05 00` for a choice that returns its result
without chaining to another text entry. Its retained ARM decoder expects the
original `05 FF` sentinel. Reading `05 00` as a two-byte text ID consumes the
first byte of the following Korean glyph. For example, the first choice in
Blade Brothers text `4602` begins `05 00 0C 50`: the old decoder selected
system text `000C` (save failure) and lost the `0C` Hangul bank byte. Anju's
`3001` has the same problem. These NPC scripts do not request an EEPROM save.

Only the recognized Korean profile now accepts both `00` and `FF` as the
result-only sentinel. Full text IDs such as `465F` still chain normally.
USA/EU decoding is unchanged. This is an intentional compatibility correction
to the translation's retained ARM behavior, so it has explicit expected-value
tests rather than an assertion that the incorrect ARM output must match.

## Dialogue width

The original renderer clips a glyph to the remaining line and consumes later
characters even when no space remains. Korean dialogue now measures a complete
glyph before drawing. A glyph that does not fit remains pending across the
normal line/page advance. This preserves nested token streams, names, numbers,
and the exact decoded character without rewinding raw bytes. Overlong centered
text also avoids a negative centering offset wrapping into a large cursor.

## Overlay graphics after SELECT

`RestoreGameTask` previously forced ordinary map-buffer uploads for BG1 and
BG2 after the room/manager callbacks restored their graphics. Mount Crenel's
weather callback puts its ash tilemap directly into screenbase 29; the forced
upload subsequently replaced it with a cleared ordinary map buffer. The
forced upload is now restricted to layers actually owned by a room map. The
same ownership rule covers overlay use of either BG1 or BG2. Existing BG3
manager restoration and the palette restoration remain in place.

## Upstream review and adaptation

Reviewed upstream through
[`2f248e0`](https://github.com/EstebanPdN/zelda-tmc-3ds/commit/2f248e0),
including the experimental-camera removal in
[`d0662c4`](https://github.com/EstebanPdN/zelda-tmc-3ds/commit/d0662c4).
Upstream replaced its earlier history and reorganized the source tree.
This fork adapts the relevant source changes to its existing layout instead
of replacing the Korean README, workflow, ROM recognition, JP data fixes or
bottom-screen touch handling.

The upstream camera change removes experimental 400×240 / 200×120 gameplay
geometry. Presentation uses the established 240/266×160 source with the
existing aspect/filter settings. Widescreen streaming moves into the upstream
`port_widescreen.c` module. This is a stability change; it does not guarantee
that every overlay or later room has been played through.

The upstream save implementation is functionally the same as this fork's
baseline; it contains no new fix for the reported NPC text-routing problem.
The fork's automatic updater remains disabled, so an upstream download cannot
overwrite Korean support.

## Validation and limits

- ROM-free production-code tests cover long dialogue, line/page transitions,
  pending characters, centering, choice controls, and all 256 choice-prefix
  values in each supported region.
- A production `RestoreGameTask` + Crenel weather callback fixture performs
  256 menu-return cycles with synthetic graphics. It checks the resulting
  VRAM for ordinary maps and overlay-owned BG1/BG2. This reproduces the
  overwrite with the pre-fix restore code.
- Real save-screen handlers write and reload all three regional EEPROM
  profiles, report injected storage failures, and succeed on retry. These
  checks retain the existing migration and interrupted-write tests.
- The upstream presentation-policy and PPU overlay/parity tests run alongside
  the existing regional-gameplay, save, touch and bottom-screen tests.
- Local, user-supplied ROM comparisons cover decoding, glyph/style addresses
  and widths; separate assertions cover the intentional choice correction.

Physical-hardware verification of K0.6, every NPC, every overlay region and a
complete ending playthrough remain separate acceptance checks. The user's
original emulator save was backed up and is never used for destructive tests.

## K0.6 runtime evidence

Diagnostic build [41306496950](https://github.com/Park-ing-lot/zelda-tmc-3ds-korean/actions/runs/41306496950)
ran in Azahar 2126.2 with a separate virtual SD. Scripted input entered
Swiftblade's actual room, spoke to the NPC, accepted training and the
understanding question, and reached the following possession explanation
(`4604`) without a system save-error detour. This was not a completed training
or full-game playthrough.

On Mount Crenel (`06/00`), actual SELECT/Ezlo dialogue, START menu, world map
and detail-map return paths preserved the ash tilemap. Its 2,048 bytes at
VRAM `E800` matched byte-for-byte before and after those paths. Cloud Tops
(`08/00`) also restored its BG3 cloud map through pause and SELECT dialogue.
These are tests at specific positions, not proof of every overlay room.

All 16 bottom-screen item cells reached their expected native equipment
values. A subsequent B-ring/sword tap updated B to sword; Quest/Map/Items,
Settings, Screen and Back taps completed without a generation rejection in
this sequence. Runtime captures and test saves stay local and are not
redistributed.
