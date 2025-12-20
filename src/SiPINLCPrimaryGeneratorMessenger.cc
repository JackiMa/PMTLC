//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//
/// \file SiPINLC/src/SiPINLCPrimaryGeneratorMessenger.cc
/// \brief Implementation of the SiPINLCPrimaryGeneratorMessenger class
//
//
//
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#include "SiPINLCPrimaryGeneratorMessenger.hh"
#include "SiPINLCPrimaryGeneratorAction.hh"
#include "G4SystemOfUnits.hh"
#include "G4UIcmdWithADoubleAndUnit.hh"
#include "G4UIdirectory.hh"
#include "G4UIcmdWithABool.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

SiPINLCPrimaryGeneratorMessenger::SiPINLCPrimaryGeneratorMessenger(
  SiPINLCPrimaryGeneratorAction* SiPINLCGun)
  : G4UImessenger()
  , fSiPINLCAction(SiPINLCGun)
{
  fGunDir = new G4UIdirectory("/SiPINLC/gun/");
  fGunDir->SetGuidance("PrimaryGenerator control");

  fPolarCmd =
    new G4UIcmdWithADoubleAndUnit("/SiPINLC/gun/optPhotonPolar", this);
  fPolarCmd->SetGuidance("Set linear polarization");
  fPolarCmd->SetGuidance("  angle w.r.t. (k,n) plane");
  fPolarCmd->SetParameterName("angle", true);
  fPolarCmd->SetUnitCategory("Angle");
  fPolarCmd->SetDefaultValue(-360.0);
  fPolarCmd->SetDefaultUnit("deg");
  fPolarCmd->AvailableForStates(G4State_Idle);

    fSetUseParticleGunCmd = new G4UIcmdWithABool("/SiPINLC/generator/useParticleGun", this);
    fSetUseParticleGunCmd->SetGuidance("Set whether to use ParticleGun or GPS.");
    fSetUseParticleGunCmd->SetParameterName("useParticleGun", true);
    fSetUseParticleGunCmd->SetDefaultValue(true);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

SiPINLCPrimaryGeneratorMessenger::~SiPINLCPrimaryGeneratorMessenger()
{
  delete fPolarCmd;
  delete fGunDir;
  delete fSetUseParticleGunCmd;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void SiPINLCPrimaryGeneratorMessenger::SetNewValue(G4UIcommand* command,
                                                    G4String newValue)
{
  if(command == fPolarCmd)
  {
    G4double angle = fPolarCmd->GetNewDoubleValue(newValue);
    if(angle == -360.0 * deg)
    {
      fSiPINLCAction->SetOptPhotonPolar();
    }
    else
    {
      fSiPINLCAction->SetOptPhotonPolar(angle);
    }
  }
    if (command == fSetUseParticleGunCmd)
    {
        fSiPINLCAction->SetUseParticleGun(fSetUseParticleGunCmd->GetNewBoolValue(newValue));
    }
}
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
