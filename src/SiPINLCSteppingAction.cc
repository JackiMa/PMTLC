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
#include "G4TransportationManager.hh"
#include "G4Navigator.hh"

#include "config.hh"
#include "SipinPDETable.hh"

namespace {
// 在 SetPosition 后同步 navigator 状态，避免 GeomNav1002 警告
void SyncNavigator(G4Track* track)
{
    auto navigator = G4TransportationManager::GetTransportationManager()
                         ->GetNavigatorForTracking();
    navigator->LocateGlobalPointAndSetup(
        track->GetPosition(),
        nullptr,
        false
    );
}
} // namespace

namespace {
// Cosine-weighted (Lambertian) hemisphere sampling around a given axis (must be unit).
// Returns a unit vector.
G4ThreeVector SampleLambertianHemisphere(const G4ThreeVector& axisUnit)
{
    // r1 -> cos-weighted
    const G4double r1 = G4UniformRand();
    const G4double r2 = G4UniformRand();
    const G4double cosTheta = std::sqrt(r1);
    const G4double sinTheta = std::sqrt(1.0 - r1);
    const G4double phi = 2.0 * CLHEP::pi * r2;

    G4ThreeVector w = axisUnit.unit();
    G4ThreeVector u = w.orthogonal().unit();
    G4ThreeVector v = w.cross(u).unit();

    G4ThreeVector dir = (sinTheta * std::cos(phi)) * u + (sinTheta * std::sin(phi)) * v + (cosTheta) * w;
    return dir.unit();
}
} // namespace

namespace {
// Transform a global point into the local coordinates of the pre-step volume.
// This follows the same pattern used in CustomScorer.cc in this repo.
G4ThreeVector ToLocalPreVolume(const G4StepPoint* preStepPoint, const G4ThreeVector& globalPoint)
{
    auto touchable = preStepPoint->GetTouchableHandle();
    if (!touchable) return globalPoint;
    const G4AffineTransform transform = touchable->GetHistory()->GetTopTransform();
    return transform.TransformPoint(globalPoint);
}
} // namespace

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
            if (creatorProcess && creatorProcess->GetProcessName() == "Cerenkov")
            {
                fEventAction->fCherenkovGenerated++;
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

            // === 用于 Run 级别均值统计（MT 下不依赖直方图合并时序）===
            fEventAction->fThetaSumDeg += thetaDeg;
            fEventAction->fThetaCount += 1;
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
                    SyncNavigator(aTrack);  // 同步 navigator 避免 GeomNav1002
                    aTrack->SetMomentumDirection(reflDir.unit());
                    aTrack->SetTrackStatus(fAlive);
                }
            }
        }

        // === 概率边界法：侧面/顶面 PTFE 贴合比例 ===
        // 
        // 物理模型：
        // - 以概率 p：光子遇到贴合的 PTFE
        //   - 以概率 R：PTFE 漫反射回晶体
        //   - 以概率 1-R：PTFE 吸收
        // - 以概率 1-p：光子遇到空气层，正常 TIR 或透射
        //
        // 实现要点：
        // - crystalToGap=true 意味着光子已透射进入 sc_gap
        // - 当抽中 PTFE 反射时，需要把光子推回晶体内
        // - 推回位置：晶体最接近当前点的坐标（只差 1μm，误差可忽略）
        // - 推回方向：PTFE 漫反射方向（Lambertian）
        
        const G4bool crystalToGap = (preVolumeName == gN_sc_crystal) && 
                                     (postVolumeName == "sc_gap");
        
        // 概率边界法：对于 p > 0 的所有情况都适用（包括 p = 1.0）
        if (crystalToGap && 
            (g_side_contact_ratio > 0.0 || g_top_contact_ratio > 0.0))
        {
            G4ThreeVector prePos = preStepPoint->GetPosition();
            G4ThreeVector postPos = postStepPoint->GetPosition();

            // === 关键修正：用晶体局部坐标来判别“跨的是哪一个面” ===
            // 不能用 global z 近似 bottomZ（会因晶体位置/容差导致误判）。
            // 用 preStep 的 touchable transform 把边界点投到 crystal local (Box) 坐标系中。
            const G4ThreeVector preLocal = ToLocalPreVolume(preStepPoint, prePos);
            const G4double halfX = 0.5 * g_crystalX;
            const G4double halfY = 0.5 * g_crystalY;
            const G4double halfZ = 0.5 * g_crystalZ;

            // 面判据：靠近哪一面（用 surface tolerance 做容差）
            const G4double tol = G4GeometryTolerance::GetInstance()->GetSurfaceTolerance();
            const G4bool onPosX = std::abs(preLocal.x() - (+halfX)) < 50.0 * tol;
            const G4bool onNegX = std::abs(preLocal.x() - (-halfX)) < 50.0 * tol;
            const G4bool onPosY = std::abs(preLocal.y() - (+halfY)) < 50.0 * tol;
            const G4bool onNegY = std::abs(preLocal.y() - (-halfY)) < 50.0 * tol;
            const G4bool onNegZ = std::abs(preLocal.z() - (-halfZ)) < 50.0 * tol; // bottom

            const G4bool isSide = (onPosX || onNegX || onPosY || onNegY);
            const G4bool isBottom = onNegZ;
            
            if (!isBottom)
            {
                G4double p = isSide ? g_side_contact_ratio : g_top_contact_ratio;
                
                if (p > 0.0)
                {
                    // 对于 p >= 1.0，总是走 PTFE 逻辑
                    // 对于 0 < p < 1.0，按概率决定
                    G4bool hitPTFE = (p >= 1.0) ? true : (G4UniformRand() < p);
                    
                    if (hitPTFE)
                    {
                        // === 贴合 PTFE 模式 ===
                        G4double R = g_ptfe_reflectivity;
                        G4double randR = G4UniformRand();
                        
                        if (randR >= R)
                        {
                            // 被 PTFE 吸收（概率 1-R = 2.5%）
                            aTrack->SetTrackStatus(fStopAndKill);
                            fEventAction->fEscapePTFE++;
                            return;
                        }
                        else
                        {
                            // PTFE 漫反射（概率 R = 97.5%）
                            // 计算法向量（指向晶体内）
                            G4ThreeVector normal;
                            if (onPosX) normal = G4ThreeVector(-1, 0, 0);
                            else if (onNegX) normal = G4ThreeVector(+1, 0, 0);
                            else if (onPosY) normal = G4ThreeVector(0, -1, 0);
                            else if (onNegY) normal = G4ThreeVector(0, +1, 0);
                            else normal = G4ThreeVector(0, 0, -1); // top face fallback
                            
                            // 生成朗伯漫反射方向
                            G4ThreeVector newDir = SampleLambertianHemisphere(normal);
                            aTrack->SetMomentumDirection(newDir);
                            
                            // 把光子推回晶体内最近的点
                            // 使用 prePos（还在晶体内的边界处）并稍微推入
                            G4ThreeVector newPos = prePos;
                            // 沿着法向量（指向晶体内）推入一小段距离
                            newPos += normal * 0.1 * um;
                            
                            aTrack->SetPosition(newPos);
                            SyncNavigator(aTrack);  // 同步 navigator 避免 GeomNav1002
                            aTrack->SetTrackStatus(fAlive);
                        }
                    }
                    // hitPTFE == false：遇到空气层，让光子继续自然传播到 wrapper
                }
            }
        }
        
        // === 补丁：边界拦截 ===
        // 当光子从 sc_gap/grease/sc_bottom_gap/wrapper 进入 World 时，应用 PTFE 反射/吸收
        // 这模拟 wrapper 完全包裹（包括底部开窗区域的侧边）
        // === 关键修正：不要把“底部开窗”也当成 PTFE 反射/吸收 ===
        // 在 TEFLON 几何中：sc_gap 底面 -> World 是通往 grease/Si 的真实通道。
        // 我们只拦截“本应被 wrapper 包裹的侧/顶方向 sc_gap->World 漏光”，以及 wrapper->World 外表面。
        const G4bool postIsWorld = (postVolumeName == "World");
        const G4bool preIsWrapper = (preVolumeName == gN_sc_wrapper);
        const G4bool preIsGap = (preVolumeName == "sc_gap");

        // 计算 wrapper 底面 z（与 DetectorConstruction 的 TEFLON 模式一致）
        const G4double sipin_top_z = g_sipin_pos.z() + 0.5 * g_sipin_thickness;
        const G4double grease_effective = (g_grease_thickness > 10 * um) ? g_grease_thickness : 0.0;
        const G4double bottom_layer_height = (grease_effective > 0.0) ? grease_effective : g_bottom_airgap_thickness;
        const G4double wrapper_bottom_z = sipin_top_z + bottom_layer_height;
        const G4double zPost = postStepPoint->GetPosition().z();
        const G4double tolWorld = G4GeometryTolerance::GetInstance()->GetSurfaceTolerance();
        const G4bool isBottomOpeningCrossing = (zPost <= wrapper_bottom_z + 50.0 * tolWorld);

        const G4bool volumeToWorld =
            postIsWorld &&
            (preIsWrapper || (preIsGap && !isBottomOpeningCrossing));
        if (volumeToWorld)
        {
            // 应用 PTFE 反射/吸收（与 wrapper SkinSurface 相同的物理）
            G4double R = g_ptfe_reflectivity;
            G4double randR = G4UniformRand();
            
            if (randR >= R)
            {
                // 被 PTFE 吸收
                aTrack->SetTrackStatus(fStopAndKill);
                fEventAction->fEscapePTFE++;
                return;
            }
            else
            {
                // PTFE 漫反射回 sc_gap（进而回晶体）
                G4ThreeVector pos = postStepPoint->GetPosition();
                G4double halfGapX = 0.5 * g_crystalX + g_gap_thickness;
                G4double halfGapY = 0.5 * g_crystalY + g_gap_thickness;
                
                // 判断是哪个面
                G4ThreeVector normal;
                if (std::abs(pos.x()) >= halfGapX) {
                    normal = G4ThreeVector(pos.x() > 0 ? -1 : 1, 0, 0);
                } else if (std::abs(pos.y()) >= halfGapY) {
                    normal = G4ThreeVector(0, pos.y() > 0 ? -1 : 1, 0);
                } else {
                    normal = G4ThreeVector(0, 0, -1);  // 顶面
                }
                
                G4ThreeVector newDir = SampleLambertianHemisphere(normal);
                aTrack->SetMomentumDirection(newDir);
                
                // 推回 sc_gap 内
                G4ThreeVector newPos = preStepPoint->GetPosition();
                newPos += normal * 0.1 * um;
                aTrack->SetPosition(newPos);
                SyncNavigator(aTrack);  // 同步 navigator 避免 GeomNav1002
                aTrack->SetTrackStatus(fAlive);
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
            
            // 获取终止进程名称
            const G4VProcess* proc = postStepPoint->GetProcessDefinedStep();
            G4String procName = proc ? proc->GetProcessName() : "unknown";
                
            // 根据终止位置和进程分类
            if (preVolumeName == gN_sc_crystal)
            {
                // 区分：晶体内自吸收 vs BorderSurface 边界吸收
                if (procName == "OpAbsorption")
                {
                    // 真正的晶体自吸收
                    fEventAction->fEscapeAbsorbed++;
                }
                else if (postVolumeName == "sc_gap" || postVolumeName == gN_sc_wrapper)
                {
                    // 在 crystal-sc_gap 或 crystal-wrapper 边界被吸收
                    // 这是 BorderSurface (dielectric_metal) 的吸收
                    fEventAction->fEscapePTFE++;
                    
                    // 调试输出
                    static G4int debugBorderCount = 0;
                    if (debugBorderCount < 5) {
                        G4cout << "[DEBUG] BorderSurface absorbed #" << debugBorderCount++
                               << " pre=" << preVolumeName 
                               << " post=" << postVolumeName
                               << " process=" << procName
                               << G4endl;
                    }
                }
                else
                {
                    // 其他情况（应该很少）
                    fEventAction->fEscapeAbsorbed++;
                    
                    static G4int debugOtherCount = 0;
                    if (debugOtherCount < 5) {
                        G4cout << "[DEBUG] CrystalOther #" << debugOtherCount++
                               << " pre=" << preVolumeName 
                               << " post=" << postVolumeName
                               << " process=" << procName
                               << G4endl;
                    }
                }
            }
            else if (preVolumeName == gN_sc_wrapper)
            {
                // 在 wrapper 内被吸收（光子进入 wrapper 后被吸收）
                fEventAction->fEscapePTFE++;
            }
            else if (preVolumeName == "sc_gap")
            {
                // 在 gap 中被终止
                if (postVolumeName == gN_sc_wrapper)
                {
                    // 被 wrapper 的 SkinSurface 吸收
                    fEventAction->fEscapePTFE++;
                }
                else
                {
                    fEventAction->fEscapeSideAir++;
                }
            }
            else if (postVolumeName == "OutOfWorld")
            {
                // 逃出世界边界
                // 根据 preVolume 和位置判断归类
                if (preVolumeName == "optical_grease" || preVolumeName == "sc_bottom_gap")
                {
                    // 从 grease 边缘逃出（合理的物理行为）
                    fEventAction->fEscapeGrease++;
                }
                else if (preVolumeName == gN_sc_wrapper || preVolumeName == "sc_gap")
                {
                    // 从 wrapper 区域逃出
                    fEventAction->fEscapePTFE++;
                }
                else
                {
                    // 从 World 边界逃出（不应发生）
                    fEventAction->fEscapeWorld++;
                    
                    static G4int debugWorldCount = 0;
                    if (debugWorldCount < 5) {
                        G4cout << "[DEBUG] WorldEscape #" << debugWorldCount++
                               << " pre=" << preVolumeName 
                               << " pos=" << preStepPoint->GetPosition()/mm << " mm"
                               << G4endl;
                    }
                }
            }
            else if (preVolumeName == "optical_grease" || preVolumeName == "sc_bottom_gap")
            {
                // 在 grease 或 bottom_gap 中被吸收（grease 有一定吸收）
                fEventAction->fEscapeGrease++;
            }
            else if (preVolumeName == gN_sipin_si)
            {
                // 在 Si 中被吸收（正常物理过程，深入 Si 后被吸收）
                // pdetMode=0 时，第一次到达 Si 就 kill 并计入 LightCollection
                // 这里的情况是光子穿过了第一次统计后继续深入被吸收
                // 不计入任何损失，因为已经被统计为探测到
            }
            else if (preVolumeName == "World")
            {
                // 在 World 中被终止
                fEventAction->fEscapeWorld++;
                
                static G4int debugWorldAbsCount = 0;
                if (debugWorldAbsCount < 5) {
                    G4cout << "[DEBUG] WorldAbsorbed #" << debugWorldAbsCount++
                           << " pre=" << preVolumeName 
                           << " post=" << postVolumeName
                           << " pos=" << preStepPoint->GetPosition()/mm << " mm"
                           << G4endl;
                }
            }
            else
            {
                // 其他未分类情况 - 输出警告并计入 World 类别
                fEventAction->fEscapeWorld++;
                
                static G4int debugUnclassifiedCount = 0;
                if (debugUnclassifiedCount < 10) {
                    G4cout << "[WARN] Unclassified photon loss #" << debugUnclassifiedCount++
                           << " pre=" << preVolumeName 
                           << " post=" << postVolumeName
                           << " process=" << procName
                           << G4endl;
                }
            }
        }
    }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

