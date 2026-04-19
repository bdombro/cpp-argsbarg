#!/usr/bin/env python3
"""
Release helper for argsbarg: bump semver, sync docs, update Keep a Changelog footer, commit, tag.

What this is
------------
Single entry point used by `just release <kind>` (and runnable as `python3 scripts/release.py …`).
It bumps the version, commits, tags, **pushes** the branch and tag, and creates a **GitHub release**
from the annotated tag (``gh release create … --notes-from-tag``). Requires a configured ``gh`` CLI.

Steps (in order)
----------------
1. **Preconditions** — Require a git checkout (any working tree state is fine; all changes are
   staged into the release commit).
2. **Read current version** — Parse `return "X.Y.Z"` from `include/argsbarg/argsbarg.hpp` (must
   match the project’s single source of truth).
3. **Compute next version** — From `major` / `minor` / `patch`, or accept an explicit `X.Y.Z`.
   Refuse if the computed version is not greater than the current version.
4. **Pin versions in source** — Write the new semver into the header; update `README.md`
   `FetchContent` `GIT_TAG` and the `find_package(argsbarg … CONFIG` example line.
5. **CHANGELOG** — Insert a dated `## [new] - YYYY-MM-DD` heading (plus blank line only) under
   `## [Unreleased]` — **no** `### Changed` / `### Added` stubs or filler bullets; bump the
   `[Unreleased]` compare link to `v{new}…HEAD`; prepend
   `[new]: compare/v{old}…v{new}` to the reference-link footer (Keep a Changelog style).
6. **Git** — `git add -A` (stage everything), `commit` with `chore: release v{new}`, and create an
   annotated tag `v{new}` whose message is ``Release v{new}`` plus the ``CHANGELOG`` section for
   that version (from ``## [new] - …`` through the line before the next ``## [`` — often only the
   dated heading until you add subsections and bullets).
7. **Publish** — ``git push``, ``git push origin v{new}``, then
   ``gh release create v{new} --verify-tag --notes-from-tag``.

Add release notes under the new heading **before** running (or amend after) if you want more than
the dated heading in the tag and GitHub release body.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
import tempfile
from datetime import date
from pathlib import Path


REPO = "https://github.com/bdombro/cpp-argsbarg"
HDR_REL = Path("include/argsbarg/argsbarg.hpp")
README_REL = Path("README.md")
CHANGELOG_REL = Path("CHANGELOG.md")


def _run(cmd: list[str], *, cwd: Path) -> None:
    """Run a subprocess in ``cwd``; raise if the command exits non-zero."""
    subprocess.run(cmd, cwd=cwd, check=True)


def _ensure_git_repo(repo: Path) -> None:
    """Require ``repo`` to be inside a git working tree."""
    try:
        _run(["git", "rev-parse", "--is-inside-work-tree"], cwd=repo)
    except (subprocess.CalledProcessError, FileNotFoundError) as e:
        raise SystemExit("error: not a git repository") from e


def _read_version_from_header(hdr: Path) -> str:
    """Parse the library semver from ``version()``'s ``return "X.Y.Z"`` line in the umbrella header."""
    text = hdr.read_text(encoding="utf-8")
    m = re.search(r'return\s+"([0-9]+\.[0-9]+\.[0-9]+)"', text)
    if not m:
        raise SystemExit(f"error: could not read semver from {hdr}")
    return m.group(1)


def _bump_version(current: str, kind: str) -> str:
    """Return the next semver from ``current`` using ``major`` / ``minor`` / ``patch``, or an explicit ``X.Y.Z``."""
    parts = current.split(".")
    if len(parts) != 3 or not all(p.isdigit() for p in parts):
        raise SystemExit(f"error: unexpected semver in header: {current!r}")
    maj, min_, pat = (int(parts[0]), int(parts[1]), int(parts[2]))
    if kind == "major":
        return f"{maj + 1}.0.0"
    if kind == "minor":
        return f"{maj}.{min_ + 1}.0"
    if kind == "patch":
        return f"{maj}.{min_}.{pat + 1}"
    if re.fullmatch(r"[0-9]+\.[0-9]+\.[0-9]+", kind):
        return kind
    raise SystemExit("usage: release major | minor | patch | X.Y.Z")


def _write_header_version(hdr: Path, new_ver: str) -> None:
    """Replace the single ``return "…"`` version string in ``argsbarg.hpp`` with ``new_ver``."""
    text = hdr.read_text(encoding="utf-8")
    text2, n = re.subn(r'return\s+"[0-9.]+"', f'return "{new_ver}"', text, count=1)
    if n != 1:
        raise SystemExit(f"error: expected one version() return in {hdr}")
    hdr.write_text(text2, encoding="utf-8")


def _write_readme_pins(readme: Path, new_ver: str) -> None:
    """Update README ``FetchContent`` ``GIT_TAG`` and the ``find_package(argsbarg …)`` example to ``new_ver``."""
    text = readme.read_text(encoding="utf-8")
    text, n_git = re.subn(r"(GIT_TAG\s+)v[0-9.]+", rf"\1v{new_ver}", text, count=1)
    if n_git != 1:
        raise SystemExit("README: expected exactly one FetchContent GIT_TAG line to update")
    text, n_fp = re.subn(
        r"find_package\(argsbarg\s+[0-9.]+\s+CONFIG",
        f"find_package(argsbarg {new_ver} CONFIG",
        text,
        count=1,
    )
    if n_fp != 1:
        raise SystemExit("README: expected exactly one find_package(argsbarg … CONFIG line")
    readme.write_text(text, encoding="utf-8")


def _write_changelog(changelog: Path, *, new_ver: str, today: str, cur_ver: str) -> None:
    """Insert only a dated ``## [new]`` heading under ``[Unreleased]``, bump compare URLs, add link line.

    Deliberately does not insert Keep a Changelog subsection stubs (e.g. ``### Changed``); authors
    add ``### Added`` / ``### Changed`` / etc. under ``[Unreleased]`` or under the new version by hand.
    """
    nl = "\n"
    text = changelog.read_text(encoding="utf-8")
    marker_unrel = "## [Unreleased]" + nl + nl
    if marker_unrel not in text:
        raise SystemExit("missing ## [Unreleased] in CHANGELOG.md")
    # Version heading + blank line only (no ### Changed / ### Added boilerplate).
    new_heading = f"## [{new_ver}] - {today}" + nl + nl
    insert = marker_unrel + new_heading
    text = text.replace(marker_unrel, insert, 1)
    text, n = re.subn(
        rf"(\[Unreleased\]: {re.escape(REPO)}/compare/)v[0-9.]+(\.\.\.HEAD)",
        rf"\1v{new_ver}\2",
        text,
        count=1,
    )
    if n != 1:
        raise SystemExit(
            "CHANGELOG: expected exactly one [Unreleased] ...compare/vX...HEAD footer line"
        )
    footer_line = f"[Unreleased]: {REPO}/compare/v{new_ver}...HEAD{nl}"
    link_line = f"[{new_ver}]: {REPO}/compare/v{cur_ver}...v{new_ver}{nl}"
    if f"[{new_ver}]: {REPO}" not in text:
        if footer_line not in text:
            raise SystemExit("CHANGELOG: [Unreleased] footer line missing after compare bump")
        text = text.replace(footer_line, footer_line + link_line, 1)
    changelog.write_text(text, encoding="utf-8")


def _extract_changelog_release_body(text: str, version: str) -> str:
    """Return the Keep a Changelog block for ``version`` (heading through the line before the next ``## [``)."""
    block = re.search(
        rf"^## \[{re.escape(version)}\]\s*-\s*[^\n]*\n([\s\S]*?)(?=^## \[|\Z)",
        text,
        flags=re.MULTILINE,
    )
    if not block:
        raise SystemExit(f"CHANGELOG: could not find ## [{version}] section to use as tag/release notes")
    return block.group(0).rstrip() + "\n"


def _annotated_tag_message(version: str, changelog_section: str) -> str:
    """Format the annotated tag message: short subject line, blank line, then the changelog section."""
    return f"Release v{version}\n\n{changelog_section.rstrip()}\n"


def main() -> None:
    """CLI entry: validate repo, bump version, edit files, commit, tag, push, GitHub release."""
    parser = argparse.ArgumentParser(
        description="Bump version, sync README/CHANGELOG, commit, tag, push, gh release.",
    )
    parser.add_argument(
        "kind",
        help="major | minor | patch | explicit X.Y.Z",
    )
    parser.add_argument(
        "--repo-root",
        type=Path,
        default=Path(__file__).resolve().parent.parent,
        help="repository root (default: parent of scripts/)",
    )
    args = parser.parse_args()
    repo: Path = args.repo_root.resolve()

    _ensure_git_repo(repo)

    hdr = repo / HDR_REL
    readme = repo / README_REL
    changelog = repo / CHANGELOG_REL

    cur = _read_version_from_header(hdr)
    new = _bump_version(cur, args.kind)
    if new == cur:
        raise SystemExit(
            f"error: already at v{new} (use patch/minor/major or a higher X.Y.Z to bump)"
        )

    today = date.today().isoformat()

    _write_header_version(hdr, new)
    _write_readme_pins(readme, new)
    _write_changelog(changelog, new_ver=new, today=today, cur_ver=cur)

    notes = _extract_changelog_release_body(changelog.read_text(encoding="utf-8"), new)
    tag_message = _annotated_tag_message(new, notes)

    _run(["git", "add", "-A"], cwd=repo)
    _run(["git", "commit", "-m", f"chore: release v{new}"], cwd=repo)

    with tempfile.NamedTemporaryFile(
        mode="w",
        encoding="utf-8",
        suffix=".tagmsg",
        delete=False,
    ) as tag_msg_file:
        tag_msg_file.write(tag_message)
        tag_msg_path = tag_msg_file.name
    try:
        _run(["git", "tag", "-a", f"v{new}", "-F", tag_msg_path], cwd=repo)
    finally:
        Path(tag_msg_path).unlink(missing_ok=True)

    _run(["git", "push"], cwd=repo)
    _run(["git", "push", "origin", f"v{new}"], cwd=repo)
    _run(
        [
            "gh",
            "release",
            "create",
            f"v{new}",
            "--verify-tag",
            "--notes-from-tag",
        ],
        cwd=repo,
    )

    print(f"Released v{new}: commit + annotated tag (CHANGELOG body), pushed, GitHub release created.")


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as e:
        print(f"error: command failed: {e.cmd}", file=sys.stderr)
        sys.exit(e.returncode or 1)
