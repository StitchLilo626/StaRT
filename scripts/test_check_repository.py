"""Regression tests for the commit-message policy."""

import unittest
from pathlib import Path
import subprocess
import sys
import tempfile

from check_repository import message_errors


class CommitMessageTests(unittest.TestCase):
    def test_supported_types_and_optional_scope(self):
        for kind in ("fix", "feat", "docs", "perf", "refactor", "test",
                     "build", "ci", "chore", "style", "revert"):
            for scope in ("", "(timer)", "(bsp/stm32)"):
                with self.subTest(kind=kind, scope=scope):
                    self.assertEqual([], message_errors(
                        kind + scope + ": describe the change\n\nExplain why.\n"))

    def test_breaking_change(self):
        self.assertEqual([], message_errors(
            "feat(ipc)!: change the return contract\n\nExplain migration.\n"))

    def test_rejects_legacy_and_malformed_messages(self):
        for message in ("", "note", '"README"', "doc: update guide\n\nWhy.",
                        "fix(timer): \n\nWhy.", "fix: change it.\n\nWhy.",
                        "fix: change it \n\nWhy.", "fix: change it\nWhy.",
                        "fix: change it\n\n", "fix: change it\n\n  "):
            with self.subTest(message=message):
                self.assertTrue(message_errors(message))

    def test_title_length_boundary(self):
        self.assertEqual([], message_errors("docs: " + "a" * 66 + "\n\nWhy."))
        self.assertTrue(message_errors("docs: " + "a" * 67 + "\n\nWhy."))

    def test_unicode_body_and_crlf(self):
        self.assertEqual([], message_errors(
            "docs: explain kernel checks\r\n\r\n说明验证范围。\r\n"))


class RepositoryIntegrationTests(unittest.TestCase):
    def test_range_and_force_added_output(self):
        script = Path(__file__).with_name("check_repository.py").resolve()
        with tempfile.TemporaryDirectory(prefix="start-git-check-") as directory:
            root = Path(directory)

            def run_git(*args):
                return subprocess.check_output(
                    ["git", "-c", "user.name=Repository test",
                     "-c", "user.email=test@example.invalid", *args],
                    cwd=root, stderr=subprocess.PIPE, text=True,
                ).strip()

            def check(base):
                return subprocess.run(
                    [sys.executable, str(script), "--base", base],
                    cwd=root, capture_output=True, text=True,
                )

            run_git("init")
            (root / ".gitignore").write_text("*.lst\n", encoding="utf-8")
            run_git("add", ".gitignore")
            run_git("commit", "-m", "legacy history")
            baseline = run_git("rev-parse", "HEAD")
            self.assertEqual(0, check(baseline).returncode)

            (root / "input.c").write_text("/* source */\n", encoding="utf-8")
            run_git("add", "input.c")
            run_git("commit", "-m", "test: add a source fixture", "-m", "Explain why.")
            self.assertEqual(0, check(baseline).returncode)
            self.assertNotEqual(0, check("missing-reference").returncode)

            run_git("commit", "--allow-empty", "-m", "note")
            self.assertIn("type(scope)", check(baseline).stderr)

            (root / "generated.lst").write_text("output\n", encoding="utf-8")
            run_git("add", "-f", "generated.lst")
            result = check(baseline)
            self.assertNotEqual(0, result.returncode)
            self.assertIn("tracked file matches ignore rules: generated.lst", result.stderr)


if __name__ == "__main__":
    unittest.main()
