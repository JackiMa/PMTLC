## SiPINLC 模拟：需求总结与当前进展（调试记录）

更新时间：2025-12-21

### 1. 你的核心要求（What you want）

- **按论文对现有 Geant4 工程做“扩充式”实现**，尽量少改动原有框架，新增必要功能与统计量。
- **几何与光学建模**需要贴近实验：
  - PTFE（Teflon）只覆盖 **晶体侧面 + 顶面**，不应盖住晶体底面。
  - 晶体与 SiPIN 之间：
    - 有 grease 时：用 grease 提升耦合与读出概率（经验上收集效率应显著上升）。
    - 无 grease 时：应是一层很薄的空气层（而不是直接“晶体贴窗”或“完全真空”）。
- **新增统计量**：入射角、hit 次数、逃逸通道、每事件产生光子数、光子在晶体内总路程（自吸收相关）。
- **增加扫描能力**：可扫描 grease 厚度、顶面 airgap、侧面贴合比例、等效吸收长度；每组模拟约 `1e5` 光子以加速调试。
- **SiPIN 边界处理**：希望最终能按 `P_det(λ,θ)`（TMM 计算表）处理 grease→SiPIN 的探测概率；可接受先在 stepping 中捕获，或更工程化地替换 `G4OpBoundaryProcess`。

### 2. 当前已实现/改动内容（What we changed）

#### 2.1 论文所需统计量 & 直方图
- `src/SiPINLCSteppingAction.cc` / `src/SiPINLCEventAction.cc` / `src/SiPINLCRunAction.cc`
  - 新增：
    - 光子入射角（SiPIN）
    - 光子命中 SiPIN 次数
    - 光子逃逸/损失通道统计
    - 每事件闪烁光子产生数
    - 光子在晶体内总路程（自吸收相关）
  - `run_data.csv` 扩展了统计列，便于扫描后汇总。

#### 2.2 PTFE 反射率
- `include/config.hh`：将 PTFE 反射率设为 **97.5%**（透过率 2.5%）。
- `src/MyMaterials.cc`：`MyMaterials::Teflon(transmittance)` 同时写入 `TRANSMITTANCE` 与 `REFLECTIVITY=1-transmittance`。

#### 2.3 Grease 光学吸收
- `src/MyMaterials.cc`：`OpticalGrease()` 添加了基于 EJ-550 透过率数据折算的 `ABSLENGTH`；折射率使用 **n=1.46**。

#### 2.4 参数控制（MAC）与扫描脚本
- 新增 `SiPINLCParameterMessenger`：支持 MAC 设置
  - `/SiPINLC/geometry/greaseThickness`
  - `/SiPINLC/geometry/topAirGap`
  - `/SiPINLC/geometry/sideGap`
  - `/SiPINLC/geometry/sideContact`（用于统计等效法的两极限）
  - **新增**：`/SiPINLC/geometry/bottomAirGap`（无 grease 时的薄空气层）
- 扫描脚本（shell）：`scripts/run_grease_scan.sh`、`run_top_airgap_scan.sh`、`run_side_contact_scan.sh`
  - 说明：因为 Geant4 `reinitializeGeometry` 在本工程结构下会触发 SD 重名/逻辑体同名问题（曾发生 fatal），所以推荐用 shell 脚本每次启动独立进程。

#### 2.5 调试模式（primary opticalphoton）
- 修复了两个关键点，使“直接发 1e5 光子”可用：
  - `src/CustomScorer.cc`：`SCLightScorer` / `CherenkovLightScorer` 对 `GetCreatorProcess()==nullptr` 做了保护（primary optical photon 没有 creator process，否则会 segfault）。
  - `src/SiPINLCPrimaryGeneratorAction.cc`：当 `/gun/particle opticalphoton`（或 GPS opticalphoton）时，不再覆盖用户在 mac 中设置的 `/gun/*` 或 `/gps/*`，避免调试源被强行改回 gamma 源逻辑。
- `src/SiPINLCRunAction.cc`：在 opticalphoton 调试模式（scint/cherenkov entries=0）下，输出 `lightCollectionEfficiency ≈ siCounts / N_events`（避免永远是 0 的误导）。

#### 2.6 变更规模概览
当前相对 git 的变更统计（`git diff --stat`）：

- 11 files changed, 468 insertions(+), 82 deletions(-)

### 3. 当前遇到的关键问题（What’s still wrong）

你的经验预期是：
- 无 grease：收集效率 ~60%+
- 有 grease：应能到 ~90%（至少显著高于无 grease）

而我们当前的宏观现象是：
- 用 **scintillation** 模式跑出来的收集效率仍然偏低（且趋势一度出现“加 grease 反而更差”）。

我们定位到一个重要建模误区并修正：
- 早期尝试把晶体“放在 grease 容器里”（等价于晶体侧面也接触 grease），会破坏晶体侧面与空气的全反射条件，导致趋势反常。
- 现在已调整为：**grease 只作为晶体底面与 SiPIN 之间的薄层；晶体侧面与顶面仍为空气层 + PTFE 包覆。**

但即使修正为“底部薄层 grease”，宏观趋势仍需要进一步验证/调参（尤其要引入合理的“无 grease 时薄空气层”）。

### 4. 我们为定位问题做的验证与推断（Debug approach）

#### 4.1 几何自检：确认 PTFE 是否盖住晶体底面、grease 相对位置是否正确
在 `TEFLON` 分支中增加了关键 z 边界打印（初始化时）。一次典型输出如下：

- `sipin_top = wrapper_bot = gap_bot = grease_bot`（全都对齐）
- `crystal_bot = grease_top`（晶体底面紧贴 grease 顶面）
- `gap_top - grease_top = 5100 um`（= 晶体高度 5mm + 顶面 airgap 0.1mm）

示例输出：

```text
Grease is a bottom slab only (sides/top remain air) [EXPECTED]
Z check (mm): sipin_top=-12.5 wrapper_bot=-12.5 wrapper_top=-5.35 gap_bot=-12.5 gap_top=-7.35
Z check (mm): grease_bot=-12.5 grease_top=-12.45 crystal_bot=-12.45 crystal_top=-7.45
Z gaps (um): (wrapper_bot - sipin_top)=0 (gap_bot - wrapper_bot)=0 ... (crystal_bot - grease_bot)=50 ...
```

结论：**当前几何没有 overlap，并且 PTFE 并未覆盖晶体下表面；grease/晶体/SiPIN 的 z 相对位置是自洽的。**

#### 4.2 用 1e5 primary optical photons 做快速对比
目的：避免 scintillation 产生大量光子导致慢，先用“可控的 1 光子/事件”做边界趋势验证。

- 固定方向（朝 SiPIN，-z）：
  - 例：1e5 光子耗时约 **6.3s**（单线程）。
- 各向同性（GPS `ang/type iso`）：
  - 例：1e5 光子耗时约 **13~15s**（单线程）。

注意：当前 `run_data.csv` 的 `Photon Hit si` 是“进入 sipin_si 的 hit 记录数”，可以直接与 N_events 做比值（调试模式效率）。

#### 4.3 重要发现：primary opticalphoton 崩溃原因
- 使用 gdb 抓到崩溃点：`SCLightScorer::ProcessHits` 访问了空的 `GetCreatorProcess()`
- 修复后：1e5 opticalphoton 调试稳定运行。

### 5. 当前进展的客观结论（Where we are now）

- **几何问题（overlap / PTFE 盖底面 / grease 与晶体重合）已基本排除**，通过 overlap check + z 边界打印确认。
- **调试基础设施已具备**：可以稳定地用 1e5 primary optical photons 做快速对比。
- **尚未引入 SiPIN AR + QE 的探测概率模型**：现在的“Photon Hit si”仍然等价于“到达 SiPIN active region 的几何计数”，并未考虑 `P_det(λ,θ)`。

### 6. 下一步计划（Next steps）

#### 6.1 先把“无 grease 的薄空气层”纳入几何参数扫描
- 已新增全局参数 `g_bottom_airgap_thickness`，并在 TEFLON 分支用于 `greaseThickness=0` 情况。
- 目标：让“无 grease ≈ 薄空气层”成为可控变量（例如 1um / 10um / 50um）。

#### 6.2 集成 `P_det(λ,θ)`（SipinPDETable）到 Geant4 边界处理
参考你提供的：
- `SiPIN_绝对光产额测量/analysis/sipin_tmm.py`
- `SiPIN_绝对光产额测量/geant4/README_sipin_boundary.md`
- `SipinPDETable.hh/.cc`

建议路线（先简单可控，后工程化）：
- **阶段 A（stepping hook）**：在 `UserSteppingAction` 中捕获 `grease -> sipin_window` 边界，计算 `(λ,θ)`，查表得 `p_det`，以 `U<p_det` 计为“探测到”并 kill。
- **阶段 B（更稳）**：实现自定义边界 process 替换 `G4OpBoundaryProcess`，避免重复反射/重复改变动量。

#### 6.3 用“合成 PDE 表”做单元级校验
为避免一开始就被复杂的 TMM/QE 曲线干扰，先做两类易验证表：
- `p_det = 1`（应接近“所有到达都被计数”）
- `p_det = 0`（应完全无计数）
- `p_det = const`（例如 0.5）用于统计一致性验证

验证通过后再换成真实 `sipin_pdet_tmm_n0_1.460.csv`。

#### 6.4 扫描实验（按你的要求）
在边界逻辑校验正确后：
- 扫不同 grease 厚度
- 扫不同侧面 airgap 覆盖率（统计等效法：全贴合 vs 全空气层，再按比例加权）
- 扫不同平均衰减率（等效吸收长度 / 吸收缩放）
- 每组 `1e5` 光子加速

最后输出一份完整分析报告（趋势、异常点、原因、建议参数区间）。

### 7. 备注：当前代码里仍需注意的点

- `run_data.csv` 的 `lightCollectionEfficiency` 对 scint/cherenkov 模式与 opticalphoton 调试模式的定义不同（已在代码中做了兼容）。
- TEFLON 分支目前是“调试优先”的结构，后续要把 `bottomAirGap`、`sideGap`、`topAirGap` 的物理含义与论文/实验结构再逐条对齐。

