#!/bin/bash
# 二维扫描：p (PTFE贴合比例) × sigma_alpha (晶体表面粗糙度)
# 输出 CSV 格式结果

cd /home/wsl2/myGeant4/SiPINLC/build

OUTPUT_FILE="/home/wsl2/myGeant4/SiPINLC/results/p_sigma_scan.csv"

# 参数范围
P_VALUES="0.0 0.2 0.4 0.6 0.8 1.0"
SIGMA_VALUES="0.01 0.05 0.1 0.2 0.3 0.5"

# 光子数
N_PHOTONS=10000

echo "=== 二维扫描：p × sigma_alpha ==="
echo "P values: $P_VALUES"
echo "Sigma values: $SIGMA_VALUES"
echo "N photons: $N_PHOTONS"
echo ""

# 写入 CSV 头
echo "p,sigma_rad,eps_col,absorbed,ptfe,grease,world" > "$OUTPUT_FILE"

total=$(( $(echo $P_VALUES | wc -w) * $(echo $SIGMA_VALUES | wc -w) ))
count=0

for sigma in $SIGMA_VALUES; do
    for p in $P_VALUES; do
        count=$((count + 1))
        echo "[$count/$total] Running p=$p, sigma=$sigma rad..."
        
        cat > /tmp/scan.mac << EOF
/control/verbose 0
/SiPINLC/sipin/pdetMode 0
/SiPINLC/geometry/greaseThickness 50 um
/SiPINLC/geometry/bottomAirGap 0 um
/SiPINLC/geometry/topAirGap 1 um
/SiPINLC/geometry/sideGap 1 um
/SiPINLC/geometry/sideContactRatio ${p}
/SiPINLC/geometry/topContactRatio ${p}
/SiPINLC/geometry/crystalSigmaAlpha ${sigma} rad
/run/initialize
/run/beamOn ${N_PHOTONS}
EOF
        
        rm -f run_data.csv
        ./SiPINLC -t 8 -m /tmp/scan.mac 2>&1 > /dev/null
        
        if [ -f run_data.csv ]; then
            line=$(tail -1 run_data.csv)
            eps=$(echo $line | cut -d',' -f5)
            absorbed=$(echo $line | cut -d',' -f6)
            ptfe=$(echo $line | cut -d',' -f9)
            grease=$(echo $line | cut -d',' -f10)
            world=$(echo $line | cut -d',' -f11)
            echo "$p,$sigma,$eps,$absorbed,$ptfe,$grease,$world" >> "$OUTPUT_FILE"
            echo "  -> eps_col = $eps"
        else
            echo "  -> FAILED"
        fi
    done
done

echo ""
echo "=== 扫描完成 ==="
echo "结果保存到: $OUTPUT_FILE"
cat "$OUTPUT_FILE"

