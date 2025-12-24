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
/// \file SiPINLCSteppingAction.hh
/// \brief Definition of the SiPINLCSteppingAction class

#ifndef SiPINLCSteppingAction_h
#define SiPINLCSteppingAction_h 1

#include "SiPINLCEventAction.hh"
#include "globals.hh"
#include "G4UserSteppingAction.hh"
#include "G4ThreeVector.hh"
#include <map>

class SiPINLCSteppingAction : public G4UserSteppingAction
{
 public:
  SiPINLCSteppingAction(SiPINLCEventAction*);
  ~SiPINLCSteppingAction();

  void UserSteppingAction(const G4Step*) override;

 private:
  SiPINLCEventAction* fEventAction;
  
  // 概率边界法状态跟踪（per-track）
  // 当光子从晶体进入 sc_gap 且触发贴合模式时，记录其原始位置
  // 等待光子与 wrapper 交互后，如果被反射则推回原始位置
  struct ProbBoundaryState {
    G4bool active = false;       // 是否处于贴合模式
    G4ThreeVector entryPos;      // 进入 sc_gap 前的位置
    G4ThreeVector entryDir;      // 进入 sc_gap 前的方向
    G4int trackID = -1;          // 跟踪的光子 ID
  };
  std::map<G4int, ProbBoundaryState> fProbBoundaryStates;  // 每个线程一个
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif
