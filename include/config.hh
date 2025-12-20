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
    CYLINDER = 3,
    TEFLON = 3,
  };

// surface properties
inline G4OpticalSurface *surf_ESR = MyMaterials::surf_ESR();
// PTFE反射率：论文中使用97.5%反射率 (2.5%透过率)
inline G4OpticalSurface *surf_Hreflex = MyMaterials::surf_Teflon(0.025);  // 97.5% reflectivity
inline G4OpticalSurface *surf_Lreflex = MyMaterials::surf_Teflon(0.4);

// g_ means global_

// switch
inline G4bool g_has_opticalPhysics = true;  // 是否模拟光学过程
inline G4bool g_has_cherenkov = false;       // 是否考虑切伦科夫光

inline G4String g_gdml_name = "";  // GDML文件名 ==''表示不保存GDML文件
inline G4double g_grease_thickness = 0.05*mm;  // 导光油厚度, >10*um表示有导光油 (论文基准值50μm)
// 晶体与 SiPIN 之间如果“没有 grease”，按经验应存在一层很薄的空气层（避免晶体直接接触窗口材料）
inline G4double g_bottom_airgap_thickness = 10*um;  // 默认 10um，可调
inline G4double g_top_airgap_thickness = 0.1*mm;  // 顶面空气层厚度 (论文基准值100μm)
inline G4double g_side_contact_ratio = 0.5;  // 侧面贴合比例 0~1, 用于概率边界法 (论文基准值50%)
/*
        ↑ z
        |
        |                *       -- Source
        |            _________
        |            |   ■   |    -- Scintillator(wrapper+crystal)
        |———→ x   =============== -- sipin(window&si)
     y↙              ceramics
*/
// world
inline G4double g_worldX = 4 * cm;
inline G4double g_worldY = 4 * cm;
inline G4double g_worldZ = 3 * cm;
inline G4Material *g_world_material = MyMaterials::Air();

// sipin = window + si + ceramics
// sipin以sipin为母体，包含window、ceramics子体。window包含si，window的下面是ceramics（加电场）
inline G4String gN_sipin_window = "sipin_window";
inline G4String gN_sipin_si = "sipin_si";
inline G4String gN_sipin_ceramics = "sipin_ceramics";
inline G4double g_sipin_X = 1 * cm;
inline G4double g_sipin_Y = 1 * cm;
inline G4double g_si_thickness = 0.3*mm; // TBD
inline G4double g_window_thickness = g_si_thickness + 0.2*mm; // TBD
inline G4double g_ceramics_thickness = 1*mm; // TBD
inline G4double g_sipin_thickness = g_window_thickness+g_ceramics_thickness;
inline G4ThreeVector g_sipin_pos = G4ThreeVector(0, 0, -0.5*g_worldZ + 0.5*g_sipin_thickness + 1*mm); 
inline G4Material *g_window_material = MyMaterials::PMMA(); // TBD
// inline G4Material *g_window_material = MyMaterials::Borosilicate(); // TBD
inline G4Material *g_si_material = MyMaterials::Silicon(); // TBD
// inline G4Material *g_si_material = MyMaterials::Air(); // TBD
inline G4Material *g_ceramics = MyMaterials::PVC();

// scintillator = crystal + wrapper
// 闪烁体以wrapper为母体，包含crystal子体。wrapper可以是反光罩也可以是紧贴晶体的封装
inline G4String gN_sc_wrapper = "Scintillator_wrapper";
inline G4String gN_sc_crystal = "Scintillator_crystal";
// inline ReflectorType g_wrapper_Type = CYLINDER; // 反光罩类型
inline ReflectorType g_wrapper_Type = TEFLON; // 反光罩类型
inline G4double g_gap_thickness = 0.1*mm;
inline G4double g_wrapper_thickness = 2*mm;
inline G4double g_crystalX = 0.5 * cm;  
inline G4double g_crystalY = 0.5 * cm;
inline G4double g_crystalZ = 0.5 * cm;                                                  
inline G4Material *g_wrapper_material = MyMaterials::PVC(); 
// inline G4Material *g_crystal_material = MyMaterials::PVC(); 
// inline G4Material *g_crystal_material = MyMaterials::LuAG_Ce(); 
inline G4Material *g_crystal_material = MyMaterials::GAGG_Ce_Mg(20000, 1, -1); 
// inline G4Material *g_crystal_material = MyMaterials::LYSO(35000, 1, -1); 
// inline G4Material *g_crystal_material = MyMaterials::BGO(8000, 1, -1); 
// inline G4Material *g_crystal_material = MyMaterials::CsI_Tl(30000, 1, -1); 

// Grease 尺寸应与晶体底面匹配（不能超出 gap 底面开口）
inline G4double g_grease_X = g_crystalX;  
inline G4double g_grease_Y = g_crystalY;
inline G4Material *g_grease_material = MyMaterials::OpticalGrease(); 

// data recording
inline G4int gID_H1_sc_wl; // 闪烁体发光的光谱
inline G4int gID_H1_ch_wl; // 所有材料切伦科夫光的光谱
inline G4int gID_H1_sipin_wl; // 光子打到光阴极上的光谱
inline G4int gID_H1_sc_ed; // 闪烁体中的能量沉积
inline G4int gID_H1_sipin_LC;  // 每一次事件收集的光子数

inline G4int gID_H1_photon_posY0;
inline G4int gID_H1_photon_posYX;
inline G4int gID_H2_source_pos;
inline G4int gID_H2_photon_pos;

// === 论文所需的新增统计量 ===
inline G4int gID_H1_sipin_theta;     // 光子到达SiPIN的入射角分布 (度)
inline G4int gID_H1_sipin_hitCount;  // 每个光子撞击SiPIN的次数分布
inline G4int gID_H1_escape_channel;  // 光子逃逸/损失通道分类 (0:被晶体吸收, 1:从顶面逃逸, 2:从侧面逃逸, 3:被PTFE吸收, 4:其他)
inline G4int gID_H1_photon_generated; // 每事件产生的闪烁光子数
inline G4int gID_H1_photon_pathlength; // 光子在晶体内的总路程 (mm)，用于理解自吸收效应


// source
inline G4double g_source_scale = 1; // 源的尺度是scintillator投影的若干倍

// other
inline G4String gN_outfile = ""; // 输出文件名
#endif // CONFIG_HH