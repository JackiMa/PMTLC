//
// ********************************************************************
// * SiPINLCParameterMessenger.cc
// * Implementation of the SiPINLCParameterMessenger class
// ********************************************************************
//

#include "SiPINLCParameterMessenger.hh"
#include "config.hh"

#include "G4UIdirectory.hh"
#include "G4UIcmdWithADouble.hh"
#include "G4UIcmdWithADoubleAndUnit.hh"
#include "G4UIcmdWithABool.hh"
#include "G4UIcmdWithAnInteger.hh"
#include "G4UIcmdWithAString.hh"
#include "G4SystemOfUnits.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

SiPINLCParameterMessenger::SiPINLCParameterMessenger()
{
    // Main directory
    fSiPINLCDir = new G4UIdirectory("/SiPINLC/");
    fSiPINLCDir->SetGuidance("SiPINLC simulation control commands.");

    // Geometry subdirectory
    fGeometryDir = new G4UIdirectory("/SiPINLC/geometry/");
    fGeometryDir->SetGuidance("Geometry parameter control.");

    // Material subdirectory
    fMaterialDir = new G4UIdirectory("/SiPINLC/material/");
    fMaterialDir->SetGuidance("Material parameter control.");

    // SiPIN subdirectory (P_det / TMM boundary)
    fSipinDir = new G4UIdirectory("/SiPINLC/sipin/");
    fSipinDir->SetGuidance("SiPIN boundary (P_det) model control.");

    // === Geometry Commands ===
    
    // Grease thickness
    fGreaseThicknessCmd = new G4UIcmdWithADoubleAndUnit("/SiPINLC/geometry/greaseThickness", this);
    fGreaseThicknessCmd->SetGuidance("Set optical grease layer thickness.");
    fGreaseThicknessCmd->SetParameterName("thickness", false);
    fGreaseThicknessCmd->SetUnitCategory("Length");
    fGreaseThicknessCmd->SetDefaultUnit("mm");
    fGreaseThicknessCmd->SetRange("thickness>=0.");
    fGreaseThicknessCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

    // Bottom air gap (when no grease)
    fBottomAirGapCmd = new G4UIcmdWithADoubleAndUnit("/SiPINLC/geometry/bottomAirGap", this);
    fBottomAirGapCmd->SetGuidance("Set bottom air gap thickness between crystal and SiPIN when greaseThickness=0.");
    fBottomAirGapCmd->SetParameterName("thickness", false);
    fBottomAirGapCmd->SetUnitCategory("Length");
    fBottomAirGapCmd->SetDefaultUnit("um");
    fBottomAirGapCmd->SetRange("thickness>=0.");
    fBottomAirGapCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

    // Top air gap
    fTopAirGapCmd = new G4UIcmdWithADoubleAndUnit("/SiPINLC/geometry/topAirGap", this);
    fTopAirGapCmd->SetGuidance("Set top air gap thickness between crystal and PTFE.");
    fTopAirGapCmd->SetParameterName("thickness", false);
    fTopAirGapCmd->SetUnitCategory("Length");
    fTopAirGapCmd->SetDefaultUnit("mm");
    fTopAirGapCmd->SetRange("thickness>=0.");
    fTopAirGapCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

    // Side gap
    fSideGapCmd = new G4UIcmdWithADoubleAndUnit("/SiPINLC/geometry/sideGap", this);
    fSideGapCmd->SetGuidance("Set side air gap thickness.");
    fSideGapCmd->SetParameterName("thickness", false);
    fSideGapCmd->SetUnitCategory("Length");
    fSideGapCmd->SetDefaultUnit("mm");
    fSideGapCmd->SetRange("thickness>=0.");
    fSideGapCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

    // Side contact mode (for statistical equivalent method)
    fSideContactCmd = new G4UIcmdWithABool("/SiPINLC/geometry/sideContact", this);
    fSideContactCmd->SetGuidance("Set side contact mode: true=direct contact, false=with air gap.");
    fSideContactCmd->SetGuidance("Use statistical equivalent method: run both and weight results.");
    fSideContactCmd->SetParameterName("contact", false);
    fSideContactCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

    // Side contact ratio (probabilistic boundary method)
    // This does NOT change geometry; it changes side-boundary handling in stepping.
    fSideContactRatioCmd = new G4UIcmdWithADouble("/SiPINLC/geometry/sideContactRatio", this);
    fSideContactRatioCmd->SetGuidance("Set side contact ratio p in [0,1] for probabilistic boundary method.");
    fSideContactRatioCmd->SetGuidance("When an optical photon hits a CRYSTAL SIDE surface: with probability p treat it as 'crystal-PTFE contact';");
    fSideContactRatioCmd->SetGuidance("with probability 1-p treat it as 'crystal-air gap' (default Fresnel/TIR).");
    fSideContactRatioCmd->SetParameterName("p", false);
    fSideContactRatioCmd->SetRange("p>=0. && p<=1.");
    fSideContactRatioCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

    // === Material Commands ===
    
    // Absorption scale factor
    fAbsorptionScaleCmd = new G4UIcmdWithADouble("/SiPINLC/material/absorptionScale", this);
    fAbsorptionScaleCmd->SetGuidance("Scale factor for crystal absorption length.");
    fAbsorptionScaleCmd->SetGuidance("1.0 = nominal, 0.5 = double absorption, 2.0 = half absorption");
    fAbsorptionScaleCmd->SetParameterName("scale", false);
    fAbsorptionScaleCmd->SetRange("scale>0.");
    fAbsorptionScaleCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

    // Effective absorption length (for uniform absorption approximation)
    fEffectiveAbsLengthCmd = new G4UIcmdWithADoubleAndUnit("/SiPINLC/material/effectiveAbsLength", this);
    fEffectiveAbsLengthCmd->SetGuidance("Set effective (uniform) absorption length for crystal.");
    fEffectiveAbsLengthCmd->SetGuidance("Set to -1 to use wavelength-dependent absorption.");
    fEffectiveAbsLengthCmd->SetParameterName("length", false);
    fEffectiveAbsLengthCmd->SetUnitCategory("Length");
    fEffectiveAbsLengthCmd->SetDefaultUnit("mm");
    fEffectiveAbsLengthCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

    // PTFE reflectivity
    fPTFEReflectivityCmd = new G4UIcmdWithADouble("/SiPINLC/material/ptfeReflectivity", this);
    fPTFEReflectivityCmd->SetGuidance("Set PTFE reflectivity (0.0 to 1.0).");
    fPTFEReflectivityCmd->SetParameterName("reflectivity", false);
    fPTFEReflectivityCmd->SetRange("reflectivity>=0. && reflectivity<=1.");
    fPTFEReflectivityCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

    // === SiPIN P_det model ===
    fSipinPdetModeCmd = new G4UIcmdWithAnInteger("/SiPINLC/sipin/pdetMode", this);
    fSipinPdetModeCmd->SetGuidance("Set SiPIN P_det mode: 0=off(legacy), 1=const, 2=csv-table.");
    fSipinPdetModeCmd->SetParameterName("mode", false);
    fSipinPdetModeCmd->SetRange("mode>=0 && mode<=2");
    fSipinPdetModeCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

    fSipinPdetConstCmd = new G4UIcmdWithADouble("/SiPINLC/sipin/pdetConst", this);
    fSipinPdetConstCmd->SetGuidance("Set constant P_det used when pdetMode=1.");
    fSipinPdetConstCmd->SetParameterName("pdet", false);
    fSipinPdetConstCmd->SetRange("pdet>=0. && pdet<=1.");
    fSipinPdetConstCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

    fSipinPdetFileCmd = new G4UIcmdWithAString("/SiPINLC/sipin/pdetFile", this);
    fSipinPdetFileCmd->SetGuidance("Set CSV file path for P_det(λ_nm, θ_deg) when pdetMode=2.");
    fSipinPdetFileCmd->SetParameterName("path", false);
    fSipinPdetFileCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

    fSipinMaxInterfaceHitsCmd = new G4UIcmdWithAnInteger("/SiPINLC/sipin/maxInterfaceHits", this);
    fSipinMaxInterfaceHitsCmd->SetGuidance("Safety cap: maximum number of grease/airgap -> Si interface hits per photon.");
    fSipinMaxInterfaceHitsCmd->SetGuidance("Used to prevent extremely long runs when p_det is small (manual reflection model).");
    fSipinMaxInterfaceHitsCmd->SetParameterName("n", false);
    fSipinMaxInterfaceHitsCmd->SetRange("n>=0");
    fSipinMaxInterfaceHitsCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

SiPINLCParameterMessenger::~SiPINLCParameterMessenger()
{
    delete fGreaseThicknessCmd;
    delete fBottomAirGapCmd;
    delete fTopAirGapCmd;
    delete fSideGapCmd;
    delete fSideContactCmd;
    delete fSideContactRatioCmd;
    delete fAbsorptionScaleCmd;
    delete fEffectiveAbsLengthCmd;
    delete fPTFEReflectivityCmd;
    delete fSipinPdetModeCmd;
    delete fSipinPdetConstCmd;
    delete fSipinPdetFileCmd;
    delete fSipinMaxInterfaceHitsCmd;
    delete fGeometryDir;
    delete fMaterialDir;
    delete fSipinDir;
    delete fSiPINLCDir;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void SiPINLCParameterMessenger::SetNewValue(G4UIcommand* command, G4String newValue)
{
    // Geometry parameters
    if (command == fGreaseThicknessCmd) {
        g_grease_thickness = fGreaseThicknessCmd->GetNewDoubleValue(newValue);
        G4cout << "=== Parameter Update ===" << G4endl;
        G4cout << "Grease thickness set to: " << g_grease_thickness/um << " um" << G4endl;
    }
    else if (command == fBottomAirGapCmd) {
        g_bottom_airgap_thickness = fBottomAirGapCmd->GetNewDoubleValue(newValue);
        G4cout << "=== Parameter Update ===" << G4endl;
        G4cout << "Bottom air gap set to: " << g_bottom_airgap_thickness/um << " um" << G4endl;
    }
    else if (command == fTopAirGapCmd) {
        g_top_airgap_thickness = fTopAirGapCmd->GetNewDoubleValue(newValue);
        G4cout << "=== Parameter Update ===" << G4endl;
        G4cout << "Top air gap set to: " << g_top_airgap_thickness/um << " um" << G4endl;
    }
    else if (command == fSideGapCmd) {
        g_gap_thickness = fSideGapCmd->GetNewDoubleValue(newValue);
        G4cout << "=== Parameter Update ===" << G4endl;
        G4cout << "Side gap set to: " << g_gap_thickness/um << " um" << G4endl;
    }
    else if (command == fSideContactCmd) {
        G4bool contact = fSideContactCmd->GetNewBoolValue(newValue);
        // 统计等效法：contact=true时设置gap=0，contact=false时使用默认gap
        if (contact) {
            g_gap_thickness = 0.0;
            G4cout << "=== Parameter Update ===" << G4endl;
            G4cout << "Side contact mode: DIRECT CONTACT (gap=0)" << G4endl;
        } else {
            g_gap_thickness = 0.1*mm;  // 默认空气层厚度
            G4cout << "=== Parameter Update ===" << G4endl;
            G4cout << "Side contact mode: AIR GAP (gap=" << g_gap_thickness/um << " um)" << G4endl;
        }
    }
    else if (command == fSideContactRatioCmd) {
        g_side_contact_ratio = fSideContactRatioCmd->GetNewDoubleValue(newValue);
        G4cout << "=== Parameter Update ===" << G4endl;
        G4cout << "Side contact ratio (probabilistic): " << g_side_contact_ratio << G4endl;
        G4cout << "NOTE: Takes effect immediately (stepping logic), no geometry rebuild needed." << G4endl;
    }
    // Material parameters
    else if (command == fAbsorptionScaleCmd) {
        // 这个需要在材料重建时使用
        G4double scale = fAbsorptionScaleCmd->GetNewDoubleValue(newValue);
        G4cout << "=== Parameter Update ===" << G4endl;
        G4cout << "Absorption scale factor: " << scale << G4endl;
        G4cout << "NOTE: Requires geometry rebuild (/run/reinitializeGeometry)" << G4endl;
    }
    else if (command == fEffectiveAbsLengthCmd) {
        G4double length = fEffectiveAbsLengthCmd->GetNewDoubleValue(newValue);
        G4cout << "=== Parameter Update ===" << G4endl;
        G4cout << "Effective absorption length: " << length/mm << " mm" << G4endl;
        G4cout << "NOTE: Requires geometry rebuild (/run/reinitializeGeometry)" << G4endl;
    }
    else if (command == fPTFEReflectivityCmd) {
        G4double refl = fPTFEReflectivityCmd->GetNewDoubleValue(newValue);
        G4cout << "=== Parameter Update ===" << G4endl;
        G4cout << "PTFE reflectivity: " << refl*100 << "%" << G4endl;
        G4cout << "NOTE: Requires geometry rebuild (/run/reinitializeGeometry)" << G4endl;
    }
    // SiPIN P_det model
    else if (command == fSipinPdetModeCmd) {
        g_sipin_pdet_mode = fSipinPdetModeCmd->GetNewIntValue(newValue);
        G4cout << "=== Parameter Update ===" << G4endl;
        G4cout << "SiPIN P_det mode set to: " << g_sipin_pdet_mode << " (0=off,1=const,2=csv)" << G4endl;
    }
    else if (command == fSipinPdetConstCmd) {
        g_sipin_pdet_const = fSipinPdetConstCmd->GetNewDoubleValue(newValue);
        G4cout << "=== Parameter Update ===" << G4endl;
        G4cout << "SiPIN P_det const set to: " << g_sipin_pdet_const << G4endl;
    }
    else if (command == fSipinPdetFileCmd) {
        g_sipin_pdet_csv = newValue;
        G4cout << "=== Parameter Update ===" << G4endl;
        G4cout << "SiPIN P_det CSV file set to: " << g_sipin_pdet_csv << G4endl;
    }
    else if (command == fSipinMaxInterfaceHitsCmd) {
        g_sipin_max_interface_hits = fSipinMaxInterfaceHitsCmd->GetNewIntValue(newValue);
        G4cout << "=== Parameter Update ===" << G4endl;
        G4cout << "SiPIN max interface hits set to: " << g_sipin_max_interface_hits << G4endl;
    }
}

