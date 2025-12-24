#!/usr/bin/env python3
"""Plot p × sigma_alpha scan results as heatmap."""
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

# 读取数据
csv_file = '/home/wsl2/myGeant4/SiPINLC/results/p_sigma_scan.csv'
df = pd.read_csv(csv_file)

# 提取唯一值
p_values = sorted(df['p'].unique())
sigma_values = sorted(df['sigma_rad'].unique())

# 创建二维数组
eps_matrix = np.zeros((len(sigma_values), len(p_values)))
absorbed_matrix = np.zeros((len(sigma_values), len(p_values)))

for i, sigma in enumerate(sigma_values):
    for j, p in enumerate(p_values):
        row = df[(df['p'] == p) & (df['sigma_rad'] == sigma)]
        if len(row) > 0:
            eps_matrix[i, j] = row['eps_col'].values[0] * 100
            absorbed_matrix[i, j] = row['absorbed'].values[0]

# 创建图形
fig, axes = plt.subplots(1, 2, figsize=(14, 5))

# 热力图 1: eps_col
ax1 = axes[0]
im1 = ax1.imshow(eps_matrix, cmap='RdYlGn', aspect='auto', origin='lower')
ax1.set_xticks(range(len(p_values)))
ax1.set_xticklabels([f'{p:.1f}' for p in p_values])
ax1.set_yticks(range(len(sigma_values)))
ax1.set_yticklabels([f'{s:.2f}' for s in sigma_values])
ax1.set_xlabel('PTFE Contact Ratio p', fontsize=12)
ax1.set_ylabel('Crystal Sigma Alpha (rad)', fontsize=12)
ax1.set_title('Light Collection Efficiency (%)', fontsize=12)
cbar1 = plt.colorbar(im1, ax=ax1)
cbar1.set_label('eps_col (%)')

# 标注数值
for i in range(len(sigma_values)):
    for j in range(len(p_values)):
        text = ax1.text(j, i, f'{eps_matrix[i, j]:.1f}',
                       ha="center", va="center", color="black", fontsize=8)

# 热力图 2: Crystal Absorption
ax2 = axes[1]
im2 = ax2.imshow(absorbed_matrix, cmap='YlOrRd', aspect='auto', origin='lower')
ax2.set_xticks(range(len(p_values)))
ax2.set_xticklabels([f'{p:.1f}' for p in p_values])
ax2.set_yticks(range(len(sigma_values)))
ax2.set_yticklabels([f'{s:.2f}' for s in sigma_values])
ax2.set_xlabel('PTFE Contact Ratio p', fontsize=12)
ax2.set_ylabel('Crystal Sigma Alpha (rad)', fontsize=12)
ax2.set_title('Crystal Self-Absorption (photon count)', fontsize=12)
cbar2 = plt.colorbar(im2, ax=ax2)
cbar2.set_label('Absorbed')

# 标注数值
for i in range(len(sigma_values)):
    for j in range(len(p_values)):
        text = ax2.text(j, i, f'{absorbed_matrix[i, j]:.0f}',
                       ha="center", va="center", color="black", fontsize=8)

plt.tight_layout()
output_file = '/home/wsl2/myGeant4/SiPINLC/results/p_sigma_heatmap.png'
plt.savefig(output_file, dpi=150)
print(f"Saved to {output_file}")

# 找出最优点
max_idx = np.unravel_index(np.argmax(eps_matrix), eps_matrix.shape)
best_sigma = sigma_values[max_idx[0]]
best_p = p_values[max_idx[1]]
best_eps = eps_matrix[max_idx]

print(f"\n=== Best Configuration ===")
print(f"p = {best_p}, sigma_alpha = {best_sigma} rad")
print(f"eps_col = {best_eps:.2f}%")

# 打印完整表格
print("\n=== Full Results Table ===")
header = "sigma\\p  "
for p in p_values:
    header += f"{p:.1f}    "
print(header)
for i, sigma in enumerate(sigma_values):
    row = f"{sigma:.2f}    "
    for j in range(len(p_values)):
        row += f"{eps_matrix[i,j]:.1f}%  "
    print(row)

