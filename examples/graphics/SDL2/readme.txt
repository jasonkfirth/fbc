Project: FreeBASIC SDL2 examples
File: readme.txt
Purpose: Describe the SDL2 example directories and their assets.
Responsibilities: Identify each library's examples and explain local checks.
This file intentionally does NOT contain: SDL library implementations.

SDL2 examples
-------------

core contains sdl2-hello.bas. image, mixer, net, ttf, gfx, gpu, rtf, sound,
bgi, and fontcache contain the examples for those SDL2 companion libraries.
Their FreeBASIC bindings remain under inc/SDL2.

The data directory supplies the horse image and Vera font for sdl2-hello.
Run that program from this SDL2 directory so its relative data/ filenames
resolve. Other examples accept their asset filenames on the command line.

The companion examples use shared helpers under ../SDL_common. Build
commands, Windows library locations, and the complete example inventory
are described in ../SDL_common/readme.txt.

From the repository root, check the core example and the companions:

  python3 build_scripts/test-sdl12-examples.py --select SDL2
  python3 build_scripts/test-sdl-addons.py --select SDL2

Both runners give each executable its own private X server and stage the
required assets. net2 uses an ephemeral loopback port.

End of readme.txt
