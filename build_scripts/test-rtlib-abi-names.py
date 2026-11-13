#!/usr/bin/env python3
"""Project: FreeBASIC native CI contracts
File: test-rtlib-abi-names.py
Purpose: Check shared C names across platform consumers and private test trees.
Responsibilities: Preserve quoted linker labels and verify the sound header closure.
This file intentionally does NOT build target SDKs or install runtime archives.
"""
import importlib.util
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    'rtlib_structure', ROOT / 'build_scripts/check-rtlib-structure.py')
structure = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(structure)


class AbiNames(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix='fbc-abi-contract-')
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.write('src/sfxlib/state.h',
                   'extern void *fb_state FB_C_SYMBOL("__fb_state");\n')

    def write(self, name, text):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding='utf-8')
        return path

    def test_platform_templates_cpp_and_helpers_are_checked(self):
        for name in ('src/sfxlib/unix/shared.inc',
                     'src/gfxlib2/drivers/haiku/event.cpp',
                     'tests/sfx/helper.c'):
            self.write(name, 'void *read_state(void) { return __fb_state; }\n')
        failures = structure.stale_abi_references(self.root)
        self.assertEqual(len(failures), 3)
        self.assertTrue(all('use fb_state' in failure for failure in failures))

    def test_linker_labels_assembly_and_notes_keep_their_spelling(self):
        self.write('src/sfxlib/state.c',
                   '/* __fb_state is the linker name. */\n'
                   'void *fb_state;\nconst char *label = "__fb_state";\n')
        self.write('src/rtlib/drivers/x86/state.s', 'movl __fb_state, %eax\n')
        self.assertEqual(structure.stale_abi_references(self.root), [])

    def test_current_platform_and_test_consumers(self):
        self.assertEqual(structure.stale_abi_references(ROOT), [])

    def test_partial_sound_tree_includes_its_shared_abi_header(self):
        compiler = shutil.which('gcc')
        if compiler is None:
            self.skipTest('native GCC is unavailable for the staged-header compile')
        # Reproduce the package runners' source-only tree, without copying the
        # runtime implementation or any installed archive into the test.
        shutil.copytree(ROOT / 'src/sfxlib', self.root / 'src/sfxlib',
                        dirs_exist_ok=True,
                        ignore=shutil.ignore_patterns('obj', '*.c', '*.cpp', '*.a', '*.o'))
        source = self.root / 'tests/sfx/midi-fm-test-rate.c'
        source.parent.mkdir(parents=True)
        shutil.copyfile(ROOT / 'tests/sfx/midi-fm-test-rate.c', source)
        command = [compiler, '-Wall', '-Wextra', '-Werror', '-c', str(source),
                   '-o', str(self.root / 'helper.o')]
        missing = subprocess.run(command, capture_output=True, text=True, timeout=30)
        self.assertNotEqual(missing.returncode, 0)
        self.assertIn('fb_abi.h', missing.stderr)
        destination = self.root / 'src/rtlib/fb_abi.h'
        destination.parent.mkdir(parents=True)
        shutil.copyfile(ROOT / 'src/rtlib/fb_abi.h', destination)
        complete = subprocess.run(command, capture_output=True, text=True, timeout=30)
        self.assertEqual(complete.returncode, 0, complete.stdout + complete.stderr)


if __name__ == '__main__':
    unittest.main()

# end of test-rtlib-abi-names.py
