'' Project: FreeBASIC SDL3 bindings
'' File: SDL_hints.bi
'' Purpose: Declare the SDL3-3.4.18 C interface for FreeBASIC.
'' Responsibilities: Preserve public types, constants, and calling conventions.
'' This file intentionally does NOT contain: the upstream library implementation.
''
'' Translated from the upstream headers; this is an altered source version.
'' Regenerate with build_scripts/generate-sdl3-bindings.py.
''

'' The common declarations live in SDL.bi, matching the existing
'' FreeBASIC SDL bindings. This entry point also supports C-style includes.

'' SDL.bi includes SDL_inline.bi for the header-only implementations.

#pragma once

#include once "SDL.bi"

'' end of SDL_hints.bi
