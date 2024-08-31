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
/// \file PMTLC/src/PMTLCStackingAction.cc
/// \brief Implementation of the PMTLCStackingAction class
//
//
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#include "PMTLCStackingAction.hh"
#include "PMTLCRun.hh"
#include "G4ios.hh"
#include "G4OpticalPhoton.hh"
#include "G4RunManager.hh"
#include "G4Track.hh"
#include "G4VProcess.hh"
#include "G4UnitsTable.hh"
#include "G4SystemOfUnits.hh"
#include "G4AnalysisManager.hh"


#include "utilities.hh"
#include "config.hh"
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

PMTLCStackingAction::PMTLCStackingAction()
    : G4UserStackingAction(), fScintillationPhotonCount(0)
{
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
PMTLCStackingAction::~PMTLCStackingAction() {}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
G4ClassificationOfNewTrack PMTLCStackingAction::ClassifyNewTrack(
    const G4Track *aTrack)
{
  if(!g_has_cherenkov){
      if (aTrack->GetDefinition() == G4OpticalPhoton::OpticalPhotonDefinition())
    { // particle is optical photon
      if (aTrack->GetParentID() > 0)
      { // particle is secondary
        if (aTrack->GetCreatorProcess()->GetProcessName() == "Cerenkov")
        {
          return fKill; // kill the particle if it is created by Cerenkov process
        }
      }
    }
  }

  return fUrgent;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
void PMTLCStackingAction::NewStage()
{
  // 当前阶段（堆栈）处理完后执行
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
void PMTLCStackingAction::PrepareNewEvent()
{
  fScintillationPhotonCount = 0;
}
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
G4int PMTLCStackingAction::GetScintillationPhotonCount() const {
    return fScintillationPhotonCount;
}

std::vector<G4double> PMTLCStackingAction::GetScintillationWavelengths() const {
    return fScintillationWavelengths;
}