#include "port_region_data.h"

#include <stddef.h>
#include <stdint.h>

#include "port_rom.h"
#include "port_types.h"
#include "region.h"

/* Compiled stubs use the USA layout. EU and JP replacements are independently
 * verified; see tools/korean/verify_gameplay.py for the local ROM checks. */
extern const u8 gUnk_080D8E50[];
extern const u8 gUnk_080D9328[];
extern const u8 gUnk_080DD750[];
extern const u8 gUnk_080DD7E0[];
extern const u8 gUnk_080DD840[];
extern const u8 gUnk_080EAE60[];
extern const u8 gUnk_080EB9F4[];
extern const u8 gUnk_080F58A8[];
extern const u8 gUnk_080F5B3C[];
extern const u8 gUnk_080F78A0[];
extern const u8 gUnk_080F9BF8[];
extern const u8 gUnk_080F09A0[];
extern const u8 gUnk_080FEAC8[];
extern const u8 gUnk_080FEE58[];

#define REGION_ENTITY_LIST(symbol, usa, eu, jp, bytes) extern const u8 symbol[];
#include "port_region_entity_lists.inc"
#undef REGION_ENTITY_LIST

#define REGION_DATA_UNAVAILABLE UINT32_MAX

typedef struct {
    const void* usaData;
    u32 euOffset;
    u32 jpOffset;
    u32 size;
    u32 jpSize;
} RegionDataEntry;

static const RegionDataEntry sRegionDataEntries[] = {
    { gUnk_080D8E50, 0xD85AC, 0xD8BF0, 96, 96 },
    { gUnk_080D9328, 0xD8A84, 0xD90C8, 16, 16 },
    /* Cloud removal and both independently persisted golden rewards must all
     * use native script addresses and native completion flags. */
    { gUnk_080DD750, 0xDCE8C, 0xDD4E0, 64, 64 },
    { gUnk_080DD750 + 0x40, 0xDCECC, 0xDD520, 80, 80 },
    { gUnk_080DD7E0, 0xDCF1C, 0xDD570, 64, 64 },
    { gUnk_080DD840, 0xDCF7C, 0xDD5D0, 64, 64 },
    { gUnk_080EAE60, 0xEA53C, 0xEAB90, 80, 96 },
    { gUnk_080EB9F4, 0xEB0C0, 0xEB724, 112, 176 },
    { gUnk_080F58A8, REGION_DATA_UNAVAILABLE, 0xF55C0, 0, 32 },
    { gUnk_080F5B3C, REGION_DATA_UNAVAILABLE, 0xF5854, 0, 32 },
    { gUnk_080F78A0, 0xF6E5C, 0xF75B8, 32, 32 },
    { gUnk_080F9BF8, 0xF9144, 0xF98B0, 64, 64 },
    { gUnk_080F09A0, 0xEFFD4, 0xF06C8, 96, 96 },
    { gUnk_080FEAC8, 0xFE00C, 0xFE778, 288, 288 },
    { gUnk_080FEE58, 0xFE39C, 0xFEB08, 32, 32 },
    /* Returning the original ROM list also preserves script provenance. A
     * ROM-native script copied into a RAM stub would otherwise look compiled
     * and be translated a second time when addresses collide with USA keys. */
#define REGION_ENTITY_LIST(symbol, usa, eu, jp, bytes) { symbol, eu, jp, bytes, bytes },
#include "port_region_entity_lists.inc"
#undef REGION_ENTITY_LIST

};

static int Port_IsRomDataPointer(const void* data) {
    uintptr_t address;
    uintptr_t rom;

    if (data == NULL || gRomData == NULL) {
        return 0;
    }
    address = (uintptr_t)data;
    rom = (uintptr_t)gRomData;
    return address >= rom && address - rom < gRomSize;
}

const void* Port_ResolveRegionData(const void* data) {
    size_t i;

    if (data == NULL || Port_IsRomDataPointer(data) || (!REGION_IS_EU && !REGION_IS_JP)) {
        return data;
    }

    for (i = 0; i < sizeof(sRegionDataEntries) / sizeof(sRegionDataEntries[0]); ++i) {
        const RegionDataEntry* entry = &sRegionDataEntries[i];
        if (data != entry->usaData) {
            continue;
        }
        const u32 offset = REGION_IS_JP ? entry->jpOffset : entry->euOffset;
        const u32 size = REGION_IS_JP ? entry->jpSize : entry->size;
        if (offset == REGION_DATA_UNAVAILABLE || gRomData == NULL || offset > gRomSize ||
            size > gRomSize - offset) {
            return NULL;
        }
        return gRomData + offset;
    }

    return data;
}
