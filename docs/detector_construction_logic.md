# DetectorConstruction：几何与表面定义流程图（对照代码找错用）

本文以 `src/SiPINLCDetectorConstruction.cc` 为准，重点回答：

- **几何层级**：World / wrapper / sc_gap / crystal / grease / sipin_si 到底怎么放
- **表面定义**：哪些是 `SkinSurface`，哪些是 `BorderSurface`，各自何时生效
- **不同材料/介质**：晶体-空气、空气-PTFE、grease-Si 等边界分别是谁处理

## 1) `Construct()`：几何搭建流程（高层）

```mermaid
flowchart TD
  A[Construct()] --> W[创建 World 逻辑体/物理体]
  W --> S1[创建 sipin (ceramics 载体)]
  S1 --> S2[在 sipin 内创建 sipin_si (active Si)]
  S1 --> S3[在 sipin 内创建 ceramics]
  W --> SC{wrapper_Type?}

  SC -- CUBE --> C1[World -> wrapper(实心)\nwrapper -> sc_gap\nsc_gap -> crystal]
  SC -- CYLINDER --> Y1[World -> wrapper(tubs)\nwrapper -> sc_gap\nsc_gap -> crystal]
  SC -- TEFLON --> T0[TEFLON: World -> wrapper(实心)\nwrapper -> sc_gap\nsc_gap -> crystal\n并在 World 放 grease/bottom_gap]

  C1 --> SURF
  Y1 --> SURF
  T0 --> SURF
  SURF --> RET[return p_world]
```

## 2) `TEFLON` 模式：最终几何层级（最重要）

> 这是你现在用于论文/概率边界法的主要模式。

```mermaid
flowchart TD
  W[World] --> SIPIN[sipin]
  SIPIN --> SI[sipin_si]
  SIPIN --> CER[sipin_ceramics]

  W --> WRAP[sc_wrapper (PTFE material)]
  WRAP --> GAP[sc_gap (air, world_material)]
  GAP --> CRY[sc_crystal]

  W --> BL{bottom_layer_height > 0 ?}
  BL -- grease_thickness>10um --> G[optical_grease (world)]
  BL -- else --> BG[sc_bottom_gap (air, world)]
  G --> SI
  BG --> SI
```

### 关键“对照点”（便于你看代码变量）

- `sipin_top_z = g_sipin_pos.z() + 0.5*g_sipin_thickness`
- `bottom_layer_height = grease_effective 或 g_bottom_airgap_thickness`
- `wrapper_bottom_z = sipin_top_z + bottom_layer_height`
- `wrapper_pos.z = wrapper_bottom_z + 0.5*wrapperZ`
- `sc_gap` 放在 wrapper 内，`gap_pos_in_wrapper = (0,0,-0.5*g_wrapper_thickness)`（用于实现“底面开窗”）
- `optical_grease/sc_bottom_gap` 放在 World，中心 z 为 `sipin_top_z + 0.5*bottom_layer_height`

## 3) 表面（Optical Surfaces）定义与生效条件

### 3.1 Wrapper 的 PTFE 反射面：`SkinSurface`

代码（TEFLON/CUBE/CYLINDER 都类似）：

- `new G4LogicalSkinSurface("WrapperGapSurface", l_wrapper, surf_Hreflex);`

含义：

- 只要光子从 **非 wrapper 体积** 撞到 **wrapper 的几何边界**，`SkinSurface` 就可能被 `G4OpBoundaryProcess` 使用。
- `surf_Hreflex` 目前来自 `MyMaterials::surf_Teflon(0.025)`（见 `include/config.hh`）。

⚠️ 注意：`MyMaterials::surf_Teflon()` 当前设置的是：

- `type = dielectric_dielectric`
- `finish = groundfrontpainted`
- 并在 MPT 里设置 `REFLECTIVITY = 1-transmittance`，`TRANSMITTANCE = transmittance`

这意味着它并不是一个“纯金属反射”边界：其行为会受到 Geant4 光学边界过程的处理顺序影响（与 `dielectric_metal` 不同）。

### 3.2 晶体表面粗糙：`BorderSurface (dielectric_dielectric + unified + ground + sigma_alpha)`

生效条件：

- `g_crystal_sigma_alpha > 0`
- 且 **非 PTFE contact 模式**：`usePtfeContact == false`
  - `usePtfeContact := (g_side_contact_ratio >= 1.0 || g_top_contact_ratio >= 1.0)`

定义：

- `CrystalToGapSurface`：`p_crystal -> p_gap`
- `GapToCrystalSurface`：`p_gap -> p_crystal`
- `type = dielectric_dielectric`
- `model = unified`
- `finish = ground`
- `sigmaAlpha = g_crystal_sigma_alpha`

物理含义（你关心的 “sigma 粗糙度如何影响 TIR” 就在这里）：

- 仍然是 **介质-介质** 的 Fresnel/TIR 框架
- 但 `unified + ground + sigmaAlpha` 会用微表面法向分布改变反射/透射的角度分布，从而“打散”困在晶体中的模式

### 3.3 “全贴合 PTFE” sanity/contact：`BorderSurface (dielectric_metal + ground)`

生效条件：

- `g_side_contact_ratio >= 1.0 || g_top_contact_ratio >= 1.0`

定义：

- `CrystalToGap_PTFE` / `GapToCrystal_PTFE`
- `type = dielectric_metal`
- `finish = ground`（Lambertian）
- `REFLECTIVITY = ptfeR`
  - **注意：这里 ptfeR 在代码里写死为 `0.975` 或 `1.0`（p>1 时）**

物理含义：

- 这条路径会 **绕过 Fresnel/TIR**（因为是 `dielectric_metal`）
- 因而也会覆盖/抑制你在 “晶体-空气” 界面想要的 TIR 物理（这是当初为了 sanity B 才这么做的）

## 4) “介质/材料边界”由谁处理？（一张表）

| 边界 | 典型 pre -> post | 由谁主导 | 备注 |
|---|---|---|---|
| grease/bottom_gap -> sipin_si | `optical_grease/sc_bottom_gap -> sipin_si` | **SteppingAction 的 p_det hook** | `pdetMode=0/1/2`；否则将镜面反射回 preVolume |
| crystal <-> sc_gap（粗糙） | `crystal <-> sc_gap` | **BorderSurface: CrystalAir_UNIFIED** | `dielectric_dielectric + sigmaAlpha`，仍有 TIR |
| crystal <-> sc_gap（全贴合 PTFE） | `crystal <-> sc_gap` | **BorderSurface: IdealPTFE_Contact** | `dielectric_metal`，无 TIR |
| sc_gap -> wrapper | `sc_gap -> wrapper` | **wrapper 的 SkinSurface** | `surf_Hreflex` |
| sc_gap -> World | `sc_gap -> World` | **几何：底部开窗路径** +（可选）**SteppingAction 的边界拦截补丁** | ⚠️ 若 SteppingAction 无条件拦截，会影响底部开窗 |

## 5) 三个“很可能藏错”的地方（建议你重点对照）

1. **底部开窗 vs `sc_gap -> World` 拦截**  
   TEFLON 模式下，代码注释希望 “底面光子从 sc_gap 进入 World，再进 grease/Si”。  
   但 `SteppingAction` 里目前存在 “volumeToWorld 就按 PTFE 处理” 的补丁，如果不区分“底面开窗区域”，就可能把底面也当 PTFE 反射/吸收。

2. **PTFE 反射率的来源不一致**  
   - 概率边界法与 volumeToWorld 拦截用的是 `g_ptfe_reflectivity`（`include/config.hh`）  
   - 但 `DetectorConstruction` 里 `dielectric_metal` 的 BorderSurface 反射率 `ptfeR` 是 **写死的 0.975/1.0**  
   这会导致“你以为改了 PTFE 反射率，但只改到一半”的错觉。

3. **晶体底面是否真的“被隔离”**  
   代码注释说 “Bottom face isolated by sc_bottom_gap/grease”。  
   实际上晶体是 `sc_gap` 的子体积；如果晶体底面与 `sc_gap` 底面/World 接触方式不理想，可能会出现：
   - 光子先 `crystal -> sc_gap`，马上 `sc_gap -> World`（甚至在几何公差内抖动）
   - 从而触发 SteppingAction 的 “volumeToWorld 拦截” 或 wrapper skin surface 的副作用


