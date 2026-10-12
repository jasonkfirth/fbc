#!/usr/bin/env python3
"""
FreeBASIC BASSMOD regression runner
File: test-bassmod.py
Purpose: Qualify the genuine Linux i386 SDK with C and FreeBASIC programs.
Responsibilities: ABI comparison, PCM equality, native output and error paths.
This file contains no substitute module engine or driver implementation.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import signal
import stat
import subprocess
import time


def run(arguments, working, environment, expected=0):
    result = subprocess.run([str(value) for value in arguments], cwd=working,
                            env=environment, capture_output=True, text=True,
                            timeout=45, check=False)
    if result.returncode != expected:
        raise RuntimeError(f"exit {result.returncode}: {arguments}\n{result.stdout}{result.stderr}")
    return result.stdout


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fbc", required=True, type=Path)
    parser.add_argument("--sdk-prefix", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--oss-proxy", type=Path)
    parser.add_argument("--oss-backend", type=Path)
    args = parser.parse_args()
    if os.name != "posix":
        parser.error("this runner qualifies the Linux i386 SDK and OSS output path")
    if bool(args.oss_proxy) != bool(args.oss_backend):
        parser.error("supply both --oss-proxy and --oss-backend")

    root = Path(__file__).resolve().parents[1]
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    fbc = args.fbc.resolve()
    prefix = args.sdk_prefix.resolve()
    library = prefix / "lib/libbassmod.so"
    assert library.read_bytes()[:5] == b"\x7fELF\x01", "the vendor SDK must be ELF32"
    environment = os.environ.copy()
    environment.pop("LD_PRELOAD", None)
    environment["LD_LIBRARY_PATH"] = str(prefix / "lib") + ":" + environment.get("LD_LIBRARY_PATH", "")
    environment["FB_EXAMPLE_AUTOPLAY"] = "1"
    node = Path("/dev/dsp")
    proxy = None
    proxy_log = None
    records = []
    source = root / "tests/bindings/bassmod-native.c"
    native = output / "bassmod-c"
    run(["gcc", "-m32", "-Wall", "-Wextra", "-Werror", "-I" + str(prefix / "include"),
         source, "-L" + str(prefix / "lib"), "-lbassmod", "-o", native], output, environment)
    module_file = root / "examples/sound/data/dne_trtn.mod"
    malformed = output / "invalid.mod"
    malformed.write_bytes(b"This is not a tracker module.\n")

    try:
        if args.oss_proxy:
            if node.exists():
                raise RuntimeError("an existing OSS driver must not be replaced")
            proxy_log = (output / "oss-proxy.log").open("w")
            proxy = subprocess.Popen([str(args.oss_proxy.resolve()), "-f", "--adsp=", "--mixer=",
                                      "--max=4", "--umax=4", "--log=4",
                                      "--dsp-slave=" + str(args.oss_backend.resolve())],
                                     stdout=proxy_log, stderr=subprocess.STDOUT,
                                     start_new_session=True)
            deadline = time.monotonic() + 10
            while not node.exists() and proxy.poll() is None and time.monotonic() < deadline:
                time.sleep(0.1)
            if proxy.poll() is not None or not node.exists():
                raise RuntimeError("the OSS proxy did not register its device")
        if not node.exists() or not stat.S_ISCHR(node.stat().st_mode):
            raise RuntimeError("native playback requires an OSS character device")

        native_abi = run([native], output, environment)
        native_facts = re.findall(r"\d+", native_abi)
        assert native_facts == ["4", "4", "4", "4", "65538"], native_abi
        c_pcm = output / "native.pcm"
        run([native, "decode", module_file, c_pcm], output, environment)
        reference = c_pcm.read_bytes()
        assert len(reference) == 1024 * 1024 and any(reference), "silent or incomplete PCM"
        assert "BASSMOD_C_OUTPUT=PASS" in run([native, "output", module_file], output, environment)
        for bad_file in (malformed, output / "does-not-exist.mod"):
            bad_pcm = output / "invalid-c.pcm"
            bad_pcm.unlink(missing_ok=True)
            run([native, "decode", bad_file, bad_pcm], output, environment, expected=1)
            assert not bad_pcm.exists(), "failed module load wrote PCM"

        for backend in ("gcc", "gas"):
            compiler = [fbc, "-target", "linux", "-arch", "686", "-prefix", root,
                        "-i", root / "inc", "-p", prefix / "lib", "-gen", backend, "-w", "all"]
            binary = output / ("bassmod-fb-" + backend)
            run(compiler + [root / "tests/bindings/bassmod-native.bas", "-x", binary], output, environment)
            fb_abi = run([binary], output, environment)
            assert re.findall(r"\d+", fb_abi) == native_facts, fb_abi
            pcm = output / (backend + ".pcm")
            run([binary, "decode", module_file, pcm], output, environment)
            assert pcm.read_bytes() == reference, "native C and FreeBASIC PCM differ"
            assert "BASSMOD_FB_OUTPUT=PASS" in run([binary, "output", module_file], output, environment)
            for bad_file in (malformed, output / "does-not-exist.mod"):
                bad_pcm = output / ("invalid-" + backend + ".pcm")
                bad_pcm.unlink(missing_ok=True)
                run([binary, "decode", bad_file, bad_pcm], output, environment, expected=1)
                assert not bad_pcm.exists(), "failed module load wrote PCM"
            for relative in ("examples/manual/libraries/bassmod.bas", "examples/sound/BASS/mod.bas"):
                example = output / (Path(relative).stem + "-" + backend)
                run(compiler + [root / relative, "-x", example], output, environment)
                observed = run([example, module_file], output, environment)
                assert "BASSMOD playback completed." in observed
                failed = run([example, output / "does-not-exist.mod"], output, environment, expected=1)
                assert "could not load" in failed
                records.append({"source": relative, "backend": backend,
                                "playback": "pass", "missing_module": "pass"})
            print("BASSMOD_NATIVE=PASS Linux32 " + backend, flush=True)
    finally:
        if proxy is not None:
            proxy.terminate()
            try:
                proxy.wait(timeout=10)
            except subprocess.TimeoutExpired:
                os.killpg(proxy.pid, signal.SIGKILL)
                proxy.wait(timeout=5)
            if proxy_log is not None:
                proxy_log.close()
            assert not node.exists(), "task-owned OSS device survived driver shutdown"

    receipt = {"status": "pass", "platform": "Linux i386", "version": 65538,
               "native_abi_facts": native_facts, "pcm_bytes_per_comparison": len(reference),
               "pcm_sha256": hashlib.sha256(reference).hexdigest(), "pcm_comparisons": 2,
               "malformed_and_missing_module_checks": 6, "native_output_checks": 3,
               "examples": records, "compiler_sha256": hashlib.sha256(fbc.read_bytes()).hexdigest(),
               "library_sha256": hashlib.sha256(library.read_bytes()).hexdigest(),
               "driver_cleanup": "pass" if proxy is not None else "existing driver retained"}
    (output / "results.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print("BASSMOD_QUALIFICATION=PASS", flush=True)


if __name__ == "__main__":
    main()

# end of test-bassmod.py
