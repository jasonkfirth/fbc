"""Project: FreeBASIC C backend regressions
File: c-asm-stack-options.py
Purpose: Preserve per-module native stack options across stage-one lifetimes.
Responsibilities: Compare plain and ASM modules in both source orders and keep
platform stack defaults. Runtime values belong to c-asm-register-effects.bas.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc', required=True)
    parser.add_argument('--prefix', required=True)
    parser.add_argument('--backend', choices=('gcc', 'clang'), required=True)
    parser.add_argument('--architecture', required=True)
    parser.add_argument('--work-dir', type=Path, required=True)
    parser.add_argument('--receipt', type=Path)
    args = parser.parse_args()
    rows = []
    with tempfile.TemporaryDirectory(prefix='asm-stack-options-', dir=args.work_dir.resolve(strict=True)) as directory:
        work = Path(directory)
        plain, assembly = work / 'plain.bas', work / 'assembly.bas'
        plain.write_text('#lang "fb"\nfunction plain_value(byval seed as long) as long\nreturn seed + 7\nend function\n')
        assembly.write_text('''#lang "fb"
private sub unused_assembly()
    asm nop
end sub

function asm_value(byval seed as long) as long
    dim observed as long = seed + 7
    asm
#ifdef __FB_64BIT__
        push rbx
#else
        push ebx
#endif
        mov eax, dword ptr [observed]
        add eax, dword ptr [seed]
        mov dword ptr [observed], eax
#ifdef __FB_64BIT__
        pop rbx
#else
        pop ebx
#endif
    end asm
    return observed
end function
''')
        common = [args.fbc, '-prefix', args.prefix, '-gen', args.backend,
                  '-asm', 'intel', '-O', '2', '-w', 'all', '-Wc', '-Wall,-Wextra,-Werror', '-c', '-R', '-v']
        baseline = None
        for sources in ((plain,), (assembly, plain), (plain, assembly)):
            command = list(common)
            for source in sources:
                command += [str(source), '-o', str(source.with_suffix('.o'))]
            result = subprocess.run(command, cwd=work, stdin=subprocess.DEVNULL, capture_output=True, text=True)
            output = result.stdout + result.stderr
            if result.returncode:
                raise RuntimeError(output)
            lines = [line for line in output.splitlines() if line.startswith('compiling C:')]
            options = {}
            for source in sources:
                cpp = source.with_suffix('.c')
                matches = [line for line in lines if cpp.name in line]
                assert len(matches) == 1, (source, output)
                flag = re.search(r'(?:^|\s)-mno-red-zone(?:\s|$)', matches[0]) is not None
                code = cpp.read_bytes()
                marker = b'/* Compile x86-64 inline ASM with -mno-red-zone. */' in code
                expected_marker = source == assembly and args.architecture == 'x86_64'
                assert marker == expected_marker, (source, marker, expected_marker)
                options[source.name] = flag
            if baseline is None:
                baseline = options[plain.name]
            assert options[plain.name] == baseline, (command, output)
            if assembly in sources:
                assert options[assembly.name] == (baseline or args.architecture == 'x86_64'), (command, output)
            rows.append(dict(command=command, output=output, native_options=options))
            assert sum(p.stat().st_size for p in work.iterdir()) < 4 * 1024 * 1024
    if args.receipt:
        args.receipt.write_text(json.dumps(dict(checks_complete=True, rows=rows,
            compiler_sha256=hashlib.sha256(Path(args.fbc).read_bytes()).hexdigest(),
            architecture=args.architecture, backend=args.backend,
            scratch_directory_removed=not work.exists()), indent=2))
    print('ASM_STACK_MODULE_OPTIONS_PASS ' + args.backend)


if __name__ == '__main__':
    main()

# end of c-asm-stack-options.py
