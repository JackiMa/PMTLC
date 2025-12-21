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
#include "SiPINLCRunStats.hh"
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
  fCherenkovGenerated = 0;
  fEscapeAbsorbed = 0;
  fEscapeTopAir = 0;
  fEscapeSideAir = 0;
  fEscapePTFE = 0;
  fEscapeOther = 0;
  fThetaSumDeg = 0.0;
  fThetaCount = 0;
  fWallHitCount = 0;
  processedTrackIDs.clear();
  photonHitCount.clear();
  photonPathInCrystal.clear();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
void SiPINLCEventAction::EndOfEventAction(const G4Event *event)
{
  // For analytic sanity-check runs, skip ROOT histogram filling (MT stability),
  // but still accumulate run-level counters for run_data.csv.
  auto analysisManager = (g_sanity_wall_model == SANITY_WALL_OFF) ? G4AnalysisManager::Instance() : nullptr;

  // 统计放射源的能谱
  // G4PrimaryVertex* primaryVertex = event->GetPrimaryVertex();
  // if (primaryVertex) {
  //     G4PrimaryParticle* primaryParticle = primaryVertex->GetPrimary();
  //     if (primaryParticle) {
  //         G4double energy = primaryParticle->GetKineticEnergy();
  //         analysisManager->FillNtupleDColumn(0, 0, energy);
  //     }
  // }

  if (analysisManager)
  {
    fEdepInCrystal = G4SDManager::GetSDMpointer()->GetCollectionID(gN_sc_crystal+"/Edep");
    auto EdepInCrystal = GetSum(GetHitsCollection(fEdepInCrystal, event));
    analysisManager->FillH1(gID_H1_sc_ed, EdepInCrystal);
    analysisManager->FillH1(gID_H1_sipin_LC, fLightCollection);
  }
  
  // === 论文所需：填充撞击次数分布直方图 ===
  if (analysisManager) {
    for (const auto& pair : photonHitCount) {
      analysisManager->FillH1(gID_H1_sipin_hitCount, pair.second);
    }
  }
  
  // === 论文所需：填充光子在晶体内路程分布直方图 ===
  if (analysisManager) {
    for (const auto& pair : photonPathInCrystal) {
      analysisManager->FillH1(gID_H1_photon_pathlength, pair.second / mm);  // 转换为mm
    }
  }
  
  // === 论文所需：填充逃逸通道统计直方图 ===
  // 通道编码: 0=被晶体吸收, 1=从顶面逃逸, 2=从侧面逃逸, 3=被PTFE吸收, 4=其他
  if (analysisManager) {
    if (fEscapeAbsorbed > 0) analysisManager->FillH1(gID_H1_escape_channel, 0, fEscapeAbsorbed);
    if (fEscapeTopAir > 0) analysisManager->FillH1(gID_H1_escape_channel, 1, fEscapeTopAir);
    if (fEscapeSideAir > 0) analysisManager->FillH1(gID_H1_escape_channel, 2, fEscapeSideAir);
    if (fEscapePTFE > 0) analysisManager->FillH1(gID_H1_escape_channel, 3, fEscapePTFE);
    if (fEscapeOther > 0) analysisManager->FillH1(gID_H1_escape_channel, 4, fEscapeOther);
  }
  
  // === 论文所需：填充每事件产生的光子数 ===
  if (analysisManager) {
    analysisManager->FillH1(gID_H1_photon_generated, fPhotonGenerated);
  }

  // === Run 级别统计（用于 MT 下可靠输出 run_data.csv）===
  // - 事件数：每事件 +1
  // - 光子计数：scint/cherenkov/Si hit/逃逸通道
  // - 入射角均值：用 sum(theta) / count(theta)
  // - 撞击次数均值：对“发生过Si界面撞击的光子”，统计其 hitCount 的平均
  G4double hitCountSum = 0.0;
  G4int hitCountN = 0;
  for (const auto& kv : photonHitCount) {
    hitCountSum += kv.second;
    hitCountN += 1;
  }

  // === analytic_check2 Gate-1: 统计闭合兜底（仅对 debug opticalphoton 模式）===
  // 在 g_debug_opticalphoton=true 时，每事件应恰好有 1 个光子终态：
  // - hit Si (fLightCollection) 或
  // - Escaped_* 之一
  // 若因某些 kill 分支/异常路径导致本事件未被归类，则将其计入 Escaped_Other，
  // 同时打印少量警告，便于后续定位真实根因。
  if (g_debug_opticalphoton)
  {
    const G4int sum =
        fLightCollection +
        fEscapeAbsorbed +
        fEscapeTopAir +
        fEscapeSideAir +
        fEscapePTFE +
        fEscapeOther;

    if (sum == 0)
    {
      fEscapeOther += 1;
      static int warn = 0;
      if (warn < 10)
      {
        warn++;
        G4cerr << "[Gate-1][WARN] Event has no terminal classification; force +1 to Escaped_Other. "
               << "eventID=" << event->GetEventID()
               << " tid=" << G4Threading::G4GetThreadId()
               << G4endl;
      }
    }
    else if (sum != 1)
    {
      static int warn2 = 0;
      if (warn2 < 10)
      {
        warn2++;
        G4cerr << "[Gate-1][WARN] Event terminal classification sum != 1 in debug opticalphoton mode. "
               << "eventID=" << event->GetEventID()
               << " sum=" << sum
               << " (hit=" << fLightCollection
               << ", escCrystal=" << fEscapeAbsorbed
               << ", escTop=" << fEscapeTopAir
               << ", escSide=" << fEscapeSideAir
               << ", escPTFE=" << fEscapePTFE
               << ", escOther=" << fEscapeOther
               << ") tid=" << G4Threading::G4GetThreadId()
               << G4endl;
      }
    }
  }

  SiPINLCRunStats::Instance().AccumulateEventStats(
      /*siHitsDetected*/ fLightCollection,
      /*scintGenerated*/ fPhotonGenerated,
      /*chGenerated*/ fCherenkovGenerated,
      /*escCrystal*/ fEscapeAbsorbed,
      /*escTop*/ fEscapeTopAir,
      /*escSide*/ fEscapeSideAir,
      /*escPTFE*/ fEscapePTFE,
      /*escOther*/ fEscapeOther,
      /*thetaSumDeg*/ fThetaSumDeg,
      /*thetaCount*/ fThetaCount,
      /*hitCountSum*/ hitCountSum,
      /*hitCountN*/ hitCountN
    );

  processedTrackIDs.clear(); // 清空已处理的 track ID (用于统计哪些光子进入数值孔径)
  photonHitCount.clear();    // 清空撞击次数统计
  photonPathInCrystal.clear(); // 清空路程统计
    // Print progress (avoid flooding): only print from master, and at coarse granularity.
    if (G4Threading::IsMasterThread()) {
      auto eventID = event->GetEventID();
      auto totalEvents = G4RunManager::GetRunManager()->GetCurrentRun()->GetNumberOfEventToBeProcessed();
      // Print 10 times per run at most (every 10%), and only for reasonably large runs.
      auto printModulo = (totalEvents >= 1000) ? (totalEvents / 10) : 0;
      if ((printModulo > 0) && (eventID % printModulo == 0))
      {
          const G4double pct = (totalEvents > 0) ? (100.0 * (G4double)eventID / (G4double)totalEvents) : 0.0;
          G4cout << "---> End of event: " << eventID << ", " << pct << "% completed" << std::endl;
      }
    }

}
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
