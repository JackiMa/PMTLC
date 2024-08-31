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
/// \file optical/PMTLC/src/PMTLCEventAction.cc
/// \brief Implementation of the PMTLCEventAction class
//
//
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#include "PMTLCEventAction.hh"
#include "PMTLCRun.hh"
#include "PMTLCStackingAction.hh"
#include "G4Event.hh"
#include "G4RunManager.hh"
#include "G4SDManager.hh"
#include "G4HCofThisEvent.hh"
#include "G4Threading.hh"
#include "G4AnalysisManager.hh"

#include "CustomScorer.hh"
#include "config.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
PMTLCEventAction::PMTLCEventAction()
    : G4UserEventAction()
{
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
PMTLCEventAction::~PMTLCEventAction() {}


G4THitsMap<G4double>* PMTLCEventAction::GetHitsCollection(G4int hcID,
                                  const G4Event* event) const
{
  auto hitsCollection
    = static_cast<G4THitsMap<G4double>*>(
        event->GetHCofThisEvent()->GetHC(hcID));

  if ( ! hitsCollection ) {
    G4ExceptionDescription msg;
    msg << "Cannot access hitsCollection ID " << hcID;
    G4Exception("EventAction::GetHitsCollection()",
      "MyCode0003", FatalException, msg);
  }

  return hitsCollection;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4double PMTLCEventAction::GetSum(G4THitsMap<G4double>* hitsMap) const
{
  G4double sumValue = 0.;
  for ( auto it : *hitsMap->GetMap() ) {
    // hitsMap->GetMap() returns the map of std::map<G4int, G4double*>
    sumValue += *(it.second);
  }
  return sumValue;
}


//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
void PMTLCEventAction::BeginOfEventAction(const G4Event *)
{

}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
void PMTLCEventAction::EndOfEventAction(const G4Event *event)
{
  auto analysisManager = G4AnalysisManager::Instance();

  // 统计放射源的能谱
  // G4PrimaryVertex* primaryVertex = event->GetPrimaryVertex();
  // if (primaryVertex) {
  //     G4PrimaryParticle* primaryParticle = primaryVertex->GetPrimary();
  //     if (primaryParticle) {
  //         G4double energy = primaryParticle->GetKineticEnergy();
  //         analysisManager->FillNtupleDColumn(0, 0, energy);
  //     }
  // }

  // fEdepInCrystal = G4SDManager::GetSDMpointer()->GetCollectionID(gN_sc_crystal+"/Edep");
  // auto EdepInCrystal = GetSum(GetHitsCollection(fEdepInCrystal, event));
  // analysisManager->FillNtupleDColumn(0, 1, EdepInCrystal);
  // analysisManager->AddNtupleRow(0);

  processedTrackIDs.clear(); // 清空已处理的 track ID (用于统计哪些光子进入数值孔径)
    // Print per event (modulo n)
    // 判断是否是主进程，在主进程中打印进度
    if (1) {
      auto eventID = event->GetEventID();
      auto totalEvents = G4RunManager::GetRunManager()->GetCurrentRun()->GetNumberOfEventToBeProcessed();
      auto printModulo = totalEvents / 1000; // 每1%的事件数
      if ((printModulo > 0) && (eventID % printModulo == 0))
      {
          G4cout << "---> End of event: " << eventID << ", " << (eventID / printModulo /10) << "% completed" << std::endl;
      }
    }

}
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
