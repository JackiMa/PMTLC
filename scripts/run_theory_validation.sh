#!/bin/bash
# Geant4 理论验证扫描脚本
# 扫描 (σ_α, p) 参数空间，与闭式理论对照

set -e
cd /home/wsl2/myGeant4/SiPINLC/build

PHOTONS=10000
THREADS=8
RESULTS_DIR="../results/theory_validation"
mkdir -p $RESULTS_DIR

echo "==============================================="
echo "Geant4 理论验证扫描"
echo "时间: $(date)"
echo "光子数: $PHOTONS"
echo "==============================================="

# 参数设置
SIGMA_VALUES="0.0 0.1 0.2 0.3 0.4 0.469 0.6 1.0"
P_VALUES="0.0 0.2 0.4 0.6 0.8 1.0"

# 初始化结果文件
echo "medium,p,sigma_alpha,eps_col,hitSi,absorbed,escaped_side,escaped_top,escaped_ptfe,other" > $RESULTS_DIR/validation_results.csv

# ============================================================
# 扫描 Grease 配置
# ============================================================
echo ""
echo "=== Grease 配置扫描 ==="

for p in $P_VALUES; do
    for sigma in $SIGMA_VALUES; do
        echo -n "  p=$p, σ=$sigma... "
        
        # 生成 macro
        cat > /tmp/scan.mac << EOF
/control/verbose 0
/run/verbose 0
/tracking/verbose 0
/SiPINLC/sipin/pdetMode 0
/SiPINLC/geometry/greaseThickness 50 um
/SiPINLC/geometry/bottomAirGap 0 um
/SiPINLC/geometry/topAirGap 100 um
/SiPINLC/geometry/sideGap 100 um
/SiPINLC/geometry/sideContactRatio ${p}
/SiPINLC/geometry/topContactRatio ${p}
/SiPINLC/geometry/crystalSigmaAlpha ${sigma} rad
/SiPINLC/material/ptfeReflectivity 0.975
/SiPINLC/sanity/wallModel 0
/run/initialize
/run/beamOn $PHOTONS
EOF
        
        rm -f run_data.csv
        timeout 300 ./SiPINLC -t $THREADS -m /tmp/scan.mac 2>&1 > /dev/null
        
        if [ -f run_data.csv ]; then
            # 解析结果
            line=$(tail -1 run_data.csv)
            hitSi=$(echo $line | cut -d',' -f4)
            eps_col=$(echo $line | cut -d',' -f5)
            absorbed=$(echo $line | cut -d',' -f6)
            escaped_other=$(echo $line | cut -d',' -f11)
            
            # 估算分量（如果有详细分解的话）
            escaped_side=0
            escaped_top=0
            escaped_ptfe=0
            
            echo "grease,$p,$sigma,$eps_col,$hitSi,$absorbed,$escaped_side,$escaped_top,$escaped_ptfe,$escaped_other" >> $RESULTS_DIR/validation_results.csv
            echo "ε_col=$eps_col"
        else
            echo "FAILED"
        fi
    done
done

# ============================================================
# 扫描 Air 配置
# ============================================================
echo ""
echo "=== Air 配置扫描 ==="

for p in $P_VALUES; do
    for sigma in $SIGMA_VALUES; do
        echo -n "  p=$p, σ=$sigma... "
        
        # 生成 macro（无 grease）
        cat > /tmp/scan.mac << EOF
/control/verbose 0
/run/verbose 0
/tracking/verbose 0
/SiPINLC/sipin/pdetMode 0
/SiPINLC/geometry/greaseThickness 0 um
/SiPINLC/geometry/bottomAirGap 10 um
/SiPINLC/geometry/topAirGap 100 um
/SiPINLC/geometry/sideGap 100 um
/SiPINLC/geometry/sideContactRatio ${p}
/SiPINLC/geometry/topContactRatio ${p}
/SiPINLC/geometry/crystalSigmaAlpha ${sigma} rad
/SiPINLC/material/ptfeReflectivity 0.975
/SiPINLC/sanity/wallModel 0
/run/initialize
/run/beamOn $PHOTONS
EOF
        
        rm -f run_data.csv
        timeout 300 ./SiPINLC -t $THREADS -m /tmp/scan.mac 2>&1 > /dev/null
        
        if [ -f run_data.csv ]; then
            line=$(tail -1 run_data.csv)
            hitSi=$(echo $line | cut -d',' -f4)
            eps_col=$(echo $line | cut -d',' -f5)
            absorbed=$(echo $line | cut -d',' -f6)
            escaped_other=$(echo $line | cut -d',' -f11)
            
            escaped_side=0
            escaped_top=0
            escaped_ptfe=0
            
            echo "air,$p,$sigma,$eps_col,$hitSi,$absorbed,$escaped_side,$escaped_top,$escaped_ptfe,$escaped_other" >> $RESULTS_DIR/validation_results.csv
            echo "ε_col=$eps_col"
        else
            echo "FAILED"
        fi
    done
done

echo ""
echo "==============================================="
echo "扫描完成！"
echo "结果保存到: $RESULTS_DIR/validation_results.csv"
echo "==============================================="

