# SteppingAction：逐步决策流程图（对照代码找错用）

本文以 `src/SiPINLCSteppingAction.cc` 为准，目标是把 **每一步会改变光子命运/统计口径的分支** 清晰列出来。

## 当前几何假设（用于理解 pre/post volume）

以 `TEFLON` 模式为例（见 `src/SiPINLCDetectorConstruction.cc`）：

```mermaid
flowchart TD
  W[World] --> SIPIN[sipin (ceramics)]
  SIPIN --> SI[sipin_si (active)]
  W --> G[optical_grease 或 sc_bottom_gap]
  W --> WRAP[Scintillator_wrapper (PTFE)]
  WRAP --> GAP[sc_gap (air)]
  GAP --> CRY[Scintillator_crystal]
```

## UserSteppingAction：严格按执行顺序的流程图

> 注意：下面是 **同一次 step 内** 的执行顺序。若某分支 `kill` 并 `return`，后续逻辑不会执行。

```mermaid
flowchart TD
  A[进入 UserSteppingAction(step)] --> B{track 是 opticalphoton?}
  B -- 否 --> Z[直接返回]
  B -- 是 --> C[取 pre/post StepPoint & pre/post VolumeName]

  C --> D{CurrentStepNumber == 1?}
  D -- 是 --> D1[按 CreatorProcess 统计\nfPhotonGenerated / fCherenkovGenerated]
  D -- 否 --> E
  D1 --> E

  E{preVolume == crystal?} -- 是 --> E1[累计 stepLength 到 photonPathInCrystal[trackID]]
  E -- 否 --> F
  E1 --> F

  F{命中 SiPIN 界面?\n(pre in grease/sc_gap/sc_bottom_gap) && (post == sipin_si)} -- 否 --> P
  F -- 是 --> F1[每次命中都统计：\nphotonHitCount++，填 theta 直方图/累计]
  F1 --> F2{pdetMode == 0?}
  F2 -- 是 --> F3{本 trackID 未 processed?}
  F3 -- 是 --> F4[recordAndKill():\nLightCollection++\nprocessedTrackIDs.insert\nkill track\nreturn?*]
  F3 -- 否 --> P
  F2 -- 否 --> F5[计算 p_det(λ,θ):\nmode1 const / mode2 CSV]
  F5 --> F6{U < p_det ?}
  F6 -- 是 --> F4
  F6 -- 否 --> F7[镜面反射：\nSetPosition(push back into preVolume)\nSetMomentumDirection(refl)\nSyncNavigator\ntrack Alive]
  F7 --> P

  %% probability boundary
  P{crystal -> sc_gap ?\n(pre==crystal && post==sc_gap)} -- 否 --> I
  P -- 是 --> P0{sideContactRatio>0 或 topContactRatio>0 ?}
  P0 -- 否 --> I
  P0 -- 是 --> P1[用 postPos 判别 side/top\n并用 isBottom 过滤底面]
  P1 --> P2{hitPTFE?\n(p>=1) 或 (U<p)}
  P2 -- 否 --> I[不处理：交给 Geant4 Fresnel/TIR]
  P2 -- 是 --> P3{U < R ?\nR=g_ptfe_reflectivity}
  P3 -- 否 --> P4[PTFE 吸收：\nfEscapePTFE++\nkill & return]
  P3 -- 是 --> P5[Lambertian 反射：\nSampleLambertianHemisphere(normal)\nSetMomentumDirection\nSetPosition(prePos + normal*0.1um)\nSyncNavigator\nAlive]
  P5 --> I

  %% volume->World interception
  I{post == World && pre in\nsc_gap/optical_grease/sc_bottom_gap/wrapper ?} -- 否 --> L
  I -- 是 --> I1{U < R ?\nR=g_ptfe_reflectivity}
  I1 -- 否 --> I2[PTFE 吸收：\nfEscapePTFE++\nkill & return]
  I1 -- 是 --> I3[Lambertian 反射回原体积：\nSetPosition(prePos + normal*0.1um)\nSyncNavigator\nAlive]
  I3 --> L

  %% loss classification
  L{trackStatus == kill?} -- 否 --> END[结束本 step]
  L -- 是 --> L0{processedTrackIDs 包含 trackID?}
  L0 -- 是 --> END
  L0 -- 否 --> L1[按 preVolume/postVolume/proc 分类：\nAbsorbed/PTFE/SideAir/Grease/World]
  L1 --> END
```

### 统计口径（对应 `SiPINLCEventAction.hh`）

- `fLightCollection`: **被认为“探测到”的光子数**（`recordAndKill()` 里加 1）
- `fEscapeAbsorbed`: 晶体内 `OpAbsorption` 导致的 kill
- `fEscapePTFE`: 任何被视为“PTFE 吸收/反射模型”导致的 kill（含概率边界吸收、BorderSurface 吸收、wrapper 内吸收、volume->World 拦截吸收等）
- `fEscapeSideAir`: 在 `sc_gap` 中 kill 且不是进入 wrapper 的情况（当前实现把它当“侧面空气逃逸”）
- `fEscapeGrease`: grease/bottom_gap 中被吸收或从 grease 边缘逃逸（`post==OutOfWorld` 且 pre 为 grease/bottom_gap）
- `fEscapeWorld`: World 内或从 World 边界逃逸（按注释“不应发生”）

## 两个“强烈建议优先核查”的高风险点（方便找错）

1. **（已修正）`sc_gap -> World` 的边界拦截不能覆盖“底部开窗”**  
   修正后逻辑：仅拦截 `wrapper -> World` 外表面，以及 `sc_gap -> World` 且**不是**底部开窗平面的“漏光”。  
   这样底部开窗路径（`sc_gap` 底面进入 `World` 再到 `optical_grease/sc_bottom_gap -> sipin_si`）不会被错误当成 PTFE。

2. **（已修正）概率边界法的面判别必须用“晶体局部坐标”**  
   修正后：用 `preStepPoint->GetTouchableHandle()->GetHistory()->GetTopTransform()` 将边界点投到晶体局部坐标系，再以 `±halfX/±halfY/±halfZ` 判别是侧/顶/底面，避免用 global z 近似造成误判。

