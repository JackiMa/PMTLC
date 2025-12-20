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
/// \file optical/SiPINLC/src/SiPINLCEventAction.cc
/// \brief Implementation of the SiPINLCEventAction class
//
//
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#include "SiPINLCEventAction.hh"
#include "SiPINLCRun.hh"
#include "SiPINLCStackingAction.hh"
#include "G4Event.hh"
#include "G4RunManager.hh"
#include "G4SDManager.hh"
#include "G4HCofThisEvent.hh"
#include "G4Threading.hh"
#include "G4AnalysisManager.hh"

#include "CustomScorer.hh"
#include "config.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
SiPINLCEventAction::SiPINLCEventAction()
    : G4UserEventAction()
{
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
SiPINLCEventAction::~SiPINLCEventAction() {}


G4THitsMap<G4double>* SiPINLCEventAction::GetHitsCollection(G4int hcID,
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

G4double SiPINLCEventAction::GetSum(G4THitsMap<G4double>* hitsMap) const
{
  G4double sumValue = 0.;
  for ( auto it : *hitsMap->GetMap() ) {
    // hitsMap->GetMap() returns the map of std::map<G4int, G4double*>
    sumValue += *(it.second);
  }
  return sumValue;
}


//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
void SiPINLCEventAction::BeginOfEventAction(const G4Event *)
{
  fLightCollection = 0;
  fPhotonGenerated = 0;
  fEscapeAbsorbed = 0;
  fEscapeTopAir = 0;
  fEscapeSideAir = 0;
  fEscapePTFE = 0;
  fEscapeOther = 0;
  photonHitCount.clear();
  photonPathInCrystal.clear();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
void SiPINLCEventAction::EndOfEventAction(const G4Event *event)
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

  fEdepInCrystal = G4SDManager::GetSDMpointer()->GetCollectionID(gN_sc_crystal+"/Edep");
  auto EdepInCrystal = GetSum(GetHitsCollection(fEdepInCrystal, event));
  analysisManager->FillH1(gID_H1_sc_ed, EdepInCrystal);
  analysisManager->FillH1(gID_H1_sipin_LC, fLightCollection);
  
  // === 论文所需：填充撞击次数分布直方图 ===
  for (const auto& pair : photonHitCount) {
    analysisManager->FillH1(gID_H1_sipin_hitCount, pair.second);
  }
  
  // === 论文所需：填充光子在晶体内路程分布直方图 ===
  for (const auto& pair : photonPathInCrystal) {
    analysisManager->FillH1(gID_H1_photon_pathlength, pair.second / mm);  // 转换为mm
  }
  
  // === 论文所需：填充逃逸通道统计直方图 ===
  // 通道编码: 0=被晶体吸收, 1=从顶面逃逸, 2=从侧面逃逸, 3=被PTFE吸收, 4=其他
  for (G4int i = 0; i < fEscapeAbsorbed; ++i) analysisManager->FillH1(gID_H1_escape_channel, 0);
  for (G4int i = 0; i < fEscapeTopAir; ++i) analysisManager->FillH1(gID_H1_escape_channel, 1);
  for (G4int i = 0; i < fEscapeSideAir; ++i) analysisManager->FillH1(gID_H1_escape_channel, 2);
  for (G4int i = 0; i < fEscapePTFE; ++i) analysisManager->FillH1(gID_H1_escape_channel, 3);
  for (G4int i = 0; i < fEscapeOther; ++i) analysisManager->FillH1(gID_H1_escape_channel, 4);
  
  // === 论文所需：填充每事件产生的光子数 ===
  analysisManager->FillH1(gID_H1_photon_generated, fPhotonGenerated);

  processedTrackIDs.clear(); // 清空已处理的 track ID (用于统计哪些光子进入数值孔径)
  photonHitCount.clear();    // 清空撞击次数统计
  photonPathInCrystal.clear(); // 清空路程统计
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
