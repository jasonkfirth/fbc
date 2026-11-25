#!/usr/bin/env python3
"""FreeBASIC browser corpus. Add reviewed entries to an existing static release.

Verify the manifests and browser receipts and write a full release and a small
deployment overlay. The previous release, compiled programs, and server are
owned by the build driver and publishing scripts.
"""

import argparse
import errno
import hashlib
import json
import os
from pathlib import Path
import re
import runpy
import shutil


CARD = re.compile(r'<li><h2><a href="([\w-]+)/(?:play\.html)?">[^\n]+</li>')
CATALOG_FILES = ("index.html", "text.html", "tools.html", "demos.html")


def digest(file):
    with file.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def manifest(site):
    backing = (site / "SHA256SUMS.txt").read_text(encoding="ascii")
    expected = dict(line.split("  ", 1)[::-1] for line in backing.splitlines())
    actual = {file.relative_to(site).as_posix(): digest(file) for file in site.rglob("*")
              if file.is_file() and file != site / "SHA256SUMS.txt"}
    if expected != actual:
        raise ValueError(f"Manifest does not match {site}")
    return expected


def cards_on_site(site):
    cards = {}
    for leaf in CATALOG_FILES:
        if not (site / leaf).is_file():
            continue
        for match in CARD.finditer((site / leaf).read_text(encoding="utf-8")):
            previous = cards.setdefault(match[1], match[0])
            if previous != match[0]:
                raise ValueError(f"Inconsistent card for {match[1]}")
    return cards


def preview(file, site):
    rows = json.loads(file.read_text(encoding="utf-8"))
    slugs = cards_on_site(site).keys()
    if len(rows) != len(slugs) or {row["slug"] for row in rows} != slugs:
        raise ValueError(f"Browser receipt does not cover {site}")
    if any(row["status"] != "passed" for row in rows):
        raise ValueError(f"Browser receipt has a failure: {file}")
    return rows


def link_file(source, destination):
    destination.parent.mkdir(parents=True, exist_ok=True)
    try:
        os.link(source, destination)
    except OSError as error:
        if error.errno not in {errno.EXDEV, errno.ENOTSUP} and getattr(error, "winerror", None) not in {1, 17, 50}:
            raise
        # Some staging volumes do not support hard links. Keep the release
        # usable there by copying each immutable file once.
        shutil.copyfile(source, destination)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for option in ("base", "base-preview", "add", "add-preview", "site", "overlay", "receipt"):
        parser.add_argument("--" + option, type=Path, required=True)
    parser.add_argument("--replace", nargs=2, action="append", default=[], metavar=("OLD", "NEW"),
                        help="replace a catalog entry and redirect prior launch URLs if the slug changes")
    parser.add_argument("--organize", action="store_true", help="organize the catalog into showcase, text, tools and demos")
    args = parser.parse_args()
    base, extra = manifest(args.base), manifest(args.add)
    base_rows = preview(args.base_preview, args.base)
    added_rows = preview(args.add_preview, args.add)
    slugs = {row["slug"] for row in added_rows}
    base_slugs = {row["slug"] for row in base_rows}
    replacements = dict(args.replace)
    if (len(replacements) != len(args.replace) or len(set(replacements.values())) != len(replacements)
            or replacements.keys() - base_slugs or set(replacements.values()) - slugs):
        raise ValueError("Each replaced base entry must point to one distinct addition")
    if slugs & base_slugs != {old for old, new in replacements.items() if old == new}:
        raise ValueError("An existing slug requires an explicit replacement")
    for row in added_rows:
        for file in ("index.js", "index.wasm", "index.data"):
            published = extra.get(row["slug"] + "/" + file)
            checked = row["artifacts"].get(file)
            if file == "index.data" and published is None and checked is None:
                continue
            if published is None or published != checked:
                raise ValueError(f"Browser receipt has a different {file} for {row['slug']}")

    base_index = (args.base / "index.html").read_text(encoding="utf-8")
    cards = cards_on_site(args.add)
    old_cards = cards_on_site(args.base)
    catalog = runpy.run_path(str(Path(__file__).with_name("fb-corpus-js-site.py")))
    organized = args.organize or any(leaf in base for leaf in ("tools.html", "demos.html"))
    redirects = {}
    if replacements:
        launch_page = catalog["legacy_launch_page"]
        for old, new in replacements.items():
            if old not in old_cards or new not in cards:
                raise ValueError("Both entries must be in their main catalog")
            card = cards[new]
            if organized or old != new:
                old_cards[old] = card
            if not organized:
                if f'href="{old}/' not in base_index:
                    raise ValueError("Use --organize to replace an entry outside the main catalog")
                [old_card] = [match[0] for match in CARD.finditer(base_index) if match[1] == old]
                base_index = base_index.replace(old_card, card, 1)
            if old == new:
                continue
            title = card.split('">', 1)[1].split("</a>", 1)[0]
            redirect = launch_page(title, f"../{new}/play.html")
            redirects[f"{old}/index.html"] = redirect
            redirects[f"{old}/play.html"] = redirect

    if organized:
        combined = {replacements.get(slug, slug): card for slug, card in old_cards.items()}
        combined.update(cards)
        pages = dict(catalog["catalog_pages"](combined))
        # Preserve a navigation destination if a section becomes empty.
        for leaf in CATALOG_FILES:
            if leaf in base and leaf not in pages:
                pages[leaf] = catalog["legacy_launch_page"](catalog["CATALOG_PAGES"][leaf][0], "./")
    else:
        list_start = base_index.find('<ul id="program-list">')
        if list_start == -1:
            list_start = base_index.find("<ul>", base_index.find("<main>"))
        list_end = base_index.find("</ul>", list_start)
        if list_start == -1 or list_end == -1 or "</main>" not in base_index[list_end:]:
            raise ValueError("Could not find the catalog list")
        added_cards = "".join(cards[slug] + "\n" for slug in sorted(slugs - set(replacements.values())))
        pages = {"index.html": base_index[:list_end] + added_cards + base_index[list_end:]}
    additions = {name: checksum for name, checksum in extra.items() if name.split("/", 1)[0] in slugs}

    args.site.mkdir()
    args.overlay.mkdir()
    for name in base.keys() - pages.keys() - redirects.keys() - additions.keys():
        link_file(args.base / name, args.site / name)
    for name in additions:
        link_file(args.add / name, args.site / name)
        link_file(args.site / name, args.overlay / name)
    result = base | additions
    for name, content in {**pages, **redirects}.items():
        (args.site / name).write_text(content, encoding="utf-8", newline="\n")
        link_file(args.site / name, args.overlay / name)
        result[name] = digest(args.site / name)
    (args.site / "SHA256SUMS.txt").write_text("".join(f"{checksum}  {name}\n" for name, checksum in sorted(result.items())),
                                             encoding="ascii", newline="\n")
    link_file(args.site / "SHA256SUMS.txt", args.overlay / "SHA256SUMS.txt")
    manifest(args.site)
    rows = [row for row in base_rows if row["slug"] not in replacements] + added_rows
    args.receipt.write_text(json.dumps(rows, indent=2) + "\n", encoding="utf-8")
    print(f"CATALOG_PREVIEW={len(rows)} ASSETS={len(result)} OVERLAY_FILES={len(additions) + len(redirects) + len(pages) + 1}")


if __name__ == "__main__":
    main()

# end of fb-corpus-js-extend.py
