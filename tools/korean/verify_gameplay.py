#!/usr/bin/env python3
"""Audit Korean gameplay tables against user-supplied, hash-checked ROMs.

No ROM data is written or bundled. Static data checks are not an end-to-end
playthrough. Usage: verify_gameplay.py KOREAN.gba EUROPE.gba-or-zip
"""
from pathlib import Path
import hashlib
import re
import struct
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[2]

def read_rom(path, sha1):
    path = Path(path)
    if path.suffix.lower() == '.zip':
        with zipfile.ZipFile(path) as z:
            names = [n for n in z.namelist() if n.lower().endswith('.gba')]
            assert len(names) == 1, 'Expected one .gba in ZIP'
            data = z.read(names[0])
    else:
        data = path.read_bytes()
    assert hashlib.sha1(data).hexdigest() == sha1, 'Unsupported ROM revision'
    return data

def u32(data, off):
    return struct.unpack_from('<I', data, off)[0]

def rom_pointer(data, address, size):
    offset = address - 0x08000000
    assert 0 <= offset <= len(data) - size, f'Invalid pointer {address:#x}'
    return data[offset:offset + size]

def main():
    assert len(sys.argv) == 3, __doc__
    k = read_rom(sys.argv[1], '5dfc388f30bec4db3965632cca8fe1a97d9c9f79')
    e = read_rom(sys.argv[2], 'cff199b36ff173fb6faf152653d1bccf87c26fb7')
    text = (ROOT / 'port/port_rom.c').read_text()
    block = text.split('const RomOffsets kRomOffsets_JP = {')[1].split('};')[0]
    offsets = {name: int(value, 16) for name, value in re.findall(r'\.(\w+) = (0x[0-9A-Fa-f]+),', block)}
    script_rows = [(int(a, 16), int(b, 16), int(c, 16)) for a,b,c in re.findall(
        r'\{ (0x[0-9A-F]+), (0x[0-9A-F]+), (0x[0-9A-F]+) \}',
        (ROOT / 'port/port_script_addrs.c').read_text())]
    eu_to_jp = {b: c for a,b,c in script_rows}
    usa_to_jp = {a: c for a,b,c in script_rows}

    def identical(field, eu, jp, size):
        assert offsets[field] == jp, f'{field}: profile differs from verified location'
        assert e[eu:eu+size] == k[jp:jp+size], f'{field}: content differs'
        print(f'PASS {field}: {size} identical bytes')

    identical('collisionMatrix', 0xB729C, 0xB7914, 1210)
    identical('tileTypeProperties', 0x3A8, 0x360, 0x400)
    identical('overlaySizeTable', 0xB2310, 0xB2988, 240)
    identical('fuserEnemyData', 0x23D6, 0x232E, 20)
    identical('fuserNpcData', 0x23EA, 0x2342, 32)
    identical('lakeHyliaEnemies', 0xF3368, 0xF3A5C, 0x160)
    identical('lakeHyliaCleared', 0xF34C8, 0xF3BBC, 0x10)
    identical('gustJarHitbox', 0x132178, 0x132754, 8)

    def pointer_table(field, eu, jp, count, payload):
        assert offsets[field] == jp, field
        for i in range(count):
            a, b = u32(e, eu + i*4), u32(k, jp + i*4)
            assert b-a == jp-eu, f'{field}[{i}]: relocation differs'
            assert rom_pointer(e,a,payload) == rom_pointer(k,b,payload), f'{field}[{i}]: payload differs'
        print(f'PASS {field}: {count} pointers and pointed-to payloads')

    pointer_table('collisionShapePtrs', 0x82D4, 0x823C, 40, 32)
    pointer_table('fusionTextPtrs', 0x1B24, 0x1A7C, 120, 6)
    pointer_table('fuserFusionPtrs', 0x1E74, 0x1DCC, 120, 12)
    pointer_table('lilypadRails', 0xFE2DC, 0xFEA48, 3, 16)
    pointer_table('guardPatrolData', 0x10EE10, 0x10F380, 6, 58)
    pointer_table('simonEntityLists', 0xF02EC, 0xF09E0, 8, 32)
    pointer_table('simonEnemyPatterns', 0xF038C, 0xF0A80, 4, 32)
    pointer_table('simonChestPatterns', 0xF043C, 0xF0B30, 5, 32)
    pointer_table('gustJarAnimTable', 0x131D64, 0x132340, 8, 8)
    for field, eu, jp, count in [('innWestEntities',0xD61D0,0xD6814,7),
                               ('innMiddleEntities',0xD6274,0xD68B8,6),
                               ('innEastEntities',0xD6314,0xD6958,7)]:
        pointer_table(field,eu,jp,count,16)

    assert offsets['figurines'] == 0x127E6C
    for i in range(137):
        a=struct.unpack_from('<4I',e,0x1278E0+i*16)
        b=struct.unpack_from('<4I',k,0x127E6C+i*16)
        assert a[2:] == b[2:] and b[2] <= 0x4000
        assert rom_pointer(e,a[0],32) == rom_pointer(k,b[0],32)
        assert rom_pointer(e,a[1],a[2]) == rom_pointer(k,b[1],b[2])
    print('PASS figurines: all 137 records, palettes and graphics')

    assert offsets['extraFrameOffsets'] == 0x9DD6F0
    compiled=(ROOT/'port/port_rom_tables.c').read_text().split('const u8 kExtraFrameOffsetsData[4352] = {')[1].split('};')[0]
    extra=bytes(int(n,16) for n in re.findall(r'0x([0-9A-Fa-f]{2})\b',compiled))
    native=k[0x9DD6F0:0x9DD6F0+len(extra)]
    for i in range(0,len(extra),4):
        a,b=u32(extra,i),u32(native,i)
        if a>>24 == 8:
            assert b-a in (0, -0x260)
        else:
            assert a == b
    assert offsets['frameObjLists'] + 200045 + 3 == offsets['mapDataBase']
    assert offsets['fixedTypeGfx'] + 526*4 == offsets['objPalettes']
    print('PASS sprite table boundaries and all 4352 extra-frame bytes/relocations')

    assert offsets['songTable'] == 0x9F3D3C
    for i in range(100):
        a,pa=struct.unpack_from('<II',e,0xB1D414+i*8)
        b,pb=struct.unpack_from('<II',k,0x9F3D3C+i*8)
        assert pa == pb
        header = rom_pointer(k,b,8)
        assert header[0] <= 16
        if header[0] == 0:
            assert rom_pointer(e,a,4) == header[:4] == bytes(4)
        if i < 12:
            assert b-a == -1218264
            assert rom_pointer(e,a,8)[:4] == header[:4]
    print('PASS song table: 100 music records; first 12 header anchors (regional arrangements may differ)')

    def relocate_scripts(data, mapping):
        data=bytearray(data)
        for i in range(12,len(data),16):
            address=u32(data,i)
            if address in mapping:
                struct.pack_into('<I',data,i,mapping[address])
        return data

    rows=re.findall(r'REGION_ENTITY_LIST\((\w+), (0x[\dA-Fa-f]+)u, (0x[\dA-Fa-f]+)u, (0x[\dA-Fa-f]+)u, (0x[\dA-Fa-f]+)u\)',
                    (ROOT/'port/port_region_entity_lists.inc').read_text())
    assert len(rows) == 26
    for name,usa,eu,jp,size in rows:
        eu,jp,size = int(eu,16),int(jp,16),int(size,16)
        a=relocate_scripts(e[eu:eu+size],eu_to_jp)
        assert a == k[jp:jp+size], name
        assert a[-16] == 255, name + ': missing terminator'
    print('PASS all 26 additional room lists, named script relocations and terminators')
    # Audit every raw compiled entity list loaded directly by roomInit.c.
    source=(ROOT/'port/data_const_stubs.c').read_text()
    room_code=(ROOT/'src/roomInit.c').read_text()
    names=set(re.findall(r'LoadRoomEntityList\(&?(gUnk_08[0-9A-Fa-f]+)\)',room_code))
    exact=0
    for name in sorted(names):
        match=re.search(r'const u8 '+name+r'\[\d+\][^=]*= \{(.*?)\};',source,re.S)
        assert match, name
        data=bytes(int(n,16) for n in re.findall(r'0x([0-9A-Fa-f]{2})',match[1]))
        end=next((i+16 for i in range(0,len(data)-15,16) if data[i]==255),None)
        assert end, name + ': unterminated compiled list'
        data=relocate_scripts(data[:end],usa_to_jp)
        origin=int(name[6:],16)&0xFFFFFF
        found=k.find(data,max(0,origin-0x1200),origin+0x1200)
        if name not in ('gUnk_080DD7E0','gUnk_080DD840'):
            assert found >= 0, name + ': unreviewed regional entity difference'
            exact += 1
    assert exact == 233
    print('PASS 235 direct room-init lists: 233 exact after script relocation; 2 native cloud lists')


    # Known divergent compiled lists: compare to the source USA blob, replacing
    # only named script pointers. Region-specific flag bytes are checked separately.
    source=(ROOT/'port/data_const_stubs.c').read_text()
    for name,jp,size in [('gUnk_080DD750',0xDD4E0,64),('gUnk_080EAE60',0xEAB90,96),
                         ('gUnk_080EB9F4',0xEB724,176),('gUnk_080F58A8',0xF55C0,32),
                         ('gUnk_080F5B3C',0xF5854,32),('gUnk_080F78A0',0xF75B8,32)]:
        block=re.search(r'const u8 '+name+r'\[\d+\][^=]*= \{(.*?)\};',source,re.S)[1]
        data=bytes(int(n,16) for n in re.findall(r'0x([\dA-Fa-f]{2})',block))[:size]
        assert relocate_scripts(data,usa_to_jp) == k[jp:jp+size],name
    for eu,jp,size in [(0xD8A84,0xD90C8,16),(0xDCECC,0xDD520,80),
                       (0xDCF1C,0xDD570,64),(0xDCF7C,0xDD5D0,64),
                       (0xEFFD4,0xF06C8,96),(0xFE00C,0xFE778,288),(0xFE39C,0xFEB08,32)]:
        assert e[eu:eu+size] == k[jp:jp+size]
    for i in range(6):
        a,b=0xD85AC+i*16,0xD8BF0+i*16
        assert e[a+4:a+16] == k[b+4:b+16]
        assert rom_pointer(e,u32(e,a),16) == rom_pointer(k,u32(k,b),16)
    for name,eu,jp,count,size in [('gUnk_080CC944',0xCC06C,0xCC6E4,4,8),
                                  ('gUnk_080CD86C',0xCCF98,0xCD60C,3,3),
                                  ('gUnk_080CD878',0xCCFA4,0xCD618,3,3),
                                  ('gUnk_08111154',0x1108A4,0x110E18,8,8)]:
        registry=(ROOT/'port/port_region_data.c').read_text()
        row=re.search(r'\{ '+name+r', (0x[0-9A-F]+), (0x[0-9A-F]+),',registry)
        assert row and int(row[1],16)==eu and int(row[2],16)==jp
        for i in range(count):
            a,b=u32(e,eu+i*4),u32(k,jp+i*4)
            assert b-a == jp-eu
            assert rom_pointer(e,a,size) == rom_pointer(k,b,size)
    print('PASS Gleerok motion, Spear Moblin and cat hitbox tables and all targets')
    assert k[0xF98B0+12] == 0xAA  # native JP Veil Falls/Ezlo hint flag
    print('PASS regional entity/tile lists, cloud reward flags, Goron wall records')
    # Linear command decoding checks every named non-unused script interval.
    # This detects missing native callbacks, but does not simulate event order.
    named_rows = [(int(jp,16)-0x08000000,name) for usa,eu,jp,name in re.findall(
        r'\{ (0x[0-9A-F]+), (0x[0-9A-F]+), (0x[0-9A-F]+) \}, /\* (.*?) \*/',
        (ROOT/'port/port_script_addrs.c').read_text())]
    named_rows.sort()
    unused = {path.stem for path in (ROOT/'data/scripts/unused').glob('*.inc')}
    functions = (ROOT/'port/port_script_funcs.c').read_text().split(
        'static const ScriptFuncEntry sScriptFuncTable_JP[] = {')[1].split('};')[0]
    functions = [int(n,16) for n in re.findall(r'\{ (0x[0-9A-F]+),',functions)]
    assert functions == sorted(set(functions)), 'JP native-call lookup must remain strictly sorted'
    commands = (ROOT/'src/script.c').read_text().split(
        'static const ScriptCommand gScriptCommands[] = {')[1].split('};')[0]
    command_count = len(re.findall('ScriptCommand',commands))
    checked, calls = 0, set()
    for i,(start,name) in enumerate(named_rows):
        if name in unused:
            continue  # the unused bytecode-demo script intentionally calls data as code
        end = named_rows[i+1][0] if i+1 < len(named_rows) else 0x16900
        if name == 'script_PlayerAtBladeBrothers':
            end = 0x16368  # SCRIPT_END is followed by an embedded Thumb helper
        pos = start
        while pos < end:
            word = struct.unpack_from('<H',k,pos)[0]
            if word in (0,65535):
                pos += 2
                continue
            size,op = word>>10,word&1023
            assert size and op < command_count and pos+size*2 <= end, (name,hex(pos),hex(word))
            if op in (11,12):
                callback = u32(k,pos+2)
                assert callback in functions, f'{name}: missing native callback {callback:#x}'
                calls.add(callback)
            pos += size*2
        checked += 1
    print(f'PASS {checked} non-unused script intervals; {len(calls)} distinct native callbacks resolve')
    print('PASS gameplay data audit (static checks; full-game playthrough remains separate)')

if __name__ == '__main__':
    main()
