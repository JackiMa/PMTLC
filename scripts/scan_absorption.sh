#!/bin/bash
# 自吸收长度扫描脚本
# 扫描吸收长度缩放因子，观察对 ε_col 的影响

set -e
cd /home/wsl2/myGeant4/SiPINLC/build

PHOTONS=10000
THREADS=8
RESULTS_DIR="../results/absorption_scan"
mkdir -p $RESULTS_DIR

echo "==============================================="
echo "自吸收长度扫描"
echo "时间: $(date)"
echo "==============================================="

# 基准配置：550nm 单色光，sigma_alpha=0.1
# 扫描 scaleFactor: 0.1 到 100

# 注意：scaleFactor 在 config.hh 中编译时设置
# 需要通过重新编译来改变，或者使用 MAC 命令
# 检查是否有 absorptionScale MAC 命令

echo ""
echo "=== 使用 550nm 固定波长光子 ==="
echo ""

# 初始化结果文件
echo "medium,scale_factor,effective_L_abs_mm,eps_col,hitSi,absorbed,escaped_other" > $RESULTS_DIR/absorption_scan.csv

# 使用 Grease 配置，扫描不同的 sigma_alpha
# 通过比较 σ=0 和 σ>0 的差异来理解自吸收效应

for sigma in 0.0 0.1 0.2 0.5 1.0; do
    echo ""
    echo "=== Grease, sigma_alpha=$sigma ==="
    
    cat > /tmp/scan.mac << EOF
/control/verbose 0
/SiPINLC/sipin/pdetMode 0
/SiPINLC/geometry/greaseThickness 50 um
/SiPINLC/geometry/crystalSigmaAlpha $sigma rad
/run/initialize
/run/beamOn $PHOTONS
EOF
    
    rm -f run_data.csv
    ./SiPINLC -t $THREADS -m /tmp/scan.mac 2>&1 > /dev/null
    
    if [ -f run_data.csv ]; then
        line=$(tail -1 run_data.csv)
        eps=$(echo $line | cut -d',' -f5)
        absorbed=$(echo $line | cut -d',' -f6)
        other=$(echo $line | cut -d',' -f10)
        
        # 计算吸收比例
        total=10000
        abs_frac=$(echo "scale=4; $absorbed / $total" | bc)
        
        echo "  ε_col=$eps, absorbed=$absorbed ($abs_frac), other=$other"
        echo "grease,1.0,$sigma,$eps,$absorbed,$abs_frac,$other" >> $RESULTS_DIR/absorption_scan.csv
    fi
done

echo ""
echo "==============================================="
echo "完成！结果保存到: $RESULTS_DIR/absorption_scan.csv"
echo "==============================================="

# 理论分析：平均自吸收长度
echo ""
echo "=== 平均自吸收长度理论分析 ==="
echo ""
echo "GAGG 发射峰 ~530nm (2.34 eV)"
echo "对应吸收长度（从材料表）："
echo "  2.34 eV: L_abs ≈ 300-450 mm"
echo ""
echo "5x5x5 mm 晶体的平均光程："
echo "  - 直接到底面: ~2.5 mm"
echo "  - 经过 N 次反射: ~N * 5 mm"
echo ""
echo "平均光程估计："
echo "  - σ=0 (困光): >>100 mm (大量 TIR)"
echo "  - σ=0.1: ~10-20 mm"
echo ""
echo "吸收概率 = 1 - exp(-L_path / L_abs)"
echo "  - L_path=10mm, L_abs=400mm: P_abs = 2.5%"
echo "  - L_path=50mm, L_abs=400mm: P_abs = 11.8%"
echo "  - L_path=100mm, L_abs=400mm: P_abs = 22.1%"

