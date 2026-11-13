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
cancel a full run on the same branch. Source changes in pushes and pull requests
run the full matrix. Manual runs default to `all` and can also override the
ARM64 image.

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

## Documentation-only commits

The main and extended workflows first compare the tested checkout against the
push's `before` commit or the pull request's base commit. Changes confined to
`readme.txt`, `changelog.txt`, Markdown under `docs/`, or Markdown/Wakka pages
under `doc/manual/cache/` run scope regression tests and the independent source
documentation checks. They skip compiler, analyzer and guest builds. Programs
and build inputs inside those directories still require full qualification.

The comparison covers the entire push, including source changes before a final
documentation commit. Renames include both the old and new paths, so moving
source into a documentation directory cannot hide its deletion. Unknown paths,
an empty comparison, an initial push or unavailable shallow history select the
full matrix. The checkout retains only two commits; scope detection does not
download a full history to optimize a documentation change. Manual, scheduled,
tag and explicit source-ref qualification also retain their existing builds.

Automatic packages inspect the completed main run's jobs using read-only Actions
access. A successful scope planner and documentation check together with an
intentionally skipped bootstrap identify a documentation-only run. That run
does not trigger packages and does not claim native compiler qualification.
Older workflow layouts retain package builds; an API error fails the package
scope job visibly. Direct manual, scheduled and tagged package requests retain
their existing behavior. Scope changes do not affect workflows already running.

## Independent validation after a successful build

Native C regression drivers use the shared compiler capability probes. GCC
versions that call C23 `gnu2x` run that mode instead of failing on the newer
`gnu23` spelling. The probe requires a C23 language-version macro; it does not
fall back to C11 or remove diagnostic flags. Native x86 object builds also
match the BASIC compiler's pointer width, including 32-bit Windows runners
that expose a 64-bit Clang.

Archive membership regressions compile a small native object and copy it for
their input lists. Text files named `.o` are not portable archive inputs:
Apple's archive tools can leave only the symbol index. The tests still require
the exact object member order, removal of stale members, bounded argument
batches, and preservation of a published archive after a tool failure.

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

Haiku package testing also runs smoke tests, fbctests and Exampleageddon in
separate shells after package installation and compiler verification succeed.
A failed phase remains fatal, but the remaining phases run and report their
own results. If guest Python is unavailable, host-driven Exampleageddon still
runs after a smoke or fbctests failure. Package setup failures remain immediate
failures because the independent suites need the installed compiler.

The Haiku TCP smoke has a 600-second outer limit, followed by a five-second
forced-termination allowance. Its source already allows three minutes for the
stream exchange and two minutes for the byte-at-a-time burst; the old 60-second
wrapper could terminate it before its own diagnostics. Set the positive integer
environment variable `HAIKU_TCP_SMOKE_TIMEOUT` when a different VM needs a
different bound. The log records the exit status and configured limit on
failure, including status 124 for a timeout. A longer allowance does not turn
timeouts or failed assertions into passes.

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
