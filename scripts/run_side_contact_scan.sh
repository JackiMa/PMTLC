#!/bin/bash
# ============================================================================
# Side Contact Scan Script (Statistical Equivalent Method)
# 运行两种配置：直接贴合 vs 有空气间隙
# 用法: ./run_side_contact_scan.sh [events_per_run]
# ============================================================================

EVENTS=${1:-1000000}  # 默认 1e6 事件
BUILD_DIR="/home/wsl2/myGeant4/SiPINLC/build"
MAC_DIR="/home/wsl2/myGeant4/SiPINLC/mac"

echo "=== Side Contact Scan (Statistical Equivalent Method) ==="
echo "Events per run: $EVENTS"
echo "Started at: $(date)"
echo ""

# 清理旧数据
rm -f "$BUILD_DIR/run_data.csv"

cd "$BUILD_DIR"

# Configuration 1: Direct Contact (p=1)
echo "--- [0] Direct Contact (sideGap = 0) ---"
cat > "$MAC_DIR/temp_contact.mac" << EOF
/SiPINLC/geometry/sideContact true
/run/initialize
/run/beamOn ${EVENTS}
EOF
./SiPINLC -m "$MAC_DIR/temp_contact.mac" 2>&1 | grep -E "(lightCollectionEfficiency)"
rm -f "$MAC_DIR/temp_contact.mac"
echo ""

# Configuration 2: Air Gap (p=0)
echo "--- [1] Air Gap (sideGap = 0.1 mm) ---"
cat > "$MAC_DIR/temp_airgap.mac" << EOF
/SiPINLC/geometry/sideContact false
/run/initialize
/run/beamOn ${EVENTS}
EOF
./SiPINLC -m "$MAC_DIR/temp_airgap.mac" 2>&1 | grep -E "(lightCollectionEfficiency)"
rm -f "$MAC_DIR/temp_airgap.mac"
echo ""

echo "=== Scan Complete ==="
echo "Finished at: $(date)"
echo ""
echo "Results saved in $BUILD_DIR/run_data.csv"
echo ""
echo "Post-processing formula:"
echo "  ε_col(p) = p * ε_contact + (1-p) * ε_airgap"
echo ""
echo "Example (Python):"
echo "  import pandas as pd"
echo "  df = pd.read_csv('run_data.csv')"
echo "  eps_contact = df.iloc[0]['lightCollectionEfficiency']"
echo "  eps_airgap = df.iloc[1]['lightCollectionEfficiency']"
echo "  p = 0.5  # 50% contact ratio"
echo "  eps_50 = p * eps_contact + (1-p) * eps_airgap"

