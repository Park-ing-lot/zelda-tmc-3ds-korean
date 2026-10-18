# Korean gameplay compatibility audit

This is an audit of this fork's compatibility work, not a claim of an ending-to-ending playthrough. The original engine and the upstream 3DS port are credited in the project README. All ROM comparisons below use locally supplied files; no game data is included in this repository.

## Problems corrected in K0.4

The Korean translation has a Japanese BZMJ gameplay layout. Booting a USA-baseline multi-region build did not establish that later gameplay was correct:

- The JP profile left 22 gameplay/audio/sprite addresses at zero. Collision shapes and traversal properties returned no data, Kinstone fusers had no valid tables, the Gust Jar had no animation/hitbox template, and figurines fell back to USA addresses. The profile now supplies independently checked JP addresses, including Lake Hylia lists, lilypad rails, guard patrols, inn rooms and Simon's simulation.
- The 26 mutable additional room-list stubs selected USA offsets for JP. This includes boss reward/warp lists in Deepwood Shrine, Cave of Flames, Temple of Droplets and Palace of Winds, plus Melari's Mine and the library. A shared table now supplies USA/EU/JP locations. All 26 JP lists match EU after named script-pointer relocation, including their terminators.
- Copied regional lists could lose script-pointer provenance: a native script in a RAM stub looked like a compiled USA constant. The regional resolver now returns the original active-ROM list before entity creation, avoiding a second translation when addresses collide.
- Known compiled entity/tile blobs now resolve to JP counterparts as well as EU counterparts. This covers Goron wall patterns, the cemetery, Cloud Tops fights/rewards, Castle Garden, Veil Falls hints and world-event chests. Longer JP-only list tails are retained. The USA-only EU omissions still retain their existing behavior; JP's corresponding shop lists are independently mapped.
- Packed Gleerok motion tables, Spear Moblin hitbox templates and cat hitboxes still unpacked embedded USA pointers against the JP ROM. Their verified EU/JP tables and targets now go through a bounded regional pointer reader.
- The JP native-call table lacked three Goron fusion callbacks. The Korean bytecode calls `0x08054D3D`, `0x08054D81` and `0x08054DE9`; these are the wall-manager, wall-breaking effect and synchronization functions. The table now links the existing native implementations. The routines were inspected in the supplied ROM and cross-checked with EU and the source, including JP flag `0x77`.

K0.3 also restored the bottom-screen English menu font from the retained original ASCII banks. A themed button without an available font now returns to the labelled fallback instead of painting an empty plate.

## Reproducible checks

Run `python3 tools/korean/run_host_tests.py --sanitize` without a ROM. It exercises production regional-data resolution, flag behavior, save layout and interrupted-write recovery, migration, Vaati/Cloud Tops guard behavior, delayed entity scripts, snapshot/equipment dispatch and bottom-screen frame visibility/animation. CI runs the same checks before packaging. Root containers drop test-child privileges so permission-failure tests remain real failures; GCC ASan fixtures supply the otherwise uncalled save-screen dependencies.

Run `python3 tools/korean/verify_gameplay.py KOREAN.gba EUROPE.gba-or-zip` with the supported local files. It checks the profile against full table/payload comparisons, all 26 additional lists, regional entity lists and sprite boundaries. It additionally audits all 235 raw entity lists called directly from room initialization: 233 match exactly after script relocation, and the two divergent Cloud Tops lists use verified native replacements. It also decodes 556 non-unused named script intervals and verifies that all 468 distinct native callbacks resolve. The 20 unused scripts include a bytecode demonstration which deliberately calls data addresses; they are excluded. One embedded Thumb helper after `script_PlayerAtBladeBrothers` is excluded from bytecode decoding. This is static command coverage, not branch or playthrough coverage.

Existing `verify_text.py` compares the production decoder/glyph/style/width logic with the supplied ROM's ARM routines (18,825 cases). `verify_eu_regression.py` compares the original upstream and modified C text paths (62,756 EU cases).

## Runtime evidence and limits

- The user's original early-game Korean dialogue screenshot demonstrated actual game execution.
- K0.3 was launched in Azahar 2126.2 with Korea selected and a separate SD directory containing a copy of the user's saved `HT` slot. The save loaded into Link's house. Actual framebuffer captures show the three bottom navigation labels correctly.
- Original user save/config files are preserved. Diagnostic input/capture is opt-in and excluded from normal packages.
- K0.5 diagnostic runtime tests in Azahar loaded the same copied `HT` slot. A user-authorized temporary inventory filled all 16 item slots. Each on-screen cell was tapped and the actual native A-slot equipment byte was read through the official emulator RPC API: all 16 matched the source item metadata. Selecting the B ring and then the sword produced A=shield (13), B=sword (1). QUEST/MAP/ITEMS/SETTINGS navigation and SCREEN/BACK transitions were observed. The Gust Jar was equipped and held active; its suction animation and particles rendered in the house. These are interaction and limited item-use checks, not dungeon playthrough coverage.

Full dungeon traversal, every optional fusion, all boss battles, the final escape, ending credits and physical Korean 3DS hardware have not all been played through. Static checks and regression tests substantially improve confidence but do not establish that every possible later bug is fixed. Existing save repairs which are explicitly limited to USA/EU remain restricted; this audit does not apply speculative progression-flag repairs to JP saves.

## K0.5 item-panel input timing follow-up

The diagnostic's 30-poll input sampling can repeatedly coincide with the
three-presentation item-cursor repaint cadence. With both the original one-item
inventory and a locally cheated 16-slot inventory, taps at that phase were
rejected as a hidden paint generation. This does not mean physical touch always
fails; manual taps can fall on other phases and work.

The 3DS adapter now retains target lists for its two upload buffers and copies
submitted/visible targets at the corresponding display boundaries. For the
ITEMS panel only, an animation paint may accept a tap when every target
rectangle, action and item argument exactly equals the visible list. Dispatch
uses a copied visible target. Task mismatches, pending interactive transitions,
changed items/rectangles/actions, map transforms and other panels retain the
strict generation rule. No hash or raw-struct padding comparison is used.

`bottom_touch_test.c` exercises the real adapter's target snapshots across
paint/submit/visibility and buffer reuse. The frame-state test separately checks
that matching animation targets cannot bypass a task or interactive transition.
Runtime verification with the same 16-slot test inventory passed all 16 item-cell
assignments with zero generation rejects during that sequence, plus the B-slot
sword assignment and subsequent settings/submenu/back/tab transitions.
The periodic diagnostic sampler is phase-sensitive; one initial MAP-panel
navigation tap was rejected under the preserved strict rule before entry to
ITEMS. This change specifically addresses animation-only ITEMS rejection and
does not claim that transition guards never reject input.
Cheats affect only the isolated emulator instance; the original user save is
untouched.

## Packaged build evidence

- Source for both K0.5 package variants: `bfb6d46f983144d0ed4d3e95597960c5c13c35e3`.
- [Diagnostic build and all 16 host checks](https://github.com/Park-ing-lot/zelda-tmc-3ds-korean/actions/runs/39107217301): success.
- [Normal CIA/3DSX build](https://github.com/Park-ing-lot/zelda-tmc-3ds-korean/actions/runs/39107384977): success.
- Normal 3DSX SHA-256: `bd458fa0a8fe4032a0eee00076523b203ba7753ded6080b619f54194ecc1952c`.
- Normal CIA SHA-256: `3754aa6216c015ce57cd95e3c66c0cd5ad0f52e55630058c70e4f6fed739cb99`.
- Normal 3DSX excludes probe input/capture filenames. SMDH region mask `0xffffffff` includes Korea; the CIA NCCH program ID is `0004000005A22E00`, product code `CTR-P-TMCK`. These metadata checks do not replace hardware testing.
- The original user EEPROM image retained its initial SHA-256 `0759a9597fc8d5819b52073ed2116cf1c4a9f0a327530bf6b802e5ccdfeb35a3` after the tests. Temporary cheats were confined to an isolated emulator instance.
