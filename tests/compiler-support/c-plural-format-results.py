"""FreeBASIC native plural-format diagnostic regression.

File: c-plural-format-results.py
Check both literal contracts and preserve diagnostics for unproven call shapes.
Runtime values belong to the accompanying BASIC and native C observers.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile


def diagnostic_source(family, variant):
    domain = family != 'ngettext'
    category = family == 'dcngettext'
    count_type = {'signed-count': 'long', 'wrong-count-width': 'ushort'}.get(variant, 'plural_count_t')
    result_type = {'wrong-result': 'any ptr', 'const-result': 'const zstring ptr'}.get(variant, 'zstring ptr')
    parameters = ['byval one as const zstring ptr', 'byval many as const zstring ptr',
                  'byval count as ' + count_type]
    arguments = ['"%s one"' if variant in ('bad-singular', 'saved-bad-singular') else '"%d one"',
                 '"%s many"' if variant in ('bad-plural', 'saved-bad-plural') else '"%d many"', '0']
    if domain:
        parameters.insert(0, 'byval domain as const zstring ptr')
        arguments.insert(0, '"domain"')
    if category:
        parameters.append('byval category as ' + ('ushort' if variant == 'wrong-category' else 'long'))
        arguments.append('5')
    if variant in ('dynamic-singular', 'dynamic-plural'):
        arguments[int(domain) + int(variant == 'dynamic-plural')] = '@dynamicFormat'
    declaration = f'declare function translate cdecl alias "{family}"(' + ', '.join(parameters) + ') as ' + result_type
    call = 'translate(' + ', '.join(arguments) + ')'
    setup = ''
    if variant.startswith('saved'):
        setup = 'var savedFormat = ' + call + '\n'
        if variant == 'saved-after-call':
            declaration += '\ndeclare sub barrier cdecl alias "probe_plural_barrier"()'
            setup += 'barrier()\n'
        call = 'savedFormat'
    elif variant == 'indirect':
        setup = 'dim as function cdecl(' + ', '.join(parameters) + ') as ' + result_type + ' translatePointer = @translate\n'
        call = 'translatePointer(' + ', '.join(arguments) + ')'
    elif variant == 'local':
        declaration = 'function translate cdecl(' + ', '.join(parameters) + ') as ' + result_type + '\nreturn 0\nend function'
    value = 'strptr("wrong")' if variant in ('bad-argument', 'saved-bad-argument') else 'clng(7)'
    return ('#lang "fb"\n#include once "crt/stdio.bi"\n'
        '#if defined(__FB_64BIT__) and defined(__FB_UNIX__)\n'
        'type plural_count_t as uinteger alias "long"\n#else\n'
        'type plural_count_t as ulong alias "long"\n#endif\n' + declaration + '\n'
        'sub checkFormat()\ndim as zstring * 32 bufferText\n'
        'dim as zstring * 16 dynamicFormat = "%d dynamic"\n' + setup +
        'snprintf(@bufferText, sizeof(bufferText), ' + call + ', ' + value + ')\nend sub\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc', type=Path, required=True)
    parser.add_argument('--prefix', type=Path, required=True)
    parser.add_argument('--backend', choices=('gcc', 'clang'), required=True)
    parser.add_argument('--cc', default=os.environ.get('GCC') or 'gcc')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    checked_cases = 0
    with tempfile.TemporaryDirectory(prefix='fbc-plural-formats-') as directory:
        work = Path(directory)
        source, cpp, obj = (work / name for name in ('case.bas', 'case.c', 'case.o'))
        for family in ('ngettext', 'dngettext', 'dcngettext'):
            variants = ['', 'bad-singular', 'bad-plural', 'bad-argument',
                'dynamic-singular', 'dynamic-plural', 'saved', 'saved-bad-singular',
                'saved-bad-plural', 'saved-bad-argument', 'saved-after-call', 'indirect', 'wrong-result',
                'const-result', 'signed-count', 'wrong-count-width', 'local']
            if family == 'dcngettext':
                variants.append('wrong-category')
            for variant in variants:
                source.write_text(diagnostic_source(family, variant), encoding='utf-8')
                command = [str(args.fbc.resolve()), '-prefix', str(args.prefix.resolve()),
                    '-i', str(root / 'inc'), '-gen', args.backend, '-w', 'all', '-m', 'plural_diagnostic',
                    '-c', '-r', str(source), '-o', str(cpp)]
                emitted = subprocess.run(command, capture_output=True, text=True)
                if emitted.returncode:
                    raise RuntimeError(emitted.stdout + emitted.stderr)
                checked = subprocess.run([args.cc, '-std=gnu11', '-O2', '-Wall', '-Wextra',
                    '-Wformat=2', '-Werror=format', '-Werror=format-nonliteral',
                    '-c', str(cpp), '-o', str(obj)], capture_output=True, text=True)
                output = checked.stdout + checked.stderr
                if variant not in ('', 'saved'):
                    expected = 'format' if variant.startswith(('bad-', 'saved-bad-')) else 'format-nonliteral'
                    diagnostic = r'\[-W(?:error=|error,-W)' + expected + r'(?:=)?\]'
                    if checked.returncode == 0 or re.search(diagnostic, output) is None:
                        raise RuntimeError(f'{family}/{variant}: expected native {expected} diagnostic\n{output}')
                elif checked.returncode:
                    raise RuntimeError(f'{family}: literal plural contract failed\n{output}')
                checked_cases += 1
    print(f'{args.backend}: PLURAL_FORMAT_DIAGNOSTICS_PASS {checked_cases}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

# end of c-plural-format-results.py
