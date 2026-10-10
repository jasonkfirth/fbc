Project: FreeBASIC SDL1 examples
File: readme.txt
Purpose: Describe the SDL1 example directories and their assets.
Responsibilities: Identify each library's examples and explain local checks.
This file intentionally does NOT contain: SDL library implementations.

SDL1 examples
-------------

core contains the SDL1 video, input, timer, CD, and OpenGL examples.
image, mixer, net, ttf, gfx, gpu, rtf, sound, pango, and fontcache contain
the examples for those SDL1 companion libraries. Their FreeBASIC bindings
remain under inc/SDL.

The data directory contains the image, audio, and font fixtures used by
the original examples. Run those examples from this SDL1 directory so
their relative data/ filenames resolve.

The companion examples use shared helpers under ../SDL_common. Build
commands, Windows library locations, and the complete example inventory
are described in ../SDL_common/readme.txt.

From the repository root, check the original examples and the companions:

  python3 build_scripts/test-sdl12-examples.py
  python3 build_scripts/test-sdl-addons.py --select SDL1

Both runners give each executable its own private X server and stage the
required assets. The companion examples accept explicit filenames.

End of readme.txt
