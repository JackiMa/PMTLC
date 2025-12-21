#include <vector>

#include "SiPINLCDetectorConstruction.hh"
#include "SiPINLCDetectorMessenger.hh"
#include "SiPINLCLayerSensitiveDetector.hh"

#include "G4Element.hh"
#include "G4GDMLParser.hh"
#include "G4Box.hh"
#include "G4LogicalBorderSurface.hh"
#include "G4LogicalSkinSurface.hh"
#include "G4LogicalVolume.hh"
#include "G4OpticalSurface.hh"
#include "G4VPhysicalVolume.hh"
#include "G4SystemOfUnits.hh"
#include "G4ThreeVector.hh"
#include "G4VisAttributes.hh"
#include "G4SubtractionSolid.hh"
#include "G4UnionSolid.hh"
#include "G4AssemblyVolume.hh"

#include "G4GlobalMagFieldMessenger.hh"
#include "G4SDManager.hh"
#include "G4SDChargedFilter.hh"
#include "G4MultiFunctionalDetector.hh"
#include "G4VPrimitiveScorer.hh"
#include "G4PSEnergyDeposit.hh"
#include "G4Exception.hh"

#include "MyMaterials.hh"
#include "MyPhysicalVolume.hh"
#include "CustomScorer.hh"
#include "utilities.hh"
#include "config.hh"
#include "SiPINLCParameterMessenger.hh"

namespace {
  void ApplyAbsorptionScaleIfNeeded(G4Material* mat, G4double targetScale)
  {
    if (!mat) return;
    if (targetScale <= 0.0) return;
    if (std::abs(targetScale - g_crystal_absorption_scale_applied) < 1e-12) return;

    auto* mpt = mat->GetMaterialPropertiesTable();
    if (!mpt) return;
    auto* absVec = mpt->GetProperty("ABSLENGTH");
    if (!absVec) return;

    const G4double e550 = (1239.841939 * eV * nm) / g_debug_opticalphoton_wavelength;
    const G4double Lbefore = absVec->Value(e550);
    const G4double factor = targetScale / g_crystal_absorption_scale_applied;

    std::vector<G4double> energies;
    std::vector<G4double> absScaled;
    energies.reserve(absVec->GetVectorLength());
    absScaled.reserve(absVec->GetVectorLength());
    for (size_t i = 0; i < absVec->GetVectorLength(); ++i)
    {
      const G4double e = absVec->Energy(i);
      energies.push_back(e);
      // IMPORTANT: Value(x) takes energy x, NOT an index. Use Value(Energy(i)) to sample at the tabulated node.
      absScaled.push_back(absVec->Value(e) * factor);
    }

    // Geant4 requires energies to be strictly increasing.
    if (energies.size() >= 2 && energies.front() > energies.back())
    {
      std::reverse(energies.begin(), energies.end());
      std::reverse(absScaled.begin(), absScaled.end());
    }

    // Replace ABSLENGTH with scaled one (avoid cumulative scaling on repeated rebuilds)
    mpt->RemoveProperty("ABSLENGTH");
    mpt->AddProperty("ABSLENGTH", energies, absScaled);

    g_crystal_absorption_scale_applied = targetScale;
    auto* absNew = mpt->GetProperty("ABSLENGTH");
    const G4double L550 = absNew ? absNew->Value(e550) : -1.0;
    G4cout << "[Material] Crystal ABSLENGTH scaled by factor=" << factor
           << " (applied=" << g_crystal_absorption_scale_applied << "), "
           << "ABSLENGTH(λ=" << (g_debug_opticalphoton_wavelength/nm) << " nm): "
           << (Lbefore/mm) << " mm -> " << (L550/mm) << " mm" << G4endl;
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
// 在这里对类相关参数进行初始化
SiPINLCDetectorConstruction::SiPINLCDetectorConstruction()
    : G4VUserDetectorConstruction()
{
  fDumpGdmlFileName = "LightCollecion.gdml";
  fVerbose = false;  // 是否输出详细信息
  fDumpGdml = false; // 是否保存GDML的几何文件
  // create a messenger for this class
  fDetectorMessenger = new SiPINLCDetectorMessenger(this);
  // create parameter messenger for MAC control
  fParameterMessenger = new SiPINLCParameterMessenger();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
SiPINLCDetectorConstruction::~SiPINLCDetectorConstruction()
{
  delete fDetectorMessenger;
  delete fParameterMessenger;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
G4VPhysicalVolume *SiPINLCDetectorConstruction::Construct()
{
  G4bool checkOverlaps = true;
  // G4bool checkOverlaps = false;
  G4String name; // 各个实体的name

  // ------------- Volumes --------------
  // s_ for soild_
  // l_ for logical_
  // p_ for physical_
  //
  // The world
  G4Box *s_world = new G4Box("World", 0.5 * g_worldX, 0.5 * g_worldY, 0.5 * g_worldZ);
  G4LogicalVolume *l_world = new G4LogicalVolume(s_world, g_world_material, "World");
  MyPhysicalVolume *p_world = new MyPhysicalVolume(0, G4ThreeVector(), "World", l_world, nullptr, false, 0, checkOverlaps);

  // Apply absorption scaling (e.g. "ABSLENGTH × 10" to suppress self-absorption for analytic sanity checks)
  ApplyAbsorptionScaleIfNeeded(g_crystal_material, g_crystal_absorption_scale);

  // ====================================
  // ====== Photomultiplier tubes =======
  // ====================================
  //
  // sipin
  name = "sipin";
  G4Box *s_sipin = new G4Box(name, 0.5 * g_sipin_X, 0.5 * g_sipin_Y, 0.5 * g_sipin_thickness);
  G4LogicalVolume *l_sipin = new G4LogicalVolume(s_sipin, g_ceramics, name);
  MyPhysicalVolume *p_sipin = new MyPhysicalVolume(0, g_sipin_pos, name, l_sipin, p_world, false, 0, checkOverlaps);
  fVolumeMap[name] = p_sipin;

  // sipin - si (active)
  // 顶面直接是 Si，与 grease / bottom airgap 相接触（用于 grease→Si 的 TMM/PDE 处理）。
  name = gN_sipin_si;
  G4Box *s_si = new G4Box(name, 0.5 * g_sipin_X, 0.5 * g_sipin_Y, 0.5 * g_si_thickness);
  G4LogicalVolume *l_si = new G4LogicalVolume(s_si, g_si_material, name);
  // 将 Si 放在 sipin 顶部：Si 上表面与 sipin 上表面齐平
  MyPhysicalVolume *p_si = new MyPhysicalVolume(
      0,
      G4ThreeVector(0, 0, 0.5 * g_sipin_thickness - 0.5 * g_si_thickness),
      name, l_si, p_sipin, false, 0, checkOverlaps);
  fVolumeMap[name] = p_si;

  // 设置 si 的颜色为银色
  G4VisAttributes *siVisAtt = new G4VisAttributes(G4Colour(0.75, 0.75, 0.75, 0.7)); // 银色
  siVisAtt->SetForceSolid(true);
  siVisAtt->SetVisibility(true);
  l_si->SetVisAttributes(siVisAtt);

  // sipin - ceramics
  name = gN_sipin_ceramics;
  G4Box *s_ceramics = new G4Box(name, 0.5 * g_sipin_X, 0.5 * g_sipin_Y, 0.5 * g_ceramics_thickness);
  G4LogicalVolume *l_ceramics = new G4LogicalVolume(s_ceramics, g_ceramics, name);
  MyPhysicalVolume *p_ceramics = new MyPhysicalVolume(0, G4ThreeVector(0, 0, -0.5 * g_sipin_thickness + 0.5 * g_ceramics_thickness), name, l_ceramics, p_sipin, false, 0, checkOverlaps);
  fVolumeMap[name] = p_ceramics;

  // 设置 ceramics 的颜色为黑色
  G4VisAttributes *ceramicsVisAtt = new G4VisAttributes(G4Colour(0.1, 0.1, 0.1, 0.3)); // 黑色
  ceramicsVisAtt->SetForceSolid(true);
  ceramicsVisAtt->SetVisibility(true);
  l_ceramics->SetVisAttributes(ceramicsVisAtt);

  // ==================================
  // =========== Scintillator ===========
  // ====================================
  //
  G4ThreeVector crystal_pos;
  G4ThreeVector wrapper_pos;
  G4double wrapperX,wrapperY,wrapperZ;
  MyPhysicalVolume *p_crystal_Container;
   G4VisAttributes *VisAtt;
  if (g_wrapper_Type == CUBE)
  {
    // wrapper
    name = gN_sc_wrapper;
    wrapperX = 5 * cm;
    wrapperY = 5 * cm;
    wrapperZ = 2.2 * cm;
    G4double wrapperThickness = g_wrapper_thickness;
    G4double gapXY = wrapperX - wrapperThickness;
    G4double gapZ = wrapperZ - 0.5*wrapperThickness;
    wrapper_pos = g_sipin_pos + G4ThreeVector(0, 0, 0.5 * g_sipin_thickness + 0.5 * wrapperZ);
    G4ThreeVector gap_pos = G4ThreeVector(0, 0, - 0.25*wrapperThickness);

    G4Box *s_wrapper = new G4Box(name, 0.5 * wrapperX, 0.5 * wrapperY, 0.5 * wrapperZ);
    G4LogicalVolume *l_wrapper = new G4LogicalVolume(s_wrapper, g_wrapper_material, name);
    MyPhysicalVolume *p_wrapper = new MyPhysicalVolume(0, wrapper_pos, name, l_wrapper, p_world, false, 0, checkOverlaps);
    fVolumeMap[name] = p_wrapper;

    name = "sc_gap";
    G4Box *s_gap = new G4Box(name, 0.5 * gapXY, 0.5 * gapXY, 0.5 * gapZ);
    G4LogicalVolume *l_gap = new G4LogicalVolume(s_gap, g_world_material, name);
    MyPhysicalVolume *p_gap = new MyPhysicalVolume(0, gap_pos, name, l_gap, p_wrapper, false, 0, checkOverlaps);
    fVolumeMap[name] = p_gap;
    p_crystal_Container = p_gap;

    VisAtt = new G4VisAttributes(G4Colour(1, 1, 1, 0.1));
    VisAtt->SetForceSolid(true);
    VisAtt->SetVisibility(true);
    l_wrapper->SetVisAttributes(VisAtt);
    G4VisAttributes *wrapperVisAtt = new G4VisAttributes(G4Colour(0.4, 0.3, 0.3, 0.3));
    wrapperVisAtt->SetForceSolid(true);
    wrapperVisAtt->SetVisibility(true);
    l_gap->SetVisAttributes(wrapperVisAtt);

    new G4LogicalSkinSurface("WrapperGapSurface", l_wrapper, surf_Hreflex);

    crystal_pos = G4ThreeVector(0, 0, -0.5 * gapZ + 0.5 * g_crystalZ + g_grease_thickness);
  }
  if (g_wrapper_Type == TEFLON)
  {
    // wrapper - 论文中的PTFE封装
    //
    // 关键修复（对齐 d44848b 的“底部开口”物理图像）：
    // - `sc_gap`（空气层）放在 World 里，底面与 SiPIN 顶面齐平
    // - Wrapper 不作为 `sc_gap` 的母体积，而是一个“壳体”（SubtractionSolid 开腔），底部不封死
    //   这样 `G4LogicalSkinSurface` 的 PTFE 反射面不会把底部通道“封住”
    //
    // 结构（从下到上）：
    //   sipin_si
    //   sc_gap (air container in World; contains optional optical_grease + crystal)
    //   wrapper shell (PTFE/PVC) around sides + top (bottom open)

    const G4double grease_effective = (g_grease_thickness > 10 * um) ? g_grease_thickness : 0.0;
    const G4double bottom_gap = (grease_effective > 0.0) ? grease_effective : g_bottom_airgap_thickness;
    const G4double sipin_top_z = g_sipin_pos.z() + 0.5 * g_sipin_thickness;

    // --- sc_gap: air container in world
    // IMPORTANT:
    // Do NOT shrink `sc_gap` relative to the wrapper cavity.
    // If `sc_gap` is even slightly smaller, Geant4 creates a thin "world air" slit
    // between `sc_gap` and the PTFE wrapper inner wall, and also between `sc_gap` and SiPIN top.
    // That breaks the intended optics topology:
    // - PTFE reflective skin surface no longer acts (photons never touch the wrapper)
    // - grease/sc_gap -> sipin_si interface hook may not trigger (extra world boundary)
    name = "sc_gap";
    const G4double gapX = g_crystalX + 2 * g_gap_thickness;
    const G4double gapY = g_crystalY + 2 * g_gap_thickness;
    const G4double gapZ = bottom_gap + g_crystalZ + g_top_airgap_thickness;
    G4Box *s_gap = new G4Box(name, 0.5 * gapX, 0.5 * gapY, 0.5 * gapZ);
    G4LogicalVolume *l_gap = new G4LogicalVolume(s_gap, g_world_material, name);
    const G4ThreeVector gap_world_pos(0, 0, sipin_top_z + 0.5 * gapZ);
    MyPhysicalVolume *p_gap = new MyPhysicalVolume(0, gap_world_pos, name, l_gap, p_world, false, 0, checkOverlaps);
    fVolumeMap[name] = p_gap;
    p_crystal_Container = p_gap;

    // --- wrapper shell in world (open bottom): outer box minus inner cavity box
    name = gN_sc_wrapper;
    wrapperX = gapX + 2 * g_wrapper_thickness;
    wrapperY = gapY + 2 * g_wrapper_thickness;
    wrapperZ = gapZ + g_wrapper_thickness; // only top cap thickness
    wrapper_pos = G4ThreeVector(0, 0, sipin_top_z + 0.5 * wrapperZ);

    G4Box *s_wrapper_outer = new G4Box("wrapper_outer", 0.5 * wrapperX, 0.5 * wrapperY, 0.5 * wrapperZ);
    G4Box *s_wrapper_inner = new G4Box("wrapper_cavity", 0.5 * gapX, 0.5 * gapY, 0.5 * gapZ);
    // cavity bottom aligned with wrapper bottom => open bottom, top cap thickness = g_wrapper_thickness
    const G4ThreeVector cavity_offset(0, 0, -0.5 * wrapperZ + 0.5 * gapZ);
    G4SubtractionSolid *s_wrapper = new G4SubtractionSolid(name, s_wrapper_outer, s_wrapper_inner, nullptr, cavity_offset);
    G4LogicalVolume *l_wrapper = new G4LogicalVolume(s_wrapper, g_wrapper_material, name);
    MyPhysicalVolume *p_wrapper = new MyPhysicalVolume(0, wrapper_pos, name, l_wrapper, p_world, false, 0, checkOverlaps);
    fVolumeMap[name] = p_wrapper;

    // PTFE reflective surface (skin) on wrapper shell
    new G4LogicalSkinSurface("WrapperGapSurface", l_wrapper, surf_Hreflex);

    // Visuals
    VisAtt = new G4VisAttributes(G4Colour(1, 1, 1, 0.1));
    VisAtt->SetForceSolid(true);
    VisAtt->SetVisibility(true);
    l_wrapper->SetVisAttributes(VisAtt);
    G4VisAttributes *gapVisAtt = new G4VisAttributes(G4Colour(0.4, 0.3, 0.3, 0.3));
    gapVisAtt->SetForceSolid(true);
    gapVisAtt->SetVisibility(true);
    l_gap->SetVisAttributes(gapVisAtt);

    // --- optional grease slab inside sc_gap (bottom only)
    if (grease_effective > 0.0)
    {
      name = "optical_grease";
      // grease slab at bottom of sc_gap, XY matches crystal bottom
      const G4ThreeVector grease_pos_in_gap(0, 0, -0.5 * gapZ + 0.5 * grease_effective);
      G4Box *s_grease = new G4Box(name, 0.5 * g_crystalX, 0.5 * g_crystalY, 0.5 * grease_effective);
      G4LogicalVolume *l_grease = new G4LogicalVolume(s_grease, g_grease_material, name);
      MyPhysicalVolume *p_grease = new MyPhysicalVolume(0, grease_pos_in_gap, name, l_grease, p_gap, false, 0, checkOverlaps);
      fVolumeMap[name] = p_grease;

      G4VisAttributes *greaseVisAtt = new G4VisAttributes(G4Colour(0.1, 0.5, 1, 0.5));
      greaseVisAtt->SetForceSolid(true);
      greaseVisAtt->SetVisibility(true);
      l_grease->SetVisAttributes(greaseVisAtt);
    }

    // Crystal position inside sc_gap: above bottom layer (grease or thin air gap)
    crystal_pos = G4ThreeVector(0, 0, -0.5 * gapZ + bottom_gap + 0.5 * g_crystalZ);

    if (grease_effective > 0) {
      G4cout << "=== Geometry Info (With Grease) ===" << G4endl;
      G4cout << "Grease is a bottom slab only (sides/top remain air) [EXPECTED]" << G4endl;
    } else {
      G4cout << "=== Geometry Info (No Grease) ===" << G4endl;
      G4cout << "Crystal bottom faces thin air layer (bottomAirGap) [EXPECTED]" << G4endl;
    }
    
    // 输出几何信息用于验证（重点：确认 bottom 开口没有被 wrapper “封死”）
    const G4double z_sipin_top = sipin_top_z;
    const G4double z_gap_bot = gap_world_pos.z() - 0.5 * gapZ;
    const G4double z_gap_top = gap_world_pos.z() + 0.5 * gapZ;
    const G4double z_wrapper_bot = wrapper_pos.z() - 0.5 * wrapperZ;
    const G4double z_wrapper_top = wrapper_pos.z() + 0.5 * wrapperZ;

    G4double z_grease_bot = 0, z_grease_top = 0, z_crystal_bot = 0, z_crystal_top = 0;
    if (grease_effective > 0.0)
    {
      const G4double z_grease_center = gap_world_pos.z() + (-0.5 * gapZ + 0.5 * grease_effective);
      z_grease_bot = z_grease_center - 0.5 * grease_effective;
      z_grease_top = z_grease_center + 0.5 * grease_effective;
    }
    // crystal (in sc_gap, above bottom layer)
    const G4double z_crystal_center = gap_world_pos.z() + crystal_pos.z();
    z_crystal_bot = z_crystal_center - 0.5 * g_crystalZ;
    z_crystal_top = z_crystal_center + 0.5 * g_crystalZ;

    G4cout << "Crystal size: " << g_crystalX/mm << " x " << g_crystalY/mm << " x " << g_crystalZ/mm << " mm^3" << G4endl;
    G4cout << "Top air gap thickness: " << g_top_airgap_thickness/um << " um" << G4endl;
    G4cout << "Side air gap thickness: " << g_gap_thickness/um << " um" << G4endl;
    G4cout << "Grease thickness: " << grease_effective/um << " um" << G4endl;
    G4cout << "Bottom air gap thickness: " << g_bottom_airgap_thickness/um << " um" << G4endl;
    G4cout << "PTFE wrapper thickness: " << g_wrapper_thickness/mm << " mm" << G4endl;
    G4cout << "Wrapper size: " << wrapperX/mm << " x " << wrapperY/mm << " x " << wrapperZ/mm << " mm^3" << G4endl;
    G4cout << "Gap size: " << gapX/mm << " x " << gapY/mm << " x " << gapZ/mm << " mm^3" << G4endl;
    G4cout << "Z check (mm): sipin_top=" << z_sipin_top / mm
           << " gap_bot=" << z_gap_bot / mm << " gap_top=" << z_gap_top / mm
           << " wrapper_bot=" << z_wrapper_bot / mm << " wrapper_top=" << z_wrapper_top / mm << G4endl;
    if (grease_effective > 0.0)
    {
      G4cout << "Z check (mm): grease_bot=" << z_grease_bot / mm << " grease_top=" << z_grease_top / mm
             << " crystal_bot=" << z_crystal_bot / mm << " crystal_top=" << z_crystal_top / mm << G4endl;
    }
    G4cout << "===========================================" << G4endl;
  }
  else if(    g_wrapper_Type == CYLINDER  ){
    // wrapper
    name = gN_sc_wrapper;
    G4double wrapperR = 3.2 * cm;
    wrapperZ = 2.2 * cm;
    G4double wrapperThickness = g_wrapper_thickness;
    G4double gapR = wrapperR - wrapperThickness;
    G4double gapH = wrapperZ - wrapperThickness; 
    wrapper_pos = g_sipin_pos + G4ThreeVector(0, 0, 0.5 * g_sipin_thickness + 0.5* wrapperZ);
    G4ThreeVector gap_pos = G4ThreeVector(0, 0, - 0.5*wrapperThickness);

    G4Tubs *s_wrapper = new G4Tubs(name, 0, wrapperR, 0.5 * wrapperZ, 0, 360 * deg);
    G4LogicalVolume *l_wrapper = new G4LogicalVolume(s_wrapper, g_wrapper_material, name);
    MyPhysicalVolume *p_wrapper = new MyPhysicalVolume(0, wrapper_pos, name, l_wrapper, p_world, false, 0, checkOverlaps);
    fVolumeMap[name] = p_wrapper;

    name = "sc_gap";
    G4Tubs *s_gap = new G4Tubs(name, 0, gapR, 0.5 * gapH, 0, 360 * deg);
    G4LogicalVolume *l_gap = new G4LogicalVolume(s_gap, g_world_material, name);
    MyPhysicalVolume *p_gap = new MyPhysicalVolume(0, gap_pos, name, l_gap, p_wrapper, false, 0, checkOverlaps);
    fVolumeMap[name] = p_gap;
    p_crystal_Container = p_gap;

    VisAtt = new G4VisAttributes(G4Colour(1, 1, 1, 0.1));
    VisAtt->SetForceSolid(true);
    VisAtt->SetVisibility(true);
    l_wrapper->SetVisAttributes(VisAtt);
    VisAtt = new G4VisAttributes(G4Colour(0.4, 0.3, 0.3, 0.3));
    VisAtt->SetForceSolid(true);
    VisAtt->SetVisibility(true);
    l_gap->SetVisAttributes(VisAtt);

    new G4LogicalSkinSurface("WrapperGapSurface", l_wrapper, surf_Hreflex);

    crystal_pos = G4ThreeVector(0, 0, -0.5 * gapH + 0.5 * g_crystalZ + g_grease_thickness);
  }

  // crystal
  name = gN_sc_crystal;
  G4Box *s_crystal = new G4Box(name, 0.5 * g_crystalX, 0.5 * g_crystalY, 0.5 * g_crystalZ);
  G4LogicalVolume *l_crystal = new G4LogicalVolume(s_crystal, g_crystal_material, name);
  MyPhysicalVolume *p_crystal = new MyPhysicalVolume(0, crystal_pos, name, l_crystal, p_crystal_Container, false, 0, checkOverlaps);
  fVolumeMap[name] = p_crystal;
  G4VisAttributes *crystalVisAtt = new G4VisAttributes(G4Colour(1.0, 0.65, 0, 0.7));
  crystalVisAtt->SetForceSolid(true); // 设置为实心，以便可以看到透明度
  crystalVisAtt->SetVisibility(true);
  l_crystal->SetVisAttributes(crystalVisAtt);
  
  // === Crystal surface micro-roughness (UNIFIED sigma_alpha) ===
  // Apply on crystal <-> sc_gap interfaces.
  // - sigma_alpha = 0: keep default smooth Fresnel interface
  // - sigma_alpha > 0: micro-facet normal spread -> angular diffusion -> helps break trapped modes
  if (g_crystal_sigma_alpha > 0.0)
  {
    auto itGap = fVolumeMap.find("sc_gap");
    if (itGap != fVolumeMap.end() && itGap->second)
    {
      auto* p_gap = itGap->second;
      auto* surfCrystal = new G4OpticalSurface("CrystalAir_UNIFIED");
      surfCrystal->SetType(dielectric_dielectric);
      surfCrystal->SetModel(unified);
      // Use ground finish so sigma_alpha drives micro-facet normal distribution (angular diffusion).
      surfCrystal->SetFinish(ground);
      surfCrystal->SetSigmaAlpha(g_crystal_sigma_alpha);

      // Border surfaces are directional; define both directions for symmetry.
      new G4LogicalBorderSurface("CrystalToGapSurface", p_crystal, p_gap, surfCrystal);
      new G4LogicalBorderSurface("GapToCrystalSurface", p_gap, p_crystal, surfCrystal);

      G4cout << "[Surface] Crystal sigma_alpha (UNIFIED) = " << g_crystal_sigma_alpha
             << " rad applied on crystal <-> sc_gap." << G4endl;
    }
    else
    {
      G4cerr << "[Surface] WARNING: sc_gap not found; cannot apply crystal sigma_alpha surface." << G4endl;
    }
  }


// Grease 创建：TEFLON 类型在上面的分支中已经在 gap 内创建了
// 这里只处理非 TEFLON 类型（CUBE, CYLINDER）
if(g_wrapper_Type != TEFLON && g_grease_thickness > 10*um){
    name = "optical_grease";
    // Grease 放在 SiPIN 上表面之上：
    G4ThreeVector grease_pos = g_sipin_pos + G4ThreeVector(0, 0, 0.5 * g_sipin_thickness + 0.5*g_grease_thickness);
    G4Box *s_grease = new G4Box(name, 0.5 * g_grease_X, 0.5 * g_grease_Y, 0.5*g_grease_thickness);
    G4LogicalVolume *l_grease = new G4LogicalVolume(s_grease, g_grease_material, name);
    MyPhysicalVolume *p_grease = new MyPhysicalVolume(0, grease_pos, name, l_grease, p_world, false, 0, checkOverlaps);
    fVolumeMap[name] = p_grease;
    VisAtt = new G4VisAttributes(G4Colour(0.1, 0.5, 1, 0.3));
    VisAtt->SetForceSolid(true);
    VisAtt->SetVisibility(true);
    l_grease->SetVisAttributes(VisAtt);
}

  myPrint(lv, f("fVolumeMap length is {}\n", fVolumeMap.size()));
  for (const auto &pair : fVolumeMap)
  {
    std::cout << "Volume name: " << pair.first << ", Volume address: " << pair.second << std::endl;
  }

  if (fDumpGdml)
  {
    std::ifstream ifile(fDumpGdmlFileName);
    if (ifile)
    {
      G4cout << fDumpGdmlFileName << " 已存在，不再写入。" << G4endl;
    }
    else
    {
      G4GDMLParser *parser = new G4GDMLParser();
      parser->Write(fDumpGdmlFileName, p_world);
    }
  }

  return p_world;
}

MyPhysicalVolume *SiPINLCDetectorConstruction::GetMyVolume(G4String volumeName) const
{
  auto it = fVolumeMap.find(volumeName);
  if (it != fVolumeMap.end())
  {
    myPrint(lv, "Volume is found!\n");
    return it->second;
  }
  else
  {
    myPrint(lv, "Volume not found!\n");
    G4Exception("SiPINLCDetectorConstruction::GetMyVolume",
                "VolumeNotFound", FatalException,
                ("Volume " + volumeName + " not found in the volume map.").c_str());
    return nullptr; // 这行代码实际上不会被执行，因为G4Exception会终止程序
  }
}

void SiPINLCDetectorConstruction::ConstructSDandField()
{
  G4SDManager* sdManager = G4SDManager::GetSDMpointer();
  sdManager->SetVerboseLevel(1);
  
  //
  // Scorers - 检查是否已存在，避免重复创建
  //
  G4VSensitiveDetector* existingSD = sdManager->FindSensitiveDetector(gN_sc_crystal, false);
  
  if (!existingSD) {
    // SD 不存在，首次创建
    G4VPrimitiveScorer *primitive;
    G4int fillH1ID;
    // declare crystal as a MultiFunctionalDetector scorer
    // include Edep and scintillation light

    auto crystal = new G4MultiFunctionalDetector(gN_sc_crystal);
    sdManager->AddNewDetector(crystal);
    primitive = new G4PSEnergyDeposit("Edep");
    crystal->RegisterPrimitive(primitive);

    fillH1ID = 0;
    primitive = new SCLightScorer("crystal_scintillation_H1", fillH1ID);
    crystal->RegisterPrimitive(primitive);

    fillH1ID = 1;
    primitive = new CherenkovLightScorer("crystal_cherenkovLight_H1", fillH1ID);
    crystal->RegisterPrimitive(primitive);

    SetSensitiveDetector(gN_sc_crystal, crystal);
  } else {
    // SD 已存在，重新关联到新的逻辑体
    SetSensitiveDetector(gN_sc_crystal, existingSD);
  }

  // declare si as a MultiFunctionalDetector scorer
  // include light spectrum and position
  // auto si = new G4MultiFunctionalDetector(gN_sipin_si);
  // G4SDManager::GetSDMpointer()->AddNewDetector(si);
  // fillH1ID = 2;
  // fillH2ID = 1;
  // primitive = new siScorer("si_Entry_H1", fillH1ID,fillH2ID);
  // si->RegisterPrimitive(primitive);
  // SetSensitiveDetector(gN_sipin_si, si);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
void SiPINLCDetectorConstruction::SetDumpGdml(G4bool val) { fDumpGdml = val; }

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
G4bool SiPINLCDetectorConstruction::IsDumpGdml() const { return fDumpGdml; }

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
void SiPINLCDetectorConstruction::SetVerbose(G4bool val) { fVerbose = val; }

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
G4bool SiPINLCDetectorConstruction::IsVerbose() const { return fVerbose; }

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
void SiPINLCDetectorConstruction::SetDumpGdmlFile(G4String filename)
{
  fDumpGdmlFileName = filename;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
G4String SiPINLCDetectorConstruction::GetDumpGdmlFile() const
{
  return fDumpGdmlFileName;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
void SiPINLCDetectorConstruction::PrintError(G4String ed)
{
  G4Exception("SiPINLCDetectorConstruction:MaterialProperty test", "op001",
              FatalException, ed);
}
