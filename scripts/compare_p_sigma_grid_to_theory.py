#!/usr/bin/env python3
"""
Compare Geant4 10x10 p×sigma grid to closed-form theory bounds (M1/M2).

Outputs:
- results/p_sigma_grid_10x10/p_sigma_grid_10x10_with_theory.csv
- delta/bounds heatmaps saved into same folder
"""

from __future__ import annotations

from pathlib import Path
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt


ROOT = Path("/home/wsl2/myGeant4/SiPINLC")
OUTDIR = ROOT / "results" / "p_sigma_grid_10x10"
CSV_IN = OUTDIR / "p_sigma_grid_10x10.csv"
CSV_OUT = OUTDIR / "p_sigma_grid_10x10_with_theory.csv"


# ===== theory model (copied minimally from scripts/theory_ecol.py) =====
n1 = 1.86
n2_grease = 1.46
R_c = 0.975
p_b = 1 / 6
theta_c = np.arcsin(1 / n1)


def mu_0(n2: float) -> float:
    if n2 >= n1:
        return 0.0
    return float(np.sqrt(1 - (n2 / n1) ** 2))


def s_A(n2: float) -> float:
    return 1 - mu_0(n2)


def s_L(n2: float) -> float:
    return (n2 / n1) ** 2


def q_sigma(sigma_alpha: float) -> float:
    return 1 - float(np.exp(-((sigma_alpha / theta_c) ** 2)))


def e_A() -> float:
    return 1 - float(np.sqrt(1 - (1 / n1) ** 2))


def e_L() -> float:
    return (1 / n1) ** 2


def e_eff(sigma_alpha: float) -> float:
    q = q_sigma(sigma_alpha)
    return (1 - q) * e_A() + q * e_L()


def A_M1(p: float) -> float:
    # air-gap escape fully recovered
    R_eff = 1 - p * (1 - R_c)
    return (1 - p_b) * R_eff


def A_M2(sigma_alpha: float, p: float) -> float:
    # air-gap escape fully lost
    e = e_eff(sigma_alpha)
    survival = p * R_c + (1 - p) * (1 - e)
    return (1 - p_b) * survival


def solve_markov(A: float, n2: float, sigma_alpha: float) -> float:
    q = q_sigma(sigma_alpha)
    sL = s_L(n2)
    sA = s_A(n2)

    a11 = 1 - A * (1 - q + q * sL)
    a12 = -A * q * (1 - sL)
    a21 = -A * q * sL
    a22 = 1 - p_b - A * (1 - q * sL)

    b1 = p_b
    b2 = 0.0

    det = a11 * a22 - a12 * a21
    if abs(det) < 1e-12:
        return 0.0

    P_U = (b1 * a22 - b2 * a12) / det
    P_T = (a11 * b2 - a21 * b1) / det
    eps_col = sA * P_U + (1 - sA) * P_T
    return float(np.clip(eps_col, 0.0, 1.0))


def eps_M1(sigma_alpha: float, p: float, n2: float) -> float:
    return solve_markov(A_M1(p), n2, sigma_alpha)


def eps_M2(sigma_alpha: float, p: float, n2: float) -> float:
    return solve_markov(A_M2(sigma_alpha, p), n2, sigma_alpha)


def heatmap(df: pd.DataFrame, col: str, title: str, out: Path, cmap: str = "coolwarm", vmin=None, vmax=None, fmt=None):
    p_vals = sorted(df["p"].unique())
    s_vals = sorted(df["sigma_rad"].unique())
    mat = np.full((len(s_vals), len(p_vals)), np.nan, dtype=float)
    for i, s in enumerate(s_vals):
        for j, p in enumerate(p_vals):
            row = df[(df["p"] == p) & (df["sigma_rad"] == s)]
            if len(row) == 1:
                mat[i, j] = float(row[col].iloc[0])

    fig, ax = plt.subplots(figsize=(10, 6))
    im = ax.imshow(mat, origin="lower", aspect="auto", cmap=cmap, vmin=vmin, vmax=vmax)
    ax.set_xticks(range(len(p_vals)))
    ax.set_xticklabels([f"{p:.1f}" for p in p_vals])
    ax.set_yticks(range(len(s_vals)))
    ax.set_yticklabels([f"{s:.2f}" for s in s_vals])
    ax.set_xlabel("p")
    ax.set_ylabel("sigma_alpha (rad)")
    ax.set_title(title)
    cbar = plt.colorbar(im, ax=ax)
    cbar.set_label(col)
    if fmt is not None:
        for i in range(len(s_vals)):
            for j in range(len(p_vals)):
                if np.isfinite(mat[i, j]):
                    ax.text(j, i, format(mat[i, j], fmt), ha="center", va="center", fontsize=7, color="black")
    fig.tight_layout()
    fig.savefig(out, dpi=180)
    plt.close(fig)


def main():
    if not CSV_IN.exists():
        raise SystemExit(f"Missing: {CSV_IN}")

    df = pd.read_csv(CSV_IN)
    df["p"] = pd.to_numeric(df["p"], errors="coerce")
    df["sigma_rad"] = pd.to_numeric(df["sigma_rad"], errors="coerce")
    df["eps_col"] = pd.to_numeric(df["eps_col"], errors="coerce")

    # theory (grease bottom medium)
    df["theory_M1"] = [eps_M1(s, p, n2_grease) for s, p in zip(df["sigma_rad"], df["p"])]
    df["theory_M2"] = [eps_M2(s, p, n2_grease) for s, p in zip(df["sigma_rad"], df["p"])]

    df["delta_to_M1"] = df["eps_col"] - df["theory_M1"]
    df["delta_to_M2"] = df["eps_col"] - df["theory_M2"]
    df["within_M2_M1"] = (df["eps_col"] >= df["theory_M2"]) & (df["eps_col"] <= df["theory_M1"])

    df.to_csv(CSV_OUT, index=False)

    # plots
    heatmap(df, "theory_M1", "Theory ε_col (M1 upper bound, grease)", OUTDIR / "heatmap_theory_M1.png", cmap="viridis", vmin=0, vmax=1, fmt=".2f")
    heatmap(df, "theory_M2", "Theory ε_col (M2 lower bound, grease)", OUTDIR / "heatmap_theory_M2.png", cmap="viridis", vmin=0, vmax=1, fmt=".2f")
    heatmap(df, "delta_to_M1", "Δ = ε_col(G4) - ε_col(M1) (grease)", OUTDIR / "heatmap_delta_to_M1.png", cmap="coolwarm", vmin=-0.5, vmax=0.5, fmt=".2f")
    heatmap(df, "delta_to_M2", "Δ = ε_col(G4) - ε_col(M2) (grease)", OUTDIR / "heatmap_delta_to_M2.png", cmap="coolwarm", vmin=-0.5, vmax=0.5, fmt=".2f")

    # boolean map
    df_bool = df.copy()
    df_bool["within_M2_M1_num"] = df_bool["within_M2_M1"].astype(int)
    heatmap(df_bool, "within_M2_M1_num", "Within theory bounds? (1=yes,0=no)", OUTDIR / "heatmap_within_bounds.png", cmap="Greys", vmin=0, vmax=1, fmt=".0f")

    # worst offenders
    worst_hi = df.sort_values("delta_to_M1", ascending=False).head(10)
    worst_lo = df.sort_values("delta_to_M2", ascending=True).head(10)
    txt = OUTDIR / "theory_comparison_summary.txt"
    with open(txt, "w", encoding="utf-8") as f:
        f.write("Theory comparison summary (grease):\n")
        f.write(f"CSV_OUT: {CSV_OUT}\n\n")
        f.write("Top 10 (G4 above M1):\n")
        f.write(worst_hi[["p", "sigma_rad", "eps_col", "theory_M1", "theory_M2", "delta_to_M1", "delta_to_M2"]].to_string(index=False))
        f.write("\n\nTop 10 (G4 below M2):\n")
        f.write(worst_lo[["p", "sigma_rad", "eps_col", "theory_M1", "theory_M2", "delta_to_M1", "delta_to_M2"]].to_string(index=False))

    print(f"Saved: {CSV_OUT}")
    print(f"Plots: {OUTDIR}/heatmap_theory_M1.png etc.")
    print(f"Summary: {txt}")


if __name__ == "__main__":
    main()


