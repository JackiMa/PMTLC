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
    // 正确的层级结构（避免缝隙）：
    //   World → wrapper → sc_gap → crystal
    //
    // 底部开窗实现：
    //   - wrapper 底部与 SiPIN 顶面齐平
    //   - sc_gap 通过负向 z 偏移，使其底面与 wrapper 底面齐平
    //   - 这样 SkinSurface 只在 sc_gap 碰到 wrapper 的侧面和顶面生效
    //   - 底面光子直接从 sc_gap 进入 World，然后进入 grease/sipin
    //
    // 结构（从下到上）：
    //   sipin_si (in World)
    //   optical_grease (in World, optional)
    //   wrapper (in World) 包含 sc_gap
    //   sc_gap (in wrapper) 包含 crystal

    const G4double grease_effective = (g_grease_thickness > 10 * um) ? g_grease_thickness : 0.0;
    const G4double sipin_top_z = g_sipin_pos.z() + 0.5 * g_sipin_thickness;
    // 底部间隙高度（grease 或 空气层）
    const G4double bottom_layer_height = (grease_effective > 0) ? grease_effective : g_bottom_airgap_thickness;

    // --- wrapper: 实心盒子，包含 sc_gap
    name = gN_sc_wrapper;
    // wrapper 尺寸：晶体 + 侧面间隙 + wrapper 厚度
    wrapperX = g_crystalX + 2 * g_gap_thickness + 2 * g_wrapper_thickness;
    wrapperY = g_crystalY + 2 * g_gap_thickness + 2 * g_wrapper_thickness;
    // wrapper 高度：晶体 + 顶面间隙 + 顶面 wrapper 厚度（底面开窗，不加底面厚度）
    wrapperZ = g_crystalZ + g_top_airgap_thickness + g_wrapper_thickness;
    
    // wrapper 位置：底面与底部间隙层顶面齐平
    G4double wrapper_bottom_z = sipin_top_z + bottom_layer_height;
    wrapper_pos = G4ThreeVector(0, 0, wrapper_bottom_z + 0.5 * wrapperZ);

    G4Box *s_wrapper = new G4Box(name, 0.5 * wrapperX, 0.5 * wrapperY, 0.5 * wrapperZ);
    G4LogicalVolume *l_wrapper = new G4LogicalVolume(s_wrapper, g_wrapper_material, name);
    MyPhysicalVolume *p_wrapper = new MyPhysicalVolume(0, wrapper_pos, name, l_wrapper, p_world, false, 0, checkOverlaps);
    fVolumeMap[name] = p_wrapper;

    // --- sc_gap: 空气层，在 wrapper 内部
    name = "sc_gap";
    G4double gapX = g_crystalX + 2 * g_gap_thickness;
    G4double gapY = g_crystalY + 2 * g_gap_thickness;
    // gap 高度与 wrapper 内腔相同（wrapper 只有顶面封闭）
    G4double gapZ = wrapperZ - g_wrapper_thickness;  // 去掉顶面 wrapper 厚度
    // gap 位置：底面与 wrapper 底面齐平（负向偏移，使底面开窗）
    G4ThreeVector gap_pos_in_wrapper(0, 0, -0.5 * g_wrapper_thickness);

    G4Box *s_gap = new G4Box(name, 0.5 * gapX, 0.5 * gapY, 0.5 * gapZ);
    G4LogicalVolume *l_gap = new G4LogicalVolume(s_gap, g_world_material, name);
    MyPhysicalVolume *p_gap = new MyPhysicalVolume(0, gap_pos_in_wrapper, name, l_gap, p_wrapper, false, 0, checkOverlaps);
    fVolumeMap[name] = p_gap;
    p_crystal_Container = p_gap;

    // PTFE reflective surface (skin) on wrapper
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

    // --- optional grease slab in World (between wrapper bottom and sipin top)
    // 注意：grease 需要覆盖整个 sc_gap 底面（不仅是晶体底面），否则光子从边缘逃逸
    if (grease_effective > 0.0)
    {
      name = "optical_grease";
      G4ThreeVector grease_pos(0, 0, sipin_top_z + 0.5 * grease_effective);
      // grease 尺寸与 sc_gap 底面相同
      G4Box *s_grease = new G4Box(name, 0.5 * gapX, 0.5 * gapY, 0.5 * grease_effective);
      G4LogicalVolume *l_grease = new G4LogicalVolume(s_grease, g_grease_material, name);
      MyPhysicalVolume *p_grease = new MyPhysicalVolume(0, grease_pos, name, l_grease, p_world, false, 0, checkOverlaps);
      fVolumeMap[name] = p_grease;

      G4VisAttributes *greaseVisAtt = new G4VisAttributes(G4Colour(0.1, 0.5, 1, 0.5));
      greaseVisAtt->SetForceSolid(true);
      greaseVisAtt->SetVisibility(true);
      l_grease->SetVisAttributes(greaseVisAtt);
    }
    else
    {
      // 无 grease 时，创建一个空气层覆盖 sc_gap 底面和 sipin 顶面之间
      // 尺寸与 sc_gap 底面相同，确保光子不会从边缘逃入 World
      name = "sc_bottom_gap";
      G4double bottom_air = bottom_layer_height;  // 已经在前面定义
      G4ThreeVector bottom_gap_pos(0, 0, sipin_top_z + 0.5 * bottom_air);
      G4Box *s_bottom_gap = new G4Box(name, 0.5 * gapX, 0.5 * gapY, 0.5 * bottom_air);
      G4LogicalVolume *l_bottom_gap = new G4LogicalVolume(s_bottom_gap, g_world_material, name);
      MyPhysicalVolume *p_bottom_gap = new MyPhysicalVolume(0, bottom_gap_pos, name, l_bottom_gap, p_world, false, 0, checkOverlaps);
      fVolumeMap[name] = p_bottom_gap;
      
      G4VisAttributes *bottomGapVisAtt = new G4VisAttributes(G4Colour(0.8, 0.8, 1.0, 0.3));
      bottomGapVisAtt->SetForceSolid(true);
      bottomGapVisAtt->SetVisibility(true);
      l_bottom_gap->SetVisAttributes(bottomGapVisAtt);
    }

    // Crystal position inside sc_gap: at bottom of gap
    crystal_pos = G4ThreeVector(0, 0, -0.5 * gapZ + 0.5 * g_crystalZ);

    // 输出几何信息用于验证
    G4cout << "=== Geometry Info (TEFLON mode - Hierarchical) ===" << G4endl;
    G4cout << "Structure: World -> wrapper -> sc_gap -> crystal" << G4endl;
    G4cout << "Crystal size: " << g_crystalX/mm << " x " << g_crystalY/mm << " x " << g_crystalZ/mm << " mm^3" << G4endl;
    G4cout << "Side gap thickness: " << g_gap_thickness/um << " um" << G4endl;
    G4cout << "Top air gap thickness: " << g_top_airgap_thickness/um << " um" << G4endl;
    G4cout << "Grease thickness: " << grease_effective/um << " um" << G4endl;
    G4cout << "PTFE wrapper thickness: " << g_wrapper_thickness/mm << " mm" << G4endl;
    G4cout << "Wrapper size: " << wrapperX/mm << " x " << wrapperY/mm << " x " << wrapperZ/mm << " mm^3" << G4endl;
    G4cout << "Gap size: " << gapX/mm << " x " << gapY/mm << " x " << gapZ/mm << " mm^3" << G4endl;
    G4cout << "Wrapper bottom z: " << wrapper_bottom_z/mm << " mm (should = sipin_top + grease)" << G4endl;
    G4cout << "SiPIN top z: " << sipin_top_z/mm << " mm" << G4endl;
    G4cout << "Bottom opening: light can exit sc_gap bottom -> (grease) -> sipin" << G4endl;
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
  //
  // NOTE: When sideContactRatio >= 1.0 or topContactRatio >= 1.0, we use dielectric_metal
  // surface instead to bypass TIR. In that case, skip this dielectric_dielectric surface.
  const G4bool usePtfeContact = (g_side_contact_ratio >= 1.0 || g_top_contact_ratio >= 1.0);
  
  if (g_crystal_sigma_alpha > 0.0 && !usePtfeContact)
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
  else if (g_crystal_sigma_alpha > 0.0 && usePtfeContact)
  {
    G4cout << "[Surface] Skipping sigma_alpha dielectric_dielectric surface (PTFE contact mode active)." << G4endl;
  }

  // === PTFE contact surface (for sanity-check scenarios B/C) ===
  // When sideContactRatio=1 and/or topContactRatio=1, we want the crystal side/top faces
  // to behave as if directly contacting PTFE with ideal reflectivity.
  // We achieve this by creating a LogicalBorderSurface between crystal and sc_gap.
  // Note: The bottom face is separated by sc_bottom_gap (or grease), so it won't be affected.
  //
  // CRITICAL BUG FIX: Must use dielectric_metal, NOT dielectric_dielectric!
  // With dielectric_dielectric, photons hitting the crystal-air interface undergo TIR
  // (total internal reflection) FIRST, which is ideal specular reflection that BYPASSES
  // all surface settings (REFLECTIVITY, SPECULARLOBECONSTANT, etc.).
  // With dielectric_metal, photons can only reflect or absorb (no Fresnel/TIR physics),
  // and "ground" finish produces true Lambertian diffuse reflection.
  if (g_side_contact_ratio >= 1.0 || g_top_contact_ratio >= 1.0)
  {
    auto itGap = fVolumeMap.find("sc_gap");
    if (itGap != fVolumeMap.end() && itGap->second)
    {
      auto* p_gap = itGap->second;
      
      // Create ideal PTFE surface with dielectric_metal for TRUE Lambertian
      auto* surfIdealPTFE = new G4OpticalSurface("IdealPTFE_Contact");
      surfIdealPTFE->SetType(dielectric_metal);  // KEY: NOT dielectric_dielectric!
      surfIdealPTFE->SetModel(unified);
      surfIdealPTFE->SetFinish(ground);  // ground = Lambertian for dielectric_metal
      
      // Set reflectivity based on contact ratio:
      // - If contactRatio = 1.0: use real PTFE reflectivity (97.5%)
      // - If contactRatio > 1.0: use ideal R=1 (for sanity-check only)
      const G4double ptfeR = (g_side_contact_ratio > 1.0 || g_top_contact_ratio > 1.0) ? 1.0 : 0.975;
      const G4int nEntries = 2;
      G4double photonEnergy[nEntries] = {1.0*eV, 6.0*eV};
      G4double reflectivity[nEntries] = {ptfeR, ptfeR};  // 97.5% or 100%
      G4double efficiency[nEntries] = {0.0, 0.0};    // no detection
      
      G4MaterialPropertiesTable* mpt = new G4MaterialPropertiesTable();
      mpt->AddProperty("REFLECTIVITY", photonEnergy, reflectivity, nEntries);
      mpt->AddProperty("EFFICIENCY", photonEnergy, efficiency, nEntries);
      surfIdealPTFE->SetMaterialPropertiesTable(mpt);

      // Apply BorderSurface between crystal and sc_gap (affects side/top faces)
      new G4LogicalBorderSurface("CrystalToGap_PTFE", p_crystal, p_gap, surfIdealPTFE);
      new G4LogicalBorderSurface("GapToCrystal_PTFE", p_gap, p_crystal, surfIdealPTFE);

      G4cout << "[Surface] PTFE contact (dielectric_metal, ground, R=" << ptfeR << ") applied on crystal <-> sc_gap." << G4endl;
      G4cout << "[Surface] Using dielectric_metal to bypass TIR and enable true Lambertian diffuse." << G4endl;
      G4cout << "[Surface] Bottom face isolated by sc_bottom_gap/grease, not affected." << G4endl;
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
