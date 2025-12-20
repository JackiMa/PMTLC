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
/// \file SiPINLCSteppingAction.cc
/// \brief Implementation of the SiPINLCSteppingAction class

#include "SiPINLCSteppingAction.hh"
#include "SiPINLCRun.hh"

#include "G4Event.hh"
#include "G4OpBoundaryProcess.hh"
#include "G4OpticalPhoton.hh"
#include "G4RunManager.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4AnalysisManager.hh"

#include "config.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

SiPINLCSteppingAction::SiPINLCSteppingAction(SiPINLCEventAction *event)
    : G4UserSteppingAction(), fEventAction(event)
{
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
SiPINLCSteppingAction::~SiPINLCSteppingAction() {}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
void SiPINLCSteppingAction::UserSteppingAction(const G4Step *step)
{
    G4Track *aTrack = step->GetTrack();

    if (aTrack->GetDefinition() == G4OpticalPhoton::Definition())
    {
        G4int trackID = aTrack->GetTrackID();
        G4StepPoint *preStepPoint = step->GetPreStepPoint();
        G4StepPoint *postStepPoint = step->GetPostStepPoint();
        
        // === 论文所需：统计产生的闪烁光子数 ===
        // 只在光子第一次出现时统计（CurrentStepNumber == 1）
        if (aTrack->GetCurrentStepNumber() == 1)
        {
            const G4VProcess* creatorProcess = aTrack->GetCreatorProcess();
            if (creatorProcess && creatorProcess->GetProcessName() == "Scintillation")
            {
                fEventAction->fPhotonGenerated++;
            }
        }
        
        // 检查postVolume是否有效
        G4VPhysicalVolume* postVolume = postStepPoint->GetTouchableHandle()->GetVolume();
        G4VPhysicalVolume* preVolume = preStepPoint->GetTouchableHandle()->GetVolume();
        
        if (!preVolume) return;
        
        G4String preVolumeName = preVolume->GetName();
        G4String postVolumeName = postVolume ? postVolume->GetName() : "OutOfWorld";
        
        auto analysisManager = G4AnalysisManager::Instance();
        
        // === 论文所需：累加光子在晶体内的路程 ===
        // 用于理解自吸收效应
        if (preVolumeName == gN_sc_crystal)
        {
            G4double stepLength = step->GetStepLength();
            fEventAction->photonPathInCrystal[trackID] += stepLength;
        }
        
        // === 论文所需：统计光子撞击SiPIN的次数和入射角 ===
        // 当光子从window进入si时（每次撞击都统计，不管是否最终被吸收）
        if (preVolumeName == gN_sipin_window && postVolumeName == gN_sipin_si)
        {
            // 记录撞击次数
            fEventAction->photonHitCount[trackID]++;
            
            // === 论文所需：计算入射角 ===
            G4ThreeVector momentumDir = aTrack->GetMomentumDirection();
            // SiPIN表面法向量（假设朝向+z，即从晶体向下看SiPIN）
            G4ThreeVector surfaceNormal(0, 0, 1);
            // 入射角 = arccos(|momentum · normal|)
            G4double cosTheta = std::abs(momentumDir.dot(surfaceNormal));
            G4double thetaDeg = std::acos(cosTheta) * 180.0 / CLHEP::pi;
            analysisManager->FillH1(gID_H1_sipin_theta, thetaDeg);
        }
        
        // === 原有逻辑：第一次进入si时统计并kill ===
        if (fEventAction->processedTrackIDs.find(trackID) == fEventAction->processedTrackIDs.end())
        {
            if (preVolumeName == gN_sipin_window && postVolumeName == gN_sipin_si)
            {
                G4ThreeVector postPosition = postStepPoint->GetPosition();
                G4double x = postPosition.x();
                G4double y = postPosition.y();

                G4double energy = aTrack->GetTotalEnergy();
                G4double wavelength = (1239.841939 * nm) / energy; // 将能量转换为波长

                // 记录打到光阴极上的光子信息
                analysisManager->FillH1(gID_H1_sipin_wl, wavelength);
                analysisManager->FillH2(gID_H2_photon_pos, x, y);
                fEventAction->fLightCollection++;
                
                if (y >= -.5 * mm && y <= .5 * mm)
                {
                    analysisManager->FillH1(gID_H1_photon_posY0, x);
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
                        analysisManager->FillH1(gID_H1_photon_posYX, distanceToOrigin);
                    }
                    else
                    {
                        analysisManager->FillH1(gID_H1_photon_posYX, -distanceToOrigin);
                    }
                }
                // 标记光子为已处理
                fEventAction->processedTrackIDs.insert(trackID);

                // 杀掉光子
                aTrack->SetTrackStatus(fStopAndKill);
            }
        }
        
        // === 论文所需：统计光子逃逸/损失通道 ===
        // 检测光子是否被终止（吸收、逃出世界等）
        if (aTrack->GetTrackStatus() == fStopAndKill || 
            aTrack->GetTrackStatus() == fKillTrackAndSecondaries)
        {
            // 跳过已经被我们统计为"成功探测"的光子
            if (fEventAction->processedTrackIDs.find(trackID) != fEventAction->processedTrackIDs.end())
                return;
                
            // 根据终止位置分类
            if (preVolumeName == gN_sc_crystal)
            {
                // 在晶体内被吸收
                fEventAction->fEscapeAbsorbed++;
            }
            else if (preVolumeName == gN_sc_wrapper)
            {
                // 被PTFE/wrapper吸收
                fEventAction->fEscapePTFE++;
            }
            else if (preVolumeName == "sc_gap")
            {
                // 在gap中逃逸 - 需要进一步判断是顶面还是侧面
                G4ThreeVector pos = preStepPoint->GetPosition();
                // 简化判断：根据z坐标判断顶面还是侧面
                // 如果z坐标较高（接近晶体顶部），认为是顶面逃逸
                // 这里需要根据实际几何调整阈值
                fEventAction->fEscapeSideAir++;  // 默认算作侧面
            }
            else if (postVolumeName == "OutOfWorld" || postVolumeName == "World")
            {
                // 逃出世界
                fEventAction->fEscapeOther++;
            }
            else
            {
                // 其他情况
                fEventAction->fEscapeOther++;
            }
        }
    }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
