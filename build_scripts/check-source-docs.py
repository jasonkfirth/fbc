#!/usr/bin/env python3
"""Project: FreeBASIC source documentation
File: check-source-docs.py
Purpose: Check source-guide links and the manual converter's real output.
Responsibilities: Validate maintained navigation and optionally render fresh pages.
This file intentionally does NOT package or publish release manuals.
"""

import argparse
from html.parser import HTMLParser
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
from urllib.parse import unquote, urlsplit


GUIDES = {
    "DevSourceCode": "README.md",
    "DevSourceCompiler": "compiler.md",
    "DevSourceRuntime": "runtime.md",
    "DevSourceGraphics": "graphics.md",
    "DevSourceSound": "sound.md",
    "DevSourceTools": "tools-and-tests.md",
}
PAGES = (*GUIDES, "DevBuildConfig")
TREES = {"DevSourceCode": "src/", "DevSourceCompiler": "compiler/",
         "DevSourceRuntime": "rtlib/", "DevSourceGraphics": "gfxlib2/", "DevSourceSound": "sfxlib/"}


class Page(HTMLParser):
    def __init__(self):
        super().__init__()
        self.links = []
        self.text = []
        self.tags = set()

    def handle_starttag(self, tag, attrs):
        self.tags.add(tag)
        attributes = dict(attrs)
        if tag == "a":
            self.links.append(attributes.get("href") or "")
        elif tag in ("img", "link"):
            self.links.append(attributes.get("src" if tag == "img" else "href") or "")

    def handle_data(self, data):
        self.text.append(data)


def require(condition, message):
    if not condition:
        raise ValueError(message)


def check_markdown(path):
    # Code examples may contain illustrative links which are not navigation.
    text = re.sub(r"(?ms)^```.*?^```[^\n]*", "", path.read_text(encoding="utf-8"))
    count = 0
    for target in re.findall(r"!?\[[^]\n]+\]\(([^)]+)\)", text):
        target = target.strip()
        if target.startswith("<"):
            target = target[1:target.index(">")]
        else:
            target = target.split(' "', 1)[0]
        parsed = urlsplit(target)
        if parsed.scheme or not parsed.path:
            continue
        destination = (path.parent / unquote(parsed.path)).resolve()
        require(destination.exists(), str(path) + ": missing link " + target)
        count += 1
    return count


def check_wakka(cache, name):
    path = cache / (name + ".wakka")
    source = path.read_text(encoding="utf-8-sig")
    require(source.count("%%") % 2 == 0, name + ": unclosed code block")
    require("```" not in source and "](" not in source, name + ": Markdown in native manual")
    title = re.search(r'^\{\{fbdoc item="title" value="([^"]+)"\}\}', source)
    require(title is not None, name + ": missing native title")
    targets = re.findall(r"\[\[([^]|]+)(?:\|[^]]*)?\]\]", source)
    targets += re.findall(r'item="back" value="([^|"]+)', source)
    for target in targets:
        if urlsplit(target).scheme:
            continue
        require((cache / (target.split("#", 1)[0] + ".wakka")).is_file(),
                name + ": missing native link " + target)
    return title.group(1), len(targets)


def check_rendered(root, rendered):
    cache = root / "doc/manual/cache"
    links = 0
    project = (rendered / "html/fbdoc.hhp").read_text(encoding="utf-8")
    for name in PAGES:
        title, _ = check_wakka(cache, name)
        page = Page()
        page.feed((rendered / "html" / (name + ".html")).read_text(encoding="utf-8"))
        text = "".join(page.text)
        require(title in text, name + ": rendered title missing")
        allowed = {"html", "head", "title", "link", "meta", "body", "div", "img", "br", "pre",
                   "b", "u", "ul", "ol", "li", "a", "tt", "p", "i", "strike"}
        require(not page.tags - allowed, name + ": unescaped source markup became an HTML tag: "
                + ", ".join(sorted(page.tags - allowed)))
        require(not re.search(r"\{\{fbdoc|\[\[|```|##", text), name + ": raw markup in HTML")
        if name in TREES:
            require("pre" in page.tags and TREES[name] in text, name + ": source tree missing")
        require("DevToc.html" in page.links, name + ": developer navigation missing")
        if name in GUIDES and name != "DevSourceCode":
            require("DevSourceCode.html" in page.links, name + ": guide navigation missing")
        require(name + ".html" in project, name + ": absent from HTML project")
        for target in page.links:
            parsed = urlsplit(target)
            if not parsed.scheme and parsed.path:
                require((rendered / "html" / unquote(parsed.path)).is_file(),
                        name + ": missing rendered link " + target)
                links += 1
        # fbhelp uses byte-oriented style markers. Do not decode it as UTF-8.
        for format_name in ("fbhelp", "txt"):
            output = (rendered / format_name / (name + ".txt")).read_bytes()
            require(len(output) > 500, name + ": incomplete " + format_name + " output")
            token = TREES.get(name, ".pending" if name == "DevBuildConfig" else "source-artifacts-test")
            require(token.encode() in output, name + ": missing source/configuration prose in " + format_name)
        if name == "DevBuildConfig":
            require(".pending" in text and "cksum" in text, name + ": configuration prose missing")
    for name in ("DocToc", "DevToc"):
        html = (rendered / "html" / (name + ".html")).read_text(encoding="utf-8")
        require(all(guide + ".html" in html for guide in GUIDES), name + ": guide navigation missing")
    print(f"SOURCE_DOCS_RENDER_PASS pages={len(PAGES)} links={links} formats=html,fbhelp,txt")


def render(root, generator):
    # fbdoc reads its ini beside the executable. Copy that small tool into an
    # owned scratch directory rather than changing the user's configuration.
    with tempfile.TemporaryDirectory(prefix="fbc-source-docs-") as temporary:
        work = Path(temporary)
        tool = work / generator.name
        shutil.copyfile(generator, tool)
        manual = work / "manual"
        shutil.copytree(root / "doc/manual/templates", manual / "templates")
        (manual / "html").mkdir()
        shutil.copyfile(root / "doc/manual/html/style.css", manual / "html/style.css")
        shutil.copytree(root / "doc/manual/html/images", manual / "html/images")
        (work / "fbdoc.ini").write_text("[dirs]\nmanual_dir = " + manual.as_posix() + "/\ncache_dir = "
                                        + (root / "doc/manual/cache").as_posix() + "/\n", encoding="utf-8")
        for mode in ("-chm", "-fbhelp", "-txt"):
            result = subprocess.run([str(tool), mode], cwd=work, capture_output=True,
                                    text=True, errors="replace", timeout=120, check=False)
            counts = re.findall(r"(?m)^(\d+) pages emitted OK\s*$", result.stdout)
            require(result.returncode == 0 and counts and int(counts[-1]) >= len(PAGES)
                    and not re.search(r"(?m)^(?:Error On:|Unable to |Warning:)", result.stdout + result.stderr),
                    "manual converter failed " + mode + "\n" + result.stdout + result.stderr)
        check_rendered(root, manual)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    formats = parser.add_mutually_exclusive_group()
    formats.add_argument("--fbdoc", type=Path, help="Render fresh pages using this native converter")
    formats.add_argument("--rendered", type=Path, help="Inspect an existing rendered directory")
    options = parser.parse_args()
    root = options.root.resolve()
    markdown = sorted((root / "docs/source-code").glob("*.md"))
    markdown += [root / name for name in ("docs/README.md", "docs/gfx-maintenance.md", "docs/gfx-locking.md", "mk/README.md",
                                        "src/compiler/README.md", "src/rtlib/README.md",
                                        "src/gfxlib2/README.md", "src/gfxlib3/README.md")]
    links = sum(check_markdown(path) for path in markdown)
    cache = root / "doc/manual/cache"
    native_links = sum(check_wakka(cache, name)[1] for name in PAGES)
    for navigation in ("DocToc", "DevToc"):
        source = (cache / (navigation + ".wakka")).read_text(encoding="utf-8")
        require(all(name in source for name in GUIDES), navigation + ": guide navigation missing")
    for native, markdown_name in GUIDES.items():
        require((root / "docs/source-code" / markdown_name).is_file(), native + ": Markdown guide missing")
    print(f"SOURCE_DOCS_PASS markdown={len(markdown)} links={links} native_links={native_links}")
    if options.fbdoc:
        render(root, options.fbdoc.resolve())
    elif options.rendered:
        check_rendered(root, options.rendered.resolve())
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, subprocess.TimeoutExpired) as error:
        raise SystemExit("SOURCE_DOCS_FAIL " + str(error))

# end of check-source-docs.py
