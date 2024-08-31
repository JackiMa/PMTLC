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
/// \file PMTLCSteppingAction.cc
/// \brief Implementation of the PMTLCSteppingAction class

#include "PMTLCSteppingAction.hh"
#include "PMTLCRun.hh"

#include "G4Event.hh"
#include "G4OpBoundaryProcess.hh"
#include "G4OpticalPhoton.hh"
#include "G4RunManager.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4AnalysisManager.hh"

#include "config.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

PMTLCSteppingAction::PMTLCSteppingAction(PMTLCEventAction *event)
    : G4UserSteppingAction(), fEventAction(event)
{
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
PMTLCSteppingAction::~PMTLCSteppingAction() {}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
void PMTLCSteppingAction::UserSteppingAction(const G4Step *step)
{
    G4Track *aTrack = step->GetTrack();

    if (aTrack->GetDefinition() == G4OpticalPhoton::Definition())
    {
        G4int trackID = aTrack->GetTrackID();
        // 检查光子是否已经处理过
        if (fEventAction->processedTrackIDs.find(trackID) == fEventAction->processedTrackIDs.end())
        {
            G4StepPoint *preStepPoint = step->GetPreStepPoint();
            G4StepPoint *postStepPoint = step->GetPostStepPoint();
            if (!postStepPoint->GetTouchableHandle()->GetVolume())
                return; // 如果没有前后步点，说明在考察的物体内，直接返回

            G4String preVolumeName = preStepPoint->GetTouchableHandle()->GetVolume()->GetName();
            G4String postVolumeName = postStepPoint->GetTouchableHandle()->GetVolume()->GetName();
            if (preVolumeName == gN_PMT_window && postVolumeName == gN_PMT_photocathode)
            {
                G4ThreeVector postPosition = postStepPoint->GetPosition();
                G4double x = postPosition.x();
                G4double y = postPosition.y();

                G4double energy = aTrack->GetTotalEnergy();
                G4double wavelength = (1239.841939 * nm) / energy; // 将能量转换为波长

                // 记录打到光阴极上的光子信息
                auto analysisManager = G4AnalysisManager::Instance();
                analysisManager->FillH1(2, wavelength);
                analysisManager->FillH2(1, x, y);
                if (y >= -.5 * mm && y <= .5 * mm)
                {
                    analysisManager->FillH1(3, x);
                }
                // 计算点到对角线 x = y 的距离
                double distanceToDiagonal = std::abs(x - y) / std::sqrt(2);

                if (distanceToDiagonal <= 0.5 * mm)
                {
                    // 计算点到原点的距离
                    double distanceToOrigin = std::sqrt(x * x + y * y);

                    // 填入FillH1(4, d)
                    if (x > 0)
                    {
                        analysisManager->FillH1(4, distanceToOrigin);
                    }
                    else
                    {
                        analysisManager->FillH1(4, -distanceToOrigin);
                    }
                }
                // 标记光子为已处理
                fEventAction->processedTrackIDs.insert(trackID);

                // 杀掉光子
                aTrack->SetTrackStatus(fStopAndKill);
            }
        }
    }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
