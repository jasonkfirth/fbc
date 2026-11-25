#!/usr/bin/env python3
"""FreeBASIC browser corpus. Stage the verified L-BASIC LLVM player.

fb-corpus-lbasic-llvm-site.py updates the existing L-BASIC entry and prepares
its source download. It reuses the existing catalog files and leaves browser
testing and publication to the catalog release workflow.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import runpy
import shutil
from zipfile import ZIP_DEFLATED, ZipFile


SLUG = "lbasic-frontend"
TITLE = "L-BASIC Compiler"
DESCRIPTION = "Parse BASIC or generate LLVM bitcode for WebAssembly in your browser."
LLVM_URL = "https://github.com/llvm/llvm-project/releases/tag/llvmorg-14.0.6"
SOURCE_FILES = ("LICENSE", "SOURCE.txt", "lbasic-llvm.bas", "runtime/foundation.a",
                "tests/for.test", "tests/print.test", "tests/udt.test")


def digest(file):
    with file.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def write_sources(output, frontend, llvm_license, scripts):
    with ZipFile(output, "w", compression=ZIP_DEFLATED) as archive:
        for relative in SOURCE_FILES:
            archive.write(frontend / relative, relative)
        for file in sorted((frontend / "runtime/foundation").iterdir()):
            if file.is_file():
                archive.write(file, file.relative_to(frontend).as_posix())
        archive.write(llvm_license, "LLVM-LICENSE.txt")
        for filename in ("fb-corpus-compiler.js", "fb-corpus-compiler-worker.js",
                         "fb-corpus-lbasic-llvm.py", "fb-corpus-lbasic-llvm-check.cjs",
                         "fb-corpus-lbasic-llvm-source.py", "fb-corpus-lbasic-frontend.py"):
            archive.write(scripts / filename, "build_scripts/" + filename)
        archive.writestr("BUILD.txt", f"""L-BASIC browser compiler

SOURCE.txt identifies the upstream revision. lbasic-llvm.bas is the exact
merged source compiled by fbc-js. The source of the bundled foundation
archive is under runtime/foundation.

LLVM 14.0.6 source and release archives are available at
{LLVM_URL}

In the FreeBASIC checkout, follow the LLVM host and wasm phases documented
in build_scripts/fb-corpus-js.md. Run the emitter phase with --frontend set
to this extracted directory. The build driver uses FreeBASIC's fbc-js SDK.

To run a program compiled by the browser, use the SDK to link its download
with the matching foundation archive, for example:

  emcc main.bc runtime/foundation.a -o main.html

Serve the resulting HTML and its companion files over HTTP.
""")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("base", "site", "overlay", "work", "emitter", "frontend", "llvm-source", "qualification"):
        parser.add_argument("--" + name, required=True, type=Path)
    args = parser.parse_args()
    base = args.base.resolve()
    destinations = [value.resolve() for value in (args.site, args.overlay, args.work)]
    if any(path.exists() or path.is_relative_to(base) for path in destinations):
        parser.error("Use new output directories outside the current catalog")
    if any(a.is_relative_to(b) for index, a in enumerate(destinations) for b in destinations[:index] + destinations[index + 1:]):
        parser.error("Site, overlay and work directories must be separate")
    site, overlay, work = destinations
    qualification = json.loads(args.qualification.read_bytes())
    if digest(args.emitter / "index.wasm") != qualification.get("emitterWasmSha256"):
        raise ValueError("The LLVM compiler changed after its browser check")
    examples = {row["test"] for row in qualification["cases"] if "observedStdout" in row}
    if not {"arithmetic", "counter", "text"} <= examples:
        raise ValueError("The LLVM compiler requires browser compilation and executable program checks")

    scripts = Path(__file__).resolve().parent
    catalog = runpy.run_path(str(scripts / "fb-corpus-js-site.py"))
    manifest = runpy.run_path(str(scripts / "fb-corpus-js-extend.py"))["manifest"]
    previous = manifest(base)
    origin = "https://github.com/flukiluke/L-BASIC"
    old_description = "Run L-BASIC&#x27;s parse-and-dump stage in your browser."
    catalog_page = (base / "index.html").read_text(encoding="utf-8")
    direct = f'href="{SLUG}/play.html"' in catalog_page
    if direct:
        card = catalog["catalog_card"](SLUG, "L-BASIC Parser", old_description, origin)
        updated_card = catalog["catalog_card"](SLUG, TITLE, DESCRIPTION, origin)
    else:
        card = f'<li><h2><a href="{SLUG}/">L-BASIC Parser</a></h2><p>{old_description}</p></li>'
        updated_card = f'<li><h2><a href="{SLUG}/">{TITLE}</a></h2><p>{DESCRIPTION}</p></li>'
    if catalog_page.count(card) != 1:
        raise ValueError("Expected the current L-BASIC entry in the catalog")
    program = overlay / SLUG
    llvm = program / "llvm"
    llvm.mkdir(parents=True)
    (overlay / "index.html").write_text(catalog_page.replace(card, updated_card), encoding="utf-8", newline="\n")
    about = f'''<nav><a href="../">Catalog</a></nav>
<h1>{TITLE}</h1>
<p>{DESCRIPTION} The download uses the L-BASIC foundation archive and an Emscripten SDK for final linking.</p>
<a class="run" href="play.html">Run in browser</a>
<footer><a href="https://github.com/flukiluke/L-BASIC">Upstream project</a> ·
<a href="llvm-source.zip">Download source</a> · <a href="license.txt">L-BASIC license</a> ·
<a href="llvm/LICENSE.txt">LLVM license</a> · <a href="{LLVM_URL}">LLVM source</a></footer>'''
    source_page = "sources.html" if direct else "index.html"
    (program / source_page).write_text(catalog["page"](TITLE, about, "../site.css"), encoding="utf-8", newline="\n")
    player = catalog["compiler_player"](TITLE, parse=True, llvm=True)
    if direct:
        player = catalog["catalog_return"](player)
    (program / "play.html").write_text(player, encoding="utf-8", newline="\n")
    for filename in ("index.js", "index.wasm"):
        shutil.copyfile(args.emitter / filename, llvm / filename)
    for filename in ("compiler.js", "compiler-worker.js"):
        shutil.copyfile(scripts / ("fb-corpus-" + filename), llvm / filename)
    llvm_license = args.llvm_source / "LICENSE.TXT"
    shutil.copyfile(llvm_license, llvm / "LICENSE.txt")
    write_sources(program / "llvm-source.zip", args.frontend, llvm_license, scripts)

    changed = {path.relative_to(overlay).as_posix(): digest(path) for path in overlay.rglob("*") if path.is_file()}
    files = previous | changed
    for name in sorted(files):
        dest = site / name
        dest.parent.mkdir(parents=True, exist_ok=True)
        os.link((overlay if name in changed else base) / name, dest)
    sums = "".join(f"{files[name]}  {name}\n" for name in sorted(files))
    (overlay / "SHA256SUMS.txt").write_text(sums, encoding="ascii", newline="\n")
    os.link(overlay / "SHA256SUMS.txt", site / "SHA256SUMS.txt")
    work.mkdir()
    build = {"slug": SLUG, "status": "linked", "artifacts": {
        "index.js": changed[f"{SLUG}/llvm/index.js"], "index.wasm": changed[f"{SLUG}/llvm/index.wasm"]}}
    (work / "build-results.json").write_text(json.dumps([build], indent=2) + "\n", encoding="utf-8")
    print(f"L-BASIC LLVM preview staged with {len(changed)} changed files and {len(files)} total assets")


if __name__ == "__main__":
    main()

# end of fb-corpus-lbasic-llvm-site.py
