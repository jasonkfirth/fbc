#!/usr/bin/env python3
"""Project: FreeBASIC XL CI regression checks
File: test-ci-change-scope.py
Purpose: Prevent prose-only scheduling from hiding source qualification.
Responsibilities: Exercise real Git changes, event fallbacks and package gates.
This file does not build a compiler, contact GitHub or retain scratch trees.
"""
import importlib.util
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parent.parent
SPEC = importlib.util.spec_from_file_location('ci_scope', ROOT / 'build_scripts/ci-change-scope.py')
scope = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(scope)


class ScopeTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix='fbc-ci-scope-')
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.git('init', '-q')
        self.git('config', 'user.name', 'CI scope fixture')
        self.git('config', 'user.email', 'fixture@example.invalid')
        for name in ('docs/guide.md', 'doc/manual/cache/Guide.wakka',
                     'src/compiler/emit.bas', 'changelog.txt'):
            self.write(name)
        self.base = self.commit()

    def git(self, *arguments):
        result = subprocess.run(['git', *arguments], cwd=self.root,
                                capture_output=True, text=True, timeout=15)
        self.assertEqual(result.returncode, 0, result.stderr)
        return result.stdout.strip()

    def write(self, name):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text('fixture\n', encoding='utf-8')

    def commit(self):
        self.git('add', '.')
        self.git('commit', '-qm', 'fixture')
        return self.git('rev-parse', 'HEAD')

    def decision(self, base=None, event='push'):
        payload = {'ref': 'refs/heads/master', 'before': base or self.base,
                   'pull_request': {'base': {'sha': base or self.base}}}
        return scope.build_scope(event, payload, self.root)[0]

    def test_documentation_change_and_cli_output(self):
        self.write('docs/new guide.md')
        self.commit()
        self.assertFalse(self.decision())
        self.assertFalse(self.decision(event='pull_request'))
        event = self.root / 'event.json'
        event.write_text(json.dumps({'ref': 'refs/heads/master', 'before': self.base}))
        output = self.root / 'output'
        environment = os.environ.copy()
        environment.update(GITHUB_EVENT_PATH=str(event), GITHUB_EVENT_NAME='push',
                           GITHUB_OUTPUT=str(output), CI_SOURCE_REF='')
        result = subprocess.run([os.sys.executable, '-B', str(SPEC.origin)],
                                cwd=self.root, env=environment,
                                capture_output=True, text=True, timeout=15)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(output.read_text().strip(), 'run_builds=false')
        environment['CI_SOURCE_REF'] = 'another-revision'
        output.write_text('')
        subprocess.run([os.sys.executable, '-B', str(SPEC.origin)], cwd=self.root,
                       env=environment, check=True, capture_output=True, timeout=15)
        self.assertEqual(output.read_text().strip(), 'run_builds=true')

    def test_source_change_with_documentation(self):
        self.write('docs/new.md')
        self.write('src/compiler/new.bas')
        self.commit()
        self.assertTrue(self.decision())

    def test_full_push_range_including_earlier_source_commit(self):
        self.write('src/compiler/new.bas')
        middle = self.commit()
        self.write('docs/new.md')
        self.commit()
        self.assertFalse(self.decision(middle))
        self.assertTrue(self.decision())

    def test_rename_cannot_hide_source_deletion(self):
        self.git('mv', 'src/compiler/emit.bas', 'docs/renamed.md')
        self.commit()
        self.assertTrue(self.decision())

    def test_documentation_deletion(self):
        self.git('rm', 'docs/guide.md')
        self.commit()
        self.assertFalse(self.decision())

    def test_unknown_missing_empty_and_initial_ranges(self):
        self.assertTrue(self.decision())
        self.assertTrue(self.decision('a' * 40))
        self.assertTrue(self.decision('0' * 40))
        self.assertTrue(self.decision('not-a-revision'))
        self.assertTrue(self.decision('--output=bad'))
        for event in ('schedule', 'workflow_dispatch', 'workflow_call'):
            self.assertTrue(self.decision(event=event))
        self.assertTrue(scope.build_scope('push', {'ref': 'refs/tags/v1',
                                                  'before': self.base}, self.root)[0])

    def test_allowlist_excludes_embedded_programs_and_workflows(self):
        for path in ('docs/guide.md', 'doc/manual/cache/Key.wakka',
                     'changelog.txt', 'readme.txt'):
            self.assertTrue(scope.documentation_path(path), path)
        for path in ('docs/test.bas', 'doc/manual/cache/program.c',
                     'docs/build.py', 'docs/template.h', 'docs/unknown.wakka', 'src/README.md',
                     '.github/workflows/ci.yml', 'makefile', 'inc/fb.bi'):
            self.assertFalse(scope.documentation_path(path), path)

    def test_automatic_packages_require_explicit_documentation_scope(self):
        jobs = [{'name': name, 'conclusion': conclusion} for name, conclusion in (
            ('Select CI scope', 'success'), ('Source documentation', 'success'),
            ('Generate native bootstrap sources', 'skipped'))]
        self.assertFalse(scope.package_scope(jobs))
        self.assertTrue(scope.package_scope([]))
        for index, conclusion in ((0, 'failure'), (1, 'failure'), (2, 'success')):
            changed = [dict(row) for row in jobs]
            changed[index]['conclusion'] = conclusion
            self.assertTrue(scope.package_scope(changed))

    def test_package_job_api_pagination(self):
        class Response:
            def __init__(self, rows):
                self.data = json.dumps({'jobs': rows}).encode()

            def __enter__(self):
                return self

            def __exit__(self, *arguments):
                pass

            def read(self, limit):
                return self.data[:limit]

        first = [{'name': 'fixture', 'conclusion': 'success'}] * 100
        last = [{'name': 'last', 'conclusion': 'success'}]
        with patch.dict(os.environ, GITHUB_REPOSITORY='fixture/fbc',
                        GITHUB_TOKEN='fixture-token'), patch.object(
                            scope.urllib.request, 'urlopen',
                            side_effect=[Response(first), Response(last)]) as request:
            self.assertEqual(scope.qualified_jobs({'workflow_run': {'id': 42}}), first + last)
            self.assertEqual(request.call_count, 2)
            urls = [call.args[0].full_url for call in request.call_args_list]
            self.assertTrue(urls[0].endswith('/42/jobs?per_page=100&page=1'))
            self.assertTrue(urls[1].endswith('/42/jobs?per_page=100&page=2'))

    def test_package_job_api_errors_remain_visible(self):
        with patch.dict(os.environ, GITHUB_REPOSITORY='fixture/fbc',
                        GITHUB_TOKEN='fixture-token'), patch.object(
                            scope.urllib.request, 'urlopen', side_effect=OSError('fixture API error')):
            with self.assertRaisesRegex(OSError, 'fixture API error'):
                scope.qualified_jobs({'workflow_run': {'id': 42}})

    def test_workflow_dependencies_and_package_controller(self):
        # Keep the contract tied to the real workflow layout, including every
        # matrix dependency. An accidentally ungated job burns runner hours.
        for filename, names in (
            ('ci.yml', ('c-lint', 'bootstrap-sources', 'linux', 'windows',
                        'linux-targets', 'macos', 'windows-arm64',
                        'windows-arm64-native', 'mips')),
            ('extended-platforms.yml', ('qemu-guests', 'aros', 'riscos', 'wince'))):
            text = (ROOT / '.github/workflows' / filename).read_text()
            for name in names:
                # Job keys have two spaces; step keys have six or more.
                start = text.index('\n  ' + name + ':\n')
                block = re.split(r'\n  [a-z][a-z0-9-]*:\n', text[start + 1:])[0]
                self.assertIn("needs.changes.outputs.run_builds == 'true'", block)
                self.assertRegex(block, r'    needs: (changes|\[changes, bootstrap-sources\])')
        text = (ROOT / '.github/workflows/packages.yml').read_text()
        block = text.split('  qualification-scope:\n', 1)[1].split('  linux-debian:\n', 1)[0]
        self.assertIn('actions: read', block)
        self.assertNotIn('ref: ${{ github.event.workflow_run.head_sha', block)
        self.assertIn('if [ "$GITHUB_EVENT_NAME" = workflow_run ]', block)
        for name in ('linux-package-plan', 'windows', 'macos'):
            block = text.split('\n  ' + name + ':\n', 1)[1]
            self.assertTrue(block.startswith('    needs: qualification-scope\n'))
            self.assertIn("needs.qualification-scope.outputs.run_builds == 'true'", block.split('    runs-on:', 1)[0])


if __name__ == '__main__':
    unittest.main(verbosity=2)

# end of test-ci-change-scope.py
