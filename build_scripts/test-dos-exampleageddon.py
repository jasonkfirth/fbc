#!/usr/bin/env python3
"""FreeBASIC DOS sweep contracts; test-dos-exampleageddon.py.

Reject incomplete or ambiguous receipts and preserve the shared applicability
gate. These host controls do not claim DOS compiler/runtime qualification.
"""
import argparse
import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('dos_examples', ROOT / 'build_scripts/msdos-exampleageddon.py')
runner = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = runner
spec.loader.exec_module(runner)


class DosReceipts(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix='fbc-dos-receipts-')
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.batch = self.root / 'sweep/B000'
        self.batch.mkdir(parents=True)
        self.entries = [dict(id='0000', path='examples/one.bas', group='self-contained',
                             reason='controlled example', runnable=True, batch='B000')]
        self.args = argparse.Namespace(root=self.root, outdir=self.batch.parent,
            report_dir=self.root / 'reports', sdk=None, fbc=['controlled-compiler'],
            target_os='dos', no_run=False)

    def test_incomplete_and_failed_runs_cannot_pass(self):
        self.assertFalse(runner.write_reports(self.entries, {}, self.args))
        summary = json.loads((self.args.report_dir / 'summary.json').read_text())
        self.assertEqual((summary['expected'], summary['recorded'], summary['complete']), (1, 0, False))
        self.assertEqual((summary['pending'], summary['self_contained_failures']), (1, 0))
        for status in ['1', 'timeout', 'output-limit', 'not-run']:
            with self.subTest(status=status):
                row = ['0000', 'self-contained', '0', 'present', status, 'not-applicable']
                self.assertFalse(runner.write_reports(self.entries, {'0000': row}, self.args))
        good = ['0000', 'self-contained', '0', 'present', '0', 'not-applicable']
        self.assertTrue(runner.write_reports(self.entries, {'0000': good}, self.args))

    def test_duplicate_extra_and_changed_category_rows_are_rejected(self):
        good = '0000\tself-contained\t0\tpresent\t0\tnot-applicable\n'
        invalid = [good + good, good.replace('0000', '0999'),
                   good.replace('self-contained', 'external-library'), '0000\t0\n',
                   good.replace('\tpresent\t', '\tunknown\t')]
        for text in invalid:
            with self.subTest(text=text):
                (self.batch / 'RESULT.TSV').write_text(text)
                with self.assertRaises(ValueError):
                    runner.read_rows(self.batch, self.entries)
        (self.batch / 'RESULT.TSV').write_text(good)
        self.assertEqual(len(runner.read_rows(self.batch, self.entries)), 1)

    def test_external_failures_remain_visible_in_complete_inventory(self):
        self.entries[0].update(group='external-library', runnable=False)
        row = ['0000', 'external-library', '1', 'missing', 'not-run', '1']
        self.assertTrue(runner.write_reports(self.entries, {'0000': row}, self.args))
        summary = json.loads((self.args.report_dir / 'summary.json').read_text())
        self.assertEqual(summary['compile_status'], {'fail(1-missing)': 1})
        self.assertEqual(summary['recorded'], 1)

    def test_source_and_resource_changes_invalidate_fingerprint(self):
        resources = self.root / 'inputs'
        resources.mkdir()
        (resources / 'example.bas').write_text('print 1')
        before = runner.fingerprint(resources)
        (resources / 'image.dat').write_bytes(b'fixture')
        self.assertNotEqual(before, runner.fingerprint(resources))
        before = runner.fingerprint(resources)
        (resources / 'example.bas').write_text('print 2')
        self.assertNotEqual(before, runner.fingerprint(resources))

    def test_dos_dependency_uses_dxe_and_short_native_batch_commands(self):
        directory = self.root / 'examples/dll'
        directory.mkdir(parents=True)
        (directory / 'test.bas').write_text('print 1\n')
        (directory / 'mydll.bas').write_text('end\n')
        self.entries[0].update(path='examples/dll/test.bas', group='platform-specific', runnable=False)
        runner.prepare_batch(self.batch, self.entries, self.args)
        batch = (self.batch / 'RUNBATCH.BAT').read_text()
        self.assertIn('C:\\FB\\FBC.EXE @D:\\TMP\\', batch)
        self.assertNotIn('BASH.EXE', batch)
        self.assertNotIn('2>&1', batch)
        self.assertIn('> D:\\LOG\\0000C.LOG', batch)
        self.assertNotIn('REDIR.EXE', batch)
        self.assertIn('-p -sD:\\TMP\\CWSDPMI.SWP >', batch)
        self.assertLess(batch.index('CWSDPMI.EXE -u'), batch.index('echo DONE='))
        responses = '\n'.join(path.read_text() for path in (self.batch / 'TMP').glob('*.RSP'))
        self.assertIn('mydll.dxe', responses)
        self.assertIn('-target\ndos\n', responses)

    def test_native_redirection_preserves_short_command_tails(self):
        self.assertEqual(runner.log_name('0123', 'dep-cxx'), '0123X.LOG')
        command = runner.redirected('C:\\FB\\FBC.EXE @D:\\TMP\\01230.RSP', 'D:/LOG/0123C.LOG')
        self.assertEqual(command, 'C:\\FB\\FBC.EXE @D:\\TMP\\01230.RSP > D:\\LOG\\0123C.LOG')
        with self.assertRaises(ValueError):
            runner.redirected('X' * 130, 'D:/LOG/0123C.LOG')

    def test_unrecorded_cases_cannot_reuse_an_old_artifact(self):
        directory = self.root / 'examples'
        directory.mkdir()
        (directory / 'one.bas').write_text('print 1\n')
        binary = self.batch / 'BIN/0000.EXE'
        binary.parent.mkdir()
        binary.write_bytes(b'old executable')
        runner.prepare_batch(self.batch, self.entries, self.args)
        self.assertFalse(binary.exists())

    def test_priority_order_keeps_the_complete_inventory(self):
        self.assertEqual(runner.batch_starts(9, 4, 1), [4, 8, 0])
        self.assertEqual(runner.batch_starts(9, 4, 0), [0, 4, 8])
        for first in (-1, 3):
            with self.assertRaises(ValueError):
                runner.batch_starts(9, 4, first)


if __name__ == '__main__':
    unittest.main(verbosity=2)

# end of test-dos-exampleageddon.py
