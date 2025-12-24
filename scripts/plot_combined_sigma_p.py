#!/usr/bin/env python3
"""
Combine two scans into one figure:
  - sigma sweep: LCE vs sigma at fixed p=0.8
  - p sweep: LCE vs p for sigma in {0,0.1,0.3,1}

Reads scan_results.csv produced by scripts/scan_sigma_p_side_contact.py.
Outputs:
  results/side_contact_sigma_p_scan/combined_sigma_p0p8.csv
  results/side_contact_sigma_p_scan/combined_sigma_p0p8.png
"""

from __future__ import annotations

from pathlib import Path

import pandas as pd


def repo_root() -> Path:
    return Path(__file__).resolve().parents[1]


def main() -> int:
    root = repo_root()
    sigma_csv = root / "results" / "side_contact_sigma_p_scan" / "sigma_p0p8_jobs24_g4t1" / "scan_results.csv"
    p_csv = root / "results" / "side_contact_sigma_p_scan" / "p_sweep_selected_sigma_jobs24_g4t1" / "scan_results.csv"
    out_dir = root / "results" / "side_contact_sigma_p_scan"
    out_dir.mkdir(parents=True, exist_ok=True)

    df_sigma = pd.read_csv(sigma_csv)
    df_p = pd.read_csv(p_csv)

    if "status" in df_sigma.columns:
        df_sigma = df_sigma[df_sigma["status"].astype(str) == "ok"]
    if "status" in df_p.columns:
        df_p = df_p[df_p["status"].astype(str) == "ok"]

    df_sigma["scan"] = "sigma_sweep_p0p8"
    df_p["scan"] = "p_sweep_selected_sigma"

    df_all = pd.concat([df_sigma, df_p], ignore_index=True)
    out_csv = out_dir / "combined_sigma_p0p8.csv"
    df_all.to_csv(out_csv, index=False)

    # Plot
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    fig, axes = plt.subplots(1, 2, figsize=(11, 4.2), dpi=160)

    # Left: sigma sweep
    ax = axes[0]
    df_sigma = df_sigma.sort_values("sigma_rad")
    ax.plot(df_sigma["sigma_rad"], df_sigma["lightCollectionEfficiency"], marker="o", linewidth=1.8)
    ax.set_title("LCE vs sigma (p = 0.8)")
    ax.set_xlabel("sigma_alpha [rad]")
    ax.set_ylabel("lightCollectionEfficiency")
    ax.grid(True, alpha=0.25)

    # Right: p sweep
    ax = axes[1]
    for s in sorted(df_p["sigma_rad"].unique()):
        d = df_p[df_p["sigma_rad"] == s].sort_values("p")
        ax.plot(d["p"], d["lightCollectionEfficiency"], marker="o", linewidth=1.6, label=f"sigma={s:g} rad")
    ax.set_title("LCE vs p (selected sigma)")
    ax.set_xlabel("sideContactRatio p")
    ax.set_ylabel("lightCollectionEfficiency")
    ax.grid(True, alpha=0.25)
    ax.legend(frameon=False, fontsize=9)

    fig.tight_layout()
    out_png = out_dir / "combined_sigma_p0p8.png"
    fig.savefig(out_png)
    plt.close(fig)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())


