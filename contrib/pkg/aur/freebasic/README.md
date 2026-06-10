# FreeBASIC AUR package

This directory is the upload-ready source for the `freebasic` AUR package.

The package is intentionally based on the release source-bootstrap archive
rather than the repository checkout.  FreeBASIC is implemented in FreeBASIC, so
the bootstrap archive is the release artifact that lets users build from source
without already having `fbc` installed.

Release update checklist:

1. Update `pkgver` and `pkgrel` in `PKGBUILD`.
2. Confirm the `source` URL exists.
3. Replace `sha256sums=('SKIP')` with the real archive checksum.
4. Run `./update-srcinfo.sh`.
5. Copy `PKGBUILD`, `.SRCINFO`, `.gitignore`, `README.md`, and
   `update-srcinfo.sh` to the AUR checkout.
6. Build locally with `makepkg -Cfs`.
7. Push the AUR repository.

The package conflicts with `fbc-git` because both packages install the `fbc`
compiler executable and the target runtime tree.

end of README.md
