#!/usr/bin/env python3
"""Project: FreeBASIC source documentation
File: test-source-docs.py
Purpose: Keep local work from concealing broken committed documentation links.
Responsibilities: Exercise real Git trees, staged changes and native manual links.
This file intentionally does NOT build the compiler or render the manual.
"""

import importlib.util
from pathlib import Path
import subprocess
import tempfile
import unittest


spec = importlib.util.spec_from_file_location(
    "source_docs", Path(__file__).with_name("check-source-docs.py"))
docs = importlib.util.module_from_spec(spec)
spec.loader.exec_module(docs)


class SnapshotContracts(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="fbc-doc-links-")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name).resolve()
        self.git("init", "--quiet")
        self.page = self.root / "docs/README.md"
        self.page.parent.mkdir()
        self.page.write_text("[Guide](guide.md)\n", encoding="utf-8")
        self.git("add", "docs/README.md")

    def git(self, *arguments):
        result = subprocess.run(["git", "-C", str(self.root), *arguments],
                                capture_output=True, text=True, timeout=30)
        self.assertEqual(result.returncode, 0, result.stderr)
        return result.stdout.strip()

    def snapshot(self):
        # write-tree inspects the index without creating commits or changing
        # global identity/configuration. Only this tiny owned repository is used.
        return docs.GitTree(self.root, self.git("write-tree"))

    def test_untracked_destination_cannot_hide_missing_link(self):
        (self.page.parent / "guide.md").write_text("Local draft\n", encoding="utf-8")
        self.assertEqual(docs.check_markdown(self.page), 1)
        with self.assertRaisesRegex(ValueError, "missing link guide.md"):
            docs.check_markdown(self.page, self.snapshot())

    def test_dirty_index_page_cannot_hide_committed_missing_link(self):
        snapshot = self.snapshot()
        self.page.write_text("No links left locally\n", encoding="utf-8")
        self.assertEqual(docs.check_markdown(self.page), 0)
        with self.assertRaisesRegex(ValueError, "missing link guide.md"):
            docs.check_markdown(self.page, snapshot)

    def test_staged_guide_survives_local_deletion(self):
        target = self.page.parent / "guide.md"
        target.write_text("Published guide\n", encoding="utf-8")
        self.git("add", "docs/guide.md")
        snapshot = self.snapshot()
        target.unlink()
        self.assertEqual(docs.check_markdown(self.page, snapshot), 1)
        self.assertEqual(snapshot.read_text(target), "Published guide\n")

    def test_snapshot_handles_directory_encoded_and_parent_links(self):
        target = self.page.parent / "source code/guide.md"
        target.parent.mkdir()
        target.write_text("Guide\n", encoding="utf-8")
        self.page.write_text("[Directory](source%20code/)\n"
                             "[Guide](../docs/source%20code/guide.md#part)\n"
                             "[Root](../)\n"
                             "[Web](https://example.invalid/)\n"
                             "```\n[Example](absent.md)\n```\n", encoding="utf-8")
        self.git("add", "docs")
        self.assertEqual(docs.check_markdown(self.page, self.snapshot()), 3)

    def test_native_manual_uses_snapshot_targets_and_content(self):
        cache = self.root / "doc/manual/cache"
        cache.mkdir(parents=True)
        page = cache / "Guide.wakka"
        page.write_text('{{fbdoc item="title" value="Guide"}}\n[[Missing]]\n',
                        encoding="utf-8")
        self.git("add", "doc")
        snapshot = self.snapshot()
        (cache / "Missing.wakka").write_text("Local only\n", encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "missing native link Missing"):
            docs.check_wakka(cache, "Guide", snapshot)
        self.git("add", "doc")
        self.assertEqual(docs.check_wakka(cache, "Guide", self.snapshot()), ("Guide", 1))


if __name__ == "__main__":
    unittest.main(verbosity=2)

# end of test-source-docs.py
