# Emulated Native Matrix Scripts

This directory holds the older Docker matrix drivers that build each target
inside a container matching the target CPU architecture.

That model is useful for validation because it exercises a target-shaped
userspace. It is not ideal as the primary package production path, because
non-host CPU architectures run full package builds through binfmt/qemu and can
be very slow.

The intended replacement flow is:

1. Open one native container for each Linux distribution release.
2. Install that release's cross toolchains and target development packages.
3. Cross-build all supported CPU architecture packages inside that container.
4. Validate each resulting package in a clean target container under
   binfmt/qemu, running compiler, fbctests, gfxlib, and sfxlib checks.

The top-level wrappers in `build_scripts/` still invoke these scripts so old
commands keep working while the cross-build pipeline is developed.

<!-- end of README.md -->
