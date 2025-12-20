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
//
/// \file SiPINLCActionInitialization.cc
/// \brief Implementation of the SiPINLCActionInitialization class

#include "SiPINLCActionInitialization.hh"
#include "SiPINLCEventAction.hh"
#include "SiPINLCPrimaryGeneratorAction.hh"
#include "SiPINLCRunAction.hh"
#include "SiPINLCStackingAction.hh"
#include "SiPINLCSteppingAction.hh"
#include "SiPINLCDetectorConstruction.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
SiPINLCActionInitialization::SiPINLCActionInitialization()
  : G4VUserActionInitialization()
{}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
SiPINLCActionInitialization::~SiPINLCActionInitialization() {}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
void SiPINLCActionInitialization::BuildForMaster() const
{
  SetUserAction(new SiPINLCRunAction());
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
void SiPINLCActionInitialization::Build() const
{
  SiPINLCDetectorConstruction* detectorConstruction = new SiPINLCDetectorConstruction();
  SiPINLCPrimaryGeneratorAction* primary = new SiPINLCPrimaryGeneratorAction(detectorConstruction);
  SetUserAction(primary);
  SetUserAction(new SiPINLCRunAction(primary));
  SiPINLCEventAction* event = new SiPINLCEventAction();
  SetUserAction(event);
  SetUserAction(new SiPINLCSteppingAction(event));
  SetUserAction(new SiPINLCStackingAction());
}
