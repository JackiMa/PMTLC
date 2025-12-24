#!/usr/bin/env python3
"""
Extract and plot histograms from SiPINLC ROOT output files.
For paper: incidence angle distribution, hit count, escape channels, path length.
Uses uproot instead of PyROOT for better compatibility.
"""

import uproot
import matplotlib.pyplot as plt
import numpy as np
import os

ROOT_FILE = '/home/wsl2/myGeant4/SiPINLC/build/LYsimulations.root'
OUTPUT_DIR = '/home/wsl2/myGeant4/SiPINLC/docs/figures'
os.makedirs(OUTPUT_DIR, exist_ok=True)

plt.rcParams['font.family'] = ['DejaVu Sans', 'sans-serif']
plt.rcParams['font.size'] = 11

# Open ROOT file
tfile = uproot.open(ROOT_FILE)
print("Available keys:", list(tfile.keys()))

def get_hist_data(tfile, histname):
    """Extract histogram data as (centers, values, edges)."""
    for key in tfile.keys():
        if histname in key:
            h = tfile[key]
            values = h.values()
            edges = h.axis().edges()
            centers = (edges[:-1] + edges[1:]) / 2
            return centers, values, edges
    print(f"Warning: histogram '{histname}' not found")
    return None, None, None

# Create 2x2 figure for paper plots
fig, axes = plt.subplots(2, 2, figsize=(14, 11))

# === Plot 1: Incidence Angle Distribution ===
ax1 = axes[0, 0]
x, y, edges = get_hist_data(tfile, "IncidenceAngle")
if x is not None and np.sum(y) > 0:
    ax1.bar(x, y, width=(edges[1]-edges[0])*0.9, color='#2E86AB', alpha=0.8, edgecolor='none')
    ax1.set_xlabel('Incidence Angle (deg)', fontsize=12)
    ax1.set_ylabel('Counts', fontsize=12)
    ax1.set_title('Photon Incidence Angle at SiPIN', fontsize=14, fontweight='bold')
    ax1.set_xlim([0, 90])
    
    # Mark critical angle for grease (n=1.46 -> theta_c = arcsin(1.46/1.86) ~ 51.7 deg)
    theta_c = np.degrees(np.arcsin(1.46/1.86))
    ax1.axvline(x=theta_c, color='red', linestyle='--', linewidth=2, label=f'Critical angle ({theta_c:.1f}°)')
    ax1.legend()
else:
    ax1.text(0.5, 0.5, 'No data', ha='center', va='center', transform=ax1.transAxes)

# === Plot 2: Hit Count Distribution ===
ax2 = axes[0, 1]
x, y, edges = get_hist_data(tfile, "HitCount")
if x is not None and np.sum(y) > 0:
    mask = y > 0
    ax2.bar(x[mask], y[mask], width=0.8, color='#E94F37', alpha=0.8, edgecolor='none')
    ax2.set_xlabel('Number of SiPIN Interface Hits per Photon', fontsize=12)
    ax2.set_ylabel('Counts', fontsize=12)
    ax2.set_title('Photon Hit Count Distribution', fontsize=14, fontweight='bold')
    ax2.set_yscale('log')
else:
    ax2.text(0.5, 0.5, 'No data', ha='center', va='center', transform=ax2.transAxes)

# === Plot 3: Escape Channel ===
ax3 = axes[1, 0]
x, y, edges = get_hist_data(tfile, "EscapeChannel")
if x is not None and np.sum(y) > 0:
    # Channel codes: 0=crystal absorbed, 1=top escape, 2=side escape, 3=PTFE absorbed, 4=other
    labels = ['Crystal\nAbsorbed', 'Top\nEscape', 'Side\nEscape', 'PTFE\nAbsorbed', 'Other']
    colors = ['#6C757D', '#28A745', '#FFC107', '#DC3545', '#17A2B8']
    n_channels = min(5, len(y))
    bars = ax3.bar(range(n_channels), y[:n_channels], color=colors[:n_channels], alpha=0.8, edgecolor='black')
    ax3.set_xticks(range(n_channels))
    ax3.set_xticklabels(labels[:n_channels], fontsize=10)
    ax3.set_ylabel('Counts', fontsize=12)
    ax3.set_title('Photon Loss/Escape Channels', fontsize=14, fontweight='bold')
    
    # Add value labels on bars
    for bar, val in zip(bars, y[:n_channels]):
        if val > 0:
            ax3.text(bar.get_x() + bar.get_width()/2, bar.get_height() + max(y)*0.02, 
                    f'{int(val)}', ha='center', va='bottom', fontsize=10)
else:
    ax3.text(0.5, 0.5, 'No data', ha='center', va='center', transform=ax3.transAxes)

# === Plot 4: Path Length in Crystal ===
ax4 = axes[1, 1]
x, y, edges = get_hist_data(tfile, "PhotonPathInCrystal")
if x is not None and np.sum(y) > 0:
    mask = y > 0
    width = (edges[1]-edges[0]) if len(edges) > 1 else 0.5
    ax4.bar(x[mask], y[mask], width=width*0.9, color='#4CAF50', alpha=0.8, edgecolor='none')
    ax4.set_xlabel('Total Path Length in Crystal (mm)', fontsize=12)
    ax4.set_ylabel('Counts', fontsize=12)
    ax4.set_title('Photon Path Length Distribution', fontsize=14, fontweight='bold')
    ax4.set_xlim([0, 50])
    
    # Calculate mean path length
    total = np.sum(y)
    if total > 0:
        mean_path = np.sum(x * y) / total
        ax4.axvline(x=mean_path, color='red', linestyle='--', linewidth=2, label=f'Mean = {mean_path:.1f} mm')
        ax4.legend()
else:
    ax4.text(0.5, 0.5, 'No data', ha='center', va='center', transform=ax4.transAxes)

plt.suptitle('SiPINLC Detailed Analysis\n(10000 events, 5x5x5mm GAGG with grease coupling)', 
             fontsize=16, fontweight='bold', y=0.98)
plt.tight_layout(rect=[0, 0, 1, 0.95])

# Save
for ext in ['png', 'pdf']:
    output_path = os.path.join(OUTPUT_DIR, f'detailed_analysis.{ext}')
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    print(f'Saved: {output_path}')

plt.close()
print('Done!')
