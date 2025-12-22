#!/usr/bin/env python3
"""
Plot sanity check results: Theory vs Simulation
"""
import matplotlib.pyplot as plt
import numpy as np

# Data from experiments
scenarios = ['A-Air', 'A-Grease', 'B-Air', 'B-Grease', 'C-Air', 'C-Grease']
theory = [0.157, 0.381, 1.0, 1.0, 0.698, 0.832]
simulation = [0.300, 0.488, 0.825, 0.914, 0.576, 0.766]

# Colors for different scenario types
colors_theory = ['#1f77b4', '#1f77b4', '#2ca02c', '#2ca02c', '#ff7f0e', '#ff7f0e']
colors_sim = ['#aec7e8', '#aec7e8', '#98df8a', '#98df8a', '#ffbb78', '#ffbb78']

x = np.arange(len(scenarios))
width = 0.35

fig, ax = plt.subplots(figsize=(12, 6))

# Bars
bars1 = ax.bar(x - width/2, theory, width, label='Theory', color='steelblue', edgecolor='navy', alpha=0.8)
bars2 = ax.bar(x + width/2, simulation, width, label='Simulation', color='coral', edgecolor='darkred', alpha=0.8)

# Labels and styling
ax.set_ylabel('Light Collection Efficiency (ε_col)', fontsize=12)
ax.set_xlabel('Scenario', fontsize=12)
ax.set_title('Sanity Check: Theory vs Geant4 Simulation\n(GAGG 5×5×5 mm³, 1000 photons)', fontsize=14)
ax.set_xticks(x)
ax.set_xticklabels(scenarios, fontsize=11)
ax.legend(fontsize=11, loc='upper left')
ax.set_ylim(0, 1.1)
ax.axhline(y=1.0, color='gray', linestyle='--', alpha=0.5, label='Ideal (100%)')

# Add value labels on bars
for bar, val in zip(bars1, theory):
    ax.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.02, 
            f'{val:.3f}', ha='center', va='bottom', fontsize=9, color='navy')
for bar, val in zip(bars2, simulation):
    ax.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.02, 
            f'{val:.3f}', ha='center', va='bottom', fontsize=9, color='darkred')

# Grid
ax.yaxis.grid(True, linestyle='--', alpha=0.3)
ax.set_axisbelow(True)

# Annotations
ax.annotate('A: TIR (specular)\nB: Lambertian R=100%\nC: PTFE R=97.5%', 
            xy=(0.98, 0.02), xycoords='axes fraction',
            ha='right', va='bottom', fontsize=9,
            bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5))

plt.tight_layout()
plt.savefig('/home/wsl2/myGeant4/SiPINLC/docs/sanity_check_results.png', dpi=150)
plt.savefig('/home/wsl2/myGeant4/SiPINLC/docs/sanity_check_results.pdf')
print("Saved: docs/sanity_check_results.png and .pdf")
plt.show()

