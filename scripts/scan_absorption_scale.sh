#!/bin/bash
# 自吸收长度缩放扫描脚本
# 修改 config.hh 中的 scaleFactor，重新编译并运行

set -e
cd /home/wsl2/myGeant4/SiPINLC

PHOTONS=10000
THREADS=8
RESULTS_DIR="results/absorption_scale_scan"
mkdir -p $RESULTS_DIR

echo "=== 吸收长度缩放扫描 ==="
echo "光子波长: 550nm, 光子数: $PHOTONS"
echo "时间: $(date)"
echo ""

# 初始化结果文件
echo "scale_factor,eps_col,absorbed,escaped_ptfe,escaped_other" > $RESULTS_DIR/absorption_scale_scan.csv

# 扫描 scaleFactor: 0.1, 0.2, 0.5, 1.0, 2.0, 5.0, 10.0, 20.0, 50.0, 100.0
for scale in 0.1 0.2 0.5 1.0 2.0 5.0 10.0 20.0 50.0 100.0; do
    echo "=== scaleFactor=$scale ==="
    
    # 修改 config.hh
    sed -i "s/MyMaterials::GAGG_Ce_Mg(20000, [0-9.]*,/MyMaterials::GAGG_Ce_Mg(20000, $scale,/" include/config.hh
    
    # 重新编译
    cd build
    make -j8 2>&1 | tail -3
    
    # 运行仿真
    cat > /tmp/scan.mac << EOF
/control/verbose 0
/SiPINLC/sipin/pdetMode 0
/SiPINLC/geometry/greaseThickness 50 um
/SiPINLC/geometry/bottomAirGap 0 um
/SiPINLC/geometry/topAirGap 1 um
/SiPINLC/geometry/sideGap 1 um
/SiPINLC/geometry/sideContactRatio 0.0
/SiPINLC/geometry/topContactRatio 0.0
/SiPINLC/geometry/crystalSigmaAlpha 0.1 rad
/run/initialize
/run/beamOn $PHOTONS
EOF
    
    rm -f run_data.csv
    START=$(date +%s.%N)
    ./SiPINLC -t $THREADS -m /tmp/scan.mac 2>&1 > /dev/null
    END=$(date +%s.%N)
    TIME=$(echo "$END - $START" | bc)
    
    if [ -f run_data.csv ]; then
        line=$(tail -1 run_data.csv)
        eps=$(echo $line | cut -d',' -f5)
        absorbed=$(echo $line | cut -d',' -f6)
        ptfe=$(echo $line | cut -d',' -f9)
        other=$(echo $line | cut -d',' -f10)
        
        echo "  ε_col=$eps, absorbed=$absorbed, time=${TIME}s"
        echo "$scale,$eps,$absorbed,$ptfe,$other" >> ../$RESULTS_DIR/absorption_scale_scan.csv
    fi
    
    cd ..
done

# 恢复原始 scaleFactor
sed -i "s/MyMaterials::GAGG_Ce_Mg(20000, [0-9.]*,/MyMaterials::GAGG_Ce_Mg(20000, 1,/" include/config.hh
cd build && make -j8 > /dev/null 2>&1

echo ""
echo "=== 完成！结果保存到: $RESULTS_DIR/absorption_scale_scan.csv ==="

