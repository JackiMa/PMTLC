#!/usr/bin/env python3
"""
Plot wavelength distribution: generated vs detected.
For paper: understand spectral changes during light transport.
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

def get_hist_data(tfile, histname):
    """Extract histogram data as (centers, values, edges)."""
    for key in tfile.keys():
        if histname in key:
            h = tfile[key]
            values = h.values()
            edges = h.axis().edges()
            centers = (edges[:-1] + edges[1:]) / 2
            return centers, values, edges
    return None, None, None

# Create figure
fig, ax = plt.subplots(figsize=(10, 7))

# Get generated scintillation spectrum
x_gen, y_gen, edges_gen = get_hist_data(tfile, "ScintillationWavelength")
x_det, y_det, edges_det = get_hist_data(tfile, "PhotonHitatPhotoncathode")

if x_gen is not None and np.sum(y_gen) > 0:
    # Normalize for comparison
    y_gen_norm = y_gen / np.max(y_gen) if np.max(y_gen) > 0 else y_gen
    ax.plot(x_gen, y_gen_norm, 'b-', linewidth=2, alpha=0.8, label='Generated (scintillation)')
    
if x_det is not None and np.sum(y_det) > 0:
    y_det_norm = y_det / np.max(y_det) if np.max(y_det) > 0 else y_det
    ax.plot(x_det, y_det_norm, 'r-', linewidth=2, alpha=0.8, label='Detected at SiPIN')

ax.set_xlabel('Wavelength (nm)', fontsize=12)
ax.set_ylabel('Normalized Intensity', fontsize=12)
ax.set_title('Scintillation Spectrum: Generated vs Detected\n(5x5x5mm GAGG)', fontsize=14, fontweight='bold')
ax.set_xlim([400, 700])
ax.legend(fontsize=12)
ax.grid(True, alpha=0.3)

# Add GAGG peak annotation
ax.axvline(x=530, color='gray', linestyle=':', alpha=0.5)
ax.annotate('GAGG peak\n~530 nm', xy=(530, 0.95), xytext=(570, 0.85),
            arrowprops=dict(arrowstyle='->', color='gray'),
            fontsize=10, color='gray')

plt.tight_layout()

# Save
for ext in ['png', 'pdf']:
    output_path = os.path.join(OUTPUT_DIR, f'wavelength_spectrum.{ext}')
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    print(f'Saved: {output_path}')

plt.close()
print('Done!')

