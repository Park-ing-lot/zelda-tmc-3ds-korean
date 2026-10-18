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
BASE, RAM = 0x08000000, 0x02000000
TOKEN, INPUT, STOP = RAM + 0x10000, RAM + 0x11000, RAM + 0x30000


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('rom', type=pathlib.Path)
    args = parser.parse_args()
    rom = args.rom.read_bytes()
    if hashlib.sha256(rom).hexdigest() != ROM_SHA256:
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

    with tempfile.TemporaryDirectory(prefix='tmc-korean-') as temp:
        probe = pathlib.Path(temp) / 'text-probe'
        strip = '-Wl,-dead_strip' if sys.platform == 'darwin' else '-Wl,--gc-sections'
        command = ['cc', '-std=gnu11', '-O2', '-DPC_PORT', '-DMULTI_REGION',
                   '-DUSA', '-DENGLISH', '-DREVISION=0', '-I.', '-Iinclude',
                   '-Iport', '-include', 'region.h', '-ffunction-sections',
                   '-fdata-sections', '-Wno-pointer-sign',
                   'tools/korean/text_probe.c', strip, '-o', str(probe)]
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
            print('FAIL {}: C={} ARM={}'.format(case, a, e))
        raise AssertionError('{} mismatches'.format(len(failures)))
    print('PASS: {} C/ARM comparisons {}'.format(len(cases), categories))


if __name__ == '__main__':
    main()
