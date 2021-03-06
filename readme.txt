
    FreeBASIC - A multi-platform BASIC Compiler
    Copyright (C) 2004-2013 The FreeBASIC development team.

    Official site:      http://www.freebasic.net/
    Forum:              http://www.freebasic.net/forum/
    Online manual:      http://www.freebasic.net/wiki/DocToc
    fbc project page:   http://www.sourceforge.net/projects/fbc/
    IRC channel:        ##freebasic at irc.freenode.net

    FreeBASIC consists of fbc (the command line compiler), the runtime libraries
    (included into FB programs by fbc), compiler tools (assembler, linker) used
    by fbc, and headers and bindings for third-party libraries.

    Documentation of language features, compiler options and many other details
    is available in the FB manual. For help & support, visit the FB forum!

    Features:           http://www.freebasic.net/wiki/CompilerFeatures
    Requirements:       http://www.freebasic.net/wiki/CompilerRequirements

  o Installation & Usage

    FreeBASIC gives you the FreeBASIC compiler program, called fbc or fbc.exe,
    plus the compiler tools and run-time libraries used by fbc. It is a
    command line program that takes FreeBASIC source code files (*.bas) and
    compiles them into executables. fbc is typically invoked by Integrated
    Development Environments (IDEs) or text editors, from a terminal or command
    prompt, or through build-systems such as makefiles.

    - Win32
      - Download the latest FreeBASIC-x.xx.x-win32.exe installer
      - Run it and click through it
      - Use fbc.exe from the install directory to compile FB code, for example:
        - Click Start -> FreeBASIC -> Open console
        - In the opened prompt, type in the following command and press ENTER:
            > fbc.exe examples\hello.bas
          This should have created examples\hello.exe, which can be executed
          right away:
            > examples\hello.exe
      - Optionally: install an IDE for FB, for example:
            FBIDE (http://fbide.freebasic.net/)
            FBEdit (http://radasm.cherrytree.at/fbedit/)

    - Linux
      - Download the latest FreeBASIC-x.xx.x-linux.tar.gz
      - Extract it
      - Open a terminal
      - cd into the extracted FreeBASIC-x.xx.x-linux directory
      - run "sudo ./install.sh -i"
      - Install the following packages (names vary depending on distribution):
          Debian/Ubuntu:
            x86:               amd64 old:           amd64 new:
              gcc                gcc-multilib         gcc-multilib
              g++                g++-multilib         g++-multilib
              libncurses5-dev    lib32ncurses5-dev    lib32ncurses5-dev
              libx11-dev         ia32-libs            libx11-dev:i386
              libxext-dev        lib32ffi-dev         libxext-dev:i386
              libxrender-dev                          libxrender-dev:i386
              libxrandr-dev                           libxrandr-dev:i386
              libxpm-dev                              libxpm-dev:i386
              libffi-dev
          OpenSUSE:
            x86:               x86_64:
              gcc                gcc-32bit
              gcc-c++            gcc-c++-32bit
              ncurses-devel      ncurses-devel-32bit
              xorg-x11-devel     xorg-x11-devel-32bit
              libffi46-devel     xorg-x11-libX11-devel-32bit
                                 xorg-x11-libXext-devel-32bit
                                 xorg-x11-libXrender-devel-32bit
                                 xorg-x11-libXpm-devel-32bit
                                 libffi46-devel-32bit
      - Use the installed fbc program to compile FB code, for example:
          $ fbc examples/hello.bas
        This should have created the examples/hello program, which can be
        executed right away:
          $ examples/hello
      - Optionally: install an IDE for FB, for example: Geany (www.geany.org)

    - DOS
      - Download the latest FreeBASIC-x.xx.x-dos.zip
      - Extract it
      - Use fbc.exe from the extracted directory to compile FB code,
        for example:
          > fbc.exe examples\hello.bas
        This should have created examples\hello.exe, which can be executed
        right away:
          > examples\hello.exe

  o Licensing

    The FreeBASIC compiler (fbc) is licensed under the GNU GPLv2 or later.

    The FreeBASIC runtime library (in both normal (libfb) and thread-safe
    (libfbmt) versions) and the FreeBASIC graphics library (libfbgfx) are
    licensed under the GNU LGPLv2 or later, with this exception to allow
    linking to it statically:
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
    the MIT/Expat license, see doc/licenses/libffi.txt.

    By default, FreeBASIC programs are linked against various system and/or
    support libraries, depending on the platform. Those include the DJGPP
    libraries used by FreeBASIC for DOS and the MinGW/GCC libraries used by
    FreeBASIC for Windows.

  o Included/used third-party tools and libraries:

    - DJGPP         http://www.delorie.com/
    - GCC           http://gcc.gnu.org/
    - GNU binutils  http://www.gnu.org/software/binutils/
    - GNU debugger  http://www.gnu.org/software/gdb/
    - GoRC          http://www.godevtool.com/
    - LibFFI        http://sourceware.org/libffi/
    - MinGW         http://www.mingw.org/
    - OpenXDK       http://www.openxdk.org/
    - TDM-GCC       http://tdm-gcc.tdragon.net/

  o Credits

    Project members:
    - Andre Victor T. Vicentini (av1ctor[at]yahoo.com.br)
        Founder, main compiler developer, author of many parts of the runtime,
        FB headers (FBSWIG)
    - Angelo Mottola (a.mottola[at]libero.it)
        Author of the FB graphics library, built-in threads, thread-safe
        runtime, ports I/O, dynamic library loading, Linux port.
    - Bryan Stoeberl (b_stoeberl[at]yahoo.com)
        SSE/SSE2 floating point math, AST vectorization.
    - Daniel C. Klauer (daniel.c.klauer[at]web.de)
        FB releases since 0.21, preprocessor-only mode, C emitter,
        miscellaneous fixes and improvements.
    - Daniel R. Verkamp (i_am_drv[at]yahoo.com)
        DOS, XBox, Darwin, *BSD ports, DLL and static library automation,
        VB-compatible runtime functions, compiler optimizations,
        miscellaneous fixes and improvements.
    - Ebben Feagan (sir_mud[at]users.sourceforge.net)
        FB headers, C emitter
    - Jeff Marshall (coder[at]execulink.com)
        FB releases since 0.17, FB documentation (wiki maintenance, fbdocs
        offline-docs generator), Gosub/Return, profiling support, dialect
        specifics, DOS serial driver, miscellaneous fixes and improvements.
    - Mark Junker (mjscod[at]gmx.de)
        Author of huge parts of the runtime (printing support, date/time
        functions, SCR/LPTx/COM/console/keyboard I/O), Cygwin port,
        first FB installer scripts.
    - Matthew Fearnley (matthew.w.fearnley[at]gmail.com)
        Print Using & Co, ImageInfo, and others, dialect specifics,
        optimization improvements in the compiler, many fixes and improvements.
    - Ruben Rodriguez (rubentbstk[at]gmail.com)
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
    - AGS
        gdbm, zlib, Mini-XML, PCRE headers
    - Claudio Tinivella (tinycla[at]yahoo.it)
        Gtk tutorials
    - Chris Davies (c.g.davies[at]gmail.com)
        OpenAL headers & examples
    - Dinosaur
        CGUI headers
    - D.J.Peters
        ODE headers & examples, Win32 API header fixes
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
    - Matthew Riley (pestery)
        OpenGL, GLFW, glext, FreeGLUT, cryptlib headers
    - Matthias Faust (matthias_faust[at]web.de)
        SDL_ttf headers & examples
    - Marzec
        SDL headers, SDL_bassgl, SDL_opengl and SDL_key examples
        First file routines for FB's rtlib
    - MOD
        wx-c, BASS headers; -lang qb support for built-in macros,
        "real" Rnd() algorithm
    - Nek (dave[at]nodtveidt.net)
        Win32 API headers
    - Plasma
        FMOD and BASS headers & examples
    - Randy Keeling (randy[at]keeling.com)
        GSL matrix example
    - Saga Musix (Jojo)
        BASS examples with sounds
    - Sisophon2001
        gfxlib2 fixes, Nehe OpenGL lesson ports
    - Sterling Christensen (sterling[at]engineer.com)
        Ex-project-member, author of FB's initial QB-like graphics library
    - TeeEmCee
        gfxlib2 fixes
    - TJF (Thomas.Freiherr[at]gmx.net)
        GTK+, glib, Cairo, Pango headers & examples, SQLiteExtensions headers
    - zydon
        Win32 API examples

    Greetings:
    - Plasma: Owner of the freebasic.net domain and main site hoster,
      many thanks to him.
    - VonGodric: Author of the first FreeBASIC IDE: FBIDE.
    - Everybody that helped writing the documentation (and in special Nexinarus 
      who started it), more details at:
        http://www.freebasic.net/wiki/wikka.php?wakka=ContributorList
    - All users that reported bugs, requested features and as such helped 
      improving the compiler, language and run-time libraries.
