#!/usr/bin/env python3
"""Compare production C text routines with ARM routines in a local Korean ROM.

Requires: pip install unicorn
Never downloads, embeds, prints or uploads ROM data. Run from any directory:
  python tools/korean/verify_text.py /path/to/your/Korean.gba
"""
import argparse
import hashlib
import pathlib
import random
import struct
import subprocess
import sys
import tempfile

from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_HOOK_CODE
from unicorn.arm_const import (UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2,
                              UC_ARM_REG_SP, UC_ARM_REG_LR, UC_ARM_REG_PC)

ROM_SHA256 = '70fd38bd851ab48034fdab04be13ced3ef8a5d95344318ddc98d3d5417830b90'
V11_SHA256 = 'ca939966543e9073f2fdaaa42b163b7ca62be26ec6af931fc07151c93da84688'
BASE, RAM = 0x08000000, 0x02000000
TOKEN, INPUT, STOP = RAM + 0x10000, RAM + 0x11000, RAM + 0x30000


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('rom', type=pathlib.Path)
    args = parser.parse_args()
    rom = args.rom.read_bytes()
    digest = hashlib.sha256(rom).hexdigest()
    v11 = digest == V11_SHA256
    if digest not in (ROM_SHA256, V11_SHA256):
        parser.error('ROM does not match the analyzed Korean revision')
    root = pathlib.Path(__file__).resolve().parents[2]
    uc = Uc(UC_ARCH_ARM, UC_MODE_THUMB)
    uc.mem_map(BASE, 0x1000000)
    uc.mem_write(BASE, rom)
    uc.mem_map(RAM, 0x40000)

    # The retail MemCopy routine programs the GBA DMA controller. Unicorn
    # models the CPU, so provide only that memory-copy hardware effect.
    # Character decoding, glyph lookup, style lookup and width code execute
    # directly from the original ROM without replacement.
    def dma_copy(machine, address, size, data):
        src = machine.reg_read(UC_ARM_REG_R0)
        dst = machine.reg_read(UC_ARM_REG_R1)
        count = machine.reg_read(UC_ARM_REG_R2)
        machine.mem_write(dst, bytes(machine.mem_read(src, count)))
        machine.reg_write(UC_ARM_REG_PC, machine.reg_read(UC_ARM_REG_LR))

    uc.hook_add(UC_HOOK_CODE, dma_copy, begin=BASE + 0x1D650, end=BASE + 0x1D650)

    def call(address, r0, r1=0):
        uc.reg_write(UC_ARM_REG_SP, RAM + 0x3F000)
        uc.reg_write(UC_ARM_REG_LR, STOP | 1)
        uc.reg_write(UC_ARM_REG_R0, r0)
        uc.reg_write(UC_ARM_REG_R1, r1)
        uc.emu_start(address | 1, STOP, count=100000)
        if uc.reg_read(UC_ARM_REG_PC) != STOP:
            raise RuntimeError('ARM routine did not return')
        return uc.reg_read(UC_ARM_REG_R0)

    def set_token(a, b, c, style=0, language=0):
        uc.mem_write(RAM + 7, bytes([language]))
        token = bytearray(48)
        token[0] = 1 | (style << 5)
        struct.pack_into('<I', token, 16, INPUT)
        uc.mem_write(TOKEN, bytes(token))
        uc.mem_write(INPUT, bytes([a, b, c, 0]))

    cases, expected, categories = [], [], {}

    def add(mode, a, b=0, c=0, style=0):
        cases.append('{} {:x} {:x} {:x} {:x}\n'.format(mode, a, b, c, style))
        categories[mode] = categories.get(mode, 0) + 1
        if mode == 'D':
            set_token(a, b, c, style & 0xFF, style >> 8)
            code = call(BASE + 0x5EE28, TOKEN)
            pointer = struct.unpack('<I', uc.mem_read(TOKEN + 16, 4))[0]
            extended = struct.unpack('<H', uc.mem_read(TOKEN + 4, 2))[0]
            expected.append((code, pointer - INPUT, extended))
        elif mode == 'G':
            uc.mem_write(RAM + 7, bytes([style]))
            expected.append((call(BASE + 0x5F09C, a) - BASE,))
        elif mode == 'S':
            uc.mem_write(RAM + 7, bytes([style]))
            expected.append((call(BASE + 0x5F7E0, a),))
        elif mode == 'W':
            set_token(a, b, c)
            expected.append((call(BASE + 0x5F164, TOKEN, 0),))
        elif mode == 'R':
            canvas, pixels = RAM + 0x21000, RAM + 0x22000
            uc.mem_write(pixels, bytes([0x55]) * 1024)
            uc.mem_write(canvas, struct.pack('<BBBBHHI', (style & 1) << 4,
                                            (style >> 1) & 1, 2, 0, c, b, pixels))
            uc.mem_write(RAM + 7, b'\0')
            call(BASE + 0x5F61C, a, canvas)
            cursor = struct.unpack('<H', uc.mem_read(canvas + 6, 2))[0]
            expected.append((cursor, int(bytes(uc.mem_read(pixels, 1024)).hex(), 16)))

    # All supported lead/escape combinations. Third-byte boundaries exercise
    # the bank carry and token advancement rather than only common dialogue.
    for lead in range(0x0B, 0x0F):
        for second in range(256):
            add('D', lead, second)
    for second in range(256):
        for third in ([0] if second <= 0x3F else [0, 1, 127, 255]):
            add('D', 0x0F, second, third)
    for lead in range(0x10, 0x100):
        add('D', lead)
    # Real control codes that can be evaluated without another text stream.
    for lead, second, third in [(1, 0, 0), (1, 11, 0), (2, 14, 0),
                                (3, 0x12, 0x34), (4, 0x11, 0),
                                (5, 0x12, 0x34), (8, 0xFF, 0),
                                (9, 5, 0), (10, 0, 0)]:
        add('D', lead, second, third)
    for style in (1, 0x101):
        for lead in range(0x0B, 0x10):
            for second in (0, 0x2F, 0x30, 0x3F, 0x40, 0x44, 0x4F, 0xFF):
                add('D', lead, second, 0x82, style)
    for language in (0, 1):
        for code in range(0x1000):
            add('G', code, style=language)
            add('S', code, style=language)
    rng = random.Random(0)
    for _ in range(256):
        lead = rng.randrange(0xB, 0x10)
        second = rng.randrange(256)
        # Avoid a zero glyph/control byte ending the string before it is read.
        third = rng.randrange(1, 256) if lead == 0xF and second > 0x3F else 0
        add('W', lead, second, third)

    # Compare actual tile pixels, not only glyph addresses/widths. Exercise
    # all alignments, transparent/opaque writes, fixed/proportional cells and
    # right-edge clipping. Both revisions must match their original ARM draw.
    for i in range(128):
        bank = rng.choice([1, 2, 3, 4, 5, 7, 9, 11, 13])
        add('R', (bank << 8) | rng.randrange(256), i % 8 + (56 if i % 3 == 0 else 0), 64, i % 4)

    if v11:
        # All 2,350 KS X 1001 Hangul syllables have their corrected 9px
        # advance. This is ROM metadata; the port does not rescale the bitmap.
        for index in range(2350):
            bank, cell = divmod(index, 256)
            bank += 4 + (bank >= 4)
            if bank <= 7:
                lead, second, third = bank + 7, cell, 0
            else:
                lead, second, third = 15, 0x40 + bank - 9, cell
            add('W', lead, second, third)
            assert expected[-1] == (9,)

    # Intentional compatibility correction: the translation's result-only
    # choices use 05 00, whereas its retained ARM decoder expects 05 FF.
    # Do not compare these to that broken ARM path: assert the intended
    # target and that the following Hangul/ASCII character is not consumed.
    for second in (0, 0xFF):
        cases.append(f'C {second:x} c 50 0\n')
        expected.append((12, 2, 0, 0x550, 4))
    cases.append('C 46 5f 41 0\n')
    expected.append((12, 3, 0x465F, 0x141, 4))
    categories['choice_compatibility'] = 3
    # The fixed Save/Continue panel has 128px of usable width. Measure the
    # real translated lines, including selected/unselected cursor variables,
    # so moving the block never merely hides its final characters.
    popup_widths = ((0x10, 0x01020058), (0x11, 0x0102004F)) if v11 else ((0x10, 0x01020078), (0x11, 0x0102006B))
    for text, width in popup_widths:
        for choice in (0, 1):
            cases.append(f'P {text:x} {choice:x} 0 0\n')
            expected.append((width,))
    categories['popup_layout'] = 4
    if v11:
        # Actual authored lines reported by the player: Kinstone acquisition,
        # Tabanta hint, Melari and both introductory story layouts. The low
        # 16 bits are the widest original line; none needs overflow wrapping.
        for text, measured in ((0x055C, 0x000200AD), (0x0C36, 0x010700AD),
                               (0x125D, 0x020E00B0), (0x0F02, 0x000100B1),
                               (0x0F03, 0x00060046)):
            cases.append(f'P {text:x} 0 0 0\n')
            expected.append((measured,))
        categories['authored_layout'] = 5

    with tempfile.TemporaryDirectory(prefix='tmc-korean-') as temp:
        probe = pathlib.Path(temp) / 'text-probe'
        strip = '-Wl,-dead_strip' if sys.platform == 'darwin' else '-Wl,--gc-sections'
        command = ['cc', '-std=gnu11', '-O2', '-DPC_PORT', '-DMULTI_REGION',
                   '-DUSA', '-DENGLISH', '-DREVISION=0', '-I.', '-Iinclude',
                   '-Iport', '-include', 'region.h', '-ffunction-sections',
                   '-fdata-sections', '-Wno-pointer-sign',
                   'tools/korean/text_probe.c', 'port/port_text_render.c', strip, '-o', str(probe)]
        subprocess.run(command, cwd=root, check=True)
        result = subprocess.run([str(probe), str(args.rom.resolve())],
                                input=''.join(cases), capture_output=True,
                                text=True, check=True)
    actual = [tuple(int(x, 16) for x in line.split()) for line in result.stdout.splitlines()]
    if len(actual) != len(expected):
        raise AssertionError('Output count {} != {}'.format(len(actual), len(expected)))
    failures = [(case.strip(), a, e) for case, a, e in zip(cases, actual, expected) if a != e]
    if failures:
        for case, a, e in failures[:12]:
            if case.startswith('R '):
                print('FAIL {}: cursor C={} ARM={}, pixels_equal={}'.format(case, a[0], e[0], a[1] == e[1]))
            else:
                print('FAIL {}: C={} ARM={}'.format(case, a, e))
        raise AssertionError('{} mismatches'.format(len(failures)))
    print('PASS: {} text cases (ARM differential checks plus explicit choice compatibility assertions) {}'.format(len(cases), categories))


if __name__ == '__main__':
    main()
