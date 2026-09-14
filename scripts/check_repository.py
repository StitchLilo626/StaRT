#!/usr/bin/env python3
"""Check tracked-file hygiene and new commit messages using only Python/Git."""

import argparse
import re
import subprocess
import sys


SUBJECT = re.compile(
    r"(?:fix|feat|docs|perf|refactor|test|build|ci|chore|style|revert)"
    r"(?:\([a-z0-9][a-z0-9_./-]*\))?!?: \S.*"
)


def message_errors(message):
    """Validate project policy, not subjective wording or authorship claims."""
    lines = message.splitlines()
    if not lines:
        return ["empty commit message"]
    subject = lines[0]
    errors = []
    if not SUBJECT.fullmatch(subject):
        errors.append("use type(scope): summary (use docs, not doc)")
    if len(subject) > 72:
        errors.append("subject exceeds 72 characters")
    if subject != subject.strip() or subject.endswith("."):
        errors.append("subject has surrounding whitespace or a final period")
    if len(lines) < 3 or lines[1] or not any(line.strip() for line in lines[2:]):
        errors.append("separate a meaningful commit body with a blank line")
    return errors


def git(*args):
    result = subprocess.run(
        ["git", *args], stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        encoding="utf-8", errors="replace", check=False,
    )
    if result.returncode:
        raise RuntimeError(result.stderr.strip() or result.stdout.strip()
                           or "git command failed: " + " ".join(args))
    return result.stdout


def check_repository(base=None, head="HEAD"):
    errors = []
    # Check the index, so this also catches a force-added ignored file.
    ignored = git("ls-files", "-ci", "--exclude-standard", "-z").split("\0")
    errors.extend("tracked file matches ignore rules: " + path
                  for path in ignored if path)
    for args in [("diff", "--check"), ("diff", "--cached", "--check")]:
        try:
            git(*args)
        except RuntimeError as exc:
            errors.append(str(exc))
    if base is not None:
        base_id = git("rev-parse", "--verify", "--end-of-options",
                      base + "^{commit}").strip()
        head_id = git("rev-parse", "--verify", "--end-of-options",
                      head + "^{commit}").strip()
        commits = git("rev-list", "--reverse", "--no-merges",
                      base_id + ".." + head_id).splitlines()
        for commit in commits:
            message = git("show", "-s", "--format=%B", commit)
            errors.extend(commit[:12] + ": " + error
                          for error in message_errors(message))
            try:
                git("show", "--format=", "--check", commit, "--")
            except RuntimeError as exc:
                errors.append(commit[:12] + ": " + str(exc))
        # Include whitespace introduced only by a merge resolution, too.
        try:
            git("diff", "--check", base_id, head_id, "--")
        except RuntimeError as exc:
            errors.append(str(exc))
        print("Checked {} non-merge commit(s).".format(len(commits)))
    return errors


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base", help="exclusive base; omit for index/worktree checks")
    parser.add_argument("--head", default="HEAD", help="inclusive head (default: HEAD)")
    args = parser.parse_args()
    try:
        errors = check_repository(args.base, args.head)
    except (RuntimeError, OSError) as exc:
        print("ERROR: " + str(exc), file=sys.stderr)
        return 1
    for error in errors:
        print("ERROR: " + error, file=sys.stderr)
    if errors:
        return 1
    print("Repository checks passed (not a firmware build or hardware test).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
