#!/usr/bin/env python3
"""Compare modified versus upstream C text behavior using your local EU ROM.

Accepts a .gba or a .zip containing a single .gba. Requires a native C compiler
and the upstream c8fc571 Git object locally; does not download game data.
"""
import argparse
import hashlib
import pathlib
import random
import subprocess
import sys
import tempfile
import zipfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('rom', type=pathlib.Path)
    args = parser.parse_args()
    if args.rom.suffix.lower() == '.zip':
        with zipfile.ZipFile(args.rom) as archive:
            names = [n for n in archive.namelist() if n.lower().endswith('.gba')]
            if len(names) != 1:
                parser.error('ZIP must contain exactly one GBA ROM')
            data = archive.read(names[0])
    else:
        data = args.rom.read_bytes()
    if hashlib.sha1(data).hexdigest() != 'cff199b36ff173fb6faf152653d1bccf87c26fb7':
        parser.error('Expected the clean European ROM')
    root = pathlib.Path(__file__).resolve().parents[2]
    source = subprocess.check_output(['git', 'show', 'c8fc571:src/text.c'], cwd=root, text=True)
    cases = []
    for language in range(7):
        for code in range(0x1000):
            for mode in ('G', 'S'):
                cases.append('{} {:x} 0 0 {:x}\n'.format(mode, code, language))
    for style in (0, 1, 0x100, 0x101):
        for lead in range(11, 256):
            for second in (0, 0x2F, 0x3F, 0x40, 0xFF):
                cases.append('D {:x} {:x} 0 {:x}\n'.format(lead, second, style))
    rng = random.Random(1)
    for _ in range(512):
        lead = rng.randrange(11, 256)
        second = rng.randrange(256) if lead <= 15 else 0
        cases.append('W {:x} {:x} 0 1\n'.format(lead, second))
    outputs = []
    with tempfile.TemporaryDirectory(prefix='tmc-eu-regression-') as temp:
        work = pathlib.Path(temp)
        rom = work / 'eu.gba'
        rom.write_bytes(data)
        original = work / 'original_text.c'
        original.write_text(source)
        strip = '-Wl,-dead_strip' if sys.platform == 'darwin' else '-Wl,--gc-sections'
        for name, include in (('original', str(original)), ('modified', 'src/text.c')):
            binary = work / name
            command = ['cc', '-std=gnu11', '-O2', '-DPC_PORT', '-DMULTI_REGION',
                       '-DUSA', '-DENGLISH', '-DREVISION=0', '-DTMC_TEST_EU',
                       '-DTMC_TEXT_SOURCE="' + include + '"', '-I.', '-Iinclude',
                       '-Iport', '-include', 'region.h', '-ffunction-sections',
                       '-fdata-sections', '-Wno-pointer-sign',
                       'tools/korean/text_probe.c', strip, '-o', str(binary)]
            subprocess.run(command, cwd=root, check=True)
            result = subprocess.run([str(binary), str(rom)], input=''.join(cases),
                                    capture_output=True, text=True, check=True)
            outputs.append(result.stdout.splitlines())
    assert len(outputs[0]) == len(cases)
    assert outputs[0] == outputs[1], 'EU text behavior differs from upstream'
    print('PASS: {} EU text comparisons against upstream C'.format(len(cases)))


if __name__ == '__main__':
    main()
