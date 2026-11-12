<!--
Project: FreeBASIC XL CI
File: ci-recovery.md
Purpose: Explain hosted-runner recovery without weakening qualification.
Responsibilities: Identify runner allocation failures and scoped manual reruns.
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

Automatic package builds accept successful CI triggered by a master push.
Manual recovery runs do not trigger package builds, even when they succeed.
Build and test failures remain failures; runner shortages are not converted
into passing qualification.

<!-- end of ci-recovery.md -->
