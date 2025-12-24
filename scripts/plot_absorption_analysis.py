#!/usr/bin/env python3
"""
Plot self-absorption analysis: effect of surface roughness on light trapping.
Key insight for paper: roughness breaks TIR, reduces self-absorption.
"""

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import os

RESULTS_DIR = '/home/wsl2/myGeant4/SiPINLC/results'
OUTPUT_DIR = '/home/wsl2/myGeant4/SiPINLC/docs/figures'
os.makedirs(OUTPUT_DIR, exist_ok=True)

plt.rcParams['font.family'] = ['DejaVu Sans', 'sans-serif']
plt.rcParams['font.size'] = 11

# Load data
df = pd.read_csv(os.path.join(RESULTS_DIR, 'scan_absorption.csv'))

# Create figure with dual y-axis
fig, ax1 = plt.subplots(figsize=(10, 7))

# Plot light collection efficiency
color1 = '#2E86AB'
ax1.set_xlabel('Crystal Surface Roughness (sigma_alpha, rad)', fontsize=12)
ax1.set_ylabel('Light Collection Efficiency (%)', fontsize=12, color=color1)
line1, = ax1.plot(df['abs_scale'], df['eps_col'] * 100, 'o-', 
                   color=color1, markersize=10, linewidth=2, label='ε_col')
ax1.tick_params(axis='y', labelcolor=color1)
ax1.set_ylim([40, 95])

# Create second y-axis for absorbed photons
ax2 = ax1.twinx()
color2 = '#E94F37'
ax2.set_ylabel('Photons Absorbed in Crystal', fontsize=12, color=color2)
line2, = ax2.plot(df['abs_scale'], df['absorbed'], 's-', 
                   color=color2, markersize=10, linewidth=2, label='Absorbed')
ax2.tick_params(axis='y', labelcolor=color2)
ax2.set_ylim([0, 5000])

# Add annotations
ax1.annotate('Light trapping regime\n(σ_α = 0, specular reflection)', 
             xy=(0, 48.9), xytext=(0.15, 55),
             arrowprops=dict(arrowstyle='->', color='gray'),
             fontsize=10, color='gray')

ax1.annotate('Normal operation\n(σ_α ≈ 0.1 rad typical)', 
             xy=(0.1, 85.8), xytext=(0.25, 75),
             arrowprops=dict(arrowstyle='->', color='green'),
             fontsize=10, color='green')

# Add legend
lines = [line1, line2]
labels = ['Light collection efficiency', 'Crystal self-absorption']
ax1.legend(lines, labels, loc='right', fontsize=11)

ax1.set_title('Effect of Surface Roughness on Light Collection\n' + 
              '(5x5x5mm GAGG, 10000 events, 50μm grease coupling)', 
              fontsize=14, fontweight='bold')
ax1.grid(True, alpha=0.3)

plt.tight_layout()

# Save
for ext in ['png', 'pdf']:
    output_path = os.path.join(OUTPUT_DIR, f'absorption_vs_roughness.{ext}')
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    print(f'Saved: {output_path}')

plt.close()
print('Done!')

