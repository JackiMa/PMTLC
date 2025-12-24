#!/usr/bin/env python3
"""
有效平均自吸收长度分析

定义：
- L_abs(λ): 波长依赖的吸收长度 [mm]
- S(λ): 发射光谱（归一化）
- L_eff = ∫ S(λ) * L_abs(λ) dλ / ∫ S(λ) dλ  (发射光谱加权平均)

对于 GAGG:Ce:Mg:
- 发射峰 ~530 nm (2.34 eV)
- 在发射峰附近的 L_abs ~300-450 mm
"""

import numpy as np

# GAGG 吸收长度数据（从 MyMaterials.cc 提取，部分数据）
# 格式: (能量 eV, 吸收长度 mm)
absorption_data = [
    (2.25455, 451.629),
    (2.26691, 415.643),
    (2.27941, 458.704),
    (2.29205, 473.048),
    (2.30483, 388.097),
    (2.31776, 445.714),
    (2.33083, 356.658),  # 接近发射峰
    (2.34405, 362.018),
    (2.35741, 323.960),
    (2.37094, 249.410),
    (2.38462, 222.770),
    (2.39845, 173.466),
    (2.41245, 126.428),
    (2.42661, 94.945),
    (2.44094, 64.389),
    (2.45545, 45.198),
    (2.47012, 30.698),
    (2.48497, 20.705),
    (2.50, 14.096),
]

# 转换为 numpy
energies = np.array([x[0] for x in absorption_data])
L_abs = np.array([x[1] for x in absorption_data])  # mm

# 能量转波长 (nm)
wavelengths = 1239.84 / energies  # nm

print("=== GAGG 自吸收分析 ===")
print()

# 发射光谱假设为高斯分布，峰值在 530 nm，FWHM ~50 nm
lambda_peak = 530  # nm
fwhm = 50  # nm
sigma = fwhm / 2.355

# 生成发射光谱
S = np.exp(-0.5 * ((wavelengths - lambda_peak) / sigma) ** 2)
S /= S.sum()  # 归一化

# 计算有效平均自吸收长度
L_eff = np.sum(S * L_abs)

print(f"发射峰: {lambda_peak} nm (FWHM={fwhm} nm)")
print(f"发射光谱加权平均吸收长度: L_eff = {L_eff:.1f} mm")
print()

# 显示各波长的贡献
print("波长分布:")
print("λ (nm)  | L_abs (mm) | 发射权重")
print("-" * 40)
for i, (wl, L, s) in enumerate(zip(wavelengths, L_abs, S)):
    if s > 0.01:
        print(f"{wl:.1f}   | {L:.1f}      | {s:.3f}")

print()
print("=== 晶体尺寸与光程分析 ===")
print()

# 5x5x5 mm 晶体
crystal_size = 5  # mm

# 平均光程估计
print("5x5x5 mm 晶体:")
print(f"  - 直接到底面: ~{crystal_size/2:.1f} mm")
print(f"  - 经过 N 次反射: ~N × {crystal_size} mm")
print()

# 不同 sigma_alpha 对应的平均光程（从实验数据反推）
# σ=0: 40.9% 吸收 → L_path / L_eff ≈ 0.52 → L_path ≈ 200 mm
# σ=0.1: 8.2% 吸收 → L_path / L_eff ≈ 0.085 → L_path ≈ 32 mm

print("从实验数据反推平均光程:")
print("  P_abs = 1 - exp(-L_path / L_eff)")
print()

for sigma, P_abs in [(0.0, 0.409), (0.1, 0.082), (0.2, 0.078), (0.5, 0.072)]:
    if P_abs > 0:
        L_path = -L_eff * np.log(1 - P_abs)
        n_reflections = L_path / crystal_size
        print(f"  σ={sigma}: P_abs={P_abs*100:.1f}% → L_path≈{L_path:.0f}mm ({n_reflections:.0f}次反射)")

print()
print("=== 结论 ===")
print(f"1. GAGG 发射峰 530nm 处有效吸收长度: ~{L_eff:.0f} mm")
print("2. σ=0（困光）: 平均光程 ~200mm，吸收 ~40%")
print("3. σ>0（打破困光）: 平均光程 ~30mm，吸收 ~8%")
print("4. 晶体自吸收是 ~8% 的系统性效率损失")

