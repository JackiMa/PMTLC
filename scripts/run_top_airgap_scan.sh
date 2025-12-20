#!/bin/bash
# ============================================================================
# Top Air Gap Scan Script
# 扫描范围: 0 - 500 um, 步长 50 um
# 用法: ./run_top_airgap_scan.sh [events_per_run]
# ============================================================================

EVENTS=${1:-1000000}  # 默认 1e6 事件
BUILD_DIR="/home/wsl2/myGeant4/SiPINLC/build"
MAC_DIR="/home/wsl2/myGeant4/SiPINLC/mac"

echo "=== Top Air Gap Scan ==="
echo "Events per run: $EVENTS"
echo "Started at: $(date)"
echo ""

# 清理旧数据
rm -f "$BUILD_DIR/run_data.csv"

cd "$BUILD_DIR"

# 扫描循环
run_id=0
for gap in 0.00 0.05 0.10 0.15 0.20 0.25 0.30 0.35 0.40 0.45 0.50; do
    echo "--- [$run_id] Top air gap = ${gap} mm ---"
    
    cat > "$MAC_DIR/temp_topgap.mac" << EOF
/SiPINLC/geometry/topAirGap ${gap} mm
/run/initialize
/run/beamOn ${EVENTS}
EOF
    
    ./SiPINLC -m "$MAC_DIR/temp_topgap.mac" 2>&1 | grep -E "(lightCollectionEfficiency)"
    
    rm -f "$MAC_DIR/temp_topgap.mac"
    run_id=$((run_id + 1))
    echo ""
done

echo "=== Scan Complete ==="
echo "Finished at: $(date)"
echo "Results saved in $BUILD_DIR/run_data.csv"

