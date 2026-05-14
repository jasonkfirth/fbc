# Cross Build Matrix

This directory is for the replacement Linux package pipeline.

The old matrix builds each package inside a Docker container whose CPU
architecture matches the target package architecture. That proves a lot, but it
also means full package builds for arm64, armhf, ppc64el, s390x, riscv64, and
similar targets run through binfmt/qemu on an x86_64 host.

The replacement model should split package production from package validation.
Package production should still happen in Docker, but the container should
match the packaging OS and release instead of the target CPU architecture:

1. Start one native Docker container for each distro release.
2. Install that release's packaging tools, cross toolchains, and target
   development packages.
3. Cross-build every supported CPU architecture package in that native
   container.
4. Start a clean target-architecture validation container for each package.
5. Install the finished package and run compiler, fbctests, gfxlib, and sfxlib
   validation under binfmt/qemu.

This keeps package builds fast while still validating the shipped artifacts in
their target userspace.

The practical rule is:

* `.deb` packages are built by Debian, Ubuntu, or Raspbian containers.
* `.apk` packages are built by Alpine or postmarketOS containers.
* `.rpm` packages are built by RPM-family containers such as Fedora, Rocky,
  AlmaLinux, and openSUSE.
* Slackware packages are built by Slackware containers.

The host machine should only orchestrate Docker and provide the source tree and
artifact directory mounts.

The top-level entry point should be:

```
build_scripts/linux-cross-build-freebasic-matrix.sh
```

That script owns the package factory graph and should eventually dispatch to
package-family builders:

* `debianubuntu-cross-build-freebasic-matrix.sh`
* `apk-cross-build-freebasic-matrix.sh`
* `rpm-cross-build-freebasic-matrix.sh`
* `slackware-cross-build-freebasic-matrix.sh`

Output directories should stay boring and predictable:

```
out/linux/<distro>/<release>/<arch>/
```

Examples:

```
out/linux/ubuntu/noble/amd64/
out/linux/debian/trixie/loong64/
out/linux/alpine/3.23/aarch64/
out/linux/alpine/edge/riscv64/
out/linux/postmarketos/edge/armv7/
out/linux/fedora/44/x86_64/
out/linux/rocky/10/riscv64/
out/linux/opensuse/tumbleweed/aarch64/
out/linux/slackware/15.0/x86_64/
```

One build-system issue must be solved before this can fully replace the older
matrix:

* `debian/rules` currently builds a target-architecture bootstrap compiler and
  then uses that compiler during the package build. That works in a target
  container, but a true cross build needs a build-architecture FreeBASIC
  compiler to generate target C, followed by the target C toolchain to produce
  target objects and the target `fbc` executable.

The new scripts should therefore grow around an explicit build/host split:

* build compiler: executable on the Docker container CPU
* host compiler: executable shipped in the target package
* target runtime: libraries/startup objects for the package architecture

<!-- end of README.md -->
