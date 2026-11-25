#!/usr/bin/env python3
"""Build Prospector's character display for the FreeBASIC browser corpus.

Stage checked-out sources and gameplay data, adapt the upstream graphics
driver to the staged SDK, and link the browser core. The browser page and
catalog publication are separate from this build.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess


def replace(path, old, new):
    # Some upstream units contain legacy character encodings. Match the
    # ASCII anchors without decoding or rewriting unrelated source text.
    source = path.read_bytes().replace(b"\r\n", b"\n")
    if source.count(old.encode()) != 1:
        raise ValueError(f"Unexpected source context in {path.name}: {old.splitlines()[0]}")
    path.write_bytes(source.replace(old.encode(), new.encode(), 1))


def copy_sources(project, stage, inputs):
    sources = {}
    for name, relative in inputs.items():
        source = (project / name).resolve()
        output = (stage / relative).resolve()
        if not source.is_relative_to(project) or not output.is_relative_to(stage):
            raise ValueError(f"Input escapes project: {name}")
        content = source.read_bytes()
        sources[name] = hashlib.sha256(content).hexdigest()
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_bytes(content)
    return sources


def stage_sources(project, stage):
    notice = "README.txt"
    tracked = subprocess.check_output(["git", "-C", str(project), "ls-files", "-z"])
    inputs = {}
    for name in filter(None, tracked.decode().split("\0")):
        leaf = Path(name)
        if (leaf.parts[0] == "data" and leaf.suffix.lower() in {".csv", ".txt", ".dat", ".pal"}
                or leaf.parts[0] == "config" and leaf.suffix.lower() == ".txt"):
            inputs[name] = Path("pack") / leaf
        elif name in {notice, "port/version.bas", "port/fbstub.js", "port/webgfx.c"} or (
                len(leaf.parts) == 1 and leaf.suffix.lower() in {".bas", ".bi"}):
            inputs[name] = leaf
    return copy_sources(project, stage, inputs)


def adapt_sources(stage):
    files = stage / "fileIO.bas"
    replace(files, "    if configflag(con_tiles)=0 then\n        _mwx=gt_mwx",
            "    ' The corpus build uses the built-in font and character maps.\n"
            "    configflag(con_tiles)=1\n    configflag(con_sysmaptiles)=1\n"
            "    configflag(con_customfonts)=1\n"
            "    if configflag(con_tiles)=0 then\n        _mwx=gt_mwx")
    replace(files, '        Font2 = ImageCreate((254-1) * 8, 17,rgba(0,0,0,0),16)\n', '')
    replace(files, '        bsave "data/F1.bmp",Font1\n        font2=font1',
            '        font2=font1\n        titlefont=font1')
    replace(files, '        titlefont=font1\n    endif\n\n    _FW1=_FH1/2+2\n    _FW2=_FH2/2+2',
            '        titlefont=font1\n    endif\n\n    _FW1=8\n    _FW2=8')
    replace(files, '        case con_end,-1 \'Exit, do nothing',
            '        case con_tiles,con_sysmaptiles,con_customfonts\n'
            '            dprint "This build uses the built-in character display."\n'
            '        case con_end,-1 \'Exit, do nothing')
    replace(stage / "poker.bas", '    pcards.LoadCards("graphics/cards2.bmp")',
            '    if not fileexists("graphics/cards2.bmp") then\n'
            '        dprint "This build does not include a deck of cards."\n'
            '        return 0\n    endif\n    pcards.LoadCards("graphics/cards2.bmp")')

    # gfxlib2 added fields in the middle of GFXDRIVER. Named fields keep
    # each upstream callback attached to its intended slot.
    driver = stage / "port/webgfx.c"
    replace(driver, '''  "web", driver_init, driver_exit, driver_nop, driver_nop, NULL, driver_wait_vsync,
  driver_get_mouse, driver_set_mouse, driver_set_window_title, NULL, driver_fetch_modes,
  NULL, driver_poll, NULL''', '''  .name = "web",
  .init = driver_init,
  .exit = driver_exit,
  .lock = driver_nop,
  .unlock = driver_nop,
  .wait_vsync = driver_wait_vsync,
  .get_mouse = driver_get_mouse,
  .set_mouse = driver_set_mouse,
  .set_window_title = driver_set_window_title,
  .fetch_modes = driver_fetch_modes,
  .poll_events = driver_poll''')
    source = driver.read_text().replace("__fb_gfx_drivers_list", "fb_gfx_drivers_list")
    driver.write_text(re.sub(r"\b__fb_gfx\b", "fb_gfx", source), newline="\n")
    # Keep high scores in the local game files. The upstream fallback sends
    # each completed run to the host's leaderboard endpoint.
    source = driver.read_text()
    start = source.index("/* RVIP stage 9:")
    finish = source.index("\n#else", start)
    signature = "void rv_beacon(const char *ev, const char *name, const char *killer, int depth, int score, unsigned turns) {}"
    driver.write_text(source[:start] + signature + source[finish:], newline="\n")


def build(stage, fbc_source, sdk, emcc):
    fbc = sdk / "bin/fbc-js.exe"
    libraries = sdk / "lib/freebasic-js/js-asmjs"
    env = os.environ.copy()
    env.setdefault("BINARYEN_CORES", "1")

    def run(*command):
        subprocess.run(command, cwd=stage, env=env, check=True)

    flags = ["-O2", "-fno-strict-aliasing", "-fwrapv", "-I", str(fbc_source / "src/rtlib"),
             "-I", str(fbc_source / "src/gfxlib2")]
    run(fbc, "-gen", "gcc", "-r", "-m", "prospector_nosound", "-i", "port",
        "-maxerr", "15", "prospector_nosound.bas")
    run(emcc, *flags, "-c", "prospector_nosound.c", "-o", "game.o")
    run(emcc, *flags, "-c", "port/webgfx.c", "-o", "webgfx.o")
    # Keep a single driver without modifying the installed SDK archive.
    shutil.copyfile(libraries / "libfbgfx.a", stage / "libfbgfx-prospector.a")
    run(emcc.with_name("emar.cmd"), "d", "libfbgfx-prospector.a", "gfx_driver.o")
    # The engine has large local arrays and yields inside its input routines.
    # Preserve its regular and Asyncify stacks when switching graphics modes.
    run(emcc, *flags, "-O1", "game.o", "webgfx.o", "libfbgfx-prospector.a", libraries / "libfb.a",
        libraries / "libz.a", "-sASYNCIFY", "-sASYNCIFY_STACK_SIZE=524288", "-sSTACK_SIZE=8MB",
        "-sALLOW_MEMORY_GROWTH", "-sINITIAL_MEMORY=128MB", "-sMAXIMUM_MEMORY=1GB", "-sENVIRONMENT=web",
        "-lidbfs.js", "-sFORCE_FILESYSTEM", "-sEXPORTED_RUNTIME_METHODS=FS,IDBFS,HEAPU8,HEAP32,addRunDependency,removeRunDependency",
        "-sEXPORTED_FUNCTIONS=_main,_rv_frame,_rv_key,_rv_layout", "--pre-js", "port/fbstub.js",
        "--preload-file", "pack@/pack", "-o", "prospector-core.js")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for argument in ("project", "fbc_source", "sdk", "emcc", "stage"):
        parser.add_argument(argument, type=Path)
    parser.add_argument("--prepared", action="store_true", help="Build altered sources from the catalog archive")
    args = parser.parse_args()
    project, fbc_source, sdk, emcc, stage = (getattr(args, name).resolve()
                                           for name in ("project", "fbc_source", "sdk", "emcc", "stage"))
    stage.mkdir(parents=True, exist_ok=False)
    if args.prepared:
        metadata = json.loads((project / "inputs.json").read_text(encoding="utf-8"))
        sources = copy_sources(project, stage, {name: name for name in metadata["sources"]})
        if sources != metadata["sources"]:
            raise ValueError("Altered source archive differs from its input manifest")
        revision = metadata["revision"]
    else:
        sources = stage_sources(project, stage)
        adapt_sources(stage)
        revision = subprocess.check_output(["git", "-C", str(project), "rev-parse", "HEAD"], text=True).strip()
    build(stage, fbc_source, sdk, emcc)
    names = ("prospector-core.js", "prospector-core.wasm", "prospector-core.data")
    artifacts = {name: hashlib.sha256((stage / name).read_bytes()).hexdigest() for name in names}
    row = {"slug": "prospector", "project": "prospector", "entry": "prospector_nosound.bas",
           "status": "linked", "revision": revision, "sources": sources, "artifacts": artifacts}
    (stage / "build-results.json").write_text(json.dumps([row], indent=2) + "\n", encoding="utf-8")
    print("PROSPECTOR_BROWSER_CORE_LINKED")


if __name__ == "__main__":
    main()

# end of fb-corpus-prospector.py
