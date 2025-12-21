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
#include "G4Threading.hh"

#include <atomic>
#include "config.hh"
#include "SipinPDETable.hh"

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

// Isotropic direction sampling constrained to a given hemisphere (NOT cosine-weighted).
// Returns a unit vector.
// NOTE: This is intentionally used in analytic sanity-check wall override to approximate "full mixing"
// assumptions in docs/analytic_check2.md (not a physical Lambertian BRDF).
G4ThreeVector SampleIsotropicHemisphere(const G4ThreeVector& axisUnit)
{
    // Isotropic in 4pi
    const G4double u = 2.0 * G4UniformRand() - 1.0; // cos in [-1,1]
    const G4double phi = 2.0 * CLHEP::pi * G4UniformRand();
    const G4double sinTheta = std::sqrt(std::max(0.0, 1.0 - u * u));
    G4ThreeVector dir(sinTheta * std::cos(phi), sinTheta * std::sin(phi), u);

    // Constrain to hemisphere oriented by axisUnit
    const G4ThreeVector ax = axisUnit.unit();
    if (dir.dot(ax) < 0.0) dir = -dir;
    return dir.unit();
}

// Extract reflectivity from an optical surface material properties table, if present.
// If missing, returns fallback.
G4double GetSurfaceReflectivity(const G4OpticalSurface* surf, G4double photonEnergy, G4double fallback = 1.0)
{
    if (!surf) return fallback;
    auto* mpt = surf->GetMaterialPropertiesTable();
    if (!mpt) return fallback;
    auto* vec = mpt->GetProperty("REFLECTIVITY");
    if (!vec) return fallback;
    return vec->Value(photonEnergy);
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

        // === Sanity-check safety cap ===
        // For analytic scenarios (A/B/C) we may create "specular + TIR trapped" tracks that would
        // otherwise take extremely many steps before being absorbed, which can stall or crash long runs.
        if (g_sanity_wall_model != SANITY_WALL_OFF && g_sanity_max_steps > 0)
        {
            if (aTrack->GetCurrentStepNumber() > g_sanity_max_steps)
            {
                fEventAction->fEscapeOther++;
                aTrack->SetTrackStatus(fStopAndKill);
                return;
            }
        }

        // === Scenario A fast path (specular) ===
        // Under ideal specular side/top walls, the polar angle (relative to bottom normal) is conserved.
        // If |cos(theta)| is below the critical value, the photon is TIR-trapped w.r.t. the bottom interface
        // and will never reach Si in the no-absorption limit. To make 1e6-statistics feasible, we can
        // short-circuit these photons at step 1 and count them as "lost".
        if (g_sanity_fast_specular_tir && g_sanity_wall_model == SANITY_WALL_SPECULAR &&
            aTrack->GetCurrentStepNumber() == 1)
        {
            const G4ThreeVector dir = aTrack->GetMomentumDirection().unit();
            const G4double cosTheta = std::abs(dir.z()); // bottom normal is ±z

            // Estimate n1 (crystal) and n2 (bottom medium) at this photon energy.
            const G4double energy = aTrack->GetTotalEnergy();
            auto* mpt1 = preStepPoint->GetMaterial()->GetMaterialPropertiesTable();
            G4double n1 = 1.0;
            if (mpt1)
            {
                if (auto* r1 = mpt1->GetProperty("RINDEX")) n1 = r1->Value(energy);
            }

            // Choose bottom medium by configuration (grease or air gap)
            G4Material* bottomMat = (g_grease_thickness > 10 * um) ? g_grease_material : g_world_material;
            G4double n2 = 1.0;
            if (bottomMat)
            {
                if (auto* mpt2 = bottomMat->GetMaterialPropertiesTable())
                {
                    if (auto* r2 = mpt2->GetProperty("RINDEX")) n2 = r2->Value(energy);
                }
            }

            if (n1 > 0.0 && n2 > 0.0 && n1 > n2)
            {
                const G4double mu0 = std::sqrt(std::max(0.0, 1.0 - (n2 / n1) * (n2 / n1)));
                // Debug a few events (once globally) to validate n1/n2/mu0 and fast-path logic.
                static std::atomic<int> dbg{0};
                const int k = dbg.fetch_add(1);
                if (k < 5)
                {
                    G4cout << "[SANITY-A fast] tid=" << G4Threading::G4GetThreadId()
                           << " n1=" << n1 << " n2=" << n2
                           << " mu0=" << mu0 << " cosTheta=" << cosTheta
                           << " preMat=" << preStepPoint->GetMaterial()->GetName()
                           << " bottomMat=" << (bottomMat ? bottomMat->GetName() : "null")
                           << G4endl;
                }
                // If trapped by TIR w.r.t bottom interface -> never reaches Si in scenario A
                if (cosTheta < mu0) {
                    fEventAction->fEscapeOther++;
                    aTrack->SetTrackStatus(fStopAndKill);
                    return;
                }

                // Otherwise, under ideal specular side/top walls, the photon will eventually transmit through bottom
                // with probability -> 1 (Fresnel only affects number of bounces, not the eventual probability).
                // For analytic sanity check we count it as "hit Si" immediately to make 1e6 feasible.
                fEventAction->fLightCollection++;
                fEventAction->processedTrackIDs.insert(trackID);
                aTrack->SetTrackStatus(fStopAndKill);
                return;
            }
        }
        
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
        
        // For analytic sanity-check runs we avoid ROOT/analysis I/O in MT for stability.
        auto analysisManager = (g_sanity_wall_model == SANITY_WALL_OFF) ? G4AnalysisManager::Instance() : nullptr;
        
        // === 论文所需：累加光子在晶体内的路程 ===
        // 用于理解自吸收效应
        if (preVolumeName == gN_sc_crystal)
        {
            G4double stepLength = step->GetStepLength();
            fEventAction->photonPathInCrystal[trackID] += stepLength;
        }
        
        // NOTE:
        // Previous debugging prints ([PATH]) were removed because they were not thread-safe in MT
        // and could trigger crashes during large-statistics analytic sanity runs.
        
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
            if (analysisManager) {
                analysisManager->FillH1(gID_H1_sipin_theta, thetaDeg);
            }

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
            const G4double wavelength = (1239.841939 * eV * nm) / energy; // nm
            const G4ThreeVector momentumDir = aTrack->GetMomentumDirection();
            const G4ThreeVector surfaceNormalToGrease(0, 0, 1); // +z 指向 wrapper/grease
            const G4double cosTheta = std::abs(momentumDir.dot(surfaceNormalToGrease));
            const G4double thetaDeg = std::acos(cosTheta) * 180.0 / CLHEP::pi;

            auto recordAndKill = [&]() {
                G4ThreeVector postPosition = postStepPoint->GetPosition();
                const G4double x = postPosition.x();
                const G4double y = postPosition.y();

                if (analysisManager) {
                    analysisManager->FillH1(gID_H1_sipin_wl, wavelength);
                    analysisManager->FillH2(gID_H2_photon_pos, x, y);
                }
                fEventAction->fLightCollection++;

                if (y >= -.5 * mm && y <= .5 * mm)
                {
                    if (analysisManager) {
                        analysisManager->FillH1(gID_H1_photon_posY0, x);
                    }
                }
                const double distanceToDiagonal = std::abs(x - y) / std::sqrt(2);
                if (distanceToDiagonal <= 0.5 * mm)
                {
                    const double distanceToOrigin = std::sqrt(x * x + y * y);
                    if (analysisManager) {
                        if (x > 0) {
                            analysisManager->FillH1(gID_H1_photon_posYX, distanceToOrigin);
                        } else {
                            analysisManager->FillH1(gID_H1_photon_posYX, -distanceToOrigin);
                        }
                    }
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

        // === 侧面贴合比例（概率边界法）===
        // 当光子尝试从晶体侧面进入 sc_gap 时：
        // - 以概率 p = g_side_contact_ratio，将该次交互视为“晶体-PTFE直接贴合”：
        //     * 用 PTFE 表面反射率做吸收/反射判决（反射使用 Lambertian 近似）
        //     * 反射则把光子推回晶体内部并改变方向
        // - 以概率 1-p，保持原 Geant4 的“晶体-空气”边界（Fresnel/TIR）行为
        //
        // 说明：这是统计等效的“面片随机化”，用于避免复杂几何分片。
        // === Analytic-friendly sanity wall override (A/B/C in docs/analytic_check2.md) ===
        // Override crystal -> sc_gap boundary on SIDE/TOP only (never bottom) with:
        // - SPECULAR mirror (R=1) or
        // - LAMBERTIAN diffuse reflector (R in [0,1])
        //
        // This is used to create simple scenes with only one loss mechanism (wall absorption),
        // while keeping the real bottom coupling (air/grease) physics.
        if (g_sanity_wall_model != SANITY_WALL_OFF)
        {
            const G4bool isBoundary = (postStepPoint->GetStepStatus() == fGeomBoundary);
            // IMPORTANT:
            // Do NOT require postVolumeName == "sc_gap".
            // For dielectric boundaries, Geant4 may perform TIR and keep the track in the pre-volume,
            // which would bypass our override and break the "ideal wall" assumption.
            // For analytic scenes, we override any SIDE/TOP boundary encounter of photons in the crystal.
            const G4bool crystalBoundary = isBoundary && (preVolumeName == gN_sc_crystal);
            if (crystalBoundary)
            {
                const auto touch = preStepPoint->GetTouchableHandle();
                const G4ThreeVector local = touch->GetHistory()->GetTopTransform().TransformPoint(preStepPoint->GetPosition());

                const G4double hx = 0.5 * g_crystalX;
                const G4double hy = 0.5 * g_crystalY;
                const G4double hz = 0.5 * g_crystalZ;
                // Robust face classification tolerance:
                // In MT + manual boundary overrides, the navigator can leave the boundary point slightly off.
                // Use a larger tolerance than the raw surface tolerance to avoid misclassifying bottom hits as side hits.
                const G4double st = G4GeometryTolerance::GetInstance()->GetSurfaceTolerance();
                const G4double tol = std::max(1000.0 * st, 1.0 * um);

                const G4bool onSideX = (std::abs(std::abs(local.x()) - hx) < tol);
                const G4bool onSideY = (std::abs(std::abs(local.y()) - hy) < tol);
                const G4bool onTop = (std::abs(local.z() - hz) < tol);
                const G4bool onBottom = (std::abs(local.z() + hz) < tol);

                const G4bool isSide = (onSideX || onSideY) && !(onTop || onBottom);
                const G4bool apply =
                    (isSide && g_sanity_wall_apply_side) ||
                    (onTop && g_sanity_wall_apply_top);

                if (apply)
                {
                    // Outward normal from crystal into gap
                    G4ThreeVector n(0, 0, 0);
                    if (onSideX) n = G4ThreeVector((local.x() > 0) ? 1.0 : -1.0, 0, 0);
                    else if (onSideY) n = G4ThreeVector(0, (local.y() > 0) ? 1.0 : -1.0, 0);
                    else if (onTop) n = G4ThreeVector(0, 0, 1.0);

                    if (n.mag2() > 0.0)
                    {
                        // Count wall interaction (per photon)
                        fEventAction->fWallHitCount++;

                        G4double R = g_sanity_wall_reflectivity;
                        if (R < 0.0) R = 0.0;
                        if (R > 1.0) R = 1.0;

                        if (G4UniformRand() < R)
                        {
                            G4ThreeVector newDir;
                            if (g_sanity_wall_model == SANITY_WALL_SPECULAR)
                            {
                                // Specular reflection (mirror): reflect current direction about surface normal
                                const G4ThreeVector d = aTrack->GetMomentumDirection();
                                newDir = (d - 2.0 * (d.dot(n.unit())) * n.unit()).unit(); // points back into crystal
                            }
                            else
                            {
                                // Analytic sanity-check: use UNIFORM hemisphere to approximate "full mixing"
                                // assumptions in docs/analytic_check2.md.
                                newDir = SampleIsotropicHemisphere((-n).unit());
                            }

                            // Navigator stability: push the photon slightly back into the PRE volume (crystal).
                            // This avoids GeomNav1002 floods during large-statistics sanity checks.
                            const G4double stTol = G4GeometryTolerance::GetInstance()->GetSurfaceTolerance();
                            // Navigator stability:
                            // Keep the displacement at the geometry tolerance scale to avoid GeomNav1002 warning floods
                            // (large manual shifts without a Locate call).
                            const G4double push = 0.5 * stTol;
                            const G4ThreeVector prePos = preStepPoint->GetPosition();
                            const G4ThreeVector safePosInCrystal = prePos - push * n.unit(); // ensure inside crystal
                            aTrack->SetPosition(safePosInCrystal);
                            aTrack->SetMomentumDirection(newDir);
                            aTrack->SetTrackStatus(fAlive);
                            return;
                        }
                        else
                        {
                            // Absorbed by wall (maps to PTFE absorption channel in our bookkeeping)
                            fEventAction->fEscapePTFE++;
                            aTrack->SetTrackStatus(fStopAndKill);
                            return;
                        }
                    }
                }
            }
        }

        if (g_side_contact_ratio > 0.0)
        {
            const G4bool isBoundary = (postStepPoint->GetStepStatus() == fGeomBoundary);
            const G4bool crystalToGap = isBoundary && (preVolumeName == gN_sc_crystal) && (postVolumeName == "sc_gap");

            if (crystalToGap)
            {
                // Identify whether this boundary point is on a side face (not top/bottom).
                // We use local position in the CRYSTAL logical volume; crystal is axis-aligned.
                const auto touch = preStepPoint->GetTouchableHandle();
                const G4ThreeVector local = touch->GetHistory()->GetTopTransform().TransformPoint(preStepPoint->GetPosition());

                const G4double hx = 0.5 * g_crystalX;
                const G4double hy = 0.5 * g_crystalY;
                const G4double hz = 0.5 * g_crystalZ;
                const G4double tol = std::max(10.0 * G4GeometryTolerance::GetInstance()->GetSurfaceTolerance(), 0.1 * um);

                const G4bool onSideX = (std::abs(std::abs(local.x()) - hx) < tol);
                const G4bool onSideY = (std::abs(std::abs(local.y()) - hy) < tol);
                const G4bool onTopOrBottom = (std::abs(std::abs(local.z()) - hz) < tol);
                const G4bool isSide = (onSideX || onSideY) && !onTopOrBottom;

                if (isSide && (G4UniformRand() < g_side_contact_ratio))
                {
                    // Determine outward normal from crystal into gap.
                    G4ThreeVector n(0, 0, 0);
                    if (onSideX)
                        n = G4ThreeVector((local.x() > 0) ? 1.0 : -1.0, 0, 0);
                    else if (onSideY)
                        n = G4ThreeVector(0, (local.y() > 0) ? 1.0 : -1.0, 0);
                    else
                        n = G4ThreeVector(0, 0, 0);

                    if (n.mag2() > 0.0)
                    {
                        // Use wrapper PTFE surface reflectivity curve (currently surf_Hreflex from config.hh).
                        const G4double energy = aTrack->GetTotalEnergy();
                        G4double R = GetSurfaceReflectivity(surf_Hreflex, energy, 1.0);
                        if (R < 0.0) R = 0.0;
                        if (R > 1.0) R = 1.0;

                        if (G4UniformRand() < R)
                        {
                            const G4ThreeVector newDir = SampleLambertianHemisphere((-n).unit()); // points into crystal

                            // Push slightly back into the crystal to avoid GeomNav1002 floods.
                            const G4double st = G4GeometryTolerance::GetInstance()->GetSurfaceTolerance();
                            const G4double push = 0.5 * st;
                            const G4ThreeVector prePos = preStepPoint->GetPosition();
                            const G4ThreeVector safePosInCrystal = prePos - push * n.unit();
                            aTrack->SetPosition(safePosInCrystal);
                            aTrack->SetMomentumDirection(newDir);
                            aTrack->SetTrackStatus(fAlive);
                            return;
                        }
                        else
                        {
                            // Absorbed by PTFE contact patch (statistical equivalent)
                            fEventAction->fEscapePTFE++;
                            aTrack->SetTrackStatus(fStopAndKill);
                            return;
                        }
                    }
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

