#!/usr/bin/env python3
"""
吸收长度缩放扫描结果可视化
"""

import matplotlib.pyplot as plt
import numpy as np

# 扫描结果 (550nm 固定波长)
data = """
scale_factor,eps_col,absorbed,other
0.1,0.526903,4260,363
0.5,0.8019,1352,584
1.0,0.858948,728,666
2.0,0.88962,385,704
10.0,0.918802,72,736
100.0,0.924286,4,750
"""

# 解析数据
lines = data.strip().split('\n')[1:]
scale_vals = []
eps_vals = []
abs_vals = []

for line in lines:
    parts = line.split(',')
    scale_vals.append(float(parts[0]))
    eps_vals.append(float(parts[1]) * 100)
    abs_vals.append(int(parts[2]) / 100)  # 转为百分比

scale_vals = np.array(scale_vals)
eps_vals = np.array(eps_vals)
abs_vals = np.array(abs_vals)

# 理论：L_eff ~ 285 mm，晶体边长 5mm
# 平均光程估计：~24 mm (从之前分析)
# P_abs = 1 - exp(-L_path / (L_eff * scale))
L_eff = 285  # mm
L_path = 24  # mm (从 sigma=0.1 的实验反推)

scale_theory = np.logspace(-1, 2, 50)
P_abs_theory = 1 - np.exp(-L_path / (L_eff * scale_theory))
eps_theory = 92.4 * (1 - P_abs_theory)  # 假设无吸收极限是 92.4%

# 创建图表
fig, axes = plt.subplots(1, 2, figsize=(12, 5))

# 左图：ε_col vs scaleFactor
ax1 = axes[0]
ax1.semilogx(scale_vals, eps_vals, 'bo-', markersize=10, linewidth=2, label='Geant4 Simulation')
ax1.semilogx(scale_theory, eps_theory, 'r--', linewidth=1.5, alpha=0.7, 
             label=f'Theory: ε = 92.4% × (1 - P_abs)\nL_path={L_path}mm, L_eff={L_eff}mm')
ax1.axhline(92.4, color='gray', linestyle=':', alpha=0.7, label='Ideal limit (no absorption): 92.4%')
ax1.axvline(1.0, color='green', linestyle='--', alpha=0.5, label='Original GAGG (scale=1)')

ax1.set_xlabel('Absorption Length Scale Factor', fontsize=12)
ax1.set_ylabel('Light Collection Efficiency ε_col (%)', fontsize=12)
ax1.set_title('ε_col vs Absorption Length Scale\n(550nm, σ=0.1, grease=50μm)', fontsize=11)
ax1.legend(loc='lower right', fontsize=9)
ax1.grid(True, alpha=0.3)
ax1.set_xlim(0.05, 200)
ax1.set_ylim(45, 100)

# 右图：晶体吸收率 vs scaleFactor
ax2 = axes[1]
ax2.semilogx(scale_vals, abs_vals, 'rs-', markersize=10, linewidth=2, label='Geant4 Simulation')
ax2.semilogx(scale_theory, P_abs_theory * 100, 'b--', linewidth=1.5, alpha=0.7,
             label=f'Theory: P_abs = 1 - exp(-L_path/L)\nL_path={L_path}mm, L_eff={L_eff}mm')
ax2.axvline(1.0, color='green', linestyle='--', alpha=0.5, label='Original GAGG (scale=1)')

ax2.set_xlabel('Absorption Length Scale Factor', fontsize=12)
ax2.set_ylabel('Crystal Self-Absorption (%)', fontsize=12)
ax2.set_title('Self-Absorption vs Absorption Length Scale\n(550nm, σ=0.1)', fontsize=11)
ax2.legend(loc='upper right', fontsize=9)
ax2.grid(True, alpha=0.3)
ax2.set_xlim(0.05, 200)
ax2.set_ylim(0, 50)

plt.tight_layout()

# 保存
output_dir = '/home/wsl2/myGeant4/SiPINLC/docs/figures'
import os
os.makedirs(output_dir, exist_ok=True)
plt.savefig(f'{output_dir}/absorption_scale_scan.png', dpi=150, bbox_inches='tight')
plt.savefig(f'{output_dir}/absorption_scale_scan.pdf', bbox_inches='tight')
print(f"Saved to {output_dir}/absorption_scale_scan.png")

# 打印摘要
print("\n=== 吸收长度缩放扫描摘要 ===")
print(f"光子波长: 550nm")
print(f"原始 GAGG (scale=1): ε_col = {eps_vals[2]:.1f}%, 自吸收 = {abs_vals[2]:.1f}%")
print(f"无吸收极限 (scale=100): ε_col = {eps_vals[-1]:.1f}%")
print(f"自吸收导致的效率损失: {eps_vals[-1] - eps_vals[2]:.1f}%")
print()
print("理论估计:")
print(f"  有效吸收长度 L_eff = {L_eff} mm (发射光谱加权)")
print(f"  平均光程 L_path ≈ {L_path} mm (从实验反推)")
print(f"  P_abs = 1 - exp(-{L_path}/{L_eff}) = {(1-np.exp(-L_path/L_eff))*100:.1f}%")

