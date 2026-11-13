<!-- FreeBASIC DOS support; File: README.md.
Purpose: Reproduce the checked host randomness device for extended DOS.
Responsibilities: Source identity, build inputs, protocol and qualification.
Application preferences and runtime ABI belong to their respective modules. -->

# Checked randomness in DOSBox-X

`checked-randomness.patch` adds the Windows-hosted `XLRNG$` character device
used by `FBC.secureRandom`. Keep this patch with the runtime source so the
host component can be rebuilt after private build directories are retired.

The patch targets DOSBox-X tag `dosbox-x-v2026.10.01`, commit
`b6abbd5980a885f5f310a4088c59a8688d1b116c`. The upstream source and its GPL
license are available in the DOSBox-X repository.
[DOSBox-X source](https://github.com/joncampbell123/dosbox-x/tree/dosbox-x-v2026.10.01)

Apply the patch in a separate copy of that source. In the MSYS2 MinGW64 shell,
use the existing SDL2 development dependencies and upstream Autotools build.
Use a bounded worker count and disable debug data.

```sh
git apply /path/to/checked-randomness.patch
export PATH=/mingw64/bin:/usr/bin:$PATH
export PKG_CONFIG_LIBDIR=/mingw64/lib/pkgconfig
./autogen.sh
CFLAGS='-O1 -g0' CXXFLAGS='-O1 -g0' LIBS='-lbcrypt' ./configure \
  --enable-sdl2 --disable-optimize --disable-freetype --disable-printer \
  --disable-mt32 --disable-libfluidsynth --disable-libslirp \
  --disable-opengl --disable-screenshots
# Keep the surface-output build consistent with disabled D3D9 support.
if grep -q '^#define C_DIRECT3D 1$' config.h; then
  sed -i 's/^#define C_DIRECT3D 1$/#define C_DIRECT3D 0/' config.h
fi
libraries=$(sed -n 's/^LIBS = //p' src/Makefile)
make -j2 LDFLAGS='-static-libgcc -static-libstdc++' LIBS="$libraries -luuid"
```

Keep the MinGW headers and package metadata ahead of MSYS headers. The native
MinGW executable needs the DLLs selected by its build configuration. The
upstream source also supplies its SDL CD-ROM compatibility files and Windows
resources. Retain upstream license notices with any redistributed executable.

This upstream release's SDL2 D3D11 detection sets the shared `C_DIRECT3D`
definition while its Automake D3D9 condition remains disabled. The recipe
sets that generated numeric definition to zero before compiling the
surface-output host. The patch also makes two shader-menu guards test that
numeric value consistently. Dynamic dependency linking avoids an incomplete static
FFmpeg dependency list. `uuid` supplies the microphone property identifiers
referenced by the upstream Windows backend.

Each device read calls Windows `BCryptGenRandom` with the system-preferred
generator. The device retains no seed or generator state. Reads are bounded
to 4096 bytes. Writes, seeks and malformed control requests fail. The exact
16-byte IOCTL response and the runtime's buffer ownership are documented in
`doc/secure-random.md`.
[Microsoft CNG API](https://learn.microsoft.com/en-us/windows/win32/api/bcrypt/nf-bcrypt-bcryptgenrandom)

Run `tests/secure-random/dos-device.c` in the candidate emulator and require
its actual DOS ERRORLEVEL to be zero. Also run the archive-linked live and
scratch-buffer checks before selecting a runtime for an application. Keep the
stock emulator refusal as a separate negative control. Application-level
persistence qualification must use independent emulator boots with the same
mounted filesystem and verify both restored values and explicit forgetting.

The November 12 Templari qualification records the exact candidate image,
application image, separate-boot results and host source identities under
`E:/Templari/build/session-proof-20261112`.

<!-- end of README.md -->
