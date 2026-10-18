#!/usr/bin/env python3
"""Run ROM-free gameplay/save/touch regression tests using the host C compiler."""
from pathlib import Path
import os
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
TESTS = {
    'profile': ['tools/korean/profile_test.c'],
    'bottom_font': ['tools/korean/bottom_font_test.c'],
    'region_runtime': ['port/port_region_runtime_data_test.c', 'port/port_offset_remap.c'],
    'region_data': ['port/port_region_data_test.c', 'port/port_region_data.c'],
    'flags': ['port/port_flag_runtime_test.c', 'src/flags.c', 'port/flag_remap_generated.c'],
    'story': ['port/port_save_story_region_test.c', 'port/flag_remap_generated.c'],
    'save_layout': ['port/port_save_layout_test.c'],
    'save_persistence': ['port/port_save.c', 'port/port_save_persistence_test.c'],
    'save_migration': ['port/port_save.c', 'src/save.c', 'port/port_save_migration_integration_test.c'],
    'vaati': ['port/port_vaati_progress.c', 'port/port_vaati_progress_test.c'],
    'cloud_tops': ['port/port_cloud_tops_fight.c', 'port/port_cloud_tops_fight_test.c'],
    'delayed_script': ['src/manager/delayedEntityLoadManager.c', 'port/port_delayed_entity_script_slot_test.c'],
    'second_screen_state': ['port/port_second_screen_state.c', 'port/port_second_screen_state_test.c'],
    'bottom_frames': ['platform/3ds/source/bottom_frame_state_3ds.c', 'platform/3ds/tests/bottom_frame_state_3ds_test.c'],
    'bottom_map': ['platform/3ds/source/bottom_map_anim_3ds.c', 'platform/3ds/tests/bottom_map_anim_3ds_test.c'],
}

def main():
    out = ROOT / '.tmp/korean-host-tests'
    out.mkdir(parents=True, exist_ok=True)
    flags = ['-std=gnu11', '-O1', '-g', '-DPC_PORT', '-DUSA', '-DENGLISH', '-DTMC_3DS',
             '-DPORT_SAVE_TEST', '-ffunction-sections', '-fdata-sections']
    if '--sanitize' in sys.argv:
        flags += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-fno-sanitize-recover=all']
    for inc in ('.', 'include', 'port', 'platform/3ds/source'):
        flags += ['-I', str(ROOT / inc)]
    flags += ['-Wl,-dead_strip' if sys.platform == 'darwin' else '-Wl,--gc-sections']
    for name, sources in TESTS.items():
        binary = out / name
        # The existing snapshot test is a single-region engine fixture.
        defines = [] if name == 'second_screen_state' else ['-DMULTI_REGION']
        command = [os.environ.get('CC', 'cc'), *flags, *defines,
                   *(str(ROOT / src) for src in sources), '-lm', '-o', str(binary)]
        subprocess.run(command, cwd=ROOT, check=True)
        case_dir = out / (name + '-work')
        case_dir.mkdir(exist_ok=True)
        subprocess.run([str(binary)], cwd=case_dir, check=True)
    print(f'PASS: {len(TESTS)} host test programs')

if __name__ == '__main__':
    main()
