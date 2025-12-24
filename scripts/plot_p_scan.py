#!/usr/bin/env python3
"""Plot probability boundary method scan results."""
import matplotlib.pyplot as plt
import numpy as np

# Results from scan (2024-12-23 updated with p=1.0 fix)
p_values = [0.0, 0.2, 0.4, 0.5, 0.6, 0.8, 0.99, 1.0]
eps_col = [0.90218, 0.898953, 0.904119, 0.908463, 0.908873, 0.916524, 0.923613, 0.908011]
absorbed = [798, 511, 358, 312, 275, 200, 146, 101]
ptfe = [37, 330, 423, 435, 466, 466, 460, 480]
grease = [87, 92, 101, 104, 109, 103, 100, 114]
world = [2, 1, 1, 3, 1, 3, 2, 5]

fig, axes = plt.subplots(2, 1, figsize=(10, 8))

# Plot 1: eps_col vs p
ax1 = axes[0]
ax1.plot(p_values, [e * 100 for e in eps_col], 'bo-', markersize=8, linewidth=2)
ax1.axhline(y=eps_col[0] * 100, color='gray', linestyle='--', alpha=0.5, label=f'p=0 baseline ({eps_col[0]*100:.1f}%)')
ax1.set_xlabel('Side/Top Contact Ratio p', fontsize=12)
ax1.set_ylabel('Light Collection Efficiency (%)', fontsize=12)
ax1.set_title('Probabilistic Boundary Method: eps_col vs p\n(sigma_alpha=0.1 rad, grease=50um, gap=1um)', fontsize=12)
ax1.grid(True, alpha=0.3)
ax1.legend()
ax1.set_xlim(-0.05, 1.05)

# Annotate key points
for i, (p, e) in enumerate(zip(p_values, eps_col)):
    if p in [0.0, 0.5, 0.99, 1.0]:
        ax1.annotate(f'{e*100:.1f}%', (p, e*100), textcoords="offset points", 
                    xytext=(0, 10), ha='center', fontsize=9)

# Plot 2: Loss channels vs p
ax2 = axes[1]
x = np.arange(len(p_values))
width = 0.2

bars1 = ax2.bar(x - 1.5*width, absorbed, width, label='Crystal Self-Absorption', color='orange')
bars2 = ax2.bar(x - 0.5*width, ptfe, width, label='PTFE Absorption', color='gray')
bars3 = ax2.bar(x + 0.5*width, grease, width, label='Grease/Edge Escape', color='blue', alpha=0.5)
bars4 = ax2.bar(x + 1.5*width, world, width, label='World Escape', color='red', alpha=0.5)

ax2.set_xlabel('Side/Top Contact Ratio p', fontsize=12)
ax2.set_ylabel('Photon Count', fontsize=12)
ax2.set_title('Loss Channels vs p', fontsize=12)
ax2.set_xticks(x)
ax2.set_xticklabels(p_values)
ax2.legend(loc='upper right')
ax2.grid(True, alpha=0.3, axis='y')

plt.tight_layout()
plt.savefig('/home/wsl2/myGeant4/SiPINLC/results/p_scan_final.png', dpi=150)
print("Saved to /home/wsl2/myGeant4/SiPINLC/results/p_scan_final.png")

# Print analysis
print("\n=== Analysis ===")
print(f"p=0 (TIR only):     eps_col = {eps_col[0]*100:.2f}%")
print(f"p=0.5 (50% PTFE):   eps_col = {eps_col[3]*100:.2f}%")
print(f"p=0.99 (99% PTFE):  eps_col = {eps_col[6]*100:.2f}%")
print(f"p=1.0 (100% PTFE):  eps_col = {eps_col[7]*100:.2f}%")
print(f"\nTrend: eps_col increases with p (from {eps_col[0]*100:.1f}% to {eps_col[6]*100:.1f}%)")
print(f"This is because higher p leads to shorter paths (less self-absorption).")
print(f"\nKey observation: p=1.0 has lowest self-absorption ({absorbed[7]}) but highest PTFE absorption ({ptfe[7]})")
print(f"The net effect depends on the balance between these two loss mechanisms.")

