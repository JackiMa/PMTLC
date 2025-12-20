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
#include "G4RandomTools.hh"
#include "G4GeometryTolerance.hh"

#include "config.hh"
#include "SipinPDETable.hh"

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
        
        // DEBUG: 追踪光子从晶体底面出来后的路径
        static G4int debugPathCount = 0;
        if (debugPathCount < 20 && 
            (preVolumeName == gN_sc_crystal || preVolumeName == "sc_gap" || preVolumeName == "optical_grease") &&
            preStepPoint->GetPosition().z() < -12.0*mm)  // 接近底面
        {
            G4cout << "[PATH] #" << debugPathCount++ 
                   << " pre=" << preVolumeName 
                   << " post=" << postVolumeName
                   << " z=" << preStepPoint->GetPosition().z()/mm << "mm"
                   << G4endl;
        }
        
        // === 论文所需：统计光子撞击SiPIN的次数和入射角 ===
        // 你的物理假设：直接处理 grease(or bottom airgap) → Si 的界面
        // 注意：Grease 和 sc_bottom_gap 现在放在 World 中，直接与 sipin_si 接触
        const G4bool hitSipinSiInterface =
            (postVolumeName == gN_sipin_si) &&
            (preVolumeName == "optical_grease" || preVolumeName == "sc_gap" || preVolumeName == "sc_bottom_gap");

        // 每次撞击都统计（不管是否最终被“探测/吸收”）
        if (hitSipinSiInterface)
        {
            // 记录撞击次数
            fEventAction->photonHitCount[trackID]++;
            
            // === 论文所需：计算入射角 ===
            G4ThreeVector momentumDir = aTrack->GetMomentumDirection();
            // SiPIN 顶面法向量（+z 指向 wrapper/grease 一侧）
            // 入射来自上方（通常 momentumDir.z() < 0），取 abs 以得到 0~90°。
            G4ThreeVector surfaceNormal(0, 0, 1);
            // 入射角 = arccos(|momentum · normal|)
            G4double cosTheta = std::abs(momentumDir.dot(surfaceNormal));
            G4double thetaDeg = std::acos(cosTheta) * 180.0 / CLHEP::pi;
            analysisManager->FillH1(gID_H1_sipin_theta, thetaDeg);
        }
        
        // === SiPIN 界面处理：p_det(λ,θ) ===
        // 你的模型：所有入射光子 = 反射 + 光电转换(吸收)
        // - U < p_det : 认为“未反射且发生光电转换/吸收” => 计数 + kill
        // - U ≥ p_det : 认为“反射” => 镜面反射回 preVolume（不计数，不 kill）
        //
        // 兼容模式：
        // - g_sipin_pdet_mode==0：保持旧行为（第一次到达 Si 就计数+kill）
        // - g_sipin_pdet_mode==1：常数 p_det（单元验证）
        // - g_sipin_pdet_mode==2：CSV 查表 p_det(λ_nm, θ_deg)
        if (hitSipinSiInterface)
        {
            // 计算 (λ, θ)
            const G4double energy = aTrack->GetTotalEnergy();
            const G4double wavelength = (1239.841939 * nm) / energy; // nm
            const G4ThreeVector momentumDir = aTrack->GetMomentumDirection();
            const G4ThreeVector surfaceNormalToGrease(0, 0, 1); // +z 指向 wrapper/grease
            const G4double cosTheta = std::abs(momentumDir.dot(surfaceNormalToGrease));
            const G4double thetaDeg = std::acos(cosTheta) * 180.0 / CLHEP::pi;

            auto recordAndKill = [&]() {
                G4ThreeVector postPosition = postStepPoint->GetPosition();
                const G4double x = postPosition.x();
                const G4double y = postPosition.y();

                analysisManager->FillH1(gID_H1_sipin_wl, wavelength);
                analysisManager->FillH2(gID_H2_photon_pos, x, y);
                fEventAction->fLightCollection++;

                if (y >= -.5 * mm && y <= .5 * mm)
                {
                    analysisManager->FillH1(gID_H1_photon_posY0, x);
                }
                const double distanceToDiagonal = std::abs(x - y) / std::sqrt(2);
                if (distanceToDiagonal <= 0.5 * mm)
                {
                    const double distanceToOrigin = std::sqrt(x * x + y * y);
                    if (x > 0)
                        analysisManager->FillH1(gID_H1_photon_posYX, distanceToOrigin);
                    else
                        analysisManager->FillH1(gID_H1_photon_posYX, -distanceToOrigin);
                }

                fEventAction->processedTrackIDs.insert(trackID);
                aTrack->SetTrackStatus(fStopAndKill);
            };

            // legacy: first time reaching Si counts as "detected"
            if (g_sipin_pdet_mode == 0)
            {
                if (fEventAction->processedTrackIDs.find(trackID) == fEventAction->processedTrackIDs.end())
                {
                    recordAndKill();
                }
            }
            else
            {
                // Safety cap: prevent pathological long runs when p_det is small/0.
                if (g_sipin_max_interface_hits > 0)
                {
                    const auto it = fEventAction->photonHitCount.find(trackID);
                    const G4int nHits = (it == fEventAction->photonHitCount.end()) ? 0 : it->second;
                    if (nHits > g_sipin_max_interface_hits)
                    {
                        aTrack->SetTrackStatus(fStopAndKill);
                        return;
                    }
                }

                double pdet = 0.0;
                if (g_sipin_pdet_mode == 1)
                {
                    pdet = g_sipin_pdet_const;
                }
                else if (g_sipin_pdet_mode == 2)
                {
                    static SipinPDETable table;
                    static G4String lastPath = "";
                    static G4bool warned = false;

                    if (g_sipin_pdet_csv != lastPath)
                    {
                        lastPath = g_sipin_pdet_csv;
                        warned = false;
                        std::string err;
                        if (!lastPath.empty())
                        {
                            const bool ok = table.LoadFromCsv(lastPath, &err);
                            if (!ok)
                            {
                                G4cerr << "[SipinPDETable] load failed: " << err << G4endl;
                            }
                            else if (!err.empty())
                            {
                                G4cout << "[SipinPDETable] loaded with warning: " << err << G4endl;
                            }
                            else
                            {
                                G4cout << "[SipinPDETable] loaded: " << lastPath << G4endl;
                            }
                        }
                    }

                    if (!table.IsLoaded())
                    {
                        if (!warned)
                        {
                            warned = true;
                            G4cerr << "[SipinPDETable] not loaded; p_det forced to 0. "
                                   << "Set /SiPINLC/sipin/pdetFile and re-run." << G4endl;
                        }
                        pdet = 0.0;
                    }
                    else
                    {
                        pdet = table.GetPdet(wavelength / nm, thetaDeg);
                    }
                }

                // physical clamp
                if (pdet < 0.0) pdet = 0.0;
                if (pdet > 1.0) pdet = 1.0;

                if (G4UniformRand() < pdet)
                {
                    recordAndKill();
                }
                else
                {
                    // mirror reflection: push photon slightly back into preVolume and flip direction
                    const G4ThreeVector dir = momentumDir;
                    const G4ThreeVector n = surfaceNormalToGrease; // points into grease
                    const G4ThreeVector reflDir = dir - 2.0 * (dir.dot(n)) * n;

                    // IMPORTANT:
                    // If we push back too little (e.g. 1 nm), the navigator may keep the track on the boundary
                    // and issue many GeomNav1002 warnings. Push by a safe distance >> surface tolerance.
                    const G4double tol = G4GeometryTolerance::GetInstance()->GetSurfaceTolerance();
                    const G4double push = std::max(100.0 * tol, 0.1 * um);
                    // Move along reflected direction to guarantee we're inside the pre-volume (grease/airgap)
                    const G4ThreeVector backPos = preStepPoint->GetPosition() + push * reflDir.unit();
                    aTrack->SetPosition(backPos);
                    aTrack->SetMomentumDirection(reflDir.unit());
                    aTrack->SetTrackStatus(fAlive);
                }
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
                // 调试输出：打印前10个被"晶体吸收"的光子的详细信息
                static G4int debugCount = 0;
                if (debugCount < 10) {
                    const G4VProcess* proc = postStepPoint->GetProcessDefinedStep();
                    G4String procName = proc ? proc->GetProcessName() : "unknown";
                    G4cout << "[DEBUG] CrystalAbsorbed #" << debugCount 
                           << " pos=" << preStepPoint->GetPosition()/mm << " mm"
                           << " dir=" << aTrack->GetMomentumDirection()
                           << " process=" << procName
                           << " postVol=" << postVolumeName
                           << G4endl;
                    debugCount++;
                }
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

