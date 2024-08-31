#ifndef CONFIG_HH
#define CONFIG_HH

#include <vector>
#include <numeric>
#include <utility>

#include "G4ThreeVector.hh"
#include "MyMaterials.hh"
#include "G4SystemOfUnits.hh"

// 定义一个结构体来存储每一层的厚度和材料
struct ShieldLayer {
    G4double thickness;
    G4Material* material;
};

enum ReflectorType {
    NONE = -1,
    CUBE = 1,
    HEMISPHERES = 2,
    CYLINDER = 3
  };

// surface properties
inline G4OpticalSurface *surf_ESR = MyMaterials::surf_ESR();
inline G4OpticalSurface *surf_Hreflex = MyMaterials::surf_Teflon(0.02);
inline G4OpticalSurface *surf_Lreflex = MyMaterials::surf_Teflon(0.4);

// g_ means global_

// switch
inline G4bool g_has_opticalPhysics = true;  // 是否模拟光学过程
inline G4bool g_has_cherenkov = true;       // 是否考虑切伦科夫光

inline G4String g_gdml_name = "";  // GDML文件名 ==''表示不保存GDML文件
inline G4double g_grease_thickness = 0*mm;  // 是否有导光油, >0表示有导光油
/*
        ↑ z
        |
        |                *       -- Source
        |            _________
        |            |   ■   |    -- Scintillator(wrapper+crystal)
        |———→ x   =============== -- PMT(window&photocathode)
     y↙              vacuum
*/
// world
inline G4double g_worldX = 8 * cm;
inline G4double g_worldY = 8 * cm;
inline G4double g_worldZ = 6 * cm;
inline G4Material *g_world_material = MyMaterials::Air();

// PMT = window + photocathode + vacuum
// PMT以PMT为母体，包含window、vacuum子体。window包含photocathode，window的下面是vacuum（加电场）
inline G4String gN_PMT_window = "PMT_window";
inline G4String gN_PMT_photocathode = "PMT_photocathode";
inline G4String gN_PMT_vacuum = "PMT_vacuum";
inline G4double g_pmt_radius = 3.8 * cm;
inline G4double g_photocathode_thickness = 0.1*mm; // TBD
inline G4double g_window_thickness = g_photocathode_thickness + 5*mm; // TBD
inline G4double g_vacuum_thickness = 1*cm; // TBD
inline G4double g_pmt_thickness = g_window_thickness+g_vacuum_thickness;
inline G4ThreeVector g_pmt_pos = G4ThreeVector(0, 0, -0.5*g_worldZ + 0.5*g_pmt_thickness + 1*mm); 
inline G4Material *g_window_material = MyMaterials::Borosilicate(); // TBD
inline G4Material *g_photocathode_material = MyMaterials::Air(); // TBD
inline G4Material *g_vacuum = MyMaterials::Vacuum();
inline G4double g_E_field = 50*volt/cm; // TBD

// scintillator = crystal + wrapper
// 闪烁体以wrapper为母体，包含crystal子体。wrapper可以是反光罩也可以是紧贴晶体的封装
inline G4String gN_sc_wrapper = "Scintillator_wrapper";
inline G4String gN_sc_crystal = "Scintillator_crystal";
inline ReflectorType g_wrapper_Type = CYLINDER; // 反光罩类型
// inline ReflectorType g_wrapper_Type = CUBE; // 反光罩类型
inline G4double g_gap_thickness = 0*mm;
inline G4double g_wrapper_thickness = 2*mm;
inline G4double g_crystalX = 0.5 * cm;  
inline G4double g_crystalY = 0.5 * cm;
inline G4double g_crystalZ = 0.5 * cm;                                                  
inline G4Material *g_wrapper_material = MyMaterials::PVC(); 
inline G4Material *g_crystal_material = MyMaterials::GAGG_Ce_Mg(20000, 1, -1); 



// source
inline G4double g_source_scale = 1; // 源的尺度是scintillator投影的若干倍

// other
inline G4String gN_outfile = ""; // 输出文件名
#endif // CONFIG_HH