#!/usr/bin/env python3
"""Project: FreeBASIC XL hosted CI
File: ci-change-scope.py
Purpose: Select documentation checks when no build inputs have changed.
Responsibilities: Inspect the tested Git range and preserve package gating.
This file does not suppress failures or qualify an untested compiler.
"""
import json
import os
from pathlib import Path, PurePosixPath
import re
import subprocess
import urllib.request


def documentation_path(name):
    """Only explicit prose locations can bypass compiler qualification.

    Documentation trees also contain programs and generated build inputs.
    Do not treat the whole tree, or arbitrary Markdown outside it, as prose.
    """
    if name in ('readme.txt', 'changelog.txt'):
        return True
    path = PurePosixPath(name)
    return ((name.startswith('docs/') and path.suffix == '.md') or
            (name.startswith('doc/manual/cache/') and
             path.suffix in ('.md', '.wakka')))


def build_scope(event_name, event, root):
    if event_name == 'push' and event.get('ref', '').startswith('refs/heads/'):
        base = event.get('before', '')
    elif event_name == 'pull_request':
        base = event.get('pull_request', {}).get('base', {}).get('sha', '')
    else:
        return True, 'manual, scheduled, reusable or tag qualification'
    if not re.fullmatch(r'[0-9a-f]{40}', base) or base == '0' * 40:
        return True, 'no reliable comparison base'
    # A two-commit checkout normally contains the push parent or PR merge
    # parents. Missing history deliberately runs the full matrix without
    # fetching an unbounded history or making assumptions about HEAD^.
    result = subprocess.run(
        ['git', 'diff', '--no-ext-diff', '--no-renames', '--name-only', '-z',
         base, 'HEAD', '--'], cwd=root, capture_output=True, timeout=30)
    if result.returncode:
        return True, 'comparison base unavailable'
    names = result.stdout.decode('utf-8', errors='surrogateescape').split('\0')
    names = [name for name in names if name]
    if not names or not all(documentation_path(name) for name in names):
        return True, 'build inputs changed or empty comparison'
    return False, f'documentation only ({len(names)} paths)'


def package_scope(jobs):
    """A successful prose check is not a successful native qualification.

    Only the explicit planner/docs success plus skipped bootstrap combination
    suppresses automatic packages. Older workflows and unknown layouts retain
    full package validation. Failed CI is rejected by the workflow job guard.
    """
    conclusions = {job['name']: job.get('conclusion') for job in jobs}
    documentation_only = (
        conclusions.get('Select CI scope') == 'success' and
        conclusions.get('Source documentation') == 'success' and
        conclusions.get('Generate native bootstrap sources') == 'skipped')
    return not documentation_only


def qualified_jobs(event):
    run = event['workflow_run']
    run_id = run['id']
    if not isinstance(run_id, int) or run_id <= 0:
        raise ValueError('invalid qualification run id')
    repository = os.environ['GITHUB_REPOSITORY']
    if not re.fullmatch(r'[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+', repository):
        raise ValueError('invalid qualification repository')
    jobs = []
    # The current matrix has fewer than 100 jobs. Retain pagination for future
    # platform additions, with a finite bound on API requests and responses.
    for page in range(1, 11):
        request = urllib.request.Request(
            f'https://api.github.com/repos/{repository}/actions/runs/'
            f'{run_id}/jobs?per_page=100&page={page}',
            headers={'Authorization': 'Bearer ' + os.environ['GITHUB_TOKEN'],
                     'User-Agent': 'FreeBASIC-CI-scope',
                     'Accept': 'application/vnd.github+json'})
        with urllib.request.urlopen(request, timeout=30) as response:
            data = response.read(2 * 1024 * 1024 + 1)
        if len(data) > 2 * 1024 * 1024:
            raise ValueError('qualification job response exceeds limit')
        rows = json.loads(data)['jobs']
        jobs.extend(rows)
        if len(rows) < 100:
            return jobs
    raise ValueError('qualification job count exceeds limit')


def main():
    event = json.loads(Path(os.environ['GITHUB_EVENT_PATH']).read_text(encoding='utf-8'))
    event_name = os.environ['GITHUB_EVENT_NAME']
    if event_name == 'workflow_run':
        run_builds = package_scope(qualified_jobs(event))
        reason = 'upstream documentation scope' if not run_builds else 'upstream build qualification'
    elif os.environ.get('CI_SOURCE_REF'):
        run_builds, reason = True, 'explicit source revision override'
    else:
        run_builds, reason = build_scope(event_name, event, Path.cwd())
    print(f'CI scope: {reason}; run_builds={str(run_builds).lower()}')
    with open(os.environ['GITHUB_OUTPUT'], 'a', encoding='utf-8') as output:
        print(f'run_builds={str(run_builds).lower()}', file=output)


if __name__ == '__main__':
    main()

# end of ci-change-scope.py
