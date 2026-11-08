"""Project: FreeBASIC native C regression tests
File: test_native_c_support.py
Purpose: Protect the tool capability boundaries used by hosted CI.
Responsibilities: Exercise older C23 spellings and Clang's GNU command alias.
This file intentionally does NOT compile fixtures or change diagnostic policy.
"""
import subprocess
from pathlib import Path
import sys
import unittest
from unittest.mock import patch
from native_c_support import c23_standard, is_gnu_compiler, native_command, resolve_executable


class NativeCompilerContracts(unittest.TestCase):
    def setUp(self):
        c23_standard.cache_clear()
        is_gnu_compiler.cache_clear()

    def test_draft_c23_spelling_keeps_the_requested_mode(self):
        def compiler(command, **unused):
            return subprocess.CompletedProcess(command, int('-std=gnu23' in command), '', '')
        with patch('native_c_support.subprocess.run', side_effect=compiler) as run:
            self.assertEqual(c23_standard('gcc-13'), 'gnu2x')
            self.assertEqual(len(run.call_args_list), 2)
            self.assertNotIn('-std=gnu11', str(run.call_args_list))

    def test_real_native_executable_path(self):
        command = sys.executable
        if sys.platform in ('msys', 'cygwin'):
            command = subprocess.check_output(['cygpath', '-w', command], text=True).strip()
        self.assertEqual(resolve_executable(command), Path(sys.executable).resolve(strict=True))

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


if __name__ == '__main__':
    unittest.main()

# end of test_native_c_support.py
