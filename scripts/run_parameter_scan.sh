#!/bin/bash
# Parameter scan script for SiPINLC simulation
# Usage: ./run_parameter_scan.sh

set -e
cd /home/wsl2/myGeant4/SiPINLC/build

PHOTONS=10000
THREADS=8
RESULTS_DIR="../results"
mkdir -p $RESULTS_DIR

echo "==============================================="
echo "SiPINLC Parameter Scan ($(date))"
echo "Photons per run: $PHOTONS"
echo "==============================================="

# ============================================================
# Scan 1: Grease thickness
# ============================================================
echo ""
echo "=== Scan 1: Grease Thickness ==="
SCAN1_FILE="$RESULTS_DIR/scan_grease_thickness.csv"
echo "grease_um,eps_col,absorbed,escaped_other,time_s" > $SCAN1_FILE

for grease in 0 10 20 50 100 200; do
    echo -n "  grease=${grease}um... "
    cat > /tmp/scan.mac << EOF
/control/verbose 0
/run/verbose 0
/tracking/verbose 0
/SiPINLC/sipin/pdetMode 0
/SiPINLC/geometry/greaseThickness ${grease} um
/SiPINLC/geometry/bottomAirGap 10 um
/SiPINLC/geometry/topAirGap 100 um
/SiPINLC/geometry/sideGap 100 um
/SiPINLC/geometry/sideContactRatio 0.0
/SiPINLC/geometry/topContactRatio 0.0
/SiPINLC/geometry/crystalSigmaAlpha 0.1 rad
/SiPINLC/sanity/wallModel 0
/run/initialize
/run/beamOn $PHOTONS
EOF
    rm -f run_data.csv
    START=$(date +%s.%N)
    timeout 300 ./SiPINLC -t $THREADS -m /tmp/scan.mac 2>&1 > /dev/null
    END=$(date +%s.%N)
    ELAPSED=$(echo "$END - $START" | bc)
    if [ -f run_data.csv ]; then
        eps=$(tail -1 run_data.csv | cut -d',' -f5)
        abs=$(tail -1 run_data.csv | cut -d',' -f6)
        oth=$(tail -1 run_data.csv | cut -d',' -f11)
        echo "${grease},${eps},${abs},${oth},${ELAPSED}" >> $SCAN1_FILE
        echo "ε_col=$eps (${ELAPSED}s)"
    else
        echo "FAILED"
    fi
done

# ============================================================
# Scan 2: Top air gap thickness
# ============================================================
echo ""
echo "=== Scan 2: Top Air Gap Thickness ==="
SCAN2_FILE="$RESULTS_DIR/scan_top_airgap.csv"
echo "top_airgap_um,eps_col,absorbed,escaped_other,time_s" > $SCAN2_FILE

for topgap in 1 10 50 100 200 500; do
    echo -n "  topAirGap=${topgap}um... "
    cat > /tmp/scan.mac << EOF
/control/verbose 0
/run/verbose 0
/tracking/verbose 0
/SiPINLC/sipin/pdetMode 0
/SiPINLC/geometry/greaseThickness 50 um
/SiPINLC/geometry/bottomAirGap 0 um
/SiPINLC/geometry/topAirGap ${topgap} um
/SiPINLC/geometry/sideGap 100 um
/SiPINLC/geometry/sideContactRatio 0.0
/SiPINLC/geometry/topContactRatio 0.0
/SiPINLC/geometry/crystalSigmaAlpha 0.1 rad
/SiPINLC/sanity/wallModel 0
/run/initialize
/run/beamOn $PHOTONS
EOF
    rm -f run_data.csv
    START=$(date +%s.%N)
    timeout 300 ./SiPINLC -t $THREADS -m /tmp/scan.mac 2>&1 > /dev/null
    END=$(date +%s.%N)
    ELAPSED=$(echo "$END - $START" | bc)
    if [ -f run_data.csv ]; then
        eps=$(tail -1 run_data.csv | cut -d',' -f5)
        abs=$(tail -1 run_data.csv | cut -d',' -f6)
        oth=$(tail -1 run_data.csv | cut -d',' -f11)
        echo "${topgap},${eps},${abs},${oth},${ELAPSED}" >> $SCAN2_FILE
        echo "ε_col=$eps (${ELAPSED}s)"
    else
        echo "FAILED"
    fi
done

# ============================================================
# Scan 3: Crystal surface roughness (sigma_alpha)
# ============================================================
echo ""
echo "=== Scan 3: Crystal Surface Roughness (sigma_alpha) ==="
SCAN3_FILE="$RESULTS_DIR/scan_sigma_alpha.csv"
echo "sigma_alpha_rad,eps_col,absorbed,escaped_other,time_s" > $SCAN3_FILE

for sigma in 0.0 0.01 0.02 0.05 0.1 0.2 0.5; do
    echo -n "  sigma_alpha=${sigma}rad... "
    cat > /tmp/scan.mac << EOF
/control/verbose 0
/run/verbose 0
/tracking/verbose 0
/SiPINLC/sipin/pdetMode 0
/SiPINLC/geometry/greaseThickness 50 um
/SiPINLC/geometry/bottomAirGap 0 um
/SiPINLC/geometry/topAirGap 100 um
/SiPINLC/geometry/sideGap 100 um
/SiPINLC/geometry/sideContactRatio 0.0
/SiPINLC/geometry/topContactRatio 0.0
/SiPINLC/geometry/crystalSigmaAlpha ${sigma} rad
/SiPINLC/sanity/wallModel 0
/run/initialize
/run/beamOn $PHOTONS
EOF
    rm -f run_data.csv
    START=$(date +%s.%N)
    timeout 300 ./SiPINLC -t $THREADS -m /tmp/scan.mac 2>&1 > /dev/null
    END=$(date +%s.%N)
    ELAPSED=$(echo "$END - $START" | bc)
    if [ -f run_data.csv ]; then
        eps=$(tail -1 run_data.csv | cut -d',' -f5)
        abs=$(tail -1 run_data.csv | cut -d',' -f6)
        oth=$(tail -1 run_data.csv | cut -d',' -f11)
        echo "${sigma},${eps},${abs},${oth},${ELAPSED}" >> $SCAN3_FILE
        echo "ε_col=$eps (${ELAPSED}s)"
    else
        echo "FAILED"
    fi
done

# ============================================================
# Scan 4: Side gap thickness (proxy for contact ratio)
# ============================================================
echo ""
echo "=== Scan 4: Side Gap Thickness ==="
SCAN4_FILE="$RESULTS_DIR/scan_side_gap.csv"
echo "side_gap_um,eps_col,absorbed,escaped_other,time_s" > $SCAN4_FILE

for sidegap in 1 10 50 100 200 500; do
    echo -n "  sideGap=${sidegap}um... "
    cat > /tmp/scan.mac << EOF
/control/verbose 0
/run/verbose 0
/tracking/verbose 0
/SiPINLC/sipin/pdetMode 0
/SiPINLC/geometry/greaseThickness 50 um
/SiPINLC/geometry/bottomAirGap 0 um
/SiPINLC/geometry/topAirGap 100 um
/SiPINLC/geometry/sideGap ${sidegap} um
/SiPINLC/geometry/sideContactRatio 0.0
/SiPINLC/geometry/topContactRatio 0.0
/SiPINLC/geometry/crystalSigmaAlpha 0.1 rad
/SiPINLC/sanity/wallModel 0
/run/initialize
/run/beamOn $PHOTONS
EOF
    rm -f run_data.csv
    START=$(date +%s.%N)
    timeout 300 ./SiPINLC -t $THREADS -m /tmp/scan.mac 2>&1 > /dev/null
    END=$(date +%s.%N)
    ELAPSED=$(echo "$END - $START" | bc)
    if [ -f run_data.csv ]; then
        eps=$(tail -1 run_data.csv | cut -d',' -f5)
        abs=$(tail -1 run_data.csv | cut -d',' -f6)
        oth=$(tail -1 run_data.csv | cut -d',' -f11)
        echo "${sidegap},${eps},${abs},${oth},${ELAPSED}" >> $SCAN4_FILE
        echo "ε_col=$eps (${ELAPSED}s)"
    else
        echo "FAILED"
    fi
done

echo ""
echo "==============================================="
echo "All scans complete! Results saved to $RESULTS_DIR/"
echo "==============================================="

