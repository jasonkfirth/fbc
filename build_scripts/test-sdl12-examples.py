#!/usr/bin/env python3
"""Project: FreeBASIC SDL1 and SDL2 examples
File: test-sdl12-examples.py
Purpose: Compile and exercise every example that includes an SDL1 or SDL2 binding.
Responsibilities: Isolate X displays, stage assets, send input, and retain evidence.
This file intentionally does NOT contain: desktop control or public network requests.
"""

from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor
import ctypes
import ctypes.util
import json
import os
from pathlib import Path
import re
import shutil
import signal
import socket
import subprocess
import threading
import time

from PIL import Image, ImageChops, ImageStat

from sdl_example_paths import ADDON_EXAMPLES
from sdl3_xvfb import private_display


ROOT = Path(__file__).resolve().parents[1]
INCLUDE = re.compile(r'#\s*include\s+(?:once\s+)?"SDL2?[/\\]', re.IGNORECASE)
CONSOLE = {"video_info.bas", "timer.bas", "cdrom.bas", "net_httpget.bas"}
BLANK_WINDOWS = {"events.bas", "keymouse.bas", "music_test1.bas", "music_test2.bas"}
HTTP_BODY = b"FreeBASIC SDL_net local response\n"
MISSING_ASSETS = {"image_test1.bas": ["free.jpg"], "image_test2.bas": ["basic.gif"],
                  "music_test1.bas": ["music.ogg"], "music_test2.bas": ["phaser.wav"],
                  "ttf.bas": ["Vera.ttf"], "sdl2-hello.bas": ["horse.tga", "Vera.ttf"]}


# -------------------------------------------------------------------------
# Process ownership and private X input
# -------------------------------------------------------------------------


def command(arguments, environment, timeout=5):
    return subprocess.run(arguments, env=environment, capture_output=True,
                          text=True, timeout=timeout)


def stop(child):
    if child.poll() is None:
        try:
            os.killpg(child.pid, signal.SIGTERM)
        except ProcessLookupError:
            child.wait(timeout=2)
            return
        try:
            child.wait(timeout=2)
        except subprocess.TimeoutExpired:
            try:
                os.killpg(child.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
            child.wait(timeout=2)


def close_window(window, environment):
    """Send WM_DELETE_WINDOW, so SDL receives its normal quit event."""
    class Data(ctypes.Union):
        _fields_ = [("longs", ctypes.c_long * 5), ("bytes", ctypes.c_char * 20)]

    class ClientMessage(ctypes.Structure):
        _fields_ = [("type", ctypes.c_int), ("serial", ctypes.c_ulong),
                    ("send_event", ctypes.c_int), ("display", ctypes.c_void_p),
                    ("window", ctypes.c_ulong), ("message_type", ctypes.c_ulong),
                    ("format", ctypes.c_int), ("data", Data)]

    class Event(ctypes.Union):
        # XEvent has 24 native longs on both supported Xlib word sizes.
        _fields_ = [("message", ClientMessage), ("pad", ctypes.c_long * 24)]

    xlib = ctypes.CDLL(ctypes.util.find_library("X11"))
    xlib.XOpenDisplay.argtypes = [ctypes.c_char_p]
    xlib.XOpenDisplay.restype = ctypes.c_void_p
    xlib.XInternAtom.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int]
    xlib.XInternAtom.restype = ctypes.c_ulong
    xlib.XSendEvent.argtypes = [ctypes.c_void_p, ctypes.c_ulong, ctypes.c_int,
                               ctypes.c_long, ctypes.POINTER(Event)]
    xlib.XCloseDisplay.argtypes = [ctypes.c_void_p]
    xlib.XFlush.argtypes = [ctypes.c_void_p]
    # Xlib reads XAUTHORITY from its own process environment. Input is sent
    # by a short child process to keep parallel cases' credentials separate.
    display = xlib.XOpenDisplay(environment["DISPLAY"].encode())
    if not display:
        raise RuntimeError("Cannot open the example's private X display")
    try:
        event = Event()
        event.message.type = 33  # X11 ClientMessage event.
        event.message.display = display
        event.message.window = window
        event.message.message_type = xlib.XInternAtom(display, b"WM_PROTOCOLS", 0)
        event.message.format = 32
        event.message.data.longs[0] = xlib.XInternAtom(display, b"WM_DELETE_WINDOW", 0)
        if not xlib.XSendEvent(display, window, 0, 0, ctypes.byref(event)):
            raise RuntimeError("XSendEvent failed")
        xlib.XFlush(display)
    finally:
        xlib.XCloseDisplay(display)


def input_command(arguments, environment):
    result = command(["xdotool"] + arguments, environment)
    if result.returncode:
        raise RuntimeError("xdotool failed: " + result.stderr.strip())


def request_close(window, environment):
    result = command(["python3", str(Path(__file__).resolve()),
                      "--close-window", str(window)], environment)
    if result.returncode:
        raise RuntimeError("Quit event failed: " + result.stderr.strip())


def capture(window, destination, environment):
    result = command(["import", "-window", str(window), str(destination)], environment)
    if result.returncode:
        raise RuntimeError("Window capture failed: " + result.stderr.strip())
    with Image.open(destination) as original:
        image = original.convert("RGB")
        colors = image.getcolors(maxcolors=256)
        return {"path": str(destination), "width": image.width, "height": image.height,
                "colors": len(colors) if colors is not None else ">256"}


def wait_message(child, log, message, timeout):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if message in log.read_text():
            return
        if child.poll() is not None:
            break
        time.sleep(0.05)
    raise RuntimeError("Example did not report: " + message)


def graphical(child, source, directory, environment):
    name = source.name
    deadline = time.monotonic() + 10
    window = None
    while child.poll() is None and time.monotonic() < deadline:
        result = command(["xdotool", "search", "--onlyvisible", "--pid", str(child.pid)], environment)
        if result.returncode == 0 and result.stdout.strip():
            window = int(result.stdout.splitlines()[-1])
            break
        time.sleep(0.05)
    if window is None:
        raise RuntimeError("Example did not create a visible X window")
    input_command(["windowfocus", str(window)], environment)
    if name == "sdl.bas":
        input_command(["key", "Right", "Left"], environment)
    time.sleep(0.25)
    screenshot = capture(window, directory / "window.png", environment)
    if name not in BLANK_WINDOWS:
        deadline = time.monotonic() + 3
        while screenshot["colors"] == 1 and child.poll() is None and time.monotonic() < deadline:
            # Mapping the X window can precede the first completed GL swap.
            time.sleep(0.1)
            screenshot = capture(window, directory / "window.png", environment)
        if screenshot["colors"] == 1:
            raise RuntimeError("Example window contained only one color")
    interactions = []
    if name == "image_test2.bas":
        input_command(["key", "Up", "Up", "Up"], environment)
        time.sleep(0.15)
        changed = capture(window, directory / "fade.png", environment)
        with Image.open(screenshot["path"]) as before, Image.open(changed["path"]) as after:
            difference = ImageStat.Stat(ImageChops.difference(before.convert("RGB"),
                                                              after.convert("RGB"))).mean
        if not any(difference):
            raise RuntimeError("Fade keys did not change the displayed image")
        interactions.append({"fade_difference": difference})
    if name == "mouse.bas":
        input_command(["mousemove", "--window", str(window), "100", "100",
                       "mousedown", "1"], environment)
        try:
            time.sleep(0.1)
            capture(window, directory / "mouse-button.png", environment)
            with Image.open(directory / "mouse-button.png") as image:
                if image.convert("RGB").getpixel((100, 100)) != (255, 0, 255):
                    raise RuntimeError("Mouse click did not change the box color")
        finally:
            input_command(["mouseup", "1"], environment)
        interactions.append("left button changed box color")
    if name in {"music_test1.bas", "music_test2.bas"}:
        if environment.get("FB_EXAMPLE_AUTOPLAY"):
            return {"window": window, "screenshot": screenshot, "interactions": ["autoplay"]}
        asset = "music.ogg" if name == "music_test1.bas" else "phaser.wav"
        input_command(["key", "m"], environment)
        wait_message(child, directory / "run.log", "Playing " + asset, 2)
        time.sleep(0.2)
        input_command(["key", "m"], environment)
        wait_message(child, directory / "run.log", "Stopped " + asset, 2)
        interactions.append("keyboard started and stopped " + asset)
        if name == "music_test1.bas":
            input_command(["key", "m"], environment)
            wait_message(child, directory / "run.log", "Music finished", 8)
            interactions.append("music restarted and completion callback was handled")
    if name == "events.bas" and environment.get("FB_SDL12_EVENT_SHORTCUT"):
        # Key events are filtered, but SDL_GetKeyState must still reflect
        # Escape while the example handles the mouse button event.
        input_command(["keydown", "Escape", "mousedown", "1"], environment)
        try:
            wait_message(child, directory / "run.log", "Bye bye...", 2)
        finally:
            input_command(["keyup", "Escape", "mouseup", "1"], environment)
        return {"window": window, "screenshot": screenshot, "interactions": ["Escape and mouse button"]}
    request_close(window, environment)
    if name == "events.bas":
        # This example deliberately filters the first close request.
        time.sleep(0.15)
        request_close(window, environment)
        interactions.append("two close requests")
    return {"window": window, "screenshot": screenshot, "interactions": interactions}


# -------------------------------------------------------------------------
# Example execution and local HTTP peer
# -------------------------------------------------------------------------


def http_server(listener, state):
    try:
        listener.settimeout(10)
        with listener.accept()[0] as connection:
            connection.settimeout(5)
            request = b""
            while b"\r\n\r\n" not in request:
                block = connection.recv(4096)
                if not block or len(request) + len(block) > 16384:
                    raise RuntimeError("Incomplete or oversized HTTP request")
                request += block
            state["request"] = request.decode("ascii")
            connection.sendall(b"HTTP/1.0 200 OK\r\nContent-Length: " +
                               str(len(HTTP_BODY)).encode() + b"\r\n\r\n" + HTTP_BODY)
    except Exception as error:
        state["error"] = str(error)


def execute(source, executable, directory, environment, legacy_http):
    arguments = []
    listener = None
    server = None
    state = {}
    if source.name == "net_httpget.bas":
        listener = socket.socket()
        listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        listener.bind(("127.0.0.1", 80 if legacy_http else 0))
        listener.listen(1)
        port = listener.getsockname()[1]
        arguments = ["127.0.0.1" + ("" if legacy_http else ":" + str(port)), "fb-sdl-check"]
        server = threading.Thread(target=http_server, args=(listener, state), daemon=True)
        server.start()
    elif source.name == "sdl.bas":
        arguments = [str(directory / "data/free.jpg")]
    record = {"command": [str(executable)] + arguments, "timed_out": False}
    child = None
    started = time.monotonic()
    try:
        with (directory / "run.log").open("w") as output:
            child = subprocess.Popen(record["command"], cwd=directory, env=environment,
                                     stdin=subprocess.PIPE, stdout=output,
                                     stderr=subprocess.STDOUT, start_new_session=True)
            # FreeBASIC's console Sleep receives a real input byte. Programs
            # with SDL windows instead receive input on their private server.
            child.stdin.write(b"\n")
            child.stdin.close()
            try:
                if source.name not in CONSOLE:
                    record["graphics"] = graphical(child, source, directory, environment)
                child.wait(timeout=max(1, 15 - (time.monotonic() - started)))
            except subprocess.TimeoutExpired:
                record["timed_out"] = True
            except Exception as error:
                record["error"] = str(error)
            finally:
                stop(child)
                record["exit_code"] = child.returncode
        text = (directory / "run.log").read_text()
        record["log"] = str(directory / "run.log")
        if source.name == "net_httpget.bas":
            server.join(timeout=1)
            record["http"] = state
            if (state.get("error") or not state.get("request", "").startswith("GET /fb-sdl-check HTTP/1.0")
                    or HTTP_BODY.decode().strip() not in text):
                record["error"] = "Local HTTP request/response check failed"
        elif source.name == "events.bas":
            if environment.get("FB_SDL12_EVENT_SHORTCUT"):
                if "Bye bye..." not in text:
                    record["error"] = "Escape and mouse button did not exit"
            elif "Quit event filtered out" not in text or "Quit requested, quitting." not in text:
                record["error"] = "Event filter did not handle both close requests"
        elif source.name == "timer.bas" and "Flag was set!" not in text:
            record["error"] = "Timer callback did not fire during the example"
        elif source.name in {"music_test1.bas", "music_test2.bas"}:
            asset = "music.ogg" if source.name == "music_test1.bas" else "phaser.wav"
            if "Playing " + asset not in text:
                record["error"] = "Audio sample did not start playing"
        if source.name == "cdrom.bas" and "No CD-ROM drives available" in text:
            record["hardware"] = "no CD-ROM drive; enumeration path checked"
        return record
    finally:
        if child is not None:
            stop(child)
        if listener is not None:
            listener.close()
        if server is not None:
            server.join(timeout=1)


# -------------------------------------------------------------------------
# Invalid input and missing asset checks
# -------------------------------------------------------------------------


def expect_failure(executable, directory, environment, arguments):
    record = {"arguments": arguments, "expected_exit_code": 1, "timed_out": False}
    try:
        with private_display(directory, environment) as (session, display):
            record["display"] = display
            with (directory / "run.log").open("w") as output:
                child = subprocess.Popen([str(executable)] + arguments, cwd=directory,
                                         env=session, stdin=subprocess.DEVNULL, stdout=output,
                                         stderr=subprocess.STDOUT, start_new_session=True)
                try:
                    child.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    record["timed_out"] = True
                finally:
                    stop(child)
                record["exit_code"] = child.returncode
        record["passed"] = record["exit_code"] == 1 and not record["timed_out"]
    except Exception as error:
        record.update(passed=False, error=str(error))
    record["log"] = str(directory / "run.log")
    return record


def failure_paths(source, executable, directory, assets, environment):
    results = []
    for filename in MISSING_ASSETS.get(source.name, []):
        case = directory / ("missing-" + filename)
        case.mkdir(exist_ok=True)
        shutil.copytree(assets, case / "data", dirs_exist_ok=True)
        (case / "data" / filename).unlink()
        results.append(expect_failure(executable, case, environment, []))
    if source.name == "sdl.bas":
        case = directory / "missing-image"
        case.mkdir(exist_ok=True)
        results.append(expect_failure(executable, case, environment, ["missing-image.jpg"]))
    if source.name == "net_httpget.bas":
        # These inputs must be rejected before a socket is opened.
        invalid = [("127.0.0.1:" + port,) for port in ("", "0", "65536", "999999", "-1", "12x")]
        invalid += [("127.0.0.1\nInjected",), ("127.0.0.1:12345", "path\r\nInjected")]
        for index, arguments in enumerate(invalid):
            case = directory / ("invalid-http-" + str(index))
            case.mkdir(exist_ok=True)
            results.append(expect_failure(executable, case, environment, list(arguments)))
    return results


def virtual_cd(source, executable, directory, environment, audio):
    case = directory / "virtual-cd"
    disc = case / "disc"
    disc.mkdir(parents=True, exist_ok=True)
    for name in ("track01.mp3", "track02.mp3"):
        shutil.copy2(audio, disc / name)
    session_environment = environment.copy()
    # This is SDL12-compat's documented virtual audio CD facility. It
    # exercises track enumeration without claiming to test a physical drive.
    session_environment["SDL12COMPAT_FAKE_CDROM_PATH"] = str(disc)
    with private_display(case, session_environment) as (session, display):
        run = execute(source, executable, case, session, False)
    text = Path(run["log"]).read_text()
    passed = (run["exit_code"] == 0 and not run["timed_out"]
              and re.search(r"Drive tracks:\s*2", text) is not None
              and text.count("Track (index") == 2)
    return {"fixture": "SDL12-compat virtual CD with two identical MP3 tracks",
            "audio": str(audio), "display": display, "run": run, "passed": passed}


# -------------------------------------------------------------------------
# Compilation, case selection, and reports
# -------------------------------------------------------------------------


def check(item, args, environment):
    backend, source = item
    relative = source.relative_to(ROOT / "examples")
    directory = args.output / backend / relative.with_suffix("")
    directory.mkdir(parents=True, exist_ok=True)
    record = {"source": str(source.relative_to(ROOT)), "backend": backend}
    executable = directory / "example"
    compile_command = [str(args.fbc), "-i", str(ROOT / "inc"), "-mt", "-gen", backend,
                       str(source), "-x", str(executable)]
    compilation = command(compile_command, environment, timeout=120)
    (directory / "compile.log").write_text(compilation.stdout + compilation.stderr)
    record["compile"] = {"command": compile_command, "exit_code": compilation.returncode,
                         "log": str(directory / "compile.log")}
    if compilation.returncode:
        record["status"] = "compile-failed"
        return record
    assets = source.parent / "data"
    if not assets.is_dir():
        generation = source.parents[1]
        assets = generation / "data"
        if not assets.is_dir():
            assets = ROOT / "examples/graphics/SDL1/data"
    shutil.copytree(assets, directory / "data", dirs_exist_ok=True)
    try:
        with private_display(directory, environment) as (session, display):
            record["display"] = display
            record["run"] = execute(source, executable, directory, session, args.legacy_http)
        run = record["run"]
        record["status"] = ("passed" if run["exit_code"] == 0 and not run["timed_out"]
                            and not run.get("error") else "run-failed")
        if record["status"] == "passed" and source.name in {"music_test1.bas", "music_test2.bas"}:
            audio_directory = directory / "interactive-audio"
            audio_directory.mkdir(exist_ok=True)
            shutil.copytree(assets, audio_directory / "data", dirs_exist_ok=True)
            interactive = environment.copy()
            interactive["FB_EXAMPLE_AUTOPLAY"] = ""
            with private_display(audio_directory, interactive) as (session, display):
                record["interactive_audio"] = {"display": display,
                    "run": execute(source, executable, audio_directory, session, args.legacy_http)}
            extra = record["interactive_audio"]["run"]
            if extra["exit_code"] or extra["timed_out"] or extra.get("error"):
                record["status"] = "run-failed"
                record["error"] = extra.get("error", "Interactive audio did not exit cleanly")
        if record["status"] == "passed" and source.name == "events.bas":
            shortcut_directory = directory / "keyboard-shortcut"
            shortcut_directory.mkdir(exist_ok=True)
            shortcut = environment.copy()
            shortcut["FB_SDL12_EVENT_SHORTCUT"] = "1"
            with private_display(shortcut_directory, shortcut) as (session, display):
                record["event_shortcut"] = {"display": display,
                    "run": execute(source, executable, shortcut_directory, session, args.legacy_http)}
            extra = record["event_shortcut"]["run"]
            if extra["exit_code"] or extra["timed_out"] or extra.get("error"):
                record["status"] = "run-failed"
                record["error"] = extra.get("error", "Event shortcut did not exit cleanly")
        if record["status"] == "passed":
            record["failure_paths"] = failure_paths(source, executable, directory, assets, environment)
            if any(not case["passed"] for case in record["failure_paths"]):
                record["status"] = "run-failed"
                record["error"] = "An invalid-input check crashed, stalled, or returned success"
        if record["status"] == "passed" and source.name == "cdrom.bas" and args.cd_audio:
            record["virtual_cd"] = virtual_cd(source, executable, directory, environment, args.cd_audio)
            if not record["virtual_cd"]["passed"]:
                record["status"] = "run-failed"
                record["error"] = "SDL12-compat virtual CD track enumeration failed"
    except Exception as error:
        record["status"] = "run-failed"
        record["error"] = str(error)
    return record


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fbc", type=Path, default=ROOT / "bin/fbc")
    parser.add_argument("--output", type=Path, default=ROOT / "out/sdl-legacy/validation")
    parser.add_argument("--backend", choices=("gcc", "gas64"), nargs="+", default=["gcc", "gas64"])
    parser.add_argument("--jobs", type=int, default=2)
    parser.add_argument("--select", help="Restrict paths to this substring")
    parser.add_argument("--cd-audio", type=Path,
                        help="Use an MP3 file to check SDL12-compat virtual CD track listing")
    parser.add_argument("--legacy-http", action="store_true", help="Check the original fixed-port HTTP example")
    parser.add_argument("--close-window", type=int, help=argparse.SUPPRESS)
    args = parser.parse_args()
    if args.close_window is not None:
        close_window(args.close_window, os.environ)
        return 0
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    if args.cd_audio:
        args.cd_audio = args.cd_audio.resolve()
        if not args.cd_audio.is_file():
            parser.error("--cd-audio must name an existing MP3 file")
    args.fbc = args.fbc.resolve()
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    environment = os.environ.copy()
    environment.update(SDL_AUDIODRIVER="dummy", SDL_RENDER_DRIVER="software",
                       LIBGL_ALWAYS_SOFTWARE="1", FB_EXAMPLE_AUTOPLAY="1")
    sources = [p for p in sorted((ROOT / "examples").rglob("*.bas"))
               if INCLUDE.search(p.read_text(errors="replace"))
               and p.relative_to(ROOT / "examples").as_posix() not in ADDON_EXAMPLES
               and (not args.select or args.select in str(p.relative_to(ROOT)))]
    if not sources:
        parser.error("No SDL1/SDL2 examples matched")
    items = [(backend, source) for backend in args.backend for source in sources]
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        results = list(pool.map(lambda item: check(item, args, environment), items))
    packages = ("sdl", "sdl2", "SDL_image", "SDL_mixer", "SDL_ttf", "SDL_net", "SDL_gfx",
                "SDL2_image", "SDL2_ttf")
    versions = {name: command(["pkg-config", "--modversion", name], environment).stdout.strip()
                for name in packages}
    pc_directory = command(["pkg-config", "--variable=pcfiledir", "sdl"], environment).stdout.strip()
    pc_file = Path(pc_directory) / "sdl.pc"
    implementation = "unknown"
    if pc_file.is_file():
        implementation = next((line.removeprefix("Name:").strip() for line in pc_file.read_text().splitlines()
                               if line.startswith("Name:")), "unknown")
    report = {"compiler": str(args.fbc), "audio_driver": "dummy", "sdl1_implementation": implementation,
              "library_versions": versions, "results": results}
    displays = []
    for result in results:
        if "display" in result:
            displays.append(result["display"])
        for extra in ("interactive_audio", "event_shortcut", "virtual_cd"):
            if extra in result:
                displays.append(result[extra]["display"])
        displays.extend(case["display"] for case in result.get("failure_paths", []) if "display" in case)
    report["private_x_servers"] = {"started": len(displays),
                                  "stopped": sum(display["stopped"] for display in displays)}
    (args.output / "results.json").write_text(json.dumps(report, indent=2) + "\n")
    failures = [r for r in results if r["status"] != "passed"]
    print(json.dumps({"examples": len(sources), "checks": len(results), "passed": len(results) - len(failures),
                      "private_x_servers": report["private_x_servers"],
                      "failures": [{"source": r["source"], "backend": r["backend"],
                                    "error": r.get("error", r.get("run", {}).get("error", r["status"]))}
                                   for r in failures], "report": str(args.output / "results.json")}, indent=2))
    return bool(failures)


if __name__ == "__main__":
    raise SystemExit(main())

# end of test-sdl12-examples.py
