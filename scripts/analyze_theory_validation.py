#!/usr/bin/env python3
"""
分析 Geant4 理论验证结果，与闭式理论对比
"""

import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
import os

# 目录
RESULTS_DIR = '/home/wsl2/myGeant4/SiPINLC/results/theory_validation'
THEORY_DIR = '/home/wsl2/myGeant4/SiPINLC/results/theory'
OUTPUT_DIR = '/home/wsl2/myGeant4/SiPINLC/docs/figures'
os.makedirs(OUTPUT_DIR, exist_ok=True)

def load_data():
    """加载 Geant4 和理论数据"""
    g4_path = os.path.join(RESULTS_DIR, 'validation_results.csv')
    theory_path = os.path.join(THEORY_DIR, 'theory_epsilon_col.csv')
    
    df_g4 = pd.read_csv(g4_path) if os.path.exists(g4_path) else None
    df_theory = pd.read_csv(theory_path) if os.path.exists(theory_path) else None
    
    return df_g4, df_theory

def plot_validation(df_g4, df_theory):
    """绘制验证对比图"""
    
    if df_g4 is None:
        print("警告：Geant4 数据未找到")
        return
    
    fig, axes = plt.subplots(2, 2, figsize=(16, 14))
    
    for row, medium in enumerate(['grease', 'air']):
        n2 = 1.46 if medium == 'grease' else 1.0
        df_g4_m = df_g4[df_g4['medium'] == medium]
        df_th_m = df_theory[df_theory['medium'] == medium] if df_theory is not None else None
        
        # 左图：p=0.8 的详细对比
        ax1 = axes[row, 0]
        
        if df_th_m is not None:
            df_th_p = df_th_m[df_th_m['p'] == 0.8]
            ax1.fill_between(df_th_p['sigma_alpha'], df_th_p['eps_col_M2'], df_th_p['eps_col_M1'],
                           alpha=0.3, color='blue', label='Theory [M2, M1]')
            ax1.plot(df_th_p['sigma_alpha'], df_th_p['eps_col_M1'], 'b--', linewidth=2, label='M1 (Upper)')
            ax1.plot(df_th_p['sigma_alpha'], df_th_p['eps_col_M2'], 'r--', linewidth=2, label='M2 (Lower)')
        
        df_g4_p = df_g4_m[df_g4_m['p'] == 0.8]
        if len(df_g4_p) > 0:
            ax1.plot(df_g4_p['sigma_alpha'], df_g4_p['eps_col'], 'ko-', 
                    markersize=10, linewidth=2, label='Geant4')
            
            # 检查是否在范围内
            for _, row_data in df_g4_p.iterrows():
                sigma = row_data['sigma_alpha']
                eps = row_data['eps_col']
                
                if df_th_m is not None:
                    th_row = df_th_m[(df_th_m['p'] == 0.8) & (abs(df_th_m['sigma_alpha'] - sigma) < 0.01)]
                    if len(th_row) > 0:
                        M1 = th_row['eps_col_M1'].values[0]
                        M2 = th_row['eps_col_M2'].values[0]
                        
                        if eps < M2 - 0.02:
                            ax1.annotate('LOW!', xy=(sigma, eps), fontsize=8, color='red')
                        elif eps > M1 + 0.02:
                            ax1.annotate('HIGH!', xy=(sigma, eps), fontsize=8, color='red')
        
        ax1.set_xlabel('σ_α (rad)', fontsize=12)
        ax1.set_ylabel('ε_col', fontsize=12)
        ax1.set_title(f'{medium.upper()} (n₂={n2}), p=0.8: Theory vs Geant4', fontsize=14, fontweight='bold')
        ax1.legend(fontsize=10)
        ax1.set_xlim([0, 1.05])
        ax1.set_ylim([0, 1])
        ax1.grid(True, alpha=0.3)
        
        # 右图：所有 p 值的 Geant4 结果
        ax2 = axes[row, 1]
        colors = plt.cm.viridis(np.linspace(0.2, 0.8, 6))
        
        for i, p in enumerate([0.0, 0.2, 0.4, 0.6, 0.8, 1.0]):
            df_p = df_g4_m[df_g4_m['p'] == p]
            if len(df_p) > 0:
                ax2.plot(df_p['sigma_alpha'], df_p['eps_col'], 'o-', 
                        color=colors[i], linewidth=2, markersize=6, label=f'p={p}')
        
        ax2.set_xlabel('σ_α (rad)', fontsize=12)
        ax2.set_ylabel('ε_col', fontsize=12)
        ax2.set_title(f'{medium.upper()}: Geant4 ε_col(σ, p)', fontsize=14, fontweight='bold')
        ax2.legend(fontsize=9)
        ax2.set_xlim([0, 1.05])
        ax2.set_ylim([0, 1])
        ax2.grid(True, alpha=0.3)
    
    plt.suptitle('Theory Validation: Geant4 vs Closed-form Model\n(n₁=1.86, R_c=0.975, p_b=1/6)', 
                fontsize=16, fontweight='bold', y=0.98)
    plt.tight_layout(rect=[0, 0, 1, 0.95])
    
    # 保存
    for ext in ['png', 'pdf']:
        path = os.path.join(OUTPUT_DIR, f'theory_validation_comparison.{ext}')
        plt.savefig(path, dpi=150, bbox_inches='tight')
        print(f"保存到: {path}")
    
    plt.close()

def print_comparison_table(df_g4, df_theory):
    """打印对比表"""
    
    if df_g4 is None:
        print("警告：Geant4 数据未找到")
        return
    
    print("\n" + "=" * 90)
    print("理论 vs Geant4 对比表 (p=0.8)")
    print("=" * 90)
    
    for medium in ['grease', 'air']:
        n2 = 1.46 if medium == 'grease' else 1.0
        print(f"\n### {medium.upper()} (n₂={n2})\n")
        print(f"| σ_α | G4 ε_col | M2 (下界) | M1 (上界) | 状态 |")
        print(f"|-----|----------|-----------|-----------|------|")
        
        df_g4_p = df_g4[(df_g4['medium'] == medium) & (df_g4['p'] == 0.8)]
        
        for _, row in df_g4_p.iterrows():
            sigma = row['sigma_alpha']
            eps_g4 = row['eps_col']
            
            # 查找理论值
            if df_theory is not None:
                th_row = df_theory[(df_theory['medium'] == medium) & 
                                   (df_theory['p'] == 0.8) & 
                                   (abs(df_theory['sigma_alpha'] - sigma) < 0.01)]
                if len(th_row) > 0:
                    M1 = th_row['eps_col_M1'].values[0]
                    M2 = th_row['eps_col_M2'].values[0]
                    
                    if eps_g4 < M2 - 0.02:
                        status = "⚠️ LOW"
                    elif eps_g4 > M1 + 0.02:
                        status = "⚠️ HIGH"
                    else:
                        status = "✅ OK"
                    
                    print(f"| {sigma:.3f} | {eps_g4:.4f} | {M2:.4f} | {M1:.4f} | {status} |")
                else:
                    print(f"| {sigma:.3f} | {eps_g4:.4f} | N/A | N/A | ? |")
            else:
                print(f"| {sigma:.3f} | {eps_g4:.4f} | N/A | N/A | ? |")

def main():
    print("=" * 60)
    print("理论验证分析")
    print("=" * 60)
    
    df_g4, df_theory = load_data()
    
    if df_g4 is not None:
        print(f"Geant4 数据: {len(df_g4)} 行")
        plot_validation(df_g4, df_theory)
        print_comparison_table(df_g4, df_theory)
    else:
        print("警告：请先运行 run_theory_validation.sh 生成 Geant4 数据")
    
    print("\n" + "=" * 60)
    print("完成！")
    print("=" * 60)

if __name__ == "__main__":
    main()

