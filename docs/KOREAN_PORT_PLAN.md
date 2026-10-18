# Korean ROM support plan

This fork targets the user's Korean translation of The Minish Cap on a
homebrew-enabled Korean Nintendo 3DS. It preserves the upstream native engine,
dual-screen interface, and USA/EU support. Original ROMs stay local and are never
uploaded to GitHub or embedded in builds.

## Confirmed inputs (2026-10-18)

| Input | Size | SHA-1 | GBA code |
| --- | ---: | --- | --- |
| Clean European ROM | 16,777,216 | `cff199b36ff173fb6faf152653d1bccf87c26fb7` | `BZMP` |
| User's Korean ROM | 15,365,627 | `5dfc388f30bec4db3965632cca8fe1a97d9c9f79` | `BZMJ` |

The Korean ROM is based on **Japanese**, not European, data. Its first
translation pointer at `0x108ED8` points to `0x08E60000`; its original font table
remains at `0x108F0C`. The upstream engine has JP data offsets and script
translation, but the 3DS frontend currently accepts only USA/EU. Its generic
loader also rejects files shorter than 16 MiB. Bypassing those checks alone
cannot port custom ARM text-rendering hooks: native C must implement them.

## Milestones and acceptance criteria

1. **Repository and reproducibility**
   - Preserve the upstream Git history and remote.
   - Connect public `Park-ing-lot/zelda-tmc-3ds-korean`.
   - Keep a build workflow producing ROM-free CIA/3DSX artifacts.
2. **ROM analysis and explicit compatibility profile**
   - Identify the exact Korean revision and its relocated font/text data.
   - Reimplement the translation's token/glyph hooks from the user's local ROM.
   - Accept the known trimmed size without weakening validation of retail ROMs.
3. **Native Korean text implementation**
   - Verify character decoding and glyph lookup against the ROM's original ARM
     routines; use local-only fixtures, never committed ROM bytes.
   - Check dialogue, menus, player names, line wrapping, and control codes.
   - Preserve JP gameplay and region-specific script/flag behavior.
4. **3DS build and packaging**
   - Give the experimental fork separate storage and application identity.
   - Prevent the upstream updater from replacing the Korean build.
   - Build CIA and 3DSX through CI; confirm package metadata and region flags.
5. **Runtime validation**
   - Emulator: startup, title, intro, character creation, Korean dialogue,
     bottom-screen menus, saving and loading.
   - User's Korean 3DS: install/launch, DSP audio, touch controls, save/restart.
   - Play through representative early rooms and progression gates before
     calling the port stable. Build success does not imply gameplay verification.

## Current status

- Upstream cloned at `c8fc571`; input sizes, hashes and header regions verified.
- GitHub CLI authenticated as `Park-ing-lot`.
- Public repository is now a **GitHub fork** with parent
  `EstebanPdN/zelda-tmc-3ds` (verified through the GitHub API). The first
  standalone work repository was retained as an archived backup.
- Production C text routines pass **18,825 comparisons** against the supplied
  ROM's ARM code using Unicorn: 2,185 decoding cases, 8,192 glyph lookups,
  8,192 stylized lookups and 256 width measurements. Only the GBA DMA memory
  copy hardware effect is substituted in the ARM harness.
- Clean European ROM text behavior matches unchanged upstream C in **62,756
  cases**, using `tools/korean/verify_eu_regression.py`. Coverage includes
  decoding, font addresses, stylized mapping and variable-width measurements.
  This is not a whole-game USA/EU regression claim.
- Korean translation credit: the supplied ROM itself contains `Francine` at
  offset `0xE6089C`. This corroborates the historical listings at
  https://windowstiger.tistory.com/534 and
  https://www.hangulogame.com/platform/gba/ . No ROM or patch was downloaded.
- CIA and 3DSX cross-build succeeded (runtime-probe run `39104123326`). The
  makerom/bannertool compatibility issue was resolved by compiling pinned
  sources inside CI. Normal builds exclude the runtime-probe source.
- Package metadata checked locally: CIA title `0004000005A22E00`, product
  `CTR-P-TMCK`; 3DSX SMDH region mask `0xFFFFFFFF` includes Korea.
- Azahar 2126.2 with Korean region settings loads the ROM and creates its
  settings/save files; this is not yet proof of correct rendered gameplay.
  macOS screen capture/accessibility are unavailable in this session. An
  explicitly enabled runtime-probe build captures the emulated screens to
  local SD and accepts scripted test inputs; normal builds omit this code.
- The user supplied a screenshot of actual early gameplay: the opening house
  event with Zelda and the smith renders Korean dialogue; Azahar reports
  60 FPS. This supersedes the earlier boot-unverified status. Do not interpret
  it as confirmation of later events, save/reload, or real-hardware behavior.
- Synthetic profile/bounds tests pass locally with AddressSanitizer and
  UndefinedBehaviorSanitizer. They cover short headers, incorrect profile
  signatures, malformed glyph pointers and final-glyph ROM bounds.
- Local devkitPro/CMake are absent; cross-builds use GitHub Actions. Azahar
  was downloaded into the ignored `.tmp/` folder for local checks. Real
  hardware testing requires the user's 3DS.
- This remains an experimental port until the runtime checks above pass.

## Working rules

- Commit source, tools, documentation, and synthetic tests only.
- Do not upload original/patched ROMs, extracted graphics, screenshots, saves,
  or local runtime dumps. Existing licenses and attribution remain intact.
- Record test results and unresolved issues here as implementation progresses.

## Bottom-screen labels follow-up (2026-10-18, K0.3)

- User observed empty QUEST/MAP/ITEMS buttons after saving and exiting.
- Root cause: theme font selection disabled both Latin banks for every JP ROM;
  the button compositor still returned success for its empty plate, so the
  caller never drew fallback lettering.
- The exact Korean profile retains the original Latin glyphs in font banks
  0 and 8 (validated visually from the user's local ROM). Enable these banks
  for that profile; keep the unverified clean-JP guard. A nonempty button
  label with no font now returns failure so the caller paints its labelled
  fallback.
- Production font/button tests pass, including a local ROM preview of all
  three tab labels and synthetic USA/EU/JP/profile selection and missing-font
  cases. AddressSanitizer/UndefinedBehaviorSanitizer checks also pass.
- The user's 8 KiB save and settings were backed up locally under the ignored
  `.tmp/save-backups/` folder before runtime testing. Test with copies only.

## Gameplay and interaction follow-up — K0.5

The expanded audit and precise remaining limits are in
[KOREAN_GAMEPLAY_AUDIT.md](KOREAN_GAMEPLAY_AUDIT.md). Verified fixes include
22 missing JP profile addresses, 26 additional room lists, regional packed
pointers and three missing Goron callbacks. All 235 direct raw room-init lists
were reviewed; 556 non-unused script intervals resolve 468 native callbacks.
Both diagnostic and normal K0.5 packages build successfully with 16 ROM-free
host test programs passing under address/undefined-behavior sanitizers.

Runtime tests loaded the user's copied save, exercised all 16 item cells and
A/B assignment, settings/submenu/back/tab interaction, and active Gust Jar use.
An animation/input timing issue was reproduced with one-item and 16-slot
inventories and corrected for matching visible ITEMS targets. Full ending and
physical Korean 3DS acceptance remain outstanding.
