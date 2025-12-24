#!/usr/bin/env python3
"""
Plot parameter scan results from SiPINLC simulations.
"""

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import os

plt.rcParams['font.family'] = ['DejaVu Sans', 'sans-serif']
plt.rcParams['font.size'] = 11
plt.rcParams['figure.figsize'] = (12, 10)
plt.rcParams['axes.grid'] = True
plt.rcParams['grid.alpha'] = 0.3

RESULTS_DIR = '/home/wsl2/myGeant4/SiPINLC/results'
OUTPUT_DIR = '/home/wsl2/myGeant4/SiPINLC/docs/figures'
os.makedirs(OUTPUT_DIR, exist_ok=True)

def load_csv(filename):
    filepath = os.path.join(RESULTS_DIR, filename)
    if os.path.exists(filepath):
        return pd.read_csv(filepath)
    return None

# Create figure with 2x2 subplots
fig, axes = plt.subplots(2, 2, figsize=(14, 11))

# === Scan 1: Grease Thickness ===
ax1 = axes[0, 0]
df1 = load_csv('scan_grease_thickness.csv')
if df1 is not None:
    ax1.plot(df1['grease_um'], df1['eps_col'] * 100, 'o-', 
             color='#2E86AB', markersize=10, linewidth=2, label='eps_col')
    ax1.set_xlabel('Optical Grease Thickness (um)', fontsize=12)
    ax1.set_ylabel('Light Collection Efficiency (%)', fontsize=12)
    ax1.set_title('Effect of Grease Layer Thickness', fontsize=14, fontweight='bold')
    ax1.set_ylim([70, 95])
    ax1.axhline(y=86, color='gray', linestyle='--', alpha=0.5, label='~86% plateau')
    ax1.legend()
    
    # Add annotation
    ax1.annotate('Grease bridges\nrefractive index gap', 
                 xy=(20, 86.4), xytext=(60, 78),
                 arrowprops=dict(arrowstyle='->', color='gray'),
                 fontsize=10, color='gray')

# === Scan 2: Top Air Gap ===
ax2 = axes[0, 1]
df2 = load_csv('scan_top_airgap.csv')
if df2 is not None:
    ax2.plot(df2['top_airgap_um'], df2['eps_col'] * 100, 's-', 
             color='#E94F37', markersize=10, linewidth=2)
    ax2.set_xlabel('Top Air Gap Thickness (um)', fontsize=12)
    ax2.set_ylabel('Light Collection Efficiency (%)', fontsize=12)
    ax2.set_title('Effect of Top Air Gap', fontsize=14, fontweight='bold')
    ax2.set_ylim([80, 92])
    
    # Add annotation
    ax2.annotate('Minimal impact\n(TIR dominates)', 
                 xy=(200, 85.7), xytext=(300, 88),
                 arrowprops=dict(arrowstyle='->', color='gray'),
                 fontsize=10, color='gray')

# === Scan 3: Crystal Surface Roughness ===
ax3 = axes[1, 0]
df3 = load_csv('scan_sigma_alpha.csv')
if df3 is not None:
    ax3.plot(df3['sigma_alpha_rad'], df3['eps_col'] * 100, '^-', 
             color='#4CAF50', markersize=10, linewidth=2)
    ax3.set_xlabel('Crystal Surface Roughness sigma_alpha (rad)', fontsize=12)
    ax3.set_ylabel('Light Collection Efficiency (%)', fontsize=12)
    ax3.set_title('Effect of Crystal Surface Roughness', fontsize=14, fontweight='bold')
    ax3.set_ylim([40, 95])
    
    # Add annotation for light trapping
    ax3.annotate('Light trapping!\n(specular reflection)', 
                 xy=(0, 48.9), xytext=(0.15, 55),
                 arrowprops=dict(arrowstyle='->', color='red'),
                 fontsize=10, color='red')
    ax3.annotate('Roughness breaks TIR,\nlight escapes', 
                 xy=(0.5, 88.7), xytext=(0.25, 93),
                 arrowprops=dict(arrowstyle='->', color='green'),
                 fontsize=10, color='green')

# === Scan 4: Side Gap ===
ax4 = axes[1, 1]
df4 = load_csv('scan_side_gap.csv')
if df4 is not None:
    ax4.plot(df4['side_gap_um'], df4['eps_col'] * 100, 'd-', 
             color='#9B59B6', markersize=10, linewidth=2)
    ax4.set_xlabel('Side Air Gap Thickness (um)', fontsize=12)
    ax4.set_ylabel('Light Collection Efficiency (%)', fontsize=12)
    ax4.set_title('Effect of Side Air Gap', fontsize=14, fontweight='bold')
    ax4.set_ylim([80, 92])
    
    # Add annotation
    ax4.annotate('Larger gap -> more TIR before PTFE', 
                 xy=(300, 87), xytext=(100, 89),
                 arrowprops=dict(arrowstyle='->', color='gray'),
                 fontsize=10, color='gray')

plt.suptitle('SiPINLC Parameter Sensitivity Study\n(10000 photons per point, 5x5x5mm GAGG)', 
             fontsize=16, fontweight='bold', y=0.98)
plt.tight_layout(rect=[0, 0, 1, 0.95])

# Save figures
for ext in ['png', 'pdf']:
    output_path = os.path.join(OUTPUT_DIR, f'parameter_scan.{ext}')
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    print(f'Saved: {output_path}')

plt.close()
print('Done!')

