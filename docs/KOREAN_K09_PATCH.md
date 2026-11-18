# K0.9: Korean patch v1.1 compatibility

The player reproduced the legacy dialogue overflow in mGBA on a 3DS, as well
as this native port. Earlier character/word wrapping avoided lost characters
but did not restore the intended text spacing.

## Source and local patch application

The player supplied `MinishCap_KoreanPatch.exe` and a Japanese base-ROM ZIP.
The EXE is a PyInstaller package using Python 3.14. Its archive and static
`patch_data` constants were extracted without executing the EXE or its Python
modules. The patcher's disassembled `build` routine describes a zlib/base64
payload beginning with `MCKP`, followed by little-endian offset/length/data
records. Applying 445 bounds-checked records to a local base copy produced
exactly the expected output CRC32.

- Supplied EXE SHA-256: `19ac3f16d52ededd54c3c83ec7a18f1d1fbd7db05ba4e25fa779a9b57cd8280b`
- Expected and observed Japanese base: 16,777,216 bytes, CRC32 `6CE771A5`.
- Expected and observed patched output: 16,777,216 bytes, CRC32 `7073540C`.
- Output SHA-256: `ca939966543e9073f2fdaaa42b163b7ca62be26ec6af931fc07151c93da84688`.

The tool credits **stygia** and identifies v1.1 dated 2026-11-09. Its embedded
history describes retranslation, control-code and line-break corrections,
font changes, a Korean name-entry feature and gameplay fixes. These are
the patch author's contributions. No patch payload, ROM, game asset or save
is committed or included in the public build.

## Text findings and native changes

All 2,350 KS X 1001 Hangul glyphs have identical visible pixels in the legacy
and v1.1 ROMs. The legacy first half has four empty leading columns counted
as printable width. Its two halves advance 8+5=13px. The new metric row marks
those leading columns as padding, giving 4+5=9px. This port uses that metadata
directly, without bitmap rescaling or substituted fonts.

The extended text/font bank locations remain unchanged. The stylized map
moves from `0xE4F000` to `0xE57C00`; reading the legacy location would break
place-name/stylized text. K0.9 selects the correct map for each accepted
revision and verifies the full frontend hash. When both Korean ROMs are
present, v1.1 takes priority regardless of directory ordering.

These v1.1 source texts fit their authored lines without overflow wrapping:

| Text | Widest authored line |
| --- | ---: |
| Kinstone acquisition `055C` | 173px |
| Tabanta hint `0C36` | 173px |
| Melari dialogue `125D` | 176px |
| Introductory story `0F02` | 177px |
| Narrow story column `0F03` | 70px |

The normal dialogue canvas is 208px wide. Save/Continue option blocks measure
88/79px respectively; the existing panel layout measures these dynamically.
The legacy ROM remains accepted with its original 13px metrics and overflow
fallback. Updating only the CIA does not transform legacy ROM assets into v1.1.

## Checks and limits

`verify_text.py` checks both accepted revisions against their actual ARM
decoder, glyph lookup, style map, metrics and rasterizer. Pixel comparisons
exercise all eight column alignments, opaque/transparent writes, fixed and
proportional cells, and clipping. The v1.1 check additionally covers every
Hangul syllable and the five authored layouts above. `verify_gameplay.py`
accepts the exact new revision and audits its regional tables, room lists,
script intervals and native callbacks. The ROM-free host suite remains in CI.

This is a native engine: new assembly hooks in a patched ROM are not executed
automatically. Recognizing v1.1 does not certify every gameplay fix or the
patch's Korean IME. The existing native name-entry UI is retained; do not
assume the patcher's new Korean-name feature has been ported. A complete
ending playthrough and hardware confirmation of this revision remain open.

The application ID and save filename remain unchanged. Local validation
uses an isolated emulator/save copy; the player's existing save is not
rewritten by patch application or build packaging.
