#!/usr/bin/env python3
"""
Compare pdfalto output between a git revision and the current working tree.

Builds both versions automatically using git worktrees, runs them on the
given PDF files, and reports structural differences in the ALTO XML output
(blocks, line numbers, text content).

Usage:
    ./scripts/compare_revisions.py <revision> <pdf> [<pdf> ...]
    ./scripts/compare_revisions.py HEAD~1 /path/to/paper.pdf
    ./scripts/compare_revisions.py v0.5.0 /path/to/*.pdf

Requirements: cmake, a C++ compiler, and the project's build dependencies.
"""

import argparse
import shutil
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET
from pathlib import Path

ALTO_NS = {"alto": "http://www.loc.gov/standards/alto/ns-v3#"}
REPO_ROOT = Path(__file__).resolve().parent.parent


# ---------------------------------------------------------------------------
# Build helpers
# ---------------------------------------------------------------------------

def resolve_revision(revision: str) -> str:
    """Resolve a revision spec to a full commit hash."""
    result = subprocess.run(
        ["git", "rev-parse", "--verify", revision],
        capture_output=True, text=True, cwd=REPO_ROOT,
    )
    if result.returncode != 0:
        sys.exit(f"Error: cannot resolve revision '{revision}': {result.stderr.strip()}")
    return result.stdout.strip()


def revision_label(revision: str) -> str:
    """Short human-readable label for a commit."""
    result = subprocess.run(
        ["git", "log", "--format=%h %s", "-1", revision],
        capture_output=True, text=True, cwd=REPO_ROOT,
    )
    return result.stdout.strip() if result.returncode == 0 else revision[:10]


def build_in_worktree(revision: str, tmpdir: Path) -> Path:
    """Create a git worktree at *revision*, build pdfalto, return the binary path."""
    worktree = tmpdir / f"worktree-{revision[:10]}"
    print(f"  Creating worktree at {worktree} ...")
    subprocess.run(
        ["git", "worktree", "add", "--detach", str(worktree), revision],
        capture_output=True, text=True, cwd=REPO_ROOT, check=True,
    )

    # Initialize submodules in the worktree
    print(f"  Initializing submodules ...")
    subprocess.run(
        ["git", "submodule", "update", "--init", "--recursive"],
        capture_output=True, text=True, cwd=worktree,
    )

    # Pre-build xpdf dependency (generates aconf.h which is gitignored)
    xpdf_build = worktree / "xpdf-4.05" / "build"
    if not (xpdf_build / "aconf.h").exists():
        xpdf_build.mkdir(parents=True, exist_ok=True)
        print(f"  Pre-configuring xpdf (generating aconf.h) ...")
        subprocess.run(
            ["cmake", ".."],
            capture_output=True, text=True, cwd=xpdf_build,
        )

    build_dir = worktree / "build"
    build_dir.mkdir()

    print(f"  Configuring ...")
    r = subprocess.run(
        ["cmake", ".."],
        capture_output=True, text=True, cwd=build_dir,
    )
    if r.returncode != 0:
        sys.exit(f"cmake configure failed:\n{r.stderr}")

    print(f"  Building ...")
    r = subprocess.run(
        ["cmake", "--build", ".", "--parallel"],
        capture_output=True, text=True, cwd=build_dir,
    )
    if r.returncode != 0:
        sys.exit(f"cmake build failed:\n{r.stderr}")

    binary = build_dir / "pdfalto"
    if not binary.exists():
        sys.exit(f"Build succeeded but binary not found at {binary}")
    return binary


def build_current(tmpdir: Path) -> Path:
    """Build the current working tree, return the binary path."""
    build_dir = REPO_ROOT / "cmake-build-compare"
    build_dir.mkdir(exist_ok=True)

    print(f"  Configuring ...")
    r = subprocess.run(
        ["cmake", ".."],
        capture_output=True, text=True, cwd=build_dir,
    )
    if r.returncode != 0:
        sys.exit(f"cmake configure failed:\n{r.stderr}")

    print(f"  Building ...")
    r = subprocess.run(
        ["cmake", "--build", ".", "--parallel"],
        capture_output=True, text=True, cwd=build_dir,
    )
    if r.returncode != 0:
        sys.exit(f"cmake build failed:\n{r.stderr}")

    binary = build_dir / "pdfalto"
    if not binary.exists():
        sys.exit(f"Build succeeded but binary not found at {binary}")
    return binary


def cleanup_worktree(revision: str, tmpdir: Path):
    worktree = tmpdir / f"worktree-{revision[:10]}"
    if worktree.exists():
        subprocess.run(
            ["git", "worktree", "remove", "--force", str(worktree)],
            capture_output=True, text=True, cwd=REPO_ROOT,
        )


# ---------------------------------------------------------------------------
# Run pdfalto
# ---------------------------------------------------------------------------

def run_pdfalto(binary: Path, pdf: Path, output_xml: Path) -> bool:
    r = subprocess.run(
        [str(binary), str(pdf), str(output_xml)],
        capture_output=True, text=True, timeout=300,
    )
    return r.returncode == 0 and output_xml.exists()


# ---------------------------------------------------------------------------
# ALTO XML analysis
# ---------------------------------------------------------------------------

class PageStats:
    def __init__(self):
        self.total_blocks = 0
        self.total_lines = 0
        self.dedicated_line_nums = 0   # in a multi-number block (>10)
        self.single_number_blocks = 0  # standalone single-digit blocks
        self.content_blocks = 0
        self.first_content = ""        # first few words of content

    def __repr__(self):
        parts = [f"blocks={self.total_blocks}"]
        if self.dedicated_line_nums:
            parts.append(f"lineNums={self.dedicated_line_nums}")
        if self.single_number_blocks:
            parts.append(f"singles={self.single_number_blocks}")
        return ", ".join(parts)


def analyze_page(page_elem) -> PageStats:
    stats = PageStats()
    first_content_found = False

    for block in page_elem.findall(".//alto:TextBlock", ALTO_NS):
        stats.total_blocks += 1
        lines = block.findall(".//alto:TextLine", ALTO_NS)
        stats.total_lines += len(lines)

        # Count number-only lines in this block
        num_only = 0
        for line in lines:
            tokens = line.findall(".//alto:String", ALTO_NS)
            if len(tokens) == 1 and tokens[0].get("CONTENT", "").isdigit():
                num_only += 1

        if num_only > 10:
            stats.dedicated_line_nums += num_only
        elif num_only == 1 and len(lines) == 1:
            stats.single_number_blocks += 1
        else:
            stats.content_blocks += 1
            if not first_content_found and lines:
                tokens = lines[0].findall(".//alto:String", ALTO_NS)
                stats.first_content = " ".join(
                    t.get("CONTENT", "") for t in tokens[:8]
                )
                first_content_found = True

    return stats


def analyze_alto(xml_path: Path) -> list[PageStats]:
    tree = ET.parse(xml_path)
    root = tree.getroot()
    return [analyze_page(p) for p in root.findall(".//alto:Page", ALTO_NS)]


# ---------------------------------------------------------------------------
# Comparison and reporting
# ---------------------------------------------------------------------------

def compare_and_report(pdf_name: str, old_stats: list[PageStats], new_stats: list[PageStats],
                       old_label: str, new_label: str):
    n_pages = max(len(old_stats), len(new_stats))

    has_diff = False
    diff_rows = []
    for i in range(n_pages):
        old = old_stats[i] if i < len(old_stats) else PageStats()
        new = new_stats[i] if i < len(new_stats) else PageStats()

        diffs = []
        if old.total_blocks != new.total_blocks:
            diffs.append(f"blocks {old.total_blocks}->{new.total_blocks}")
        if old.dedicated_line_nums != new.dedicated_line_nums:
            diffs.append(f"lineNums {old.dedicated_line_nums}->{new.dedicated_line_nums}")
        if old.single_number_blocks != new.single_number_blocks:
            diffs.append(f"singles {old.single_number_blocks}->{new.single_number_blocks}")
        if old.total_lines != new.total_lines:
            diffs.append(f"lines {old.total_lines}->{new.total_lines}")

        if diffs:
            has_diff = True
            diff_rows.append((i + 1, ", ".join(diffs)))

    # Summary line
    old_total_ln = sum(s.dedicated_line_nums for s in old_stats)
    new_total_ln = sum(s.dedicated_line_nums for s in new_stats)
    old_total_sn = sum(s.single_number_blocks for s in old_stats)
    new_total_sn = sum(s.single_number_blocks for s in new_stats)
    old_total_bl = sum(s.total_blocks for s in old_stats)
    new_total_bl = sum(s.total_blocks for s in new_stats)

    status = "CHANGED" if has_diff else "IDENTICAL"
    print(f"\n{'='*70}")
    print(f"  {pdf_name}  [{status}]")
    print(f"{'='*70}")
    print(f"  Pages: {n_pages}")
    print(f"  {'':30s} {'OLD':>10s} {'NEW':>10s} {'DIFF':>10s}")
    print(f"  {'Total blocks':30s} {old_total_bl:10d} {new_total_bl:10d} {new_total_bl-old_total_bl:+10d}")
    print(f"  {'Detected line numbers':30s} {old_total_ln:10d} {new_total_ln:10d} {new_total_ln-old_total_ln:+10d}")
    print(f"  {'Single-number blocks':30s} {old_total_sn:10d} {new_total_sn:10d} {new_total_sn-old_total_sn:+10d}")

    if diff_rows:
        print(f"\n  Per-page differences:")
        for page_num, desc in diff_rows:
            print(f"    Page {page_num:3d}: {desc}")
    else:
        print(f"\n  No per-page differences.")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Compare pdfalto output between a git revision and the current working tree.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("revision", help="Git revision to compare against (tag, branch, commit hash)")
    parser.add_argument("pdfs", nargs="+", type=Path, help="PDF file(s) to process")
    parser.add_argument("--keep-tmpdir", action="store_true", help="Don't delete temporary build directory")
    args = parser.parse_args()

    # Validate PDFs exist
    for pdf in args.pdfs:
        if not pdf.exists():
            sys.exit(f"Error: PDF not found: {pdf}")

    commit = resolve_revision(args.revision)
    old_label = revision_label(commit)
    new_label = "current working tree"

    print(f"Comparing:")
    print(f"  OLD: {old_label}")
    print(f"  NEW: {new_label}")
    print()

    tmpdir = Path(tempfile.mkdtemp(prefix="pdfalto-compare-"))

    try:
        # Build old revision
        print(f"Building OLD ({args.revision}) ...")
        old_binary = build_in_worktree(commit, tmpdir)
        print(f"  -> {old_binary}")

        # Build current
        print(f"\nBuilding NEW (working tree) ...")
        new_binary = build_current(tmpdir)
        print(f"  -> {new_binary}")

        # Process each PDF
        for pdf in args.pdfs:
            pdf = pdf.resolve()
            pdf_name = pdf.name

            old_out = tmpdir / "output-old" / pdf_name.replace(".pdf", ".xml")
            new_out = tmpdir / "output-new" / pdf_name.replace(".pdf", ".xml")
            old_out.parent.mkdir(parents=True, exist_ok=True)
            new_out.parent.mkdir(parents=True, exist_ok=True)

            print(f"\nProcessing {pdf_name} ...")

            ok_old = run_pdfalto(old_binary, pdf, old_out)
            ok_new = run_pdfalto(new_binary, pdf, new_out)

            if not ok_old:
                print(f"  WARNING: OLD version failed on {pdf_name}")
            if not ok_new:
                print(f"  WARNING: NEW version failed on {pdf_name}")

            if ok_old and ok_new:
                old_stats = analyze_alto(old_out)
                new_stats = analyze_alto(new_out)
                compare_and_report(pdf_name, old_stats, new_stats, old_label, new_label)
            elif ok_old:
                old_stats = analyze_alto(old_out)
                print(f"  OLD only: {len(old_stats)} pages, "
                      f"{sum(s.total_blocks for s in old_stats)} blocks")
            elif ok_new:
                new_stats = analyze_alto(new_out)
                print(f"  NEW only: {len(new_stats)} pages, "
                      f"{sum(s.total_blocks for s in new_stats)} blocks")

    finally:
        cleanup_worktree(commit, tmpdir)
        # Clean up the in-tree build directory for the current version
        current_build = REPO_ROOT / "cmake-build-compare"
        if current_build.exists():
            shutil.rmtree(current_build, ignore_errors=True)
        if args.keep_tmpdir:
            print(f"\nTemporary directory kept at: {tmpdir}")
        else:
            shutil.rmtree(tmpdir, ignore_errors=True)


if __name__ == "__main__":
    main()
