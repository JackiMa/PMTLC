#include <vector>

#include "PMTLCDetectorConstruction.hh"
#include "PMTLCDetectorMessenger.hh"
#include "PMTLCLayerSensitiveDetector.hh"

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

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
// 在这里对类相关参数进行初始化
PMTLCDetectorConstruction::PMTLCDetectorConstruction()
    : G4VUserDetectorConstruction()
{
  fDumpGdmlFileName = "LightCollecion.gdml";
  fVerbose = false;  // 是否输出详细信息
  fDumpGdml = false; // 是否保存GDML的几何文件
  // create a messenger for this class
  fDetectorMessenger = new PMTLCDetectorMessenger(this);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
PMTLCDetectorConstruction::~PMTLCDetectorConstruction()
{
  delete fDetectorMessenger;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
G4VPhysicalVolume *PMTLCDetectorConstruction::Construct()
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
  // PMT
  name = "PMT";
  G4Tubs *s_PMT = new G4Tubs(name, 0, g_pmt_radius, 0.5 * g_pmt_thickness, 0, 360 * deg);
  G4LogicalVolume *l_PMT = new G4LogicalVolume(s_PMT, g_vacuum, name);
  MyPhysicalVolume *p_PMT = new MyPhysicalVolume(0, g_pmt_pos, name, l_PMT, p_world, false, 0, checkOverlaps);
  fVolumeMap[name] = p_PMT;

  // PMT - window
  name = gN_PMT_window;
  G4Tubs *s_window = new G4Tubs(name, 0, g_pmt_radius, 0.5 * g_window_thickness, 0, 360 * deg);
  G4LogicalVolume *l_window = new G4LogicalVolume(s_window, g_window_material, name);
  MyPhysicalVolume *p_window = new MyPhysicalVolume(0, G4ThreeVector(0, 0, 0.5 * g_pmt_thickness - 0.5 * g_window_thickness), name, l_window, p_PMT, false, 0, checkOverlaps);
  fVolumeMap[name] = p_window;

  // 设置 window 的颜色为棕色
  G4VisAttributes *windowVisAtt = new G4VisAttributes(G4Colour(0.6, 0.3, 0.0, 0.3)); // 棕色
  windowVisAtt->SetForceSolid(true);
  windowVisAtt->SetVisibility(true);
  l_window->SetVisAttributes(windowVisAtt);

  // PMT - window - photocathode
  name = gN_PMT_photocathode;
  G4Tubs *s_photocathode = new G4Tubs(name, 0, g_pmt_radius, 0.5 * g_photocathode_thickness, 0, 360 * deg);
  G4LogicalVolume *l_photocathode = new G4LogicalVolume(s_photocathode, g_photocathode_material, name);
  MyPhysicalVolume *p_photocathode = new MyPhysicalVolume(0, G4ThreeVector(0, 0, -0.5 * g_window_thickness + 0.5 * g_photocathode_thickness), name, l_photocathode, p_window, false, 0, checkOverlaps);
  fVolumeMap[name] = p_photocathode;

  // 设置 photocathode 的颜色为银色
  G4VisAttributes *photocathodeVisAtt = new G4VisAttributes(G4Colour(0.75, 0.75, 0.75, 0.7)); // 银色
  photocathodeVisAtt->SetForceSolid(true);
  photocathodeVisAtt->SetVisibility(true);
  l_photocathode->SetVisAttributes(photocathodeVisAtt);

  // PMT - vacuum
  name = gN_PMT_vacuum;
  G4Tubs *s_vacuum = new G4Tubs(name, 0, g_pmt_radius, 0.5 * g_vacuum_thickness, 0, 360 * deg);
  G4LogicalVolume *l_vacuum = new G4LogicalVolume(s_vacuum, g_vacuum, name);
  MyPhysicalVolume *p_vacuum = new MyPhysicalVolume(0, G4ThreeVector(0, 0, -0.5 * g_pmt_thickness + 0.5 * g_vacuum_thickness), name, l_vacuum, p_PMT, false, 0, checkOverlaps);
  fVolumeMap[name] = p_vacuum;

  // 设置 vacuum 的颜色为黑色
  G4VisAttributes *vacuumVisAtt = new G4VisAttributes(G4Colour(0.1, 0.1, 0.1, 0.3)); // 黑色
  vacuumVisAtt->SetForceSolid(true);
  vacuumVisAtt->SetVisibility(true);
  l_vacuum->SetVisAttributes(vacuumVisAtt);

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
    wrapper_pos = g_pmt_pos + G4ThreeVector(0, 0, 0.5 * g_pmt_thickness + 0.5 * wrapperZ);
    G4ThreeVector gap_pos = G4ThreeVector(0, 0, - 0.25*wrapperThickness);

    G4Box *s_wrapper = new G4Box(name, 0.5 * wrapperX, 0.5 * wrapperY, 0.5 * wrapperZ);
    G4LogicalVolume *l_wrapper = new G4LogicalVolume(s_wrapper, g_wrapper_material, name);
    MyPhysicalVolume *p_wrapper = new MyPhysicalVolume(0, wrapper_pos, name, l_wrapper, p_world, false, 0, checkOverlaps);
    fVolumeMap[name] = p_wrapper;

    name = "sc_gap";
    G4Box *s_gap = new G4Box(name, 0.5 * gapXY, 0.5 * gapXY, 0.5 * gapZ);
    G4LogicalVolume *l_gap = new G4LogicalVolume(s_gap, g_world_material, name);
    MyPhysicalVolume *p_gap = new MyPhysicalVolume(0, gap_pos, name, l_gap, p_wrapper, false, 0, checkOverlaps);
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
    // wrapper
    name = gN_sc_wrapper;
    wrapperX = g_crystalX + g_gap_thickness + g_wrapper_thickness;
    wrapperY = g_crystalY + g_gap_thickness + g_wrapper_thickness;
    wrapperZ = g_crystalZ + g_gap_thickness*0.5 + g_wrapper_thickness*0.5; // 晶体放在下侧紧靠PMT
    G4double wrapperThickness = g_wrapper_thickness;
    G4double gapX = wrapperX - wrapperThickness;
    G4double gapY = wrapperY - wrapperThickness;
    G4double gapZ = wrapperZ - 0.5*wrapperThickness;
    wrapper_pos = g_pmt_pos + G4ThreeVector(0, 0, 0.5 * g_pmt_thickness + 0.5 * wrapperZ);
    G4ThreeVector gap_pos = G4ThreeVector(0, 0, - 0.25*wrapperThickness);

    G4Box *s_wrapper = new G4Box(name, 0.5 * wrapperX, 0.5 * wrapperY, 0.5 * wrapperZ);
    G4LogicalVolume *l_wrapper = new G4LogicalVolume(s_wrapper, g_wrapper_material, name);
    MyPhysicalVolume *p_wrapper = new MyPhysicalVolume(0, wrapper_pos, name, l_wrapper, p_world, false, 0, checkOverlaps);
    fVolumeMap[name] = p_wrapper;

    name = "sc_gap";
    G4Box *s_gap = new G4Box(name, 0.5 * gapX, 0.5 * gapY, 0.5 * gapZ);
    G4LogicalVolume *l_gap = new G4LogicalVolume(s_gap, g_world_material, name);
    MyPhysicalVolume *p_gap = new MyPhysicalVolume(0, gap_pos, name, l_gap, p_wrapper, false, 0, checkOverlaps);
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
  else if(    g_wrapper_Type == CYLINDER  ){
    // wrapper
    name = gN_sc_wrapper;
    G4double wrapperR = 3.2 * cm;
    wrapperZ = 2.2 * cm;
    G4double wrapperThickness = g_wrapper_thickness;
    G4double gapR = wrapperR - wrapperThickness;
    G4double gapH = wrapperZ - wrapperThickness; 
    wrapper_pos = g_pmt_pos + G4ThreeVector(0, 0, 0.5 * g_pmt_thickness + 0.5* wrapperZ);
    G4ThreeVector gap_pos = G4ThreeVector(0, 0, - 0.5*wrapperThickness);

    G4Tubs *s_wrapper = new G4Tubs(name, 0, wrapperR, 0.5 * wrapperZ, 0, 360 * deg);
    G4LogicalVolume *l_wrapper = new G4LogicalVolume(s_wrapper, g_wrapper_material, name);
    MyPhysicalVolume *p_wrapper = new MyPhysicalVolume(0, wrapper_pos, name, l_wrapper, p_world, false, 0, checkOverlaps);
    fVolumeMap[name] = p_wrapper;

    name = "sc_gap";
    G4Tubs *s_gap = new G4Tubs(name, 0, gapR, 0.5 * gapH, 0, 360 * deg);
    G4LogicalVolume *l_gap = new G4LogicalVolume(s_gap, g_world_material, name);
    MyPhysicalVolume *p_gap = new MyPhysicalVolume(0, gap_pos, name, l_gap, p_wrapper, false, 0, checkOverlaps);
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
  

if(g_grease_thickness > 10*um){
    name = "optical_grease";
    G4ThreeVector gap_pos = wrapper_pos - G4ThreeVector(0, 0, 0.5 * wrapperZ + 0.5*g_grease_thickness);
    G4Box *s_grease = new G4Box(name, 0.5 * g_grease_X, 0.5 * g_grease_Y, 0.5*g_grease_thickness);
    G4LogicalVolume *l_grease = new G4LogicalVolume(s_grease, g_grease_material, name);
    MyPhysicalVolume *p_grease = new MyPhysicalVolume(0, gap_pos, name, l_grease, p_world, false, 0, checkOverlaps);
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

MyPhysicalVolume *PMTLCDetectorConstruction::GetMyVolume(G4String volumeName) const
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
    G4Exception("PMTLCDetectorConstruction::GetMyVolume",
                "VolumeNotFound", FatalException,
                ("Volume " + volumeName + " not found in the volume map.").c_str());
    return nullptr; // 这行代码实际上不会被执行，因为G4Exception会终止程序
  }
}

void PMTLCDetectorConstruction::ConstructSDandField()
{

  G4SDManager::GetSDMpointer()->SetVerboseLevel(1);
  //
  // Scorers
  //
  G4VPrimitiveScorer *primitive;
  G4int fillH1ID;
  // declare crystal as a MultiFunctionalDetector scorer
  // include Edep and scintillation light

  auto crystal = new G4MultiFunctionalDetector(gN_sc_crystal);
  G4SDManager::GetSDMpointer()->AddNewDetector(crystal);
  primitive = new G4PSEnergyDeposit("Edep");
  crystal->RegisterPrimitive(primitive);

  fillH1ID = 0;
  primitive = new SCLightScorer("crystal_scintillation_H1", fillH1ID);
  crystal->RegisterPrimitive(primitive);

  fillH1ID = 1;
  primitive = new CherenkovLightScorer("crystal_cherenkovLight_H1", fillH1ID);
  crystal->RegisterPrimitive(primitive);

  SetSensitiveDetector(gN_sc_crystal, crystal);

  // declare photocathode as a MultiFunctionalDetector scorer
  // include light spectrum and position
  // auto photocathode = new G4MultiFunctionalDetector(gN_PMT_photocathode);
  // G4SDManager::GetSDMpointer()->AddNewDetector(photocathode);
  // fillH1ID = 2;
  // fillH2ID = 1;
  // primitive = new PhotocathodeScorer("photocathode_Entry_H1", fillH1ID,fillH2ID);
  // photocathode->RegisterPrimitive(primitive);
  // SetSensitiveDetector(gN_PMT_photocathode, photocathode);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
void PMTLCDetectorConstruction::SetDumpGdml(G4bool val) { fDumpGdml = val; }

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
G4bool PMTLCDetectorConstruction::IsDumpGdml() const { return fDumpGdml; }

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
void PMTLCDetectorConstruction::SetVerbose(G4bool val) { fVerbose = val; }

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
G4bool PMTLCDetectorConstruction::IsVerbose() const { return fVerbose; }

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
void PMTLCDetectorConstruction::SetDumpGdmlFile(G4String filename)
{
  fDumpGdmlFileName = filename;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
G4String PMTLCDetectorConstruction::GetDumpGdmlFile() const
{
  return fDumpGdmlFileName;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
void PMTLCDetectorConstruction::PrintError(G4String ed)
{
  G4Exception("PMTLCDetectorConstruction:MaterialProperty test", "op001",
              FatalException, ed);
}
