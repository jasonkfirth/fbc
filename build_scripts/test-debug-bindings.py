#!/usr/bin/env python3
"""Project: FreeBASIC C backend qualification
File: test-debug-bindings.py
Purpose: Check compiler-owned scoped values and private debug array metadata.
Responsibilities: Run existing value controls with both C generators and native
compilers, inspect named debugger storage, and keep one bounded scratch directory.
This file intentionally does NOT execute graphics calls or foreign objects.
"""
from __future__ import annotations

import argparse
from collections import Counter
import importlib.util
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[1]
FIXTURES = (
    ('c-debug-unused-temporaries', 'C_DEBUG_UNUSED_TEMPORARIES_PASS'),
    ('c-unused-call-results', 'C_UNUSED_CALL_RESULTS_PASS'),
    ('c-unused-pointer-stores', 'POINTER_STORE_VALUES_PASS'),
    ('c-unused-store-bindings', 'C_UNUSED_STORE_BINDINGS_PASS'),
    ('c-unused-scalar-stores', 'C_UNUSED_SCALAR_STORES_PASS'),
    ('c-opaque-local-metadata', 'C_OPAQUE_LOCAL_METADATA_PASS'),
    ('c-debug-array-descriptors', 'C_DEBUG_ARRAY_DESCRIPTORS_PASS'),
    ('c-debug-compiler-bindings', 'C_DEBUG_COMPILER_BINDINGS_PASS'),
    ('c-debug-implicit-user', 'C_DEBUG_IMPLICIT_USER_PASS'),
)



def named_debug_shape(dump, token):
    """Read the named variable's C type independently of DIE address changes."""
    entries, current, stack = {}, None, []
    for line in dump.splitlines():
        match = re.match(r'\s*<(\d+)><([0-9a-fA-F]+)>:.*?(DW_TAG_\w+)?\)?$', line)
        if match and 'Abbrev Number:' in line:
            depth, offset = int(match[1]), int(match[2], 16)
            tag = re.search(r'DW_TAG_\w+', line)
            while stack and stack[-1][0] >= depth:
                stack.pop()
            current = None
            if tag:
                current = {'tag': tag[0], 'attrs': {}, 'children': []}
                entries[offset] = current
                if stack:
                    stack[-1][1]['children'].append(current)
                stack.append((depth, current))
        elif current is not None:
            attr = re.search(r'(DW_AT_\w+)\s*:\s*(.*)', line)
            if attr:
                current['attrs'][attr[1]] = attr[2]
    variables = [e for e in entries.values() if e['tag'] == 'DW_TAG_variable'
                 and token in e['attrs'].get('DW_AT_name', '')]
    assert len(variables) == 1, (token, len(variables))
    def referenced(entry):
        return entries[int(re.search(r'<0x([0-9a-fA-F]+)>', entry['attrs']['DW_AT_type'])[1], 16)]
    def number(value):
        return int(re.search(r'(?:0x[0-9a-fA-F]+|\d+)\s*$', value)[0], 0)
    entry = referenced(variables[0])
    bounds = []
    for unused in range(32):
        if entry['tag'] == 'DW_TAG_array_type':
            for child in entry['children']:
                if child['tag'] != 'DW_TAG_subrange_type':
                    continue
                attrs = child['attrs']
                lower = number(attrs.get('DW_AT_lower_bound', '0'))
                count = number(attrs['DW_AT_count']) if 'DW_AT_count' in attrs else number(attrs['DW_AT_upper_bound']) - lower + 1
                bounds.append((lower, count))
        if entry['tag'] == 'DW_TAG_base_type':
            return {'element_bytes': number(entry['attrs']['DW_AT_byte_size']), 'bounds': bounds,
                    'encoding': entry['attrs'].get('DW_AT_encoding', '')}
        entry = referenced(entry)
    raise AssertionError('unexpected debug type chain')


def typed_body(text, name):
    match = re.search(r'(?m)^static\s+[^;\n]+?\b' + re.escape(name) +
                      r'\s*\([^;\n]*\)\s*(?:#line[^\n]*\n\s*)?\{', text)
    assert match, name
    depth = 1
    for end in range(match.end(), len(text)):
        depth += (text[end] == '{') - (text[end] == '}')
        if depth == 0:
            return text[match.end():end]
    raise AssertionError('unterminated body')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc', type=Path, required=True)
    parser.add_argument('--reference', type=Path)
    parser.add_argument('--prefix', type=Path, required=True)
    parser.add_argument('--tool-dir', type=Path, required=True)
    parser.add_argument('--work-dir', type=Path)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    environment = os.environ.copy()
    environment['PATH'] = str(args.tool_dir.resolve()) + os.pathsep + environment.get('PATH', '')
    spec = importlib.util.spec_from_file_location('debug_scratch_checks',
        ROOT / 'tests/compiler-support/c-debug-unused-temporaries.py')
    old_checks = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(old_checks)
    state = {'checks_complete': False, 'rows': [], 'native_value_runs': 0,
             'foreign_objects_executed': False, 'graphics_library_required': False,
             'source_fixtures_changed': False, 'debug_binding_checks': 0, 'named_debug_type_identities': 0, 'named_debug_types': []}
    roles = [('candidate', args.fbc)]
    if args.reference:
        roles.insert(0, ('reference', args.reference))
    with tempfile.TemporaryDirectory(prefix='debug-scratch-', dir=args.work_dir) as temporary:
        work = Path(temporary)
        cpp, obj, exe = (work / ('unit.' + ext) for ext in ('c', 'o', 'exe'))

        def run(command):
            result = subprocess.run([str(p) for p in command], cwd=work, env=environment,
                capture_output=True, timeout=600, check=False)
            output = (result.stdout + result.stderr).decode(errors='replace')
            assert result.returncode == 0, (command, output[-5000:])
            return output

        for fixture, marker in FIXTURES:
            source = ROOT / 'tests/compiler-support' / (fixture + '.bas')
            for bits in (64, 32):
                for generator in ('gcc', 'clang'):
                    for opt in (0, 2):
                        baseline = {}
                        debug_baseline = {}
                        for role, compiler in roles:
                            run([compiler.resolve(), '-prefix', args.prefix.resolve(), '-gen', generator,
                                '-asm', 'intel', '-g', '-O', opt, *(['-exx'] if opt == 2 else []),
                                *(['-target', 'win32'] if bits == 32 else []), '-m', fixture,
                                '-r', source, '-o', cpp])
                            text = cpp.read_text(encoding='utf-8')
                            # The existing body checker expects the Win64 spelling.
                            # Normalize only its input text; compile the original C.
                            parsed_text = text.replace('static void __stdcall ', 'static void ')
                            if fixture == 'c-debug-unused-temporaries':
                                for name, keep in (('CHECKDEAD', False), ('CHECKOPAQUE', True)):
                                    body = old_checks.function_body(parsed_text, name)
                                    names = re.findall(r'\b(fb_c_sym_TMP_\w+)\s*;', body)
                                    unused = [n for n in names if len(re.findall(r'\b' + re.escape(n) + r'\b', body)) == 1]
                                    assert bool(unused) == keep, (role, bits, generator, name)
                                body = old_checks.function_body(parsed_text, 'CHECKVISIBLE')
                                assert 'fb_c_sym_WATCHONLY_' in body and 'fb_c_sym_USERVALUE_' in body
                                state['debug_binding_checks'] += 1
                            if fixture == 'c-debug-compiler-bindings':
                                parsed = re.sub(r'\b__stdcall\s+', '', text)
                                body = typed_body(parsed, 'DIRECTSTATIC')
                                assert 'fb_c_sym_DEBUGSTATICDATA_' in body
                                descriptor = bool(re.search(r'\.DATA\s*=.*fb_c_sym_DEBUGSTATICDATA_', body))
                                assert descriptor == (role == 'reference'), (role, bits, generator, body)
                                assert re.search(r'\.DATA\s*=.*fb_c_sym_PASSEDSTATICDATA_', typed_body(parsed, 'PASSEDSTATIC'))
                                early = typed_body(parsed, 'EARLYLOOP')
                                temporaries = re.findall(r'\b(fb_c_sym_(?:TMP|tmp)_\w+)\s*;', early)
                                unused = [n for n in temporaries if len(re.findall(r'\b' + re.escape(n) + r'\b', early)) == 1]
                                if role == 'candidate':
                                    assert not unused, (bits, generator, opt, unused)
                                dead_strings = typed_body(parsed, 'CHECKDEADSTRINGS')
                                assert not re.search(r'FBSTRING\s+fb_c_sym_TMP_', dead_strings), (role, bits, generator, dead_strings)
                                state['debug_binding_checks'] += 1
                            if fixture == 'c-debug-implicit-user':
                                assert 'fb_c_sym_WATCHIMPLICIT_' in text
                                state['debug_binding_checks'] += 1
                            for cc in ('gcc', 'clang'):
                                flags = ['-std=gnu11', '-nostdinc', '-fno-strict-aliasing', '-fwrapv', '-masm=intel',
                                    '-O' + str(opt), '-g', '-Wall', '-Wextra', '-Wpedantic', '-Wconversion',
                                    '-Wsign-conversion', '-Wshadow', '-Wformat=2', '-Wnull-dereference',
                                    '-Warray-bounds', '-Wunused-but-set-variable']
                                if cc == 'clang':
                                    flags.append('-Weverything')
                                output = run([args.tool_dir / (cc + '.exe'), *flags,
                                    *(['-m32'] if bits == 32 else []), '-c', cpp, '-o', obj])
                                warnings = Counter(re.findall(r'\[(-W[^\]]+)\]', output))
                                if role == 'reference':
                                    baseline[cc] = warnings
                                elif args.reference:
                                    assert not warnings - baseline[cc], (fixture, bits, generator, opt, cc, warnings, baseline[cc])
                                if bits == 64 and cc == 'gcc' and opt == 0 and fixture == 'c-debug-unused-temporaries':
                                    dwarf = run([args.tool_dir / 'objdump.exe', '--dwarf=info', obj])
                                    assert 'WATCHONLY' in dwarf and 'USERVALUE' in dwarf
                                    state['debug_binding_checks'] += 1
                                if cc == 'gcc' and opt == 0 and fixture in ('c-debug-compiler-bindings', 'c-debug-implicit-user'):
                                    dwarf = run([args.tool_dir / 'objdump.exe', '--dwarf=info', obj])
                                    token = 'DEBUGSTATICDATA' if fixture == 'c-debug-compiler-bindings' else 'WATCHIMPLICIT'
                                    shape = named_debug_shape(dwarf, token)
                                    if fixture == 'c-debug-compiler-bindings':
                                        assert shape['element_bytes'] == 4 and shape['bounds'] == [(0, 3)], shape
                                    if role == 'reference':
                                        debug_baseline[cc] = shape
                                    elif args.reference:
                                        assert shape == debug_baseline[cc], (fixture, bits, generator, shape, debug_baseline[cc])
                                        state['named_debug_type_identities'] += 1
                                    state['named_debug_types'].append({'fixture': fixture, 'role': role,
                                        'bits': bits, 'generator': generator, 'shape': shape})
                                runs = 0
                                if bits == 64:
                                    # These graphics-free controls can carry an unused fbgfx link
                                    # directive. Link their actual native dependencies explicitly;
                                    # no graphics archive, stub or alternate runtime is substituted.
                                    library = args.prefix.resolve() / 'lib/freebasic/win64'
                                    for mt in (False, True):
                                        run([args.tool_dir / 'gcc.exe', '-mconsole', obj,
                                            library / 'fbrt0.o', library / ('libfbmt.a' if mt else 'libfb.a'),
                                            '-lm', '-mthreads', '-o', exe])
                                        values = run([exe])
                                        assert values.strip() == marker, (fixture, role, values)
                                        state['native_value_runs'] += 1
                                        runs += 1
                                        exe.unlink()
                                state['rows'].append({'fixture': fixture, 'role': role, 'bits': bits,
                                    'generator': generator, 'optimization': opt, 'native_compiler': cc,
                                    'warnings': dict(warnings), 'native_value_runs': runs})
                                obj.unlink()
                            cpp.unlink()
            print('DEBUG_BINDINGS_FIXTURE_PASS', fixture, flush=True)
    state['checks_complete'] = True
    if args.report:
        args.report.write_text(json.dumps(state, separators=(',', ':')) + '\n', encoding='utf-8')
    print('DEBUG_BINDINGS_PASS', len(state['rows']), 'native checks', state['native_value_runs'],
          'value runs', state['debug_binding_checks'], 'debug binding checks', flush=True)


if __name__ == '__main__':
    main()

# end of test-debug-bindings.py
