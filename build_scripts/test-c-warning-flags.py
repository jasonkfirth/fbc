"""Project: FreeBASIC build regression tests
File: test-c-warning-flags.py
Purpose: Check the generated-C warning flags selected by Make.
Responsibilities: Inspect native GCC/Clang flags and explicit user overrides.
This script does not build or install project files.
"""

import argparse
import subprocess
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = next(path for path in HERE.parents if (path / 'mk/compiler-sources.mk').is_file())
DEFAULT_FLAGS = ROOT / 'mk/toolchain-flags.mk'


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--flags', type=Path, default=DEFAULT_FLAGS)
    parser.add_argument('--make', default='make')
    args = parser.parse_args()
    flags = args.flags.resolve()
    with tempfile.TemporaryDirectory(prefix='fbc-make-warnings-') as directory:
        probe = Path(directory) / 'probe.mk'
        probe.write_text('$(info BASE_C=$(BASE_WARN_CFLAGS))\n'
                         '$(info FBC_FLAGS=$(ALLFBCFLAGS))\n'
                         '$(info FBRTC_FLAGS=$(ALLFBRTCFLAGS))\n'
                         '$(info FBRTL_FLAGS=$(ALLFBRTLFLAGS))\n'
                         '.PHONY: probe\nprobe:\n\t@:\n', encoding='utf-8')
        for target in ('win64', 'linux'):
            for cc in ('gcc', 'clang'):
                for explicit in (False, True):
                    command = [args.make, '-n', '-f', str(flags), '-f', str(probe),
                               'TARGET_OS=' + target, 'TARGET_ARCH=x86_64', 'CC=' + cc, 'probe']
                    if explicit:
                        command.append('CFLAGS=-Wno-unused-parameter')
                    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True,
                                            errors='replace', timeout=30)
                    if result.returncode:
                        raise RuntimeError('Make flag inspection failed:\n' + result.stdout + result.stderr)
                    values = dict(line.split('=', 1) for line in result.stdout.splitlines() if '=' in line)
                    for name in ('BASE_C', 'FBC_FLAGS', 'FBRTC_FLAGS', 'FBRTL_FLAGS'):
                        if name not in values:
                            raise RuntimeError('Make did not report ' + name)
                    if '-Wall' not in values['BASE_C'] or '-Wextra' not in values['BASE_C']:
                        raise RuntimeError('the normal C warning flags are missing')
                    if '-Wno-' in values['BASE_C']:
                        raise RuntimeError('the base C flags suppress a warning')
                    for name in ('FBC_FLAGS', 'FBRTC_FLAGS', 'FBRTL_FLAGS'):
                        if not explicit and '-Wno-' in values[name]:
                            raise RuntimeError(name + ' suppresses a generated-C warning')
                    if explicit and '-Wno-unused-parameter' not in values['FBC_FLAGS']:
                        raise RuntimeError('an explicit user CFLAGS override was dropped')
    print('MAKE_WARNING_FLAGS_PASS', flush=True)


if __name__ == '__main__':
    main()

# end of test-c-warning-flags.py
