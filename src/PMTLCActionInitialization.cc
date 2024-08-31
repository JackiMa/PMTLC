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
/// \file PMTLCActionInitialization.cc
/// \brief Implementation of the PMTLCActionInitialization class

#include "PMTLCActionInitialization.hh"
#include "PMTLCEventAction.hh"
#include "PMTLCPrimaryGeneratorAction.hh"
#include "PMTLCRunAction.hh"
#include "PMTLCStackingAction.hh"
#include "PMTLCSteppingAction.hh"
#include "PMTLCDetectorConstruction.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
PMTLCActionInitialization::PMTLCActionInitialization()
  : G4VUserActionInitialization()
{}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
PMTLCActionInitialization::~PMTLCActionInitialization() {}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
void PMTLCActionInitialization::BuildForMaster() const
{
  SetUserAction(new PMTLCRunAction());
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
void PMTLCActionInitialization::Build() const
{
  PMTLCDetectorConstruction* detectorConstruction = new PMTLCDetectorConstruction();
  PMTLCPrimaryGeneratorAction* primary = new PMTLCPrimaryGeneratorAction(detectorConstruction);
  SetUserAction(primary);
  SetUserAction(new PMTLCRunAction(primary));
  PMTLCEventAction* event = new PMTLCEventAction();
  SetUserAction(event);
  SetUserAction(new PMTLCSteppingAction(event));
  SetUserAction(new PMTLCStackingAction());
}
