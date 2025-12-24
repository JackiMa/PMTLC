#!/usr/bin/env python3
"""
Plot 10x10 p × sigma_alpha scan results:
- eps_col heatmap
- loss-channel heatmaps (CrystalAbsorbed, PTFE, Grease, SideAir, World)
- representative slices: eps_col vs p for selected sigma

All plots saved to results/p_sigma_grid_10x10/
"""

from __future__ import annotations

import os
from pathlib import Path

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt


ROOT = Path("/home/wsl2/myGeant4/SiPINLC")
OUTDIR = ROOT / "results" / "p_sigma_grid_10x10"
CSV = OUTDIR / "p_sigma_grid_10x10.csv"


def heatmap(df: pd.DataFrame, value_col: str, title: str, cmap: str, out_png: Path, scale: float = 1.0, fmt: str | None = None):
    p_vals = sorted(df["p"].unique())
    s_vals = sorted(df["sigma_rad"].unique())

    mat = np.full((len(s_vals), len(p_vals)), np.nan, dtype=float)
    for i, s in enumerate(s_vals):
        for j, p in enumerate(p_vals):
            row = df[(df["p"] == p) & (df["sigma_rad"] == s)]
            if len(row) == 1:
                mat[i, j] = float(row[value_col].iloc[0]) * scale

    fig, ax = plt.subplots(figsize=(10, 6))
    im = ax.imshow(mat, origin="lower", aspect="auto", cmap=cmap)
    ax.set_xticks(range(len(p_vals)))
    ax.set_xticklabels([f"{p:.1f}" for p in p_vals])
    ax.set_yticks(range(len(s_vals)))
    ax.set_yticklabels([f"{s:.2f}" for s in s_vals])
    ax.set_xlabel("p (side/top contact ratio)")
    ax.set_ylabel("sigma_alpha (rad)")
    ax.set_title(title)
    cbar = plt.colorbar(im, ax=ax)
    cbar.set_label(value_col)

    if fmt is not None:
        for i in range(len(s_vals)):
            for j in range(len(p_vals)):
                if np.isfinite(mat[i, j]):
                    ax.text(j, i, format(mat[i, j], fmt), ha="center", va="center", fontsize=7, color="black")

    fig.tight_layout()
    fig.savefig(out_png, dpi=180)
    plt.close(fig)


def main():
    if not CSV.exists():
        raise SystemExit(f"Missing CSV: {CSV}")

    df = pd.read_csv(CSV)

    # Convert numeric columns (in case of stray spaces)
    for col in ["p", "sigma_rad", "wallTime_s", "eps_col", "Esc_CrystalAbsorbed", "Esc_TopAir", "Esc_SideAir", "Esc_PTFE", "Esc_Grease", "Esc_World", "PhotonHitSi", "ScintPhotonCount", "CherenkovPhotonCount"]:
        if col in df.columns:
            df[col] = pd.to_numeric(df[col], errors="coerce")

    # Loss fractions:
    # NOTE: In our current runs Scint/Cherenkov counts can be 0 (debug optical-photon primary),
    # so we must NOT use (Scint+Cherenkov) as denominator.
    # Use accounted photons per run:
    #   N_accounted = PhotonHitSi + sum(loss channels)
    loss_cols = ["Esc_CrystalAbsorbed", "Esc_TopAir", "Esc_SideAir", "Esc_PTFE", "Esc_Grease", "Esc_World"]
    df["N_accounted"] = df["PhotonHitSi"]
    for ch in loss_cols:
        df["N_accounted"] = df["N_accounted"] + df[ch]

    # If classification misses some photons, keep track of the residual vs 1000 events.
    # (We still compute percentages vs N_accounted to avoid divide-by-zero.)
    df["N_target"] = 1000.0
    df["N_unaccounted"] = df["N_target"] - df["N_accounted"]

    df["eps_pct"] = df["eps_col"] * 100.0
    for ch in loss_cols:
        df[ch + "_frac"] = df[ch] / df["N_accounted"].clip(lower=1.0)
        df[ch + "_pct"] = df[ch + "_frac"] * 100.0
    df["Unaccounted_pct"] = df["N_unaccounted"] / df["N_accounted"].clip(lower=1.0) * 100.0

    OUTDIR.mkdir(parents=True, exist_ok=True)

    # Heatmaps
    heatmap(df, "eps_col", "Light collection efficiency ε_col", "RdYlGn", OUTDIR / "heatmap_eps_col.png", scale=100.0, fmt=".1f")

    heatmap(df, "Esc_CrystalAbsorbed_pct", "Loss: crystal self-absorption (%)", "YlOrRd", OUTDIR / "heatmap_loss_crystal_abs_pct.png", scale=1.0, fmt=".1f")
    heatmap(df, "Esc_PTFE_pct", "Loss: PTFE absorption (%)", "YlOrRd", OUTDIR / "heatmap_loss_ptfe_pct.png", scale=1.0, fmt=".1f")
    heatmap(df, "Esc_Grease_pct", "Loss: grease/bottom-gap (%)", "YlOrRd", OUTDIR / "heatmap_loss_grease_pct.png", scale=1.0, fmt=".1f")
    heatmap(df, "Esc_SideAir_pct", "Loss: side air escape (%)", "YlOrRd", OUTDIR / "heatmap_loss_sideair_pct.png", scale=1.0, fmt=".1f")
    heatmap(df, "Esc_World_pct", "Loss: world escape (%) (should be ~0)", "YlOrRd", OUTDIR / "heatmap_loss_world_pct.png", scale=1.0, fmt=".2f")
    heatmap(df, "Unaccounted_pct", "Unaccounted (%) = (1000 - accounted)/accounted", "PuBuGn", OUTDIR / "heatmap_unaccounted_pct.png", scale=1.0, fmt=".1f")

    # Slices: eps vs p for selected sigma
    sigmas = sorted(df["sigma_rad"].unique())
    pick = []
    for target in [0.0, 0.05, 0.1, 0.2, 0.5]:
        # nearest
        s = min(sigmas, key=lambda x: abs(x - target))
        if s not in pick:
            pick.append(s)

    fig, ax = plt.subplots(figsize=(8, 5))
    for s in pick:
        sub = df[df["sigma_rad"] == s].sort_values("p")
        ax.plot(sub["p"], sub["eps_pct"], marker="o", label=f"sigma={s:.2f}")
    ax.set_xlabel("p")
    ax.set_ylabel("ε_col (%)")
    ax.set_title("ε_col vs p (selected sigma)")
    ax.grid(True, alpha=0.3)
    ax.legend()
    fig.tight_layout()
    fig.savefig(OUTDIR / "slice_eps_vs_p.png", dpi=180)
    plt.close(fig)

    # Best point summary
    best = df.loc[df["eps_col"].idxmax()]
    with open(OUTDIR / "best_point.txt", "w", encoding="utf-8") as f:
        f.write("Best point (by eps_col):\n")
        f.write(best.to_string() + "\n")

    print(f"Saved plots to {OUTDIR}")
    print("Best point:")
    print(best[["p", "sigma_rad", "eps_col", "Esc_CrystalAbsorbed", "Esc_PTFE", "Esc_Grease", "Esc_World", "wallTime_s"]])


if __name__ == "__main__":
    main()


