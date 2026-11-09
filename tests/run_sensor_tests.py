"""Run production sensors.c against a fake I2C bus, clock and NVS on Windows.

Usage: python tests/run_sensor_tests.py --clang PATH_TO_CLANG
Only platform includes are replaced; sensor logic is compiled unchanged.
"""
import argparse
import ctypes
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def without_includes(path):
    return re.sub(r'^\s*#include[^\n]*', '', path.read_text(encoding='utf-8'), flags=re.M)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--clang', required=True)
    args = parser.parse_args()
    out = ROOT / 'build' / 'sensor_host_tests'
    out.mkdir(parents=True, exist_ok=True)
    source = (ROOT / 'tests' / 'sensor_platform_stubs.c').read_text(encoding='utf-8')
    source += without_includes(ROOT / 'main/user/ui/ui_user_config.h')
    source += without_includes(ROOT / 'main/user/periphery/sensors.h')
    source += without_includes(ROOT / 'main/user/periphery/sensors.c')
    source += (ROOT / 'tests' / 'sensor_scenarios.c').read_text(encoding='utf-8')
    generated = out / 'sensors_test.c'
    generated.write_text(source, encoding='utf-8')
    dll = out / 'sensors_test.dll'
    env = dict(os.environ, TMP=str(out), TEMP=str(out))
    subprocess.run([args.clang, '-target', 'x86_64-pc-windows-msvc', '-fuse-ld=lld',
                    '-std=c11', '-O0', '-fno-builtin', '-nostdlib', '-shared',
                    '-Wl,/noentry,/nodefaultlib', str(generated), '-o', str(dll)], check=True, env=env)
    lib = ctypes.CDLL(str(dll))
    result = lib.run_sensor_tests()
    if result:
        raise AssertionError(f'Sensor scenario failed at generated C line {result}; see {generated}')
    print('PASS: startup, filtering, CRC/I2C errors, recovery, stale data, 12h learning,')
    print('      hourly persistence, baseline word order, expiry, sensor identity, unknown clock.')


if __name__ == '__main__':
    main()
