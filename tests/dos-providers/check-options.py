#!/usr/bin/env python3
"""FreeBASIC DOS tests: check-options.py.

Check provider selection and compatibility diagnostics in compiler emit-only
mode. No assembler, linker, DOS execution or installed target libraries needed.
"""

import argparse
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc', required=True, type=Path)
    args = parser.parse_args()
    cases = [
        ('ordinary DOS', ['-target', 'dos'], True, '', 'print "DOS"'),
        ('provider', ['-target', 'dos', '-dos-threads', 'pdmlwp'], True, '',
         'dim handle as any ptr = ThreadCreate(0)\nThreadWait(handle)'),
        ('plain mt', ['-target', 'dos', '-mt'], False, '-mt requires', 'print "DOS"'),
        ('ordinary ThreadCreate', ['-target', 'dos'], False, 'Unsupported function',
         'dim handle as any ptr = ThreadCreate(0)'),
        ('SSE', ['-target', 'dos', '-dos-threads', 'pdmlwp', '-fpu', 'sse'],
         False, 'requires -fpu x87', 'print "DOS"'),
        ('wrong target', ['-target', 'win64', '-dos-threads', 'pdmlwp'],
         False, 'requires -target dos', 'print "DOS"'),
        ('unknown provider', ['-target', 'dos', '-dos-threads', 'unknown'],
         False, 'unknown', 'print "DOS"'),
        ('ThreadCall', ['-target', 'dos', '-dos-threads', 'pdmlwp'],
         False, 'Unsupported function', 'sub worker()\nend sub\ndim handle as any ptr = ThreadCall worker()'),
    ]
    with tempfile.TemporaryDirectory(prefix='fb-dos-options-') as work:
        source = Path(work) / 'probe.bas'
        for name, flags, accepted, message, code in cases:
            source.write_text(code + '\n')
            result = subprocess.run([str(args.fbc.resolve()), *flags, '-r', str(source)],
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
            if (result.returncode == 0) != accepted or message not in result.stdout:
                raise RuntimeError(name + ': unexpected result\n' + result.stdout)
            print('PASS ' + name)


if __name__ == '__main__':
    main()

# end of check-options.py
