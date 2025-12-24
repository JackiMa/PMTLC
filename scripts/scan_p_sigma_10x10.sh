#!/bin/bash
# 10x10 扫描：p (side/top contact ratio) × sigma_alpha (crystal UNIFIED sigmaAlpha)
# - 每个点默认 1e3 光子（铁律）
# - 记录 run_data.csv 的全部通道统计 + wall time
# - 输出一个总 CSV，便于画热力图和给理论同事复现

set -euo pipefail

ROOT="/home/wsl2/myGeant4/SiPINLC"
BUILD="$ROOT/build"
OUTDIR="$ROOT/results/p_sigma_grid_10x10"
mkdir -p "$OUTDIR"

OUTPUT_CSV="$OUTDIR/p_sigma_grid_10x10.csv"
META_TXT="$OUTDIR/meta.txt"

# ====== 可调参数（默认满足铁律） ======
N_PHOTONS="${N_PHOTONS:-1000}"
N_THREADS="${N_THREADS:-8}"
GREASE_UM="${GREASE_UM:-50}"
TOP_AIRGAP_UM="${TOP_AIRGAP_UM:-1}"
SIDE_GAP_UM="${SIDE_GAP_UM:-1}"
BOTTOM_AIRGAP_UM="${BOTTOM_AIRGAP_UM:-0}"
PDET_MODE="${PDET_MODE:-0}"

# 10 个 p 点（包含 1.0）
P_VALUES=(0.0 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 1.0)
# 10 个 sigma 点（包含 0）
SIGMA_VALUES=(0.0 0.02 0.05 0.08 0.10 0.15 0.20 0.30 0.40 0.50)

now_utc() { date -u +"%Y-%m-%dT%H:%M:%SZ"; }

echo "=== scan_p_sigma_10x10 ==="
echo "Start (UTC): $(now_utc)"
echo "N_PHOTONS=$N_PHOTONS, N_THREADS=$N_THREADS"
echo "Grease=${GREASE_UM}um, bottomAirGap=${BOTTOM_AIRGAP_UM}um, topAirGap=${TOP_AIRGAP_UM}um, sideGap=${SIDE_GAP_UM}um"
echo "pdetMode=$PDET_MODE"
echo "Output: $OUTPUT_CSV"

cat > "$META_TXT" <<EOF
scan_p_sigma_10x10
StartUTC: $(now_utc)
Workspace: $ROOT
Binary: $BUILD/SiPINLC
N_PHOTONS: $N_PHOTONS
N_THREADS: $N_THREADS
grease_um: $GREASE_UM
bottom_airgap_um: $BOTTOM_AIRGAP_UM
top_airgap_um: $TOP_AIRGAP_UM
side_gap_um: $SIDE_GAP_UM
pdetMode: $PDET_MODE
P_VALUES: ${P_VALUES[*]}
SIGMA_VALUES: ${SIGMA_VALUES[*]}
EOF

cd "$BUILD"

# CSV header：包含 run_data.csv 的全部列 + 我们额外的 p,sigma,wallTime
echo "p,sigma_rad,wallTime_s,runID,ScintPhotonCount,CherenkovPhotonCount,PhotonHitSi,eps_col,Esc_CrystalAbsorbed,Esc_TopAir,Esc_SideAir,Esc_PTFE,Esc_Grease,Esc_World,Mean_IncidenceAngle_deg,Mean_HitCount" > "$OUTPUT_CSV"

total=$(( ${#P_VALUES[@]} * ${#SIGMA_VALUES[@]} ))
idx=0

for sigma in "${SIGMA_VALUES[@]}"; do
  for p in "${P_VALUES[@]}"; do
    idx=$((idx+1))
    echo "[$idx/$total] p=$p sigma=$sigma ..."

    # macro
    cat > /tmp/p_sigma_run.mac <<EOF
/control/verbose 0
/SiPINLC/sipin/pdetMode ${PDET_MODE}
/SiPINLC/geometry/greaseThickness ${GREASE_UM} um
/SiPINLC/geometry/bottomAirGap ${BOTTOM_AIRGAP_UM} um
/SiPINLC/geometry/topAirGap ${TOP_AIRGAP_UM} um
/SiPINLC/geometry/sideGap ${SIDE_GAP_UM} um
/SiPINLC/geometry/sideContactRatio ${p}
/SiPINLC/geometry/topContactRatio ${p}
/SiPINLC/geometry/crystalSigmaAlpha ${sigma} rad
/run/initialize
/run/beamOn ${N_PHOTONS}
EOF

    rm -f run_data.csv
    rm -f /tmp/p_sigma_time.txt

    # time the run (wall time seconds)
    /usr/bin/time -f "%e" -o /tmp/p_sigma_time.txt \
      ./SiPINLC -t "${N_THREADS}" -m /tmp/p_sigma_run.mac 2>/dev/null 1>/dev/null

    wall="$(cat /tmp/p_sigma_time.txt || echo "")"
    if [ -z "$wall" ]; then wall=""; fi

    if [ ! -f run_data.csv ]; then
      echo "ERROR: run_data.csv missing for p=$p sigma=$sigma" >&2
      exit 1
    fi

    line="$(tail -1 run_data.csv | tr -d '\r')"
    echo "${p},${sigma},${wall},${line}" >> "$OUTPUT_CSV"
  done
done

echo "Done (UTC): $(now_utc)"
echo "Saved: $OUTPUT_CSV"


