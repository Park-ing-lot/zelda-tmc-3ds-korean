# The Minish Cap 3DS — Korean ROM Compatibility Experiment

This is an **experimental fork** of [EstebanPdN/zelda-tmc-3ds](https://github.com/EstebanPdN/zelda-tmc-3ds), maintained in this repository by [Park-ing-lot](https://github.com/Park-ing-lot).

**한국어: [간단 설치·사용 설명서](docs/INSTALL_KO.md) · [CIA 릴리스 다운로드](https://github.com/Park-ing-lot/zelda-tmc-3ds-korean/releases/latest)**

**We did not create the native 3DS port.** The native engine integration, dual-screen interface, renderer, audio implementation, and existing USA/European support come from the upstream project and its contributors. This fork adapts it to the specific Korean fan-translated GBA ROM revisions listed below on a homebrew-enabled Korean Nintendo 3DS.

## Current status

**Development prototype. The player reports working gameplay on a physical Korean 3DS. A complete ending playthrough has not been verified.**

K0.9 adds **stygia's Korean patch v1.1 (2026-11-09)**. This revision is recommended for dialogue: its font metadata advances Hangul by 9px instead of the legacy ROM's 13px, retaining the same visible glyph pixels. It also supplies revised text and authored line breaks. The port reads its relocated stylized-font map and prefers this revision when both accepted Korean ROMs are present. Earlier overflow wrapping remains a fallback; it does not trigger for the checked v1.1 Kinstone, Tabanta, Melari or introductory-story lines. See [K0.9 evidence and limits](docs/KOREAN_K09_PATCH.md).

K0.8's word-preserving overflow guard, K0.7's Save/Continue panels and the previous NPC-choice, overlay and upstream camera fixes remain included. The legacy ROM still has its original wider font metrics. See the [earlier investigation](docs/KOREAN_K06_FIXES.md).

The initial Korean changes build to CIA and 3DSX. Local comparisons of the production C code against the translation's original ARM routines pass 18,825 cases covering character decoding, glyph addresses, stylized lookup, and variable-width measurement. Another 62,756 text comparisons using the clean European ROM match the unmodified upstream C implementation. These are text-system tests, not full-game regression tests.

A user-provided gameplay screenshot confirms Korean dialogue during the opening house event in Azahar 2126.2, whose status bar reports 60 FPS. This is an early runtime check, not a full-game compatibility claim. A separate K0.3 diagnostic run also loaded a copy of the user’s saved slot and displayed the bottom tab labels correctly. Full later progression and full Korean text coverage remain open.

The K0.4/K0.5 gameplay audit fixes missing Japanese-profile tables, 26 regional room
lists (including dungeon boss rewards), native script-pointer provenance and
three missing Goron script callbacks. K0.5 also improves ITEMS touch timing across cursor repaints. Sixteen host test programs pass with memory sanitizers; local runtime checks assign all 16 item slots through the actual UI and confirm A/B equipment changes, settings navigation and Gust Jar use. See [the audit and its verification limits](docs/KOREAN_GAMEPLAY_AUDIT.md).
This does not claim that a complete ending playthrough has been verified.

## What this fork changes

- Recognizes the two specific Korean ROM revisions below, both based on Japanese `BZMJ` data.
- Verifies their full-file SHA-256 in the 3DS frontend and supports the legacy trimmed and v1.1 full sizes.
- Reimplements its extended character decoding and relocated glyph-bank lookup in native C.
- Adjusts variable-width text measurement, two-half glyph rendering, the stylized font lookup, and dialogue symbol palette selection to match the analyzed translation routines.
- Restores the bottom-screen English tab/settings labels for the Korean ROM profile using the ASCII glyphs retained in its original font banks. Buttons fall back to a labelled rendering when their font is unavailable.
- Uses a separate experimental application ID (`0004000005A22E00`), product code, and SD directory.
- Disables upstream automatic update downloads in this fork so an upstream binary cannot replace the Korean implementation.
- Includes local analysis/test tools and a GitHub Actions build workflow inherited from upstream.

These changes port the native engine to work with an existing Korean-patched ROM. This repository does **not** claim authorship of the original game, the native port, or the Korean translation. The translation and its game assets are not included here.

## Supported input under investigation

| Property | Recommended: stygia v1.1 | Legacy Korean ROM |
| --- | --- | --- |
| Header game code | `BZMJ` | `BZMJ` |
| File size | 16,777,216 bytes | 15,365,627 bytes |
| CRC32 | `7073540C` | — |
| SHA-1 | `5dd6bdbce68a47bd8dc0599385b85116f6fc9b4b` | `5dfc388f30bec4db3965632cca8fe1a97d9c9f79` |
| SHA-256 | `ca939966543e9073f2fdaaa42b163b7ca62be26ec6af931fc07151c93da84688` | `70fd38bd851ab48034fdab04be13ced3ef8a5d95344318ddc98d3d5417830b90` |

Other Korean patches and the clean Japanese ROM are not accepted by the 3DS frontend. Prepare the translation from your own Japanese base ROM with the patch author's tool; the v1.1 patch expects base CRC32 `6CE771A5`. The upstream USA/EU loading paths are retained.

## Experimental installation target

Install the CIA through FBI or launch the 3DSX through the Homebrew Launcher on an appropriately configured system. Put your own matching `.gba` file in:

```text
sdmc:/3ds/The Minish Cap 3DS Korean/
```

Use the separate Korean directory; the original upstream application's directory is different. This is a homebrew application, not a retail-signed title. Builds never contain a ROM. K0.9 retains the existing Korean title ID, directory and `tmc_jp.sav` filename. Updating this application or replacing the ROM does not delete that save. Back up your save before migrating between translation revisions.

The generated 3DSX icon metadata includes Korea in its region mask. The CIA uses title ID `0004000005A22E00` and product code `CTR-P-TMCK`; these differ from the upstream application. Upstream automatic update checks/downloads are disabled. Updates for this experimental fork are installed manually.

## Build and development

The 3DS build uses upstream's devkitPro/devkitARM, libctru, Citro2D/Citro3D and CMake setup. CIA packaging also needs makerom and bannertool:

```sh
./platform/3ds/build.sh
```

Outputs use the experimental version from `platform/3ds/version.txt` and are placed in `build-3ds/game/`. Automated builds are available under [Actions](https://github.com/Park-ing-lot/zelda-tmc-3ds-korean/actions). Availability of an artifact does not imply runtime validation.

The optional `runtime_probe` workflow input produces a separately named **development** artifact with SD-based screen capture and scripted test input. Normal `tmc-3ds-packages` builds exclude that diagnostic code.

See the [Korean port plan and evidence](docs/KOREAN_PORT_PLAN.md) for milestones, known gaps, and test results. The [archived upstream README](docs/UPSTREAM_README.md) documents the original project's features and installation; it is not a statement of this fork's test coverage.

## Attribution and license

- **Nintendo / Capcom** — the original The Legend of Zelda: The Minish Cap game and its content. This is an unofficial fan project, unaffiliated with Nintendo or Capcom.
- **[Esteban PDN and contributors](https://github.com/EstebanPdN/zelda-tmc-3ds)** — the native Nintendo 3DS port on which this fork is directly based.
- **[samyost1/tmc-android](https://github.com/samyost1/tmc-android)** — the dual-screen Android source base credited by upstream.
- **[Project Picori](https://github.com/999sian/tmc)** and **[zeldaret/tmc](https://github.com/zeldaret/tmc)** — native port infrastructure and the original decompilation, respectively, as credited by upstream.
- **ipatix and agbplay contributors** — audio library, under LGPL v3.0; see the bundled notices.
- **Francine** — the pre-existing Korean fan translation. The supplied ROM contains a `Francine` credit in its translated text data at file offset `0xE6089C`; the [Korean translation catalog](https://www.hangulogame.com/platform/gba/) and [historical translation listing](https://windowstiger.tistory.com/534) also credit Francine. This fork adapts the native port to that patched ROM; it does not create or distribute the translation.
- **stygia** — Korean patch v1.1, including its revised translation, corrected font metrics and patch tooling. Authorship and version are credited in the user-supplied `MinishCap_KoreanPatch.exe` interface and embedded history. The user supplied [this author/source blog](https://blog.naver.com/rnslek89); its contents could not be independently retrieved during this investigation. The new translation and its fixes are the patch author's work, not this fork's.
- **Park-ing-lot / this fork** — the Korean compatibility investigation and changes listed above, developed with assistance from Codex.

The original Git history, licenses and notices are preserved. Original port contributions are under **GPL v3.0 or later**. Third-party components retain their own licenses. See [LICENSE](LICENSE), [COPYRIGHT.md](COPYRIGHT.md), and [THIRD-PARTY-LICENSES.md](THIRD-PARTY-LICENSES.md).

**You must obtain and prepare your own legally obtained, Korean-patched ROM. This repository provides no ROM files, ROM downloads, translation patch files, extracted game assets, saves, or firmware.**
