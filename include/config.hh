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

// ========= Sanity-check wall override (analytic-friendly simplified scenarios) =========
// Implemented in SteppingAction at the crystal->sc_gap boundary.
// Bottom face is never overridden (keeps real bottom coupling physics: air/grease).
enum SanityWallModel {
  SANITY_WALL_OFF = 0,
  SANITY_WALL_SPECULAR = 1,
  SANITY_WALL_LAMBERTIAN = 2
};

inline G4int g_sanity_wall_model = SANITY_WALL_OFF;
inline G4double g_sanity_wall_reflectivity = 1.0; // 0..1 (used when wall_model != OFF)
inline G4bool g_sanity_wall_apply_side = true;
inline G4bool g_sanity_wall_apply_top = true;
// Safety cap for analytic sanity runs: maximum number of steps per optical photon track.
// Prevents pathological long runs for "specular + TIR-trapped" configurations.
inline G4int g_sanity_max_steps = 20000;
// For scenario A (specular mirror), optionally short-circuit TIR-trapped photons at step 1 (fast).
inline G4bool g_sanity_fast_specular_tir = false;

// surface properties
inline G4OpticalSurface *surf_ESR = MyMaterials::surf_ESR();
// PTFE反射率：论文中使用97.5%反射率 (2.5%透过率)
// 采用论文基准：97.5% reflectivity（2.5% transmittance）
inline G4OpticalSurface *surf_Hreflex = MyMaterials::surf_Teflon(0.025);
inline G4OpticalSurface *surf_Lreflex = MyMaterials::surf_Teflon(0.4);

// g_ means global_

// switch
inline G4bool g_has_opticalPhysics = true;  // 是否模拟光学过程
inline G4bool g_has_cherenkov = false;       // 是否考虑切伦科夫光

// ========== 调试开关：直接发射 opticalphoton（绕过 UI 命令问题） ==========
// 设为 true 时，PrimaryGeneratorAction 会直接在晶体中心发射 opticalphoton，
// 方向向下（-z），能量 3 eV（约 413 nm）。用于验证 P_det hook。
// 设为 false 时，恢复正常的 gamma/electron 源逻辑。
inline G4bool g_debug_opticalphoton = true;  // ← 调试模式：发射单色光子

// 调试光子波长（用于解析 sanity-check / 单色对照）
// 550 nm 对应 2.254 eV（E = 1240 eV·nm / λ）
inline G4double g_debug_opticalphoton_wavelength = 550.0 * nm;

// ========== 基准源参数 ==========
// 正常模式下（g_debug_opticalphoton=false）使用的粒子类型和能量
// 按你的要求：GAGG 中心 662 keV 电子激发闪烁谱
inline G4String g_primary_particle = "e-";    // 粒子类型
inline G4double g_primary_energy = 662*keV;   // 粒子能量

inline G4String g_gdml_name = "";  // GDML文件名 ==''表示不保存GDML文件
inline G4double g_grease_thickness = 0*mm;  // 无grease，测试底部空气层场景
// 晶体与 SiPIN 之间如果“没有 grease”，按经验应存在一层很薄的空气层（避免晶体直接接触窗口材料）
inline G4double g_bottom_airgap_thickness = 10*um;  // 默认 10um，可调
inline G4double g_top_airgap_thickness = 0.1*mm;  // 顶面空气层厚度 (论文基准值100μm)
// 侧面贴合比例 0~1, 用于概率边界法（建议默认关闭；做贴合实验时在 mac 中显式打开）
inline G4double g_side_contact_ratio = 0.0;

// 晶体表面微粗糙（UNIFIED sigma_alpha, 单位 rad）
// 0 表示理想镜面界面（Fresnel + 完全平整）；>0 表示微表面法线分布展宽（角度扩散），可打破“困光/导波”。
inline G4double g_crystal_sigma_alpha = 0.0;  // rad

// 晶体自吸收强度：通过缩放 ABSLENGTH 来实现（>1 表示吸收长度变长，自吸收更弱）
// 注意：MyMaterials::GAGG_Ce_Mg(scaleFactor) 本身也支持这个含义；这里用于运行时通过 UI 调整。
inline G4double g_crystal_absorption_scale = 1.0;
// 用于避免重复累乘的“当前已应用值”（由 DetectorConstruction 在构建几何时维护）
inline G4double g_crystal_absorption_scale_applied = 1.0;

// SiPIN 探测概率模型（用于 grease(or airgap) → Si 界面）
// p_det = 1 - R(λ,θ) 由你拟合 Si3N4 厚度的 TMM + 厂商 QE 得到
// mode:
//   0: 关闭（保持“几何到达就计数+kill”的旧行为，仅用于debug）
//   1: 常数 p_det = g_sipin_pdet_const（单元验证用）
//   2: CSV 查表 p_det(λ_nm, θ_deg)（真实模型）
inline G4int g_sipin_pdet_mode = 2;  // 1=常数模式, 2=CSV查表
inline G4double g_sipin_pdet_const = 1.0;  // 常数模式参数（mode=2时不用）
inline G4String g_sipin_pdet_csv = "spectrum/sipin_pdet_dummy.csv";  // CSV 查表文件
// 防止 p_det 很小/为0 时光子在 Si 界面附近来回反射导致运行时间爆炸
// 该上限只对 pdetMode!=0 的“手动反射模型”生效；超过后直接 kill（视为未探测到）
inline G4int g_sipin_max_interface_hits = 20000;
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

// sipin = si(active) + ceramics(substrate)
// 顶面直接是 Si，与 grease / bottom airgap 相接触。
// 说明：此前的 sipin_window 是为了表示“保护窗/封装层”，但你当前物理假设中该层可忽略，
// 因此这里将界面简化为 grease(or airgap) → Si。
inline G4String gN_sipin_si = "sipin_si";
inline G4String gN_sipin_ceramics = "sipin_ceramics";
inline G4double g_sipin_X = 1 * cm;
inline G4double g_sipin_Y = 1 * cm;
inline G4double g_si_thickness = 0.3*mm; // TBD
inline G4double g_ceramics_thickness = 1*mm; // TBD
inline G4double g_sipin_thickness = g_si_thickness + g_ceramics_thickness;
inline G4ThreeVector g_sipin_pos = G4ThreeVector(0, 0, -0.5*g_worldZ + 0.5*g_sipin_thickness + 1*mm); 
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
inline G4double g_wrapper_thickness = 1*mm;
inline G4double g_crystalX = 0.5 * cm;  
inline G4double g_crystalY = 0.5 * cm;
inline G4double g_crystalZ = 0.5 * cm;                                                  
inline G4Material *g_wrapper_material = MyMaterials::PVC(); 
// inline G4Material *g_crystal_material = MyMaterials::PVC(); 
// inline G4Material *g_crystal_material = MyMaterials::LuAG_Ce(); 
inline G4Material *g_crystal_material = MyMaterials::GAGG_Ce_Mg(20000, 1, -1);  // 恢复正常吸收长度 
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

// Sanity-check helper: wall hit count (crystal side/top override)
inline G4int gID_H1_wall_hitCount; // number of wall interactions per photon (side/top only)


// source
inline G4double g_source_scale = 1; // 源的尺度是scintillator投影的若干倍

// other
inline G4String gN_outfile = ""; // 输出文件名
#endif // CONFIG_HH