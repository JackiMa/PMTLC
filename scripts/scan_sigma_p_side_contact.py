#!/usr/bin/env python3
"""
Batch scan for:
  1) sigma = 0,0.1,...,1.0 with p = 0.8
  2) p = 0,0.5,1.0 with sigma in {0,0.1,0.3,1.0}

Each point runs 1e4 optical photons (beamOn 10000) with:
  - greaseThickness = 50 um
  - topAirGap = 0.10 mm
  - sideGap = 0.10 mm
  - pdetMode = 0

Outputs:
  results/side_contact_sigma_p_scan/<timestamp>/
    - scan_results.csv
    - scan_plot.png
    - logs/*.log
    - mac/*.mac
"""

from __future__ import annotations

import argparse
import csv
import datetime as dt
import os
import subprocess
import sys
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor, as_completed
from typing import Dict, List, Tuple


def repo_root() -> Path:
    return Path(__file__).resolve().parents[1]


def timestamp() -> str:
    return dt.datetime.now().strftime("%Y%m%d_%H%M%S")


def fmt_num(x: float) -> str:
    # stable macro formatting
    if abs(x - round(x)) < 1e-12:
        return f"{int(round(x))}"
    return f"{x:.10g}"


def write_mac(
    mac_path: Path,
    sigma_rad: float,
    p: float,
    n: int = 10_000,
    grease_mm: float = 0.05,
    top_gap_mm: float = 0.10,
    side_gap_mm: float = 0.10,
    g4_threads: int = 1,
) -> None:
    mac_path.parent.mkdir(parents=True, exist_ok=True)
    mac_path.write_text(
        "\n".join(
            [
                f"# Auto-generated scan macro",
                f"# sigma_alpha = {fmt_num(sigma_rad)} rad",
                f"# sideContactRatio p = {fmt_num(p)}",
                "",
                "/control/verbose 0",
                "/run/verbose 0",
                "/tracking/verbose 0",
                "/process/optical/verbose 0",
                # NOTE:
                # This build is MT-enabled. We default to 1 thread per process, and use multi-process
                # parallelism (jobs) to utilize many CPU cores without file-contention headaches.
                f"/run/numberOfThreads {int(g4_threads)}",
                "",
                "/SiPINLC/sipin/pdetMode 0",
                "",
                f"/SiPINLC/geometry/greaseThickness {fmt_num(grease_mm)} mm",
                f"/SiPINLC/geometry/topAirGap {fmt_num(top_gap_mm)} mm",
                f"/SiPINLC/geometry/sideGap {fmt_num(side_gap_mm)} mm",
                "",
                f"/SiPINLC/geometry/crystalSigmaAlpha {fmt_num(sigma_rad)} rad",
                f"/SiPINLC/geometry/sideContactRatio {fmt_num(p)}",
                "",
                "/run/initialize",
                f"/run/beamOn {n}",
                "",
            ]
        )
    )


def parse_run_data_csv(run_data_path: Path) -> Dict[str, str]:
    if not run_data_path.exists():
        raise RuntimeError(f"run_data.csv not found at: {run_data_path}")

    with run_data_path.open("r", newline="") as f:
        rows = list(csv.DictReader(f))
    if not rows:
        raise RuntimeError(f"run_data.csv has no data rows: {run_data_path}")

    # We remove run_data.csv before each run, so there should be exactly 1 row.
    # Still, to be robust, take the last row.
    return rows[-1]


def run_one_point(
    exe: Path,
    work_dir: Path,
    mac_path: Path,
    log_path: Path,
) -> Dict[str, str]:
    work_dir.mkdir(parents=True, exist_ok=True)
    run_data_path = work_dir / "run_data.csv"
    if run_data_path.exists():
        run_data_path.unlink()

    log_path.parent.mkdir(parents=True, exist_ok=True)
    with log_path.open("w") as logf:
        proc = subprocess.run(
            [str(exe), "-m", str(mac_path)],
            cwd=str(work_dir),
            stdout=logf,
            stderr=subprocess.STDOUT,
            check=False,
            env=os.environ.copy(),
        )

    if proc.returncode != 0:
        raise RuntimeError(
            f"Run failed (exit={proc.returncode}). See log: {log_path}"
        )

    return parse_run_data_csv(run_data_path)


def to_float(d: Dict[str, str], key: str) -> float:
    v = d.get(key, "")
    try:
        return float(v)
    except Exception as e:
        raise RuntimeError(f"Cannot parse float for key '{key}': '{v}'") from e


def main() -> int:
    ap = argparse.ArgumentParser(description="Scan sigma_alpha and sideContactRatio p (each point: 1e4 photons).")
    ap.add_argument(
        "--out",
        type=str,
        default="",
        help="Output directory. If omitted, a new timestamped folder under results/side_contact_sigma_p_scan/ is created.",
    )
    ap.add_argument(
        "--resume",
        action="store_true",
        help="Resume from an existing output directory created by a previous run (skips points already in scan_results.csv).",
    )
    ap.add_argument(
        "--mode",
        type=str,
        choices=["full", "sigma_sweep", "p_sweep"],
        default="full",
        help=(
            "Which scan to run. "
            "'full' runs both scans (23 points). "
            "'sigma_sweep' runs sigma=0..1 step 0.1 at fixed p. "
            "'p_sweep' runs p=0,0.5,1 at sigma in {0,0.1,0.3,1}."
        ),
    )
    ap.add_argument(
        "--p",
        type=float,
        default=0.8,
        help="Fixed sideContactRatio p for sigma_sweep mode.",
    )
    ap.add_argument(
        "--jobs",
        type=int,
        default=1,
        help="Number of parallel Geant4 processes to run (multi-process parallelism).",
    )
    ap.add_argument(
        "--g4threads",
        type=int,
        default=1,
        help="Geant4 threads per process (/run/numberOfThreads). Default 1; use jobs to utilize many cores safely.",
    )
    ap.add_argument(
        "--rebuild",
        action="store_true",
        help="Do not run Geant4. Rebuild scan_results.csv by scanning out/work/*/run_data.csv, then (re)make plot.",
    )
    args = ap.parse_args()

    root = repo_root()
    build_dir = root / "build"
    exe = build_dir / "SiPINLC"

    if not exe.exists():
        print(f"ERROR: executable not found: {exe}", file=sys.stderr)
        return 2

    if args.out:
        out_dir = Path(args.out).expanduser()
        if not out_dir.is_absolute():
            out_dir = (root / out_dir).resolve()
        out_dir.mkdir(parents=True, exist_ok=True)
    else:
        out_dir = root / "results" / "side_contact_sigma_p_scan" / timestamp()
        out_dir.mkdir(parents=True, exist_ok=True)
    mac_dir = out_dir / "mac"
    log_dir = out_dir / "logs"

    csv_path = out_dir / "scan_results.csv"
    done_keys = set()
    if args.resume and csv_path.exists():
        with csv_path.open("r", newline="") as f:
            for row in csv.DictReader(f):
                try:
                    # Only treat successful rows as done (so failed/missing rows can be re-run).
                    if str(row.get("status", "ok")) != "ok":
                        continue
                    k = (row["group"], float(row["sigma_rad"]), float(row["p"]))
                    done_keys.add(k)
                except Exception:
                    continue

    # Scan definitions
    scan_points: List[Tuple[float, float, str]] = []
    if args.mode == "sigma_sweep":
        # sigma sweep at fixed p
        for i in range(0, 11):
            sigma = 0.1 * i
            scan_points.append((sigma, float(args.p), f"sigma_sweep_p{fmt_num(float(args.p))}"))
    elif args.mode == "p_sweep":
        for sigma in (0.0, 0.1, 0.3, 1.0):
            for p in (0.0, 0.5, 1.0):
                scan_points.append((sigma, p, "p_sweep_selected_sigma"))
    else:
        # (1) sigma sweep at fixed p=0.8
        for i in range(0, 11):
            sigma = 0.1 * i
            scan_points.append((sigma, 0.8, "sigma_sweep_p0p8"))
        # (2) p sweep at selected sigmas
        for sigma in (0.0, 0.1, 0.3, 1.0):
            for p in (0.0, 0.5, 1.0):
                scan_points.append((sigma, p, "p_sweep_selected_sigma"))

    results: List[Dict[str, object]] = []
    if args.resume and csv_path.exists():
        # Load existing results into memory (so we can append and re-write).
        with csv_path.open("r", newline="") as f:
            for row in csv.DictReader(f):
                results.append({k: (float(v) if k in ("idx", "sigma_rad", "p",
                                                     "Photon_Hit_si", "lightCollectionEfficiency",
                                                     "Escaped_CrystalAbsorbed", "Escaped_TopAir",
                                                     "Escaped_SideAir", "Escaped_PTFE", "Escaped_Other",
                                                     "Mean_IncidenceAngle_deg", "Mean_HitCount") and v != "" else v)
                                for k, v in row.items()})

    def _write_checkpoint_csv() -> None:
        if not results:
            return
        # Deterministic ordering for CSV/plot
        def _idx_key(r: Dict[str, object]) -> int:
            try:
                return int(float(r.get("idx", 10**9)))
            except Exception:
                return 10**9
        results.sort(key=_idx_key)
        with csv_path.open("w", newline="") as f:
            fieldnames = list(results[0].keys())
            w = csv.DictWriter(f, fieldnames=fieldnames)
            w.writeheader()
            for r in results:
                w.writerow(r)

    def _parse_tag(tag: str) -> Tuple[int, str, float, float]:
        # tag format: "{idx:02d}_{group}_sigma{sigma}_p{p}"
        # group may contain underscores, so locate "_sigma" and "_p".
        first_us = tag.find("_")
        if first_us <= 0:
            raise ValueError(f"Bad tag: {tag}")
        idx = int(tag[:first_us])
        i_sigma = tag.rfind("_sigma")
        i_p = tag.rfind("_p")
        if i_sigma < 0 or i_p < 0 or i_p <= i_sigma:
            raise ValueError(f"Bad tag: {tag}")
        group = tag[first_us + 1 : i_sigma]
        sigma_s = tag[i_sigma + len("_sigma") : i_p]
        p_s = tag[i_p + len("_p") :]
        return idx, group, float(sigma_s), float(p_s)

    if args.rebuild:
        work_root = out_dir / "work"
        if not work_root.exists():
            print(f"ERROR: no work directory to rebuild from: {work_root}", file=sys.stderr)
            return 2
        results = []
        for d in sorted(work_root.iterdir()):
            if not d.is_dir():
                continue
            tag = d.name
            try:
                idx, group, sigma, p = _parse_tag(tag)
            except Exception:
                continue
            rd = d / "run_data.csv"
            if not rd.exists():
                results.append(
                    {
                        "idx": idx,
                        "group": group,
                        "sigma_rad": sigma,
                        "p": p,
                        "runID": "",
                        "Photon_Hit_si": "",
                        "lightCollectionEfficiency": "",
                        "Escaped_CrystalAbsorbed": "",
                        "Escaped_TopAir": "",
                        "Escaped_SideAir": "",
                        "Escaped_PTFE": "",
                        "Escaped_Other": "",
                        "Mean_IncidenceAngle_deg": "",
                        "Mean_HitCount": "",
                        "log": f"logs/{tag}.log",
                        "mac": f"mac/{tag}.mac",
                        "status": "failed",
                        "error": "missing run_data.csv",
                    }
                )
                continue
            row = parse_run_data_csv(rd)
            results.append(
                {
                    "idx": idx,
                    "group": group,
                    "sigma_rad": sigma,
                    "p": p,
                    "runID": row.get("runID", ""),
                    "Photon_Hit_si": to_float(row, "Photon Hit si"),
                    "lightCollectionEfficiency": to_float(row, "lightCollectionEfficiency"),
                    "Escaped_CrystalAbsorbed": to_float(row, "Escaped_CrystalAbsorbed"),
                    "Escaped_TopAir": to_float(row, "Escaped_TopAir"),
                    "Escaped_SideAir": to_float(row, "Escaped_SideAir"),
                    "Escaped_PTFE": to_float(row, "Escaped_PTFE"),
                    "Escaped_Other": to_float(row, "Escaped_Other"),
                    "Mean_IncidenceAngle_deg": to_float(row, "Mean_IncidenceAngle_deg"),
                    "Mean_HitCount": to_float(row, "Mean_HitCount"),
                    "log": f"logs/{tag}.log",
                    "mac": f"mac/{tag}.mac",
                    "status": "ok",
                    "error": "",
                }
            )
        _write_checkpoint_csv()
        # Proceed to plot section below.

    # Prepare tasks (skip done)
    tasks: List[Tuple[int, float, float, str]] = []
    for idx, (sigma, p, group) in enumerate(scan_points):
        if (group, sigma, p) in done_keys:
            continue
        tasks.append((idx, sigma, p, group))

    def _run_task(t: Tuple[int, float, float, str]) -> Tuple[int, float, float, str, Dict[str, str], Path, Path]:
        idx, sigma, p, group = t
        tag = f"{idx:02d}_{group}_sigma{fmt_num(sigma)}_p{fmt_num(p)}"
        mac_path = mac_dir / f"{tag}.mac"
        log_path = log_dir / f"{tag}.log"
        work_dir = out_dir / "work" / tag
        write_mac(mac_path, sigma_rad=sigma, p=p, n=10_000, g4_threads=int(args.g4threads))
        row = run_one_point(exe=exe, work_dir=work_dir, mac_path=mac_path, log_path=log_path)
        return idx, sigma, p, group, row, log_path, mac_path

    if tasks and not args.rebuild:
        jobs = max(1, int(args.jobs))
        with ThreadPoolExecutor(max_workers=jobs) as ex:
            futs = [ex.submit(_run_task, t) for t in tasks]
            for fut in as_completed(futs):
                try:
                    idx, sigma, p, group, row, log_path, mac_path = fut.result()
                    results.append(
                        {
                            "idx": idx,
                            "group": group,
                            "sigma_rad": sigma,
                            "p": p,
                            "runID": row.get("runID", ""),
                            "Photon_Hit_si": to_float(row, "Photon Hit si"),
                            "lightCollectionEfficiency": to_float(row, "lightCollectionEfficiency"),
                            "Escaped_CrystalAbsorbed": to_float(row, "Escaped_CrystalAbsorbed"),
                            "Escaped_TopAir": to_float(row, "Escaped_TopAir"),
                            "Escaped_SideAir": to_float(row, "Escaped_SideAir"),
                            "Escaped_PTFE": to_float(row, "Escaped_PTFE"),
                            "Escaped_Other": to_float(row, "Escaped_Other"),
                            "Mean_IncidenceAngle_deg": to_float(row, "Mean_IncidenceAngle_deg"),
                            "Mean_HitCount": to_float(row, "Mean_HitCount"),
                            "log": str(log_path.relative_to(out_dir)),
                            "mac": str(mac_path.relative_to(out_dir)),
                            "status": "ok",
                            "error": "",
                        }
                    )
                    print(
                        f"[done] idx={idx:02d} sigma={sigma:.3g} rad, p={p:.3g} -> LCE={to_float(row,'lightCollectionEfficiency'):.6g}"
                    )
                except Exception as e:
                    # Best-effort: keep going, but record failure.
                    results.append(
                        {
                            "idx": "",
                            "group": "",
                            "sigma_rad": "",
                            "p": "",
                            "runID": "",
                            "Photon_Hit_si": "",
                            "lightCollectionEfficiency": "",
                            "Escaped_CrystalAbsorbed": "",
                            "Escaped_TopAir": "",
                            "Escaped_SideAir": "",
                            "Escaped_PTFE": "",
                            "Escaped_Other": "",
                            "Mean_IncidenceAngle_deg": "",
                            "Mean_HitCount": "",
                            "log": "",
                            "mac": "",
                            "status": "failed",
                            "error": repr(e),
                        }
                    )
                    print(f"[failed] {e}", file=sys.stderr)

                _write_checkpoint_csv()

    # Plot
    try:
        import matplotlib

        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except Exception as e:
        print(f"WARNING: matplotlib not available, skipping plot: {e}", file=sys.stderr)
        print(f"Results CSV saved to: {csv_path}")
        return 0

    if args.mode == "sigma_sweep":
        group_name = f"sigma_sweep_p{fmt_num(float(args.p))}"
        sigma_sweep = [r for r in results if r["group"] == group_name]
        sigma_sweep.sort(key=lambda r: float(r["sigma_rad"]))
        fig, ax = plt.subplots(1, 1, figsize=(6.8, 4.2), dpi=160)
        ax.plot(
            [float(r["sigma_rad"]) for r in sigma_sweep],
            [float(r["lightCollectionEfficiency"]) for r in sigma_sweep],
            marker="o",
            linewidth=1.8,
        )
        ax.set_title(f"LCE vs sigma (p = {float(args.p):g})")
        ax.set_xlabel("sigma_alpha [rad]")
        ax.set_ylabel("lightCollectionEfficiency")
        ax.grid(True, alpha=0.25)
        fig.tight_layout()
    elif args.mode == "p_sweep":
        p_sweep = [r for r in results if r["group"] == "p_sweep_selected_sigma" and str(r.get("status", "ok")) == "ok"]
        sigmas_sel = sorted({float(r["sigma_rad"]) for r in p_sweep})
        fig, ax = plt.subplots(1, 1, figsize=(6.8, 4.2), dpi=160)
        for s in sigmas_sel:
            series = [r for r in p_sweep if abs(float(r["sigma_rad"]) - s) < 1e-12]
            series.sort(key=lambda r: float(r["p"]))
            ax.plot(
                [float(r["p"]) for r in series],
                [float(r["lightCollectionEfficiency"]) for r in series],
                marker="o",
                linewidth=1.6,
                label=f"sigma={s:g} rad",
            )
        ax.set_title("LCE vs p (selected sigma)")
        ax.set_xlabel("sideContactRatio p")
        ax.set_ylabel("lightCollectionEfficiency")
        ax.grid(True, alpha=0.25)
        ax.legend(frameon=False, fontsize=9)
        fig.tight_layout()
    else:
        # Prepare series
        sigma_sweep = [r for r in results if r["group"] == "sigma_sweep_p0p8" and abs(float(r["p"]) - 0.8) < 1e-12]
        sigma_sweep.sort(key=lambda r: float(r["sigma_rad"]))

        p_sweep = [r for r in results if r["group"] == "p_sweep_selected_sigma"]
        # group by sigma
        sigmas_sel = sorted({float(r["sigma_rad"]) for r in p_sweep})

        fig, axes = plt.subplots(1, 2, figsize=(11, 4.2), dpi=160)

        ax = axes[0]
        ax.plot(
            [float(r["sigma_rad"]) for r in sigma_sweep],
            [float(r["lightCollectionEfficiency"]) for r in sigma_sweep],
            marker="o",
            linewidth=1.8,
        )
        ax.set_title("LCE vs sigma (p = 0.8)")
        ax.set_xlabel("sigma_alpha [rad]")
        ax.set_ylabel("lightCollectionEfficiency")
        ax.grid(True, alpha=0.25)

        ax = axes[1]
        for s in sigmas_sel:
            series = [r for r in p_sweep if abs(float(r["sigma_rad"]) - s) < 1e-12]
            series.sort(key=lambda r: float(r["p"]))
            ax.plot(
                [float(r["p"]) for r in series],
                [float(r["lightCollectionEfficiency"]) for r in series],
                marker="o",
                linewidth=1.6,
                label=f"sigma={s:g} rad",
            )
        ax.set_title("LCE vs p (selected sigma)")
        ax.set_xlabel("sideContactRatio p")
        ax.set_ylabel("lightCollectionEfficiency")
        ax.grid(True, alpha=0.25)
        ax.legend(frameon=False, fontsize=9)

        fig.tight_layout()
    plot_path = out_dir / "scan_plot.png"
    fig.savefig(plot_path)
    plt.close(fig)

    print(f"\nDone.\n- Results: {csv_path}\n- Plot: {plot_path}\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())


