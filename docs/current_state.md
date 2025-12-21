## 当前状态总结（阶段报告）

### 背景与目标（你交给我的任务）

- **目的**：用 Geant4 计算 SiPINLC 的光收集效率（几何收集 + PDE），重点研究 grease 耦合与 PTFE 包覆条件下的“90%+”结果，并最终给出“90 多少”。
- **几何要求**：
  - 晶体：GAGG 5×5×5 mm³
  - 包覆：1 mm 厚 PTFE（目前用 `PVC` 作为实体材料，光学反射通过 `surf_Teflon(transmittance)` 实现）
  - 顶面：100 µm 空气层（提高全反射概率）
  - 底面：有 grease（典型 50 µm）或无 grease（薄空气层 10 µm）
  - 侧面：需要支持“贴合比例/空气层”讨论（参数 `g_side_contact_ratio`）
- **SiPIN 界面要求**：
  - 忽略 window（只保留 `grease(or airgap) → Si`）
  - 在该边界用 TMM 预计算二维表 \(P_\mathrm{det}(\lambda,\theta)\) 做判决
  - 探测到立刻 kill + 计数（你只关心“进入 Si 且不反射”的那一支）
- **源的演进**：
  - Debug：单色 optical photon（550 nm/2.25 eV 或邻近）用于几何/边界 sanity check
  - 基准：晶体中心 662 keV e- 激发闪烁谱
  - 最终：上方 662 keV 照射 + 球面均匀抽样方向并限制覆盖
- **工程要求**：
  - 用 git 分支/commit 管理实验
  - 用 `docs/` 记调试日志与阶段总结

---

### 目前“已完成”的实现（对齐需求）

- **移除 `sipin_window`**：界面简化为 `grease(or airgap) → sipin_si`。
- **P_det hook 已接入**：
  - `pdetMode=1`（常数）、`pdetMode=2`（CSV 查表）均已跑通
  - `U < p_det`：记录 + kill；`U ≥ p_det`：按当前模型镜面反射回 preVolume，并加 `maxInterfaceHits` 防死循环
- **调试源**：
  - 增加 `g_debug_opticalphoton`，可在晶体中心发射单色光子（用于快速验收几何/边界）
- **性能问题缓解**：
  - 处理过 `GeomNav1002` flood（反射回推位移加大）
  - 加入 `g_sipin_max_interface_hits`（防无限界面抖动）

---

### 目前遇到的主要问题（最新定位，优先级最高）

### 1) TEFLON 包覆的“底部通道被封死”（几何回归）

你指出“无 grease 过去能到 ~80%”，这个是关键线索。

我们后来对 wrapper 使用 `G4LogicalSkinSurface(..., surf_Hreflex)` 来表示 PTFE 反射面；如果 `sc_gap` 被建成 `wrapper` 的子体积，
那么光子从 `sc_gap` 底面离开时，会接触 wrapper 的底面 skin surface，从而被“PTFE 反射回去”，导致：

- 光子难以从晶体底部真正进入 `sipin_si`
- `Photon Hit si` 显著偏低（无 grease 测到 ~35% 量级），与历史 ~80% 冲突

**结论**：当前阶段的核心问题不是材料参数，而是 **TEFLON 结构的几何/边界拓扑错误**。

### 2) `g_side_contact_ratio` 尚未实现

目前侧面空气层由 `g_gap_thickness`（100 µm）统一决定，“贴合比例”没有进入几何/光学逻辑，无法做你想要的扫描。

---

### 我正在进行的修复尝试（为什么这么做）

目标：恢复 `d44848b` 版本“底部开口”的物理结构，同时保留现在的 `grease→Si` 与 P_det hook。

我正在把 TEFLON wrapper 改成：

- `sc_gap`（空气容器）不再作为 wrapper 的子体积（避免 skin surface 作用于“底部通道”）
- 用 `G4SubtractionSolid(outer - cavity)` 做一个 **wrapper 壳体**（侧壁+顶盖，底部开口）
- grease（若有）作为 `sc_gap` 的底部薄层 `optical_grease`
- crystal 放在 `sc_gap` 内，位于 grease/底部空气层之上

这样 `WrapperGapSurface` 的 PTFE 反射只作用在侧壁+顶盖上，不会“封住底部”。

---

### 近期验收计划（最小闭环）

先用 debug optical photon（各向同性）做几何验收：

- **验收 A：无 grease**
  - `Photon Hit si`（纯几何到达）应回到“合理高”的量级（目标接近你记忆中的 ~80%）
- **验收 B：有 grease**
  - 有 grease 时应明显高于无 grease（临界角更大，底面透过更多）
- **验收 C：P_det hook 回归**
  - `pdetMode=1`：p=0/0.5/1 三档结果应线性可解释
  - `pdetMode=2`：接入真实 TMM CSV 后再做小统计（1e4 events）看 90%+ 趋势

完成以上闭环后，再切回 662 keV e- 的闪烁基准源推进最终目标。



