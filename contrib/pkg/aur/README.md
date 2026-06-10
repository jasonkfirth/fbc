# FreeBASIC AUR packaging

This directory contains packaging files intended for upload to the Arch User
Repository.  These files are kept separate from `contrib/pkg/arch` because the
two jobs are different.

`contrib/pkg/arch` is used by the repository build matrix.  It is allowed to
consume staged source trees, generated bootstrap archives, target sysroots, and
cross-build metadata prepared by `build_scripts/`.

`contrib/pkg/aur` is for AUR source package repositories.  It should remain a
small set of files that can be copied to an AUR checkout and built with normal
`makepkg`.

The stable AUR package is in `freebasic/`.

Before uploading a release:

1. Confirm the release source-bootstrap archive exists at the URL in the
   package's `source` array.
2. Replace `sha256sums=('SKIP')` in the `PKGBUILD` with the release archive's
   real checksum.
3. Run `./update-srcinfo.sh` from the package directory.
4. Copy or stage the package into the AUR checkout.
5. Commit and push only `PKGBUILD`, `.SRCINFO`, and any intentionally included
   helper files.

The source-bootstrap archive must contain bootstrap compiler sources for every
architecture listed in the `arch` array.  If a release archive does not include
one of those targets, remove that architecture before uploading the AUR update.

end of README.md
