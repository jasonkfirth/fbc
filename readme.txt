
    FreeBASIC - A multi-platform BASIC Compiler
    Copyright (C) 2004-2026 The FreeBASIC development team.

    Official site:      https://freebasic.net/
    Forum:              https://freebasic.net/forum/
    Online manual:      https://freebasic.net/wiki/DocToc
    fbc project page:   https://sourceforge.net/projects/fbc/
    GitHub mirror:      https://github.com/freebasic/fbc
    Package repository: https://deb.fbxl.net/
    Release documents:  https://deb.fbxl.net/docs/
    1.10-1.20 history:   https://deb.fbxl.net/docs/guides/release-history-1.10-to-1.20.md
    Discord:            https://discord.gg/286rSdK
    IRC channel:        ##freebasic at https://webchat.freenode.net
    Features:           https://freebasic.net/wiki/CompilerFeatures
    Requirements:       https://freebasic.net/wiki/CompilerRequirements

    FreeBASIC consists of fbc (the command line compiler), the core runtime
    library (libfb), the default graphics library (libfbgfx), the opt-in GPU
    graphics library (libfbgfx3), the sound library (libsfx), and FreeBASIC
    header files for third-party libraries. In order to produce executables,
    fbc normally uses the GNU binutils and GCC or a target-specific compiler
    driver. Some packages include the required cross-target tools.

    Documentation of language features, compiler options and many other details
    is available in the FB manual. For help & support, visit the FB forum!

  o Installation & Usage

    FreeBASIC gives you the FreeBASIC compiler program (fbc or fbc.exe),
    plus the tools and libraries used by it. fbc is a command line program
    that takes FreeBASIC source code files (*.bas) and compiles them into
    executables.  In the combined standalone packages for windows, the main
    executable is named fbc32.exe (for 32-bit) and fbc64.exe (for 64-bit)

    fbc is typically invoked by Integrated Development Environments (IDEs) or
    text editors, from a terminal or command prompt, or through build-systems
    such as makefiles. fbc itself is not a graphical code editor or IDE!

    Windows:
      Download the current Win32, Win64, or Win32-on-ARM64 package from
      https://deb.fbxl.net/mingw32/ or use an official upstream package.
      Android, JavaScript, Wii, and Xbox target packages have their own
      mingw32-* directories at the same site. Extract a standalone archive or
      run its installer, then compile from a command prompt:

        > fbc.exe examples\hello.bas
        > examples\hello.exe

      The separate Windows 95 package carries an i486-compatible compiler,
      runtime libraries, and old GNU tools. Do not replace those tools with a
      current MinGW toolchain when targeting Windows 95.

      Optionally, you can install a text editor or IDE which will invoke fbc.exe
      for you, for example:
        Tiko editor:   https://github.com/PaulSquires/tiko/releases
        VisualFBEditor https://github.com/XusinboyBekchanov/VisualFBEditor/releases
       Or even though is older and unmaintained will work (with some effort):
        FBIDE:         https://fbide.freebasic.net/

    Linux and other Unix-like systems:
      The experimental package repository can detect the current system and
      install a matching package:

        $ curl -fsSLO https://deb.fbxl.net/install.sh
        $ sh install.sh

      The repository currently qualifies selected Debian, Ubuntu, Raspbian,
      Alpine/postmarketOS, RPM, BSD, Haiku, illumos, macOS, and Cygwin hosts.
      Package presence alone does not mean that an older retained release is
      still tested. See https://deb.fbxl.net/docs/ for the current matrix,
      package names, checksums, and optional target SDKs.

      An official standalone Linux archive can still be installed with its
      included "sudo ./install.sh -i" command. Its compiler requires the host
      GCC/binutils and the development libraries used by each program.

      After either installation method:

        $ fbc examples/hello.bas
        $ ./examples/hello

    Android phone development:
      A Termux script can provision a native ARM64 Ubuntu PRoot and build a
      signed Android APK entirely on the phone. It uses Termux's native Clang,
      NDK sysroot, Java, and APK tools instead of QEMU or x86-64 host tools:

        $ curl -fLO https://deb.fbxl.net/install/termux-ubuntu-android-bootstrap.sh
        $ chmod 700 termux-ubuntu-android-bootstrap.sh
        $ ./termux-ubuntu-android-bootstrap.sh

      Optionally, you can install a text editor or IDE which will invoke fbc for
      you, for example:
        Geany: https://geany.org (sudo apt-get install geany)

    DOS:
      Download and extract the latest FreeBASIC-x.xx.x-dos.zip.

      Now you can use fbc.exe from the installation directory to compile FB
      programs (*.bas files) into executables (*.exe files). For example:
        > fbc.exe examples\hello.bas
      This should have created examples\hello.exe. You can run it by entering:
        > examples\hello.exe

  o Licensing

    The FreeBASIC compiler (fbc) is licensed under the GNU GPLv2 or later.

    The FreeBASIC runtime library (libfb and the thread-safe version, libfbmt)
    and the FreeBASIC graphics library (libfbgfx and the thread-safe version,
    libfbgfxmt) are licensed under the GNU LGPLv2 or later, with this exception
    to allow linking to it statically:
        As a special exception, the copyright holders of this library give
        you permission to link this library with independent modules to
        produce an executable, regardless of the license terms of these
        independent modules, and to copy and distribute the resulting
        executable under terms of your choice, provided that you also meet,
        for each linked independent module, the terms and conditions of the
        license of that module. An independent module is a module which is
        not derived from or based on this library. If you modify this library,
        you may extend this exception to your version of the library, but
        you are not obligated to do so. If you do not wish to do so, delete
        this exception statement from your version.

    The FreeBASIC documentation is licensed under the GNU FDL.

    Dependencies on third-party libraries:

    The FreeBASIC runtime library uses LibFFI to implement the Threadcall
    functionality. This means that, by default, FreeBASIC programs will be
    linked against LibFFI when using Threadcall. LibFFI is released under
    the MIT/Expat license, see doc/libffi-license.txt.

    By default, FreeBASIC programs are linked against various system and/or
    support libraries, depending on the platform. Those include the DJGPP
    libraries used by FreeBASIC for DOS and the MinGW/GCC libraries used by
    FreeBASIC for Windows.

  o Included/used third-party tools and libraries:

    - DJGPP         http://www.delorie.com/
    - GCC           https://gcc.gnu.org/
    - GNU binutils  https://gnu.org/software/binutils/
    - GNU debugger  https://gnu.org/software/gdb/
    - GoRC          http://godevtool.com/
    - LibFFI        https://sourceware.org/libffi/
    - MinGW         https://osdn.net/projects/mingw/
    - MinGW-w64     https://mingw-w64.org/
                    https://github.com/niXman/mingw-builds-binaries/
    - OpenXDK       https://openxdk.sourceforge.net/
    - TDM-GCC       https://jmeubank.github.io/tdm-gcc/
    - WinLibs       https://www.winlibs.com/

  o Credits

    Project members:
    - Andre Victor T. Vicentini (av1ctor[at]yahoo.com.br)
        Founder, main compiler developer, author of many parts of the runtime,
        lead developer 2004 to 2010
        FB headers (FBSWIG) & emscripten port
        too many additions and improvements to list
    - Angelo Mottola (a.mottola[at]libero.it)
        Author of the FB graphics library, built-in threads, thread-safe
        runtime, ports I/O, dynamic library loading, Linux port.
    - Bryan Stoeberl (b_stoeberl[at]yahoo.com)
        SSE/SSE2 floating point math, AST vectorization.
    - Daniel C. Klauer (daniel.c.klauer[at]web.de)
        lead developer 2010 to 2017
        automatic header / bindings generation (fbfrog)
        FB releases 0.21 to 1.05.0, C & LLVM backends, 64bit port,
        dynamic arrays in UDTs, virtual methods, preprocessor-only mode,
        many fixes and improvements.to compiler, rtlib & gfxlib2
        too many additions and improvements to list
    - Daniel R. Verkamp (i_am_drv[at]yahoo.com)
        DOS, XBox, Darwin, *BSD ports, DLL and static library automation,
        VB-compatible runtime functions, compiler optimizations,
        miscellaneous fixes and improvements.
    - Ebben Feagan (sir_mud[at]users.sourceforge.net)
        FB headers, C emitter
    - Jeff Marshall (coder[at]execulink.com)
        FB releases 0.17 to 0.20, and later 1.06.0 and up
        FB documentation (wiki maintenance, fbdocs offline-docs generator),
        Gosub/Return, profiling support, dialect specifics, DOS serial driver,
        miscellaneous fixes and improvements.to compiler, rtlib & gfxlib2
        lead developer since 2017
    - Mark Junker (mjscod[at]gmx.de)
        Author of huge parts of the runtime (printing support, date/time
        functions, SCR/LPTx/COM/console/keyboard I/O), Cygwin port,
        first FB installer scripts.
    - Matthew Fearnley (matthew.w.fearnley[at]gmail.com)
        Print Using & Co, ImageInfo, and others, dialect specifics,
        optimization improvements in the compiler, many fixes and improvements.
        rtlib & gfxlib2 fixes and improvements
        documentation and examples
        forum administrator and moderator since forever
    - Ruben Rodriguez (fbc[at]cha0s.io)
        Var keyword, const specifier, placement new, operator overloading and
        other OOP-related work, C BFD wrapper, many fixes and improvements.
    - Simon Nash
        AndAlso/OrElse operators, ellipsis for array initializers,
        miscellaneous fixes and improvements.

    Contributors:
    - 1000101
        gfxlib2 patches, e.g. image buffer alignment
    - Abdullah Ali (voodooattack[at]hotmail.com)
        Windows NT DDK headers & examples
    - adeyblue
        Direct2D windows driver
        run time and gfx library improvements and fixes
    - AGS
        gdbm, zlib, Mini-XML, PCRE headers
    - Angelo Rosina (angros47[at]gmail.com)
        gfxlib2 extensions for OpenGL 2D rendering
        integration of threading and dynamic libraries for DOS port (by monochromator)
        integration of emscripten branch and improvements
    - Claudio Tinivella (tinycla[at]yahoo.it)
        Gtk tutorials
    - Chris Davies (c.g.davies[at]gmail.com)
        OpenAL headers & examples
    - Dinosaur
        CGUI headers
    - D.J.Peters
        ARM port, ODE headers & examples, Win32 API header fixes
    - Dumbledore
        wx-c headers & examples
    - dr0p (dr0p[at]perfectbg.com)
        PostgreSQL headers & examples
    - Edmond Leung (leung.edmond[at]gmail.com)
        SDL headers & examples
    - Eric Lope (vic_viperph[at]yahoo.com)
        OpenGL & GLU headers & examples, examples/gfx/rel-*.bas demos
    - Florent Heyworth (florent.heyworth[at]swissonline.ch)
        Win32 API sql/obdc headers
    - fsw (fsw.fb[at]comcast.net)
        Win32 API headers, Gtk/Glade/wx-c examples
    - fxm
        documentation writer and manager for many years
        detailed technical articles, bug tracking and investigations
        documentation forum moderator
    - Garvan O'Keeffe (sisophon2001[at]yahoo.com)
        FB ports of many NeHe OpenGL lessons, PDFlib examples
    - Hans L. Nemeschkal (Hans.Leo.Nemeschkal[at]univie.ac.at)
        DISLIN headers
    - Jofers (spam[at]betterwebber.com)
        ThreadCall keyword, libffi/libjit headers, FreeType examples
    - Jose Manuel Postigo (postigo[at]uma.es)
        Linux serial devices support
    - Laanan Fisher (laananfisher[at]gmail.com)
        FB test suite using CUnit
    - Laurent Gras / SARG (debug[at]aliceadsl.fr)
        gas64 backend emitter
        improvements and fixes for stabs debugging
        fbdebugger project https://users.freebasic-portal.de/sarg
    - Luther Ramsey (luther.ramsey[at]gmail.com)
        freebasic.net forums moderator
    - Matthew Riley (pestery)
        OpenGL, GLFW, glext, FreeGLUT, cryptlib headers
    - Matthias Faust (matthias_faust[at]web.de)
        SDL_ttf headers & examples
    - Marzec
        SDL headers, SDL_bassgl, SDL_opengl and SDL_key examples
        First file routines for FB's rtlib
    - MJK
        big_int header fixes
    - MOD
        wx-c, BASS headers; -lang qb support for built-in macros,
        "real" Rnd() algorithm
    - Nek (dave[at]nodtveidt.net)
        Win32 API headers
    - Hung Nguyen Gia (gh_origin[at]zohomail.com)
        Solaris and DragonFly porting and testing
    - Paul Squires (support[at]planetsquires.com)
        Tiko Editor project and fbc compiler distribution bundle
    - Plasma
        FMOD and BASS headers & examples
    - Ralph Versteegen
        fixes / improvements to compiler, rtlib, gfxlib2, tests and headers
    - Randy Keeling (randy[at]keeling.com)
        GSL matrix example
    - Saga Musix (Jojo)
        BASS examples with sounds
    - Sisophon2001
        gfxlib2 fixes, Nehe OpenGL lesson ports
    - Stefan Wurzinger (swurzinger[at]gmx.at)
        compiler, runtime library and documentation generator improvements
        daily development builds, documentation builds and testing
        header/bindings updates
    - Sterling Christensen (sterling[at]engineer.com)
        Ex-project-member, author of FB's initial QB-like graphics library
    - TJF (Thomas.Freiherr[at]gmx.net)
        ARM port, GTK+, glib, Cairo, Pango headers & examples,
        SQLiteExtensions headers
    - zydon
        Win32 API examples

    Greetings:
    - Plasma
        Owner of the freebasic.net domain and main site hoster, many thanks to
        him.
    - VonGodric
        Author of the first FreeBASIC IDE: FBIDE.
    - Everybody that helped writing the documentation (and in special Nexinarus
      who started it)
        https://freebasic.net/wiki/ContributorList
    - All users that reported bugs, requested features and as such helped
      improving the compiler, language and run-time libraries.
