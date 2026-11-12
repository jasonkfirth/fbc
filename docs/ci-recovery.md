<!--
Project: FreeBASIC XL CI
File: ci-recovery.md
Purpose: Explain hosted-runner recovery without weakening qualification.
Responsibilities: Explain run scheduling, allocation failures and scoped reruns.
This file does not describe compiler fixes or release publication.
-->

# Recovering hosted CI jobs

Check the job annotations as well as its conclusion. A cancelled job with no
steps and "The job was not acquired by Runner of type hosted" did not build or
test the source. GitHub must supply a runner before repository code can run;
retry logic inside a build step cannot repair that allocation failure.

The macOS ARM64 lane defaults to `macos-26`. If that image cannot acquire a
runner, open **FreeBASIC XL CI**, select **Run workflow**, choose the branch,
set `ci_scope` to `macos-arm64`, and select `macos-15` as
`macos_arm64_runner`. Either image runs the complete native ARM64 build,
self-host, smoke tests, fbctests, syntax tests, and Exampleageddon. A rerun on
another image qualifies that image; it does not prove that the unavailable
image worked.

The scoped run still generates its source bootstrap. It skips unrelated
platform jobs and uses a separate concurrency group so recovery does not
cancel a full run on the same branch. Pushes and pull requests always run the
full matrix. Manual runs default to `all` and can also override the ARM64 image.

For a native Windows ARM64 failure, select `windows-arm64` as `ci_scope`.
This runs the source bootstrap and the complete Windows ARM64 lane, including
self-host, runtime libraries, smoke tests, fbctests, syntax tests and
Exampleageddon. It does not replace native execution with cross-compilation.
Use **Re-run failed jobs** when retrying the same revision and runner is enough;
use a scoped manual run to qualify a repaired revision without repeating all
other platforms. Both ARM64 recovery scopes leave a full run undisturbed.

Master pushes no longer cancel an active main or extended-platform run.
GitHub retains the newest pending run in each workflow's concurrency group;
intermediate pending revisions can be replaced before they start. The active
revision finishes qualification, then the newest pending revision starts.
Each result belongs to its recorded commit, so a passing older run does not
qualify a newer commit. Pull requests and pushes to other branches still
cancel superseded work. To stop an obsolete master run, cancel it explicitly
in Actions. Workflow scheduling changes take effect in runs using the updated
workflow; they cannot change the policy of a run already started.

Automatic package builds accept successful CI triggered by a master push.
Manual recovery runs do not trigger package builds, even when they succeed.
Build and test failures remain failures; runner shortages are not converted
into passing qualification.

## Independent validation after a successful build

Source documentation has a small independent job, so missing links are reported
without waiting for a compiler build. Native jobs still check documentation as
part of `quick-test`. They run that target serially with Make's `-k` option to
collect failures from independent prerequisites. Make still returns failure
when any required check fails.

After a successful native compiler/runtime build, fbctests, syntax tests,
Exampleageddon and the Darwin binding checks run even if an earlier validation
step fails. A failed or skipped build prevents those tests from running, and a
cancelled job stops them. Each test retains its own failing status; no test is
made optional, and package qualification still requires the entire CI run to
succeed. These conditions apply to Linux, Windows x86/x86_64, macOS and native
Windows ARM64.

## Checking exactly what will be committed

Local drafts can conceal missing links. The source checker can read an immutable
Git tree directly, without copying the repository or using untracked files:

```sh
python3 -B build_scripts/check-source-docs.py --revision HEAD
```

To check staged documentation before committing, use the tree written by
`git write-tree` as `--revision`. Stage the intended files first. The checker
reads both navigation pages and their destinations from that tree, so local
edits, deletions or untracked drafts cannot change the result. Without
`--revision`, the existing working-tree and manual-rendering checks remain
available. Snapshot checks do not render documentation.

<!-- end of ci-recovery.md -->
