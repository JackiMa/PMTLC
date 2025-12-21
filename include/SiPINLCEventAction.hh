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
/// \file optical/SiPINLC/include/SiPINLCEventAction.hh
/// \brief Definition of the SiPINLCEventAction class
//
//
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#ifndef SiPINLCEventAction_h
#define SiPINLCEventAction_h 1

#include "G4UserEventAction.hh"
#include "G4THitsMap.hh"
#include "globals.hh"

#include <set>
#include <map>

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

class SiPINLCEventAction : public G4UserEventAction
{
 public:
  SiPINLCEventAction();
  ~SiPINLCEventAction();

  void BeginOfEventAction(const G4Event*) override;
  void EndOfEventAction(const G4Event*) override;
  
  // methods
  G4THitsMap<G4double>* GetHitsCollection(G4int hcID,
                                          const G4Event* event) const;
  G4double GetSum(G4THitsMap<G4double>* hitsMap) const;
  void PrintEventStatistics(G4double absoEdep) const;

  // 用于考虑光子是否穿过数值孔径，记录trackID，避免重复统计
  std::set<G4int> processedTrackIDs;
  
  // === 论文所需：追踪每个光子撞击SiPIN的次数 ===
  std::map<G4int, G4int> photonHitCount;  // trackID -> 撞击次数
  
  // === 论文所需：追踪每个光子在晶体内的路程 ===
  std::map<G4int, G4double> photonPathInCrystal;  // trackID -> 晶体内累计路程 (mm)

  // data members
  G4int fAbsoEdepHCID = -1;
  G4int fTotalEnergyHCID = -1;
  G4int fHEPhotonHCID = -1;
  G4int fNeutEdepHCID = -1;

  G4int fEdepInCrystal = -1;
  G4int fEngPassingSD1 = -1;

  G4int fLightCollection = 0;
  
  // === 论文所需的统计量 ===
  G4int fPhotonGenerated = 0;     // 本事件产生的闪烁光子数
  G4int fCherenkovGenerated = 0;  // 本事件产生的切伦科夫光子数
  G4int fEscapeAbsorbed = 0;      // 被晶体自吸收的光子数
  G4int fEscapeTopAir = 0;        // 从顶面空气层逃逸的光子数
  G4int fEscapeSideAir = 0;       // 从侧面空气层逃逸的光子数
  G4int fEscapePTFE = 0;          // 被PTFE吸收的光子数
  G4int fEscapeOther = 0;         // 其他损失通道

  // === 论文所需：用于 run 级别均值统计（避免 MT 下读取直方图未合并）===
  G4double fThetaSumDeg = 0.0;    // 本事件所有“撞击SiPIN界面”的入射角求和（deg）
  G4int fThetaCount = 0;          // 本事件入射角样本数（=撞击次数）

  // === analytic sanity-check: wall interaction count (side/top only, from stepping override) ===
  G4int fWallHitCount = 0;

};
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
#endif
