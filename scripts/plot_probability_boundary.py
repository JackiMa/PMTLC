#!/usr/bin/env python3
"""
概率边界法结果可视化（修复后）
"""

import matplotlib.pyplot as plt
import numpy as np

# 扫描结果（修复后，p=1 现在正确使用 dielectric_metal）
data = """
p,eps_col,absorbed,ptfe,other
0.0,0.84441,901,0,620
0.2,0.752546,792,434,1200
0.4,0.678488,713,849,1609
0.5,0.643491,676,1105,1713
0.6,0.614578,641,1324,1857
0.8,0.540878,541,2084,1936
0.99,0.46724,394,3275,1595
1.0,0.781603,2064,0,78
"""

# 理想 R=1 的结果
p_ideal = 2.0
eps_ideal = 0.917308

# 解析数据
lines = data.strip().split('\n')[1:]
p_vals = []
eps_vals = []
absorbed_vals = []
ptfe_vals = []
other_vals = []

for line in lines:
    parts = line.split(',')
    p_vals.append(float(parts[0]))
    eps_vals.append(float(parts[1]) * 100)  # 转为百分比
    absorbed_vals.append(int(parts[2]))
    ptfe_vals.append(int(parts[3]))
    other_vals.append(int(parts[4]))

p_vals = np.array(p_vals)
eps_vals = np.array(eps_vals)

# 创建图表
fig, axes = plt.subplots(1, 2, figsize=(12, 5))

# 左图：ε_col vs p
ax1 = axes[0]
# 分离 0<p<1 和 p=1 的点
mask_prob = p_vals < 1.0
mask_border = p_vals >= 1.0

ax1.plot(p_vals[mask_prob], eps_vals[mask_prob], 'bo-', label='Probabilistic Boundary (0<p<1)', markersize=8, linewidth=2)
ax1.plot(p_vals[mask_border], eps_vals[mask_border], 'rs', label=f'BorderSurface (p=1, R=97.5%): {eps_vals[-1]:.1f}%', markersize=12)

# 添加理想 R=1 的点
ax1.plot([1.0], [eps_ideal*100], 'g^', markersize=12, label=f'BorderSurface (R=100%): {eps_ideal*100:.1f}%')

# 添加 p=0 作为参考线
ax1.axhline(eps_vals[0], color='gray', linestyle='--', alpha=0.7, label=f'p=0 (TIR+wrapper): {eps_vals[0]:.1f}%')

ax1.set_xlabel('Side/Top Contact Ratio (p)', fontsize=12)
ax1.set_ylabel('Light Collection Efficiency ε_col (%)', fontsize=12)
ax1.set_title('Probabilistic Boundary Method: ε_col vs p\n(sideGap=1μm, grease=50μm, σ=0.1)', fontsize=11)
ax1.legend(loc='upper right', fontsize=9)
ax1.grid(True, alpha=0.3)
ax1.set_xlim(-0.05, 1.15)
ax1.set_ylim(40, 100)

# 添加注释
ax1.annotate(f'p=1 drops due to\nlong path (self-abs)', 
             xy=(1.0, eps_vals[-1]), xytext=(0.7, eps_vals[-1]-10),
             arrowprops=dict(arrowstyle='->', color='red'),
             fontsize=9, color='red')

# 右图：损失通道分析
ax2 = axes[1]
width = 0.08
x = np.arange(len(p_vals))

# 堆叠柱状图
bars1 = ax2.bar(p_vals - width, [a/100 for a in absorbed_vals], width*2, label='Crystal Absorbed', color='#d62728')
bars2 = ax2.bar(p_vals + width, [p/100 for p in ptfe_vals], width*2, label='PTFE Absorbed', color='#1f77b4')
# bars3 = ax2.bar(p_vals + width, [o/100 for o in other_vals], width, label='Other', color='#2ca02c')

ax2.set_xlabel('Side/Top Contact Ratio (p)', fontsize=12)
ax2.set_ylabel('Fraction (per 100 photons)', fontsize=12)
ax2.set_title('Loss Channels vs Contact Ratio', fontsize=11)
ax2.legend()
ax2.grid(True, alpha=0.3, axis='y')
ax2.set_xlim(-0.1, 1.1)

plt.tight_layout()

# 保存
output_dir = '/home/wsl2/myGeant4/SiPINLC/docs/figures'
import os
os.makedirs(output_dir, exist_ok=True)
plt.savefig(f'{output_dir}/probability_boundary.png', dpi=150, bbox_inches='tight')
plt.savefig(f'{output_dir}/probability_boundary.pdf', bbox_inches='tight')
print(f"Saved to {output_dir}/probability_boundary.png")

# 打印摘要
print("\n=== 概率边界法摘要 ===")
print("p=0 (air-gap): ε_col = {:.1f}%, 利用 TIR 无损反射".format(eps_vals[0]))
print("p=0.5 (50%贴合): ε_col = {:.1f}%, PTFE吸收 {:.1f}%".format(
    eps_vals[4], ptfe_vals[4]/100))
print("p=1 (BorderSurface): ε_col = {:.1f}%, 效果与 p=0 相似".format(eps_vals[-1]))
print("\n物理结论: 有 air-gap 时可利用 TIR，比直接 PTFE 贴合效率更高")

