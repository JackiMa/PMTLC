#!/usr/bin/env python3
"""
Plot sanity check results: Theory vs Simulation
Run: python3 scripts/plot_sanity_check.py
"""
import matplotlib
matplotlib.use('Agg')  # Non-interactive backend
import matplotlib.pyplot as plt
import numpy as np
import os

# Create output directory
os.makedirs('/home/wsl2/myGeant4/SiPINLC/docs/figures', exist_ok=True)

# Data from experiments (10000 photons)
scenarios = ['A-Air', 'A-Grease', 'B-Air', 'B-Grease', 'C-Air', 'C-Grease']
theory = [0.157, 0.381, 1.0, 1.0, 0.698, 0.832]
simulation = [0.284, 0.481, 0.821, 0.911, 0.583, 0.776]
absorbed = [6174, 4092, 1760, 709, 4157, 2074]  # Crystal absorption counts

x = np.arange(len(scenarios))
width = 0.35

# === Figure 1: Theory vs Simulation bar chart ===
fig, ax = plt.subplots(figsize=(12, 6))

bars1 = ax.bar(x - width/2, theory, width, label='Theory (Analytic)', color='steelblue', edgecolor='navy', alpha=0.8)
bars2 = ax.bar(x + width/2, simulation, width, label='Geant4 Simulation', color='coral', edgecolor='darkred', alpha=0.8)

ax.set_ylabel('Light Collection Efficiency (ε_col)', fontsize=12)
ax.set_xlabel('Scenario', fontsize=12)
ax.set_title('Sanity Check: Theory vs Geant4 Simulation\n(GAGG 5×5×5 mm³, 10000 photons, original absorption)', fontsize=13)
ax.set_xticks(x)
ax.set_xticklabels(scenarios, fontsize=11)
ax.legend(fontsize=11, loc='upper left')
ax.set_ylim(0, 1.15)
ax.axhline(y=1.0, color='gray', linestyle='--', alpha=0.5)

# Value labels
for bar, val in zip(bars1, theory):
    ax.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.02, 
            f'{val:.3f}', ha='center', va='bottom', fontsize=9, color='navy')
for bar, val in zip(bars2, simulation):
    ax.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.02, 
            f'{val:.3f}', ha='center', va='bottom', fontsize=9, color='darkred')

ax.yaxis.grid(True, linestyle='--', alpha=0.3)
ax.set_axisbelow(True)

# Legend box
ax.annotate('A: TIR (specular mirror)\nB: Lambertian R=100%\nC: PTFE R=97.5%', 
            xy=(0.98, 0.02), xycoords='axes fraction',
            ha='right', va='bottom', fontsize=9,
            bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5))

plt.tight_layout()
plt.savefig('/home/wsl2/myGeant4/SiPINLC/docs/figures/sanity_check_results.png', dpi=150)
plt.savefig('/home/wsl2/myGeant4/SiPINLC/docs/figures/sanity_check_results.pdf')
plt.close()

# === Figure 2: Absorption analysis ===
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))

# Left: Absorption counts
colors = ['#1f77b4', '#1f77b4', '#2ca02c', '#2ca02c', '#ff7f0e', '#ff7f0e']
ax1.bar(scenarios, absorbed, color=colors, edgecolor='black', alpha=0.7)
ax1.set_ylabel('Crystal Absorbed Photons', fontsize=12)
ax1.set_xlabel('Scenario', fontsize=12)
ax1.set_title('Crystal Self-Absorption by Scenario', fontsize=13)
ax1.yaxis.grid(True, linestyle='--', alpha=0.3)
for i, (s, a) in enumerate(zip(scenarios, absorbed)):
    ax1.text(i, a + 100, str(a), ha='center', fontsize=9)

# Right: Efficiency breakdown
detected = [int(10000 * e) for e in simulation]
lost = [10000 - d - a for d, a in zip(detected, absorbed)]
bottom1 = np.array(detected)
bottom2 = bottom1 + np.array(absorbed)

ax2.bar(scenarios, detected, label='Detected (Si hit)', color='green', alpha=0.7)
ax2.bar(scenarios, absorbed, bottom=bottom1, label='Crystal Absorbed', color='red', alpha=0.7)
ax2.bar(scenarios, lost, bottom=bottom2, label='Other Loss', color='gray', alpha=0.7)
ax2.set_ylabel('Photon Count', fontsize=12)
ax2.set_xlabel('Scenario', fontsize=12)
ax2.set_title('Photon Fate Distribution (10000 total)', fontsize=13)
ax2.legend(loc='upper right', fontsize=10)
ax2.set_ylim(0, 11000)
ax2.yaxis.grid(True, linestyle='--', alpha=0.3)

plt.tight_layout()
plt.savefig('/home/wsl2/myGeant4/SiPINLC/docs/figures/sanity_check_absorption.png', dpi=150)
plt.savefig('/home/wsl2/myGeant4/SiPINLC/docs/figures/sanity_check_absorption.pdf')
plt.close()

print("Figures saved to docs/figures/:")
print("  - sanity_check_results.png/pdf")
print("  - sanity_check_absorption.png/pdf")
