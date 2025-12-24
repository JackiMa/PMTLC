#!/usr/bin/env python3
"""
SiPIN Antireflection Coating TMM Analysis

基于论文 Section 4.3 的方法：
1. 假设 SiPIN 表面有 Si3N4 单层增透涂层
2. 从厂家空气 QE 曲线反推膜厚 d*
3. 用拟合得到的 d*，计算 grease 入射时的 P_det(λ, θ)

物理原理：
- 增透涂层：n_optimal = sqrt(n1 * n2)
- 厚度条件：d = λ/(4*n_coating) 实现相消干涉
- Si3N4: n ≈ 2.0
- Si: n ≈ 3.5 + 0.01i (有吸收)
- Air: n = 1.0
- Grease: n = 1.46

Transfer Matrix Method 实现
"""

import numpy as np
import matplotlib.pyplot as plt
import os

# 输出目录
OUTPUT_DIR = '/home/wsl2/myGeant4/SiPINLC/docs/figures'
SPECTRUM_DIR = '/home/wsl2/myGeant4/SiPINLC/spectrum'
os.makedirs(OUTPUT_DIR, exist_ok=True)
os.makedirs(SPECTRUM_DIR, exist_ok=True)

# =============================================================================
# 材料光学常数
# =============================================================================

def n_Si3N4(wl_nm):
    """Si3N4 折射率 (Philipp, 1973)
    简化模型：n ≈ 2.0 + 色散修正
    """
    # 简化：假设弱色散
    return 2.0 + 0.03 * (530 / wl_nm)  # 短波长处略高

def n_k_Si(wl_nm):
    """Si 的复折射率 (Green, 2008 简化模型)
    返回 (n, k) 元组
    """
    # 简化模型：GAGG 发射带 (450-650nm) 内的典型值
    # 实际应该读取 Green 2008 数据
    wl_um = wl_nm / 1000.0
    
    # 拟合公式 (简化)
    if wl_nm < 400:
        n = 5.0
        k = 3.0
    elif wl_nm < 500:
        n = 4.5 - 0.005 * (wl_nm - 400)
        k = 0.3 - 0.002 * (wl_nm - 400)
    elif wl_nm < 600:
        n = 4.0 - 0.003 * (wl_nm - 500)
        k = 0.1 - 0.0008 * (wl_nm - 500)
    else:
        n = 3.7 - 0.001 * (wl_nm - 600)
        k = 0.02 * np.exp(-(wl_nm - 600) / 200)
    
    return n, max(k, 0.001)

# =============================================================================
# Transfer Matrix Method
# =============================================================================

def fresnel_coefficients(n1, n2, theta_i):
    """计算 Fresnel 反射/透射系数
    
    Args:
        n1: 入射介质折射率 (可为复数)
        n2: 出射介质折射率 (可为复数)
        theta_i: 入射角 (rad)
    
    Returns:
        rs, rp, ts, tp: s和p偏振的反射/透射系数
    """
    cos_i = np.cos(theta_i)
    
    # Snell 定律计算折射角
    sin_t = (n1 / n2) * np.sin(theta_i)
    
    # 检查全反射
    if np.abs(sin_t) > 1:
        # 全反射：r = 1, t = 0
        return 1.0, 1.0, 0.0, 0.0
    
    cos_t = np.sqrt(1 - sin_t**2)
    
    # Fresnel 公式
    rs = (n1 * cos_i - n2 * cos_t) / (n1 * cos_i + n2 * cos_t)
    rp = (n2 * cos_i - n1 * cos_t) / (n2 * cos_i + n1 * cos_t)
    ts = 2 * n1 * cos_i / (n1 * cos_i + n2 * cos_t)
    tp = 2 * n1 * cos_i / (n2 * cos_i + n1 * cos_t)
    
    return rs, rp, ts, tp

def tmm_single_layer(n0, n1, n2_complex, d_nm, wl_nm, theta_deg):
    """单层薄膜的 TMM 计算
    
    结构: n0 (入射) | n1 (薄膜, 厚度d) | n2 (基底, 复折射率)
    
    Args:
        n0: 入射介质折射率
        n1: 薄膜折射率
        n2_complex: 基底复折射率 (n + i*k)
        d_nm: 薄膜厚度 (nm)
        wl_nm: 波长 (nm)
        theta_deg: 入射角 (degree)
    
    Returns:
        R: 能量反射率
        T: 能量透射率 (进入基底的功率)
    """
    theta_rad = np.radians(theta_deg)
    
    # 入射介质中的角度
    cos_0 = np.cos(theta_rad)
    sin_0 = np.sin(theta_rad)
    
    # 薄膜中的角度 (Snell)
    sin_1 = (n0 / n1) * sin_0
    if np.abs(sin_1) >= 1:
        return 1.0, 0.0  # 全反射
    cos_1 = np.sqrt(1 - sin_1**2)
    
    # 基底中的角度 (复数)
    sin_2 = (n0 / n2_complex) * sin_0
    cos_2 = np.sqrt(1 - sin_2**2)
    
    # 传播常数
    k0 = 2 * np.pi / wl_nm
    delta = k0 * n1 * d_nm * cos_1  # 薄膜相位厚度
    
    # 非偏振光：平均 s 和 p 偏振
    R_total = 0
    T_total = 0
    
    for pol in ['s', 'p']:
        if pol == 's':
            # s 偏振
            r01 = (n0 * cos_0 - n1 * cos_1) / (n0 * cos_0 + n1 * cos_1)
            r12 = (n1 * cos_1 - n2_complex * cos_2) / (n1 * cos_1 + n2_complex * cos_2)
            t01 = 2 * n0 * cos_0 / (n0 * cos_0 + n1 * cos_1)
            t12 = 2 * n1 * cos_1 / (n1 * cos_1 + n2_complex * cos_2)
        else:
            # p 偏振
            r01 = (n1 * cos_0 - n0 * cos_1) / (n1 * cos_0 + n0 * cos_1)
            r12 = (n2_complex * cos_1 - n1 * cos_2) / (n2_complex * cos_1 + n1 * cos_2)
            t01 = 2 * n0 * cos_0 / (n1 * cos_0 + n0 * cos_1)
            t12 = 2 * n1 * cos_1 / (n2_complex * cos_1 + n1 * cos_2)
        
        # 总反射系数 (考虑多次反射干涉)
        exp_2delta = np.exp(2j * delta)
        r_total = (r01 + r12 * exp_2delta) / (1 + r01 * r12 * exp_2delta)
        
        R = np.abs(r_total)**2
        R_total += 0.5 * R
    
    return R_total, 1 - R_total

def calculate_bare_si_reflectance(n0, n_si, k_si, theta_deg):
    """计算无涂层 Si 的反射率"""
    theta_rad = np.radians(theta_deg)
    n2 = n_si + 1j * k_si
    
    cos_0 = np.cos(theta_rad)
    sin_0 = np.sin(theta_rad)
    sin_2 = (n0 / n2) * sin_0
    cos_2 = np.sqrt(1 - sin_2**2)
    
    # s 偏振
    rs = (n0 * cos_0 - n2 * cos_2) / (n0 * cos_0 + n2 * cos_2)
    # p 偏振
    rp = (n2 * cos_0 - n0 * cos_2) / (n2 * cos_0 + n0 * cos_2)
    
    R = 0.5 * (np.abs(rs)**2 + np.abs(rp)**2)
    return R

# =============================================================================
# 厂家 QE 数据 (S3590-09)
# =============================================================================

def manufacturer_qe_air(wl_nm):
    """厂家 QE 曲线 (空气, 正入射)
    基于 Hamamatsu S3590-09 datasheet
    峰值约 97% @ 640nm, 530nm 处约 80%
    """
    # 简化的高斯-多项式拟合
    if wl_nm < 320 or wl_nm > 1100:
        return 0.0
    
    # 双峰模型
    peak1 = 0.97 * np.exp(-0.5 * ((wl_nm - 640) / 150)**2)
    peak2 = 0.80 * np.exp(-0.5 * ((wl_nm - 530) / 100)**2)
    
    # 综合
    qe = max(peak1, peak2)
    
    # 边界衰减
    if wl_nm < 400:
        qe *= (wl_nm - 320) / 80
    if wl_nm > 1000:
        qe *= (1100 - wl_nm) / 100
    
    return max(0, min(qe, 1.0))

# =============================================================================
# 拟合膜厚
# =============================================================================

def fit_coating_thickness():
    """从厂家 QE 反推 Si3N4 膜厚
    
    原理：
    QE_factory = η_in * (1 - R_air(d))
    假设 η_in ≈ 1 在 GAGG 发射带
    最小化 |QE_calc - QE_factory|
    """
    print("=== Step 1: 从厂家 QE 反推 Si3N4 膜厚 ===")
    
    # 测试波长范围
    wavelengths = np.arange(450, 700, 10)
    
    # 候选膜厚
    d_candidates = np.arange(50, 120, 2)  # nm
    
    best_d = 0
    best_error = float('inf')
    
    for d in d_candidates:
        error = 0
        for wl in wavelengths:
            n_si, k_si = n_k_Si(wl)
            n_ar = n_Si3N4(wl)
            
            R_calc, _ = tmm_single_layer(1.0, n_ar, n_si + 1j*k_si, d, wl, 0)
            qe_calc = 1 - R_calc  # 假设 η_in = 1
            qe_factory = manufacturer_qe_air(wl)
            
            error += (qe_calc - qe_factory)**2
        
        if error < best_error:
            best_error = error
            best_d = d
    
    print(f"  最佳拟合膜厚: d* = {best_d} nm")
    print(f"  拟合误差 (MSE): {best_error / len(wavelengths):.6f}")
    
    return best_d

# =============================================================================
# 计算 P_det 二维表
# =============================================================================

def calculate_pdet_table(d_nm, n0, output_name):
    """计算 P_det(λ, θ) 二维表
    
    Args:
        d_nm: Si3N4 膜厚 (nm)
        n0: 入射介质折射率 (1.0 for air, 1.46 for grease)
        output_name: 输出文件名
    """
    print(f"=== 计算 P_det 表 (n0={n0}, d={d_nm}nm) ===")
    
    wavelengths = np.arange(400, 701, 10)  # nm
    angles = np.arange(0, 90, 5)  # degree
    
    # 结果存储
    results = []
    
    for wl in wavelengths:
        n_si, k_si = n_k_Si(wl)
        n_ar = n_Si3N4(wl)
        
        for theta in angles:
            # 检查 grease 侧是否全反射到达不了 Si
            sin_limit = n0 / n_ar
            if np.sin(np.radians(theta)) > 1:
                pdet = 0.0
            else:
                R, T = tmm_single_layer(n0, n_ar, n_si + 1j*k_si, d_nm, wl, theta)
                pdet = 1 - R  # P_det = 透射率
            
            results.append((wl, theta, pdet))
    
    # 保存 CSV
    output_path = os.path.join(SPECTRUM_DIR, output_name)
    with open(output_path, 'w') as f:
        f.write("# wavelength_nm, theta_deg, p_det\n")
        f.write("# Generated by TMM calculation\n")
        f.write(f"# AR coating: Si3N4, d = {d_nm} nm\n")
        f.write(f"# Incident medium: n = {n0}\n")
        for wl, theta, pdet in results:
            f.write(f"{wl}, {theta}, {pdet:.6f}\n")
    
    print(f"  保存到: {output_path}")
    return results

# =============================================================================
# 验证和可视化
# =============================================================================

def plot_comparison(d_nm):
    """绘制对比图：空气 vs Grease，有涂层 vs 无涂层"""
    print("=== 生成对比图 ===")
    
    wavelengths = np.arange(400, 701, 5)
    angles = [0, 15, 30, 45, 60]
    
    fig, axes = plt.subplots(2, 2, figsize=(14, 12))
    
    # === 图1: 正入射，不同入射介质的 P_det ===
    ax1 = axes[0, 0]
    
    # 有涂层
    pdet_air_coated = []
    pdet_grease_coated = []
    # 无涂层
    pdet_air_bare = []
    pdet_grease_bare = []
    # 厂家 QE
    qe_factory = []
    
    for wl in wavelengths:
        n_si, k_si = n_k_Si(wl)
        n_ar = n_Si3N4(wl)
        
        # 有涂层
        R_air, _ = tmm_single_layer(1.0, n_ar, n_si + 1j*k_si, d_nm, wl, 0)
        R_grease, _ = tmm_single_layer(1.46, n_ar, n_si + 1j*k_si, d_nm, wl, 0)
        pdet_air_coated.append(1 - R_air)
        pdet_grease_coated.append(1 - R_grease)
        
        # 无涂层
        R_air_bare = calculate_bare_si_reflectance(1.0, n_si, k_si, 0)
        R_grease_bare = calculate_bare_si_reflectance(1.46, n_si, k_si, 0)
        pdet_air_bare.append(1 - R_air_bare)
        pdet_grease_bare.append(1 - R_grease_bare)
        
        qe_factory.append(manufacturer_qe_air(wl))
    
    ax1.plot(wavelengths, qe_factory, 'k--', linewidth=2, label='Factory QE (Air, 0°)')
    ax1.plot(wavelengths, pdet_air_coated, 'b-', linewidth=2, label=f'TMM: Air + Si3N4 ({d_nm}nm)')
    ax1.plot(wavelengths, pdet_grease_coated, 'r-', linewidth=2, label=f'TMM: Grease + Si3N4 ({d_nm}nm)')
    ax1.plot(wavelengths, pdet_air_bare, 'b:', linewidth=2, label='Bare Si: Air')
    ax1.plot(wavelengths, pdet_grease_bare, 'r:', linewidth=2, label='Bare Si: Grease')
    
    ax1.set_xlabel('Wavelength (nm)', fontsize=12)
    ax1.set_ylabel('P_det (= 1 - R)', fontsize=12)
    ax1.set_title('Normal Incidence: Effect of AR Coating', fontsize=14, fontweight='bold')
    ax1.legend(fontsize=10)
    ax1.set_xlim([400, 700])
    ax1.set_ylim([0.5, 1.05])
    ax1.grid(True, alpha=0.3)
    ax1.axvline(x=530, color='green', linestyle=':', alpha=0.5)
    ax1.annotate('GAGG peak\n530nm', xy=(530, 0.55), fontsize=9, color='green')
    
    # === 图2: 530nm处，P_det vs 角度 ===
    ax2 = axes[0, 1]
    wl_ref = 530  # GAGG 发射峰
    n_si, k_si = n_k_Si(wl_ref)
    n_ar = n_Si3N4(wl_ref)
    
    angles_plot = np.arange(0, 85, 1)
    pdet_air_angle = []
    pdet_grease_angle = []
    pdet_air_bare_angle = []
    pdet_grease_bare_angle = []
    
    for theta in angles_plot:
        R_air, _ = tmm_single_layer(1.0, n_ar, n_si + 1j*k_si, d_nm, wl_ref, theta)
        R_grease, _ = tmm_single_layer(1.46, n_ar, n_si + 1j*k_si, d_nm, wl_ref, theta)
        pdet_air_angle.append(1 - R_air)
        pdet_grease_angle.append(1 - R_grease)
        
        R_air_bare = calculate_bare_si_reflectance(1.0, n_si, k_si, theta)
        R_grease_bare = calculate_bare_si_reflectance(1.46, n_si, k_si, theta)
        pdet_air_bare_angle.append(1 - R_air_bare)
        pdet_grease_bare_angle.append(1 - R_grease_bare)
    
    ax2.plot(angles_plot, pdet_air_angle, 'b-', linewidth=2, label=f'Air + Si3N4')
    ax2.plot(angles_plot, pdet_grease_angle, 'r-', linewidth=2, label=f'Grease + Si3N4')
    ax2.plot(angles_plot, pdet_air_bare_angle, 'b:', linewidth=2, label='Bare Si: Air')
    ax2.plot(angles_plot, pdet_grease_bare_angle, 'r:', linewidth=2, label='Bare Si: Grease')
    
    ax2.set_xlabel('Incidence Angle (deg)', fontsize=12)
    ax2.set_ylabel('P_det (= 1 - R)', fontsize=12)
    ax2.set_title(f'Angular Dependence @ {wl_ref}nm', fontsize=14, fontweight='bold')
    ax2.legend(fontsize=10)
    ax2.set_xlim([0, 85])
    ax2.set_ylim([0, 1.05])
    ax2.grid(True, alpha=0.3)
    
    # === 图3: Grease 入射的 2D 热图 ===
    ax3 = axes[1, 0]
    
    wl_grid = np.arange(400, 701, 5)
    theta_grid = np.arange(0, 85, 2)
    pdet_2d = np.zeros((len(theta_grid), len(wl_grid)))
    
    for i, theta in enumerate(theta_grid):
        for j, wl in enumerate(wl_grid):
            n_si, k_si = n_k_Si(wl)
            n_ar = n_Si3N4(wl)
            R, _ = tmm_single_layer(1.46, n_ar, n_si + 1j*k_si, d_nm, wl, theta)
            pdet_2d[i, j] = 1 - R
    
    im = ax3.imshow(pdet_2d, extent=[400, 700, 84, 0], aspect='auto', 
                    cmap='viridis', vmin=0.5, vmax=1.0)
    plt.colorbar(im, ax=ax3, label='P_det')
    ax3.set_xlabel('Wavelength (nm)', fontsize=12)
    ax3.set_ylabel('Incidence Angle (deg)', fontsize=12)
    ax3.set_title(f'P_det(λ,θ) for Grease Incidence\n(Si3N4 d={d_nm}nm)', fontsize=14, fontweight='bold')
    
    # === 图4: 关键结论 ===
    ax4 = axes[1, 1]
    ax4.axis('off')
    
    # 计算一些关键数值
    n_si, k_si = n_k_Si(530)
    n_ar = n_Si3N4(530)
    
    R_air_0, _ = tmm_single_layer(1.0, n_ar, n_si + 1j*k_si, d_nm, 530, 0)
    R_grease_0, _ = tmm_single_layer(1.46, n_ar, n_si + 1j*k_si, d_nm, 530, 0)
    R_grease_45, _ = tmm_single_layer(1.46, n_ar, n_si + 1j*k_si, d_nm, 530, 45)
    R_air_bare = calculate_bare_si_reflectance(1.0, n_si, k_si, 0)
    R_grease_bare = calculate_bare_si_reflectance(1.46, n_si, k_si, 0)
    
    summary = f"""
关键结论 (@ 530nm, Si3N4 d={d_nm}nm)

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
有 Si3N4 增透涂层:
  Air,  θ=0°:   P_det = {1-R_air_0:.3f}  (反射率 {R_air_0*100:.1f}%)
  Grease, θ=0°: P_det = {1-R_grease_0:.3f}  (反射率 {R_grease_0*100:.1f}%)
  Grease, θ=45°: P_det = {1-R_grease_45:.3f} (反射率 {R_grease_45*100:.1f}%)

无涂层 (裸 Si):
  Air,  θ=0°:   P_det = {1-R_air_bare:.3f}  (反射率 {R_air_bare*100:.1f}%)
  Grease, θ=0°: P_det = {1-R_grease_bare:.3f}  (反射率 {R_grease_bare*100:.1f}%)

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
结论:
1. 有涂层 + Air 最好 (增透层为 Air 优化)
2. 有涂层 + Grease 仍然比裸 Si + Grease 好
3. Grease 的主要优势在于减少 Crystal-Grease TIR
   而非 Grease-Si 界面透射率
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    """
    ax4.text(0.05, 0.95, summary, transform=ax4.transAxes, fontsize=11,
             verticalalignment='top', fontfamily='monospace',
             bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5))
    
    plt.suptitle('SiPIN AR Coating TMM Analysis', fontsize=16, fontweight='bold', y=0.98)
    plt.tight_layout(rect=[0, 0, 1, 0.96])
    
    # 保存
    for ext in ['png', 'pdf']:
        output_path = os.path.join(OUTPUT_DIR, f'tmm_analysis.{ext}')
        plt.savefig(output_path, dpi=150, bbox_inches='tight')
        print(f"  保存到: {output_path}")
    
    plt.close()

# =============================================================================
# 主程序
# =============================================================================

def main():
    print("=" * 60)
    print("SiPIN Antireflection Coating TMM Analysis")
    print("=" * 60)
    
    # Step 1: 拟合膜厚
    d_star = fit_coating_thickness()
    
    # Step 2: 计算 Air 入射的 P_det 表
    calculate_pdet_table(d_star, 1.0, 'sipin_pdet_air_tmm.csv')
    
    # Step 3: 计算 Grease 入射的 P_det 表
    calculate_pdet_table(d_star, 1.46, 'sipin_pdet_grease_tmm.csv')
    
    # Step 4: 生成对比图
    plot_comparison(d_star)
    
    print("\n" + "=" * 60)
    print("完成！")
    print("=" * 60)

if __name__ == "__main__":
    main()

