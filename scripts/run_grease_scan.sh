#!/bin/bash
# ============================================================================
# Grease Thickness Scan Script
# 扫描范围: 0 - 200 um, 步长 20 um
# 用法: ./run_grease_scan.sh [events_per_run]
# ============================================================================

EVENTS=${1:-1000000}  # 默认 1e6 事件
BUILD_DIR="/home/wsl2/myGeant4/SiPINLC/build"
MAC_DIR="/home/wsl2/myGeant4/SiPINLC/mac"

echo "=== Grease Thickness Scan ==="
echo "Events per run: $EVENTS"
echo "Started at: $(date)"
echo ""

# 清理旧数据
rm -f "$BUILD_DIR/run_data.csv"

# 创建临时 mac 文件
create_mac() {
    local thickness_mm=$1
    local run_id=$2
    local mac_file="$MAC_DIR/temp_grease_${thickness_mm}.mac"
    cat > "$mac_file" << EOF
# Grease scan: thickness = ${thickness_mm} mm
/SiPINLC/geometry/greaseThickness ${thickness_mm} mm
/run/initialize
/run/beamOn ${EVENTS}
EOF
    echo "$mac_file"
}

# 扫描循环
run_id=0
for thickness in 0.00 0.02 0.04 0.06 0.08 0.10 0.12 0.14 0.16 0.18 0.20; do
    echo "--- [$run_id] Running grease thickness = ${thickness} mm ---"
    
    mac_file=$(create_mac $thickness $run_id)
    
    cd "$BUILD_DIR"
    ./SiPINLC -m "$mac_file" 2>&1 | grep -E "(lightCollectionEfficiency)" 
    
    rm -f "$mac_file"
    run_id=$((run_id + 1))
    echo ""
done

echo "=== Scan Complete ==="
echo "Finished at: $(date)"
echo "Results saved in $BUILD_DIR/run_data.csv"
