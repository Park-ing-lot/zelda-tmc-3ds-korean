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
- Local machine has no devkitPro, CMake, or 3DS emulator installed. Use GitHub
  Actions for the first cross-build; hardware testing requires the user's 3DS.
- This remains an experimental port until the runtime checks above pass.

## Working rules

- Commit source, tools, documentation, and synthetic tests only.
- Do not upload original/patched ROMs, extracted graphics, screenshots, saves,
  or local runtime dumps. Existing licenses and attribution remain intact.
- Record test results and unresolved issues here as implementation progresses.
