"""Project: FreeBASIC native C regression tests
File: test_native_c_support.py
Purpose: Protect the tool capability boundaries used by hosted CI.
Responsibilities: Exercise C23 spellings, BASIC pointer widths, Clang aliases
and exact assembler entry labels.
This file intentionally does NOT compile fixtures or change diagnostic policy.
"""
import subprocess
import sys
import unittest
from unittest.mock import patch
from native_c_support import (c23_standard, is_gnu_compiler, native_command,
                              native_machine_flags, resolve_executable,
                              has_assembly_function)


class NativeCompilerContracts(unittest.TestCase):
    def setUp(self):
        c23_standard.cache_clear()
        is_gnu_compiler.cache_clear()
        native_machine_flags.cache_clear()

    def test_draft_c23_spelling_keeps_the_requested_mode(self):
        def compiler(command, **unused):
            return subprocess.CompletedProcess(command, int('-std=gnu23' in command), '', '')
        with patch('native_c_support.subprocess.run', side_effect=compiler) as run:
            self.assertEqual(c23_standard('gcc-13'), 'gnu2x')
            self.assertEqual(len(run.call_args_list), 2)
            self.assertNotIn('-std=gnu11', str(run.call_args_list))

    def test_real_native_executable_path(self):
        paths = [sys.executable]
        if sys.platform in ('msys', 'cygwin'):
            paths.append(sys.executable[:-4] if sys.executable.endswith('.exe') else sys.executable + '.exe')
        for executable in paths:
            with self.subTest(executable=executable):
                command = executable
                if sys.platform in ('msys', 'cygwin'):
                    command = subprocess.check_output(['cygpath', '-w', command], text=True).strip()
                # MSYS can omit .exe in sys.executable while executable discovery
                # supplies it. Compare the file, not those equivalent spellings.
                self.assertTrue(resolve_executable(command).samefile(executable))

    def test_c23_command_preserves_diagnostics_and_machine_mode(self):
        command = ['gcc-13', '-m32', '-std=gnu23', '-Wall', '-Wextra',
                   '-Werror=unused-parameter', '-c', 'unit.c', '-o', 'unit.o']
        with patch('native_c_support.c23_standard', return_value='gnu2x'):
            expected = ['-std=gnu2x' if flag == '-std=gnu23' else flag for flag in command]
            self.assertEqual(native_command(command), expected)
        self.assertEqual(command[2], '-std=gnu23')

    def test_native_machine_mode_matches_basic_target(self):
        for target, expected in (('win32', ('-m32',)), ('linux-x86', ('-m32',)),
                                 ('win64', ('-m64',)), ('linux-x86_64', ('-m64',)),
                                 ('darwin-aarch64', ()), ('win32-aarch64', ())):
            with self.subTest(target=target), patch('native_c_support.subprocess.run',
                    return_value=subprocess.CompletedProcess([], 0, target + '\n', '')):
                self.assertEqual(native_machine_flags('fbc-' + target), expected)

    def test_missing_c23_mode_fails_instead_of_downgrading(self):
        with patch('native_c_support.subprocess.run', return_value=
                   subprocess.CompletedProcess([], 1, '', 'unsupported mode')):
            with self.assertRaises(RuntimeError):
                c23_standard('old-compiler')

    def test_clang_named_gcc_retains_type_diagnostics(self):
        with patch('native_c_support.subprocess.run', return_value=
                   subprocess.CompletedProcess([], 0, '#define __GNUC__ 4\n#define __clang__ 1\n', '')):
            self.assertFalse(is_gnu_compiler('gcc'))
            command = native_command(['gcc', '-Wcast-align=strict', '-Warray-bounds=2',
                                      '-Werror=incompatible-pointer-types', '-c', 'unit.c'])
            self.assertIn('-Wcast-align', command)
            self.assertIn('-Warray-bounds', command)
            self.assertIn('-Werror=incompatible-pointer-types', command)
            self.assertFalse(any(flag.startswith('-Wno-') for flag in command))

    def test_assembly_entry_uses_explicit_abi_identity(self):
        generated = 'static int BODY(int) __asm__("_BODY@4");\n'
        for assembly in ('_BODY@4:\n', '"_BODY@4": # @BODY\n'):
            with self.subTest(assembly=assembly):
                self.assertTrue(has_assembly_function(generated, assembly, 'BODY'))
        for assembly in ('BODY:\n', '_BODY@8:\n', '_BODY@4_EXTRA:\n',
                         '.ascii "_BODY@4"\n', '.def _BODY@4; .scl 3;\n'):
            with self.subTest(assembly=assembly):
                self.assertFalse(has_assembly_function(generated, assembly, 'BODY'))

    def test_assembly_entry_without_an_explicit_alias(self):
        for label in ('BODY', '_BODY'):
            self.assertTrue(has_assembly_function('static int BODY(int);', label + ':\n', 'BODY'))
        self.assertFalse(has_assembly_function('static int BODY(int);', 'OTHER_BODY:\n', 'BODY'))


if __name__ == '__main__':
    unittest.main()

# end of test_native_c_support.py
