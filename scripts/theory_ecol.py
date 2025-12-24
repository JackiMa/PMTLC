#!/usr/bin/env python3
"""
闭式理论模型：ε_col(σ_α, p)

基于两态马尔可夫链的闭式求解。
提供 M1（上界）和 M2（下界）两套曲线。
"""

import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
import os

# 输出目录
OUTPUT_DIR = '/home/wsl2/myGeant4/SiPINLC/results/theory'
os.makedirs(OUTPUT_DIR, exist_ok=True)

# =============================================================================
# 固定参数
# =============================================================================

n1 = 1.86           # 晶体折射率
n2_air = 1.0        # 空气折射率
n2_grease = 1.46    # Grease 折射率
R_c = 0.975         # PTFE 反射率
p_b = 1/6           # 底面命中概率（立方体 mean-field）

# 临界角
theta_c = np.arcsin(1/n1)  # ≈ 0.5688 rad

# =============================================================================
# 辅助函数
# =============================================================================

def mu_0(n1, n2):
    """cos(临界角)"""
    if n2 >= n1:
        return 0.0
    return np.sqrt(1 - (n2/n1)**2)

def s_A(n2):
    """体内各向同性的可出射概率"""
    return 1 - mu_0(n1, n2)

def s_L(n2):
    """朗伯混合的可出射概率"""
    return (n2/n1)**2

def q_sigma(sigma_alpha):
    """混合概率 q(σ_α)"""
    return 1 - np.exp(-(sigma_alpha / theta_c)**2)

def e_A():
    """体内各向同性的逃逸概率（晶体-空气）"""
    return 1 - np.sqrt(1 - (1/n1)**2)

def e_L():
    """朗伯混合的逃逸概率（晶体-空气）"""
    return (1/n1)**2

def e_eff(sigma_alpha):
    """有效逃逸概率 e_eff(σ)"""
    q = q_sigma(sigma_alpha)
    return (1 - q) * e_A() + q * e_L()

# =============================================================================
# 墙面存活概率
# =============================================================================

def A_M1(p):
    """M1: air-gap 逃逸全回收"""
    R_eff = 1 - p * (1 - R_c)
    return (1 - p_b) * R_eff

def A_M2(sigma_alpha, p):
    """M2: air-gap 逃逸全丢失"""
    e = e_eff(sigma_alpha)
    survival = p * R_c + (1 - p) * (1 - e)
    return (1 - p_b) * survival

# =============================================================================
# 两态马尔可夫链求解
# =============================================================================

def solve_markov(A, n2, sigma_alpha):
    """
    求解两态马尔可夫链，返回 ε_col
    
    状态：
    - U: 下次到底面为非 TIR（成功）
    - T: 下次到底面为 TIR（不成功）
    
    Args:
        A: 墙面存活概率
        n2: 底面外介质折射率
        sigma_alpha: 粗糙度参数
    
    Returns:
        ε_col: 光收集效率
    """
    q = q_sigma(sigma_alpha)
    sL = s_L(n2)
    sA = s_A(n2)
    
    # 线性方程组：
    # [1 - A(1-q+q*sL)] P_U - A*q*(1-sL) P_T = p_b
    # -A*q*sL P_U + [1 - p_b - A(1-q*sL)] P_T = 0
    
    a11 = 1 - A * (1 - q + q * sL)
    a12 = -A * q * (1 - sL)
    a21 = -A * q * sL
    a22 = 1 - p_b - A * (1 - q * sL)
    
    b1 = p_b
    b2 = 0
    
    # 解线性方程组
    det = a11 * a22 - a12 * a21
    if abs(det) < 1e-12:
        return 0.0
    
    P_U = (b1 * a22 - b2 * a12) / det
    P_T = (a11 * b2 - a21 * b1) / det
    
    # 初始条件：体内各向同性
    eps_col = sA * P_U + (1 - sA) * P_T
    
    return max(0, min(1, eps_col))

# =============================================================================
# 计算 ε_col 曲线
# =============================================================================

def calc_epsilon_col_M1(sigma_alpha, p, n2):
    """M1: 上界（air-gap 全回收）"""
    A = A_M1(p)
    return solve_markov(A, n2, sigma_alpha)

def calc_epsilon_col_M2(sigma_alpha, p, n2):
    """M2: 下界（air-gap 全丢失）"""
    A = A_M2(sigma_alpha, p)
    return solve_markov(A, n2, sigma_alpha)

# =============================================================================
# 生成数据表
# =============================================================================

def generate_theory_table():
    """生成理论数值表"""
    
    sigma_values = [0.0, 0.1, 0.2, 0.3, 0.4, 0.469, 0.6, 1.0]
    p_values = [0.0, 0.2, 0.4, 0.6, 0.8, 1.0]
    
    results = []
    
    for medium, n2 in [('air', n2_air), ('grease', n2_grease)]:
        for p in p_values:
            for sigma in sigma_values:
                eps_M1 = calc_epsilon_col_M1(sigma, p, n2)
                eps_M2 = calc_epsilon_col_M2(sigma, p, n2)
                
                results.append({
                    'medium': medium,
                    'n2': n2,
                    'p': p,
                    'sigma_alpha': sigma,
                    'eps_col_M1': eps_M1,
                    'eps_col_M2': eps_M2
                })
    
    df = pd.DataFrame(results)
    
    # 保存 CSV
    csv_path = os.path.join(OUTPUT_DIR, 'theory_epsilon_col.csv')
    df.to_csv(csv_path, index=False)
    print(f"保存到: {csv_path}")
    
    return df

# =============================================================================
# 绘图
# =============================================================================

def plot_theory_curves(df):
    """绘制理论曲线"""
    
    fig, axes = plt.subplots(2, 2, figsize=(14, 12))
    
    colors = plt.cm.viridis(np.linspace(0.2, 0.8, 6))
    
    for row, (medium, n2) in enumerate([('grease', n2_grease), ('air', n2_air)]):
        df_medium = df[df['medium'] == medium]
        
        # M1 (上界)
        ax1 = axes[row, 0]
        for i, p in enumerate([0.0, 0.2, 0.4, 0.6, 0.8, 1.0]):
            df_p = df_medium[df_medium['p'] == p]
            ax1.plot(df_p['sigma_alpha'], df_p['eps_col_M1'], 'o-', 
                    color=colors[i], linewidth=2, markersize=6, label=f'p={p}')
        
        ax1.set_xlabel('σ_α (rad)', fontsize=12)
        ax1.set_ylabel('ε_col', fontsize=12)
        ax1.set_title(f'{medium.upper()} (n₂={n2}): M1 (Upper Bound)', fontsize=14, fontweight='bold')
        ax1.legend(fontsize=9)
        ax1.set_xlim([0, 1.05])
        ax1.set_ylim([0, 1])
        ax1.grid(True, alpha=0.3)
        
        # M2 (下界)
        ax2 = axes[row, 1]
        for i, p in enumerate([0.0, 0.2, 0.4, 0.6, 0.8, 1.0]):
            df_p = df_medium[df_medium['p'] == p]
            ax2.plot(df_p['sigma_alpha'], df_p['eps_col_M2'], 's-', 
                    color=colors[i], linewidth=2, markersize=6, label=f'p={p}')
        
        ax2.set_xlabel('σ_α (rad)', fontsize=12)
        ax2.set_ylabel('ε_col', fontsize=12)
        ax2.set_title(f'{medium.upper()} (n₂={n2}): M2 (Lower Bound)', fontsize=14, fontweight='bold')
        ax2.legend(fontsize=9)
        ax2.set_xlim([0, 1.05])
        ax2.set_ylim([0, 1])
        ax2.grid(True, alpha=0.3)
        
        # 标记峰值位置
        if medium == 'grease':
            ax2.axvline(x=0.469, color='red', linestyle=':', alpha=0.5)
            ax2.annotate('Peak @ 0.469', xy=(0.469, 0.6), fontsize=9, color='red')
    
    plt.suptitle('Theoretical ε_col(σ_α, p): M1 vs M2\n(n₁=1.86, R_c=0.975, p_b=1/6)', 
                fontsize=16, fontweight='bold', y=0.98)
    plt.tight_layout(rect=[0, 0, 1, 0.95])
    
    # 保存
    for ext in ['png', 'pdf']:
        path = os.path.join(OUTPUT_DIR, f'theory_curves.{ext}')
        plt.savefig(path, dpi=150, bbox_inches='tight')
        print(f"保存到: {path}")
    
    plt.close()

def plot_comparison_for_validation(df):
    """生成用于验证的对比图（固定 p=0.8）"""
    
    fig, axes = plt.subplots(1, 2, figsize=(14, 6))
    
    for col, (medium, n2) in enumerate([('grease', n2_grease), ('air', n2_air)]):
        ax = axes[col]
        df_p = df[(df['medium'] == medium) & (df['p'] == 0.8)]
        
        # 绘制 M1 和 M2
        ax.fill_between(df_p['sigma_alpha'], df_p['eps_col_M2'], df_p['eps_col_M1'],
                       alpha=0.3, color='blue', label='Valid range [M2, M1]')
        ax.plot(df_p['sigma_alpha'], df_p['eps_col_M1'], 'b-', linewidth=2, 
                marker='o', markersize=8, label='M1 (Upper bound)')
        ax.plot(df_p['sigma_alpha'], df_p['eps_col_M2'], 'r-', linewidth=2, 
                marker='s', markersize=8, label='M2 (Lower bound)')
        
        ax.set_xlabel('σ_α (rad)', fontsize=12)
        ax.set_ylabel('ε_col', fontsize=12)
        ax.set_title(f'{medium.upper()} (n₂={n2}), p=0.8', fontsize=14, fontweight='bold')
        ax.legend(fontsize=10)
        ax.set_xlim([0, 1.05])
        ax.set_ylim([0, 1])
        ax.grid(True, alpha=0.3)
        
        # 添加数值标注
        for i, row in df_p.iterrows():
            ax.annotate(f'({row["eps_col_M2"]:.3f}, {row["eps_col_M1"]:.3f})',
                       xy=(row['sigma_alpha'], (row['eps_col_M1'] + row['eps_col_M2'])/2),
                       fontsize=7, ha='center', alpha=0.7)
    
    plt.suptitle('Geant4 Validation: ε_col should fall between M2 and M1\n(p=0.8, n₁=1.86, R_c=0.975)', 
                fontsize=14, fontweight='bold', y=0.98)
    plt.tight_layout(rect=[0, 0, 1, 0.93])
    
    # 保存
    for ext in ['png', 'pdf']:
        path = os.path.join(OUTPUT_DIR, f'validation_bounds.{ext}')
        plt.savefig(path, dpi=150, bbox_inches='tight')
        print(f"保存到: {path}")
    
    plt.close()

# =============================================================================
# 打印参考表
# =============================================================================

def print_reference_table(df):
    """打印参考表（对照用）"""
    
    print("\n" + "="*70)
    print("理论数值参考表 (p=0.8)")
    print("="*70)
    
    for medium in ['grease', 'air']:
        n2 = n2_grease if medium == 'grease' else n2_air
        print(f"\n### {medium.upper()} (n₂={n2})\n")
        print(f"| σ_α (rad) | M1 (上界) | M2 (下界) |")
        print(f"|-----------|-----------|-----------|")
        
        df_p = df[(df['medium'] == medium) & (df['p'] == 0.8)]
        for _, row in df_p.iterrows():
            peak = " ⬅ PEAK" if abs(row['sigma_alpha'] - 0.469) < 0.01 else ""
            print(f"| {row['sigma_alpha']:.3f} | {row['eps_col_M1']:.4f} | {row['eps_col_M2']:.4f} |{peak}")

# =============================================================================
# 主程序
# =============================================================================

def main():
    print("=" * 60)
    print("理论 ε_col(σ_α, p) 计算")
    print("=" * 60)
    
    # 生成数据表
    df = generate_theory_table()
    
    # 绘制曲线
    plot_theory_curves(df)
    plot_comparison_for_validation(df)
    
    # 打印参考表
    print_reference_table(df)
    
    print("\n" + "=" * 60)
    print("完成！")
    print("=" * 60)

if __name__ == "__main__":
    main()

