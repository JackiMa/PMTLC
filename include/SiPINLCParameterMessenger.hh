//
// ********************************************************************
// * SiPINLCParameterMessenger.hh
// * Messenger class for controlling simulation parameters via MAC
// ********************************************************************
//

#ifndef SiPINLCParameterMessenger_h
#define SiPINLCParameterMessenger_h 1

#include "G4UImessenger.hh"
#include "globals.hh"

class G4UIdirectory;
class G4UIcmdWithADouble;
class G4UIcmdWithADoubleAndUnit;
class G4UIcmdWithABool;
class G4UIcmdWithAnInteger;
class G4UIcmdWithAString;

/// Messenger class for controlling simulation parameters
/// Allows parameter scanning via MAC scripts

class SiPINLCParameterMessenger : public G4UImessenger
{
public:
    SiPINLCParameterMessenger();
    ~SiPINLCParameterMessenger() override;

    void SetNewValue(G4UIcommand*, G4String) override;

private:
    // Directory
    G4UIdirectory* fSiPINLCDir;
    G4UIdirectory* fGeometryDir;
    G4UIdirectory* fMaterialDir;
    G4UIdirectory* fSipinDir;

    // Geometry commands
    G4UIcmdWithADoubleAndUnit* fGreaseThicknessCmd;
    G4UIcmdWithADoubleAndUnit* fBottomAirGapCmd;
    G4UIcmdWithADoubleAndUnit* fTopAirGapCmd;
    G4UIcmdWithADoubleAndUnit* fSideGapCmd;
    G4UIcmdWithABool* fSideContactCmd;  // true=贴合, false=有空气层
    G4UIcmdWithADouble* fSideContactRatioCmd; // 0~1 概率边界法（贴合比例）
    
    // Material commands
    G4UIcmdWithADouble* fAbsorptionScaleCmd;  // 晶体吸收长度缩放系数
    G4UIcmdWithADoubleAndUnit* fEffectiveAbsLengthCmd;  // 等效吸收长度
    G4UIcmdWithADouble* fPTFEReflectivityCmd;  // PTFE反射率

    // SiPIN PDE/TMM boundary model
    G4UIcmdWithAnInteger* fSipinPdetModeCmd;     // 0/1/2
    G4UIcmdWithADouble*   fSipinPdetConstCmd;    // const p_det
    G4UIcmdWithAString*   fSipinPdetFileCmd;     // CSV file
    G4UIcmdWithAnInteger* fSipinMaxInterfaceHitsCmd; // safety cap
};

#endif

