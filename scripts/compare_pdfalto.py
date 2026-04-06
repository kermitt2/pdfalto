#!/usr/bin/env python3
"""
Compare multiple pdfalto versions on a directory of PDFs.

This script runs multiple pdfalto executables on PDF files, converts the ALTO XML
output to plain text, and compares the results using various metrics.
"""

import argparse
import csv
import html
import subprocess
import sys
from concurrent.futures import ProcessPoolExecutor, as_completed
from pathlib import Path
from typing import Dict, List, Tuple

try:
    from rapidfuzz.distance import Levenshtein
    HAS_RAPIDFUZZ = True
except ImportError:
    HAS_RAPIDFUZZ = False
    print("Warning: rapidfuzz not installed. Install it before contitnue.", file=sys.stderr)
    sys.exit(-1)

try:
    from rouge_score import rouge_scorer
    HAS_ROUGE = True
except ImportError:
    HAS_ROUGE = False

try:
    import sacrebleu
    HAS_SACREBLEU = True
except ImportError:
    HAS_SACREBLEU = False



def run_pdfalto(pdfalto_exe: Path, pdf_path: Path, output_xml: Path) -> bool:
    """Run pdfalto on a PDF file."""
    # Using the command pattern from pdfalto2text.sh
    cmd = [
        str(pdfalto_exe),
        '-noImageInline',
        '-fullFontName',
        '-noImage',
        '-readingOrder',
        str(pdf_path),
        str(output_xml)
    ]
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=300)
        if result.returncode != 0:
            return False
        # Validate that output file exists and is non-empty
        if not output_xml.exists() or output_xml.stat().st_size == 0:
            print(f"Warning: pdfalto produced empty output for {pdf_path}", file=sys.stderr)
            return False
        return True
    except subprocess.TimeoutExpired:
        print(f"Timeout processing {pdf_path}", file=sys.stderr)
        return False
    except Exception as e:
        print(f"Error running pdfalto on {pdf_path}: {e}", file=sys.stderr)
        return False


def alto_to_text(xml_path: Path, xslt_path: Path, output_txt: Path) -> bool:
    """Convert ALTO XML to plain text using xsltproc."""
    cmd = ['xsltproc', str(xslt_path), str(xml_path)]
    try:
        result = subprocess.run(cmd, capture_output=True, encoding='utf-8', timeout=60)
        if result.returncode == 0:
            with open(output_txt, 'w', encoding='utf-8') as f:
                f.write(result.stdout)
            # Warn if output is empty (possibly namespace mismatch)
            if not result.stdout.strip():
                print(f"Warning: xsltproc produced empty output for {xml_path}", file=sys.stderr)
            return True
        print(f"Error: xsltproc failed for {xml_path}: {result.stderr}", file=sys.stderr)
        return False
    except Exception as e:
        print(f"Error converting {xml_path} to text: {e}", file=sys.stderr)
        return False


def process_single_pdf(
    pdf_path: Path,
    input_dir: Path,
    output_dir: Path,
    pdfalto_dirs: List[Path],
    xslt_path: Path,
    no_resume: bool = False
) -> List[Tuple[str, Path, bool]]:
    """Process a single PDF with all pdfalto versions. Returns list of (version_id, txt_path, success)."""
    results = []

    # Calculate relative path
    rel_path = pdf_path.relative_to(input_dir)

    for pdfalto_dir in pdfalto_dirs:
        version_id = pdfalto_dir.name
        pdfalto_exe = pdfalto_dir / 'pdfalto'

        if not pdfalto_exe.exists():
            print(f"Warning: pdfalto executable not found in {pdfalto_dir}", file=sys.stderr)
            results.append((version_id, None, False))
            continue

        # Output paths
        version_output_dir = output_dir / version_id / rel_path.parent
        version_output_dir.mkdir(parents=True, exist_ok=True)

        output_xml = version_output_dir / f"{pdf_path.stem}.xml"
        output_txt = version_output_dir / f"{pdf_path.stem}.txt"

        # Resume logic: skip if text file already exists (and is non-empty)
        if not no_resume and output_txt.exists() and output_txt.stat().st_size > 0:
            results.append((version_id, output_txt, True))
            continue

        # Run pdfalto
        success = run_pdfalto(pdfalto_exe, pdf_path, output_xml)

        if success and output_xml.exists():
            # Convert to text
            success = alto_to_text(output_xml, xslt_path, output_txt)

        results.append((version_id, output_txt if success else None, success))

    return results


def find_pdfs(input_dir: Path) -> List[Path]:
    """Find all PDF files in the input directory tree."""
    return list(input_dir.rglob('*.pdf'))


def read_text_file(filepath: Path) -> str:
    """Read text content from a file."""
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            return f.read()
    except Exception as e:
        print(f"Error reading {filepath}: {e}", file=sys.stderr)
        return ""


def normalize_spaces(text: str) -> str:
    """Normalize spaces in text (collapse whitespace)."""
    return ''.join(text.split())


def compute_edit_distance(text1: str, text2: str) -> float:
    """Compute normalized edit distance between two texts."""
    if not text1 and not text2:
        return 0.0
    if not text1 or not text2:
        return 1.0

    distance = Levenshtein.distance(text1, text2)

    max_len = max(len(text1), len(text2))
    return distance / max_len if max_len > 0 else 0.0


def compute_rouge(text1: str, text2: str) -> Dict[str, float]:
    """Compute ROUGE scores between two texts."""
    if not HAS_ROUGE:
        return {}

    # Guard against empty texts which can cause issues with ROUGE
    if not text1.strip() or not text2.strip():
        return {
            'rouge1_f': 0.0,
            'rouge2_f': 0.0,
            'rougeL_f': 0.0
        }

    scorer = rouge_scorer.RougeScorer(['rouge1', 'rouge2', 'rougeL'], use_stemmer=True)
    scores = scorer.score(text1, text2)

    return {
        'rouge1_f': scores['rouge1'].fmeasure,
        'rouge2_f': scores['rouge2'].fmeasure,
        'rougeL_f': scores['rougeL'].fmeasure
    }


def compute_bleu(reference: str, hypothesis: str) -> float:
    """Compute BLEU score."""
    if not HAS_SACREBLEU:
        return 0.0

    try:
        bleu = sacrebleu.sentence_bleu(hypothesis, [reference])
        return bleu.score / 100.0  # Normalize to 0-1
    except Exception:
        return 0.0


def compare_texts(
    text1: str,
    text2: str,
    metrics: List[str]
) -> Dict[str, float]:
    """Compare two texts using specified metrics."""
    results = {}

    if 'edit_distance' in metrics:
        results['edit_distance'] = compute_edit_distance(text1, text2)

    if 'edit_distance_no_spaces' in metrics:
        t1_normalized = normalize_spaces(text1)
        t2_normalized = normalize_spaces(text2)
        results['edit_distance_no_spaces'] = compute_edit_distance(t1_normalized, t2_normalized)

    if 'rouge' in metrics and HAS_ROUGE:
        rouge_scores = compute_rouge(text1, text2)
        results.update(rouge_scores)

    if 'bleu' in metrics and HAS_SACREBLEU:
        results['bleu'] = compute_bleu(text1, text2)

    return results


def generate_comparison_report(
    output_dir: Path,
    pdfalto_dirs: List[Path],
    input_dir: Path,
    metrics: List[str]
) -> Path:
    """Generate comparison CSV report."""
    report_path = output_dir / 'comparison_results.csv'

    version_ids = [d.name for d in pdfalto_dirs]

    # Find all processed text files
    all_txt_files = {}
    for version_id in version_ids:
        version_dir = output_dir / version_id
        if version_dir.exists():
            for txt_file in version_dir.rglob('*.txt'):
                rel_path = txt_file.relative_to(version_dir)
                if rel_path not in all_txt_files:
                    all_txt_files[rel_path] = {}
                all_txt_files[rel_path][version_id] = txt_file

    # Prepare CSV columns
    metric_columns = []
    if 'edit_distance' in metrics:
        metric_columns.append('edit_distance')
    if 'edit_distance_no_spaces' in metrics:
        metric_columns.append('edit_distance_no_spaces')
    if 'rouge' in metrics and HAS_ROUGE:
        metric_columns.extend(['rouge1_f', 'rouge2_f', 'rougeL_f'])
    if 'bleu' in metrics and HAS_SACREBLEU:
        metric_columns.append('bleu')

    # Generate pairwise comparisons
    rows = []
    partial_failures = []  # Track PDFs missing some versions

    for rel_path, versions in sorted(all_txt_files.items()):
        version_list = list(versions.keys())

        # Warn about partial failures (PDF processed by some versions but not all)
        missing_versions = set(version_ids) - set(version_list)
        if missing_versions and len(version_list) > 0:
            partial_failures.append((rel_path, missing_versions))

        for i, v1 in enumerate(version_list):
            for v2 in version_list[i+1:]:
                text1 = read_text_file(versions[v1])
                text2 = read_text_file(versions[v2])

                comparison = compare_texts(text1, text2, metrics)

                row = {
                    'pdf_path': str(rel_path).replace('.txt', '.pdf'),
                    'version_a': v1,
                    'version_b': v2
                }
                row.update(comparison)
                rows.append(row)

    # Report partial failures
    if partial_failures:
        print(f"\nWarning: {len(partial_failures)} PDF(s) had partial processing failures:", file=sys.stderr)
        for rel_path, missing in partial_failures[:10]:  # Show first 10
            print(f"  {rel_path}: missing {', '.join(sorted(missing))}", file=sys.stderr)
        if len(partial_failures) > 10:
            print(f"  ... and {len(partial_failures) - 10} more", file=sys.stderr)

    # Write CSV
    if rows:
        fieldnames = ['pdf_path', 'version_a', 'version_b'] + metric_columns
        with open(report_path, 'w', newline='', encoding='utf-8') as f:
            writer = csv.DictWriter(f, fieldnames=fieldnames)
            writer.writeheader()
            for row in rows:
                writer.writerow({k: row.get(k, '') for k in fieldnames})

        # Print summary to console
        print(f"\n{'='*60}")
        print(f"SUMMARY BY SUBFOLDER")
        print(f"{'='*60}")

        # Group results by subfolder
        from collections import defaultdict
        subfolder_results = defaultdict(list)
        for row in rows:
            pdf_path = Path(row['pdf_path'])
            subfolder = str(pdf_path.parent) if pdf_path.parent != Path('.') else '(root)'
            subfolder_results[subfolder].append(row)

        # Print per-subfolder averages
        for subfolder in sorted(subfolder_results.keys()):
            subfolder_rows = subfolder_results[subfolder]
            print(f"\n{subfolder}/ ({len(subfolder_rows)} comparisons)")
            for metric in metric_columns:
                values = [r.get(metric, 0) for r in subfolder_rows if r.get(metric) is not None]
                if values:
                    avg = sum(values) / len(values)
                    print(f"  {metric}: {avg:.4f}")

        print(f"\n{'='*60}")
        print(f"OVERALL SUMMARY")
        print(f"{'='*60}")
        print(f"Total comparisons: {len(rows)}")

        # Calculate and print averages for each metric
        if metric_columns:
            print(f"\nAverage metrics across all comparisons:")
            for metric in metric_columns:
                values = [row.get(metric, 0) for row in rows if row.get(metric) is not None]
                if values:
                    avg = sum(values) / len(values)
                    print(f"  {metric}: {avg:.4f}")

        # Print detailed results
        print(f"\n{'='*60}")
        print(f"DETAILED RESULTS")
        print(f"{'='*60}")
        for row in rows:
            print(f"\n{row['pdf_path']}")
            print(f"  {row['version_a']} vs {row['version_b']}:")
            for metric in metric_columns:
                if metric in row:
                    print(f"    {metric}: {row[metric]:.4f}")
    else:
        print("\nNo comparisons were generated. Check that:")
        print("  - PDF files were successfully processed")
        print("  - Text files were generated for at least 2 versions")

    return report_path


def main():
    parser = argparse.ArgumentParser(
        description='Compare multiple pdfalto versions on PDF files.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s --input-dir ./pdfs --output-dir ./results --pdfalto-dirs ./pdfalto-v1 ./pdfalto-v2
  %(prog)s --input-dir ./pdfs --output-dir ./results --pdfalto-dirs ./pdfalto-* --n-workers 4
  %(prog)s --input-dir ./pdfs --output-dir ./results --pdfalto-dirs ./builds/* --metrics edit_distance,rouge
"""
    )

    parser.add_argument(
        '--input-dir', '-i',
        type=Path,
        required=True,
        help='Directory containing PDF files (recursive)'
    )

    parser.add_argument(
        '--output-dir', '-o',
        type=Path,
        required=True,
        help='Output directory for results'
    )

    parser.add_argument(
        '--pdfalto-dirs', '-p',
        type=Path,
        nargs='+',
        required=True,
        help='Directories containing pdfalto executables (format: pdfalto-{REV}/pdfalto)'
    )

    parser.add_argument(
        '--xslt-path',
        type=Path,
        default=None,
        help='Path to alto2txt.xsl (default: ../schema/alto2txt.xsl relative to script)'
    )

    parser.add_argument(
        '--metrics', '-m',
        type=str,
        default='edit_distance,edit_distance_no_spaces',
        help='Comma-separated metrics: edit_distance,edit_distance_no_spaces,rouge,bleu'
    )

    parser.add_argument(
        '--n-workers', '-w',
        type=int,
        default=1,
        help='Number of parallel workers (default: 1)'
    )

    parser.add_argument(
        '--no-resume',
        action='store_true',
        help='Disable resume (reprocess all PDFs)'
    )

    args = parser.parse_args()

    # Validate input directory
    if not args.input_dir.exists():
        print(f"Error: Input directory does not exist: {args.input_dir}", file=sys.stderr)
        sys.exit(1)

    # Set default XSLT path
    if args.xslt_path is None:
        script_dir = Path(__file__).parent.resolve()
        args.xslt_path = (script_dir / '..' / 'schema' / 'alto2txt.xsl').resolve()

    # Resolve to absolute path
    args.xslt_path = args.xslt_path.resolve()

    if not args.xslt_path.exists():
        print(f"Error: XSLT file not found: {args.xslt_path}", file=sys.stderr)
        sys.exit(1)

    if not args.xslt_path.is_file():
        print(f"Error: XSLT path is not a file: {args.xslt_path}", file=sys.stderr)
        sys.exit(1)

    # Validate pdfalto directories
    valid_pdfalto_dirs = []
    for pdfalto_dir in args.pdfalto_dirs:
        if pdfalto_dir.exists():
            valid_pdfalto_dirs.append(pdfalto_dir)
        else:
            print(f"Warning: pdfalto directory not found: {pdfalto_dir}", file=sys.stderr)

    if len(valid_pdfalto_dirs) < 2:
        print("Error: At least 2 valid pdfalto directories are required for comparison", file=sys.stderr)
        sys.exit(1)

    # Parse metrics
    metrics = [m.strip() for m in args.metrics.split(',')]

    # Check metric dependencies
    if 'rouge' in metrics and not HAS_ROUGE:
        print("Warning: rouge-score not installed. ROUGE metrics will be skipped.", file=sys.stderr)
    if 'bleu' in metrics and not HAS_SACREBLEU:
        print("Warning: sacrebleu not installed. BLEU metric will be skipped.", file=sys.stderr)

    # Create output directory
    args.output_dir.mkdir(parents=True, exist_ok=True)

    # Find PDFs
    pdfs = find_pdfs(args.input_dir)
    print(f"Found {len(pdfs)} PDF files to process")
    print(f"Using {len(valid_pdfalto_dirs)} pdfalto versions: {[d.name for d in valid_pdfalto_dirs]}")
    print(f"Metrics: {metrics}")
    print(f"Workers: {args.n_workers}")
    print(f"XSLT: {args.xslt_path}")
    if not args.no_resume:
        print("Resume enabled: skipping PDFs with existing text output")

    # Process PDFs
    if args.n_workers == 1:
        # Sequential processing
        for i, pdf_path in enumerate(pdfs, 1):
            rel_path = pdf_path.relative_to(args.input_dir)
            print(f"[{i}/{len(pdfs)}] Processing {rel_path}...")
            process_single_pdf(
                pdf_path,
                args.input_dir,
                args.output_dir,
                valid_pdfalto_dirs,
                args.xslt_path,
                args.no_resume
            )
    else:
        # Parallel processing
        with ProcessPoolExecutor(max_workers=args.n_workers) as executor:
            futures = {
                executor.submit(
                    process_single_pdf,
                    pdf_path,
                    args.input_dir,
                    args.output_dir,
                    valid_pdfalto_dirs,
                    args.xslt_path,
                    args.no_resume
                ): pdf_path
                for pdf_path in pdfs
            }

            for i, future in enumerate(as_completed(futures), 1):
                pdf_path = futures[future]
                rel_path = pdf_path.relative_to(args.input_dir)
                try:
                    future.result()
                    print(f"[{i}/{len(pdfs)}] Completed {rel_path}")
                except Exception as e:
                    print(f"[{i}/{len(pdfs)}] Error processing {rel_path}: {e}", file=sys.stderr)

    # Generate comparison report
    print("\nGenerating comparison report...")
    report_path = generate_comparison_report(
        args.output_dir,
        valid_pdfalto_dirs,
        args.input_dir,
        metrics
    )
    print(f"Comparison report saved to: {report_path}")


if __name__ == '__main__':
    main()
