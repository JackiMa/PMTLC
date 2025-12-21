# 调试日志：光收集效率问题排查

> **总目标**：GAGG(5×5×5 mm³) + 1mm PTFE 包覆 + grease 耦合到 SiPIN，得到 **(几何收集×PDE) ≈ 90%+**，并输出“90 多少”。
>
> **关键实现要求**：
> - 忽略 SiPIN window（只保留 `grease(or airgap) → Si`）
> - 在 `grease(or airgap) → sipin_si` 用 `P_det(λ,θ)`（TMM+QE 预计算二维表）判决
> - 探测到立刻 kill + 计数
> - 基准源：晶体中心 662 keV 电子（后续再做“上方照射 + 球面均匀抽样方向并限制覆盖”）

## 2025-12-21 调试记录

### 问题现象
- 662 keV 电子激发闪烁光：收集效率约 46%，远低于预期的 90%+
- 用户提示：550nm 光子吸收长度约 400mm，不应该有大量自吸收

### 调试步骤

#### 1. 修改调试模式发射 550nm (2.25 eV) 光子
- 吸收长度约 400mm >> 晶体尺寸 5mm，应该几乎无自吸收

#### 2. 测试固定方向（向下）vs 各向同性发射

| 光子方向 | 有 grease | 无 grease | 备注 |
|---------|----------|----------|------|
| 固定向下 (-z) | 99.3% | 99.0% | 光子直接射向 SiPIN，几乎不反射 |
| 各向同性 | **50.7%** | **30.6%** | 需要多次反射才能到达 SiPIN |

#### 3. 问题定位
- 各向同性发射时，`Escaped_CrystalAbsorbed` 仍然很高（40-60%）
- 550nm 光子吸收长度 400mm，不应该有这么高的"晶体吸收"
- 怀疑 escape channel 统计逻辑有问题

#### 4. 关键发现：grease 提高了临界角
- 无 grease（空气层 n=1.0）：晶体 n=1.86，临界角 ≈ 32.5°
- 有 grease（n=1.46）：临界角 ≈ 51.7°
- 有 grease 时更多光子能穿透底面到达 SiPIN

### 待排查

1. **`Escaped_CrystalAbsorbed` 的归类是否正确**
   - 当前逻辑：光子被 kill 时 `preVolumeName == gN_sc_crystal` 就算"晶体吸收"
   - 可能把其他原因的 kill 也错误归类了

2. **晶体侧面/顶面的边界条件是否正确**
   - 晶体 → 空气应该是 dielectric-dielectric，发生全反射
   - 需要确认没有设置多余的吸收表面

3. **PTFE 表面设置**
   - 当前：97.5% 反射 + 2.5% 透射
   - 透过 PTFE 的光子应该被归类为 `fEscapePTFE` 而不是 `fEscapeAbsorbed`

### 下一步
- 增加调试输出，追踪光子被 kill 的具体原因
- 检查是否有多余的吸收表面设置
- 对比有无 grease 时的光子路径

### Git 状态
```
28e5ecb P_det hook 验证通过 + 662keV电子基准源（光收集46%，需排查自吸收）
4453fb9 WIP: add SipinPDETable and test macros (UI cmd issue pending)
8ff1476 cursor_v1
```

---

## 2025-12-21（补充）：当前阶段的“真实问题”与修复方向

### 1) 目前遇到的核心问题（最新定位）

**问题 A：PTFE 包覆把底部通道“封死”，光子到不了 `sipin_si`（几何回归）**

- 现象：debug 各向同性光子时，底部附近出现 `Scintillator_crystal ↔ sc_gap` 来回，且 `Photon Hit si` 明显偏低（无 grease 曾跑到 ~35%）。
- 根因：我们对 `l_wrapper` 使用了 `G4LogicalSkinSurface("WrapperGapSurface", l_wrapper, surf_Hreflex)`。
  当 `sc_gap` 被建成 `wrapper` 的子体积时，光子从 `sc_gap` 底部“离开子体积”会接触 `wrapper` 的底面皮肤表面，
  **被 PTFE 反射回去**，导致底部等效“封住”，很难进入 `sipin_si`。
- 这也解释了你指出的“无 grease 过去能到 80%”：那份正确版本（`d44848b`）的几何并没有把底部通道用 skin surface 逻辑封死。

**问题 B：`g_side_contact_ratio` 尚未实现**

- 当前只有 `g_gap_thickness`（默认 100 µm）控制侧面空气层厚度；“贴合比例”没有进入几何/光学逻辑。

### 2) 已做过/正在做的尝试（最新）

- **PTFE 反射率 sanity check**
  - 将 `surf_Hreflex` 临时设为 100%（排除透过损失）后，确实能降低“透过导致的逃逸”，但并不能解决“到不了 Si”的问题。
  - 结论：反射率不是根因，底部通道被封才是根因。

- **TEFLON 几何重构（进行中）**
  - 目标：恢复“底部开口”的物理结构，同时保留我们的新需求（`grease→Si` + P_det hook）。
  - 方向：用 `G4SubtractionSolid` 做一个 **wrapper 壳体（侧壁+顶盖，底部开口）**，并把 `sc_gap` 放到 world（或至少不做 wrapper 的子体积）。
  - 这样 `skin surface` 不会把底面封住，光子能从底部空气/grease 进入 `sipin_si`。

### 3) 下一步最小闭环（立刻验收）

- **验收 1：无 grease**
  - 在修复后的几何里，debug 各向同性光子应能再次看到“合理高”的几何到达率（目标接近你记忆中的 ~80% 量级）。
- **验收 2：有 grease**
  - 有 grease 时几何到达率应显著高于无 grease（临界角变大）。
- **验收 3：P_det hook**
  - 在 `preVolume ∈ {optical_grease, sc_gap(底部空气)} && postVolume == sipin_si` 触发
  - `pdetMode=1` 三档（0/0.5/1）快速回归，再上真实 CSV 表。

### 4) 记录与 git 管理（你要求的工作方式）

- 每次只验证一个假设：改动 → 预期 → 跑数 → 结论 → 写入本日志
- 每次稳定推进都要 commit（commit message 说明“改了什么/验证了什么/结果是什么”）

---

## 2025-12-21（补充2）：晶体表面微粗糙（sigma\_alpha）对“困光/出光”的决定性影响（已验证）

### 背景：为什么要先做这个？

用户指出：当前 ~35% / ~55% 的几何到达率来自“晶体界面理想镜面（sigma\_alpha=0）”假设，这会形成类似光纤的困光（初始角度在某些范围内的光子被锁在晶体内），与真实晶体的微观粗糙不符。  
因此：**先把“微粗糙打破困光、显著提升读出”做对**，再讨论侧面贴合比例。

### 框架改动（只记录可复现实验要点）

- 新增晶体表面微粗糙参数（UNIFIED）：`/SiPINLC/geometry/crystalSigmaAlpha <value> rad`
- 在 `crystal <-> sc_gap` 边界上施加 UNIFIED 表面（`finish=ground`, `sigma_alpha>0`），用于引入角度扩散（微表面法线分布展宽）
- 为避免贴合模型干扰粗糙度结论，sanity 测试中显式设置：`/SiPINLC/geometry/sideContactRatio 0.0`

### 运行条件（sanity，纯几何到达率）

- 源：`g_debug_opticalphoton=true`（晶体中心各向同性发射单色光子）
- `pdetMode=0`（到达 `sipin_si` 就计数+kill，等价于 \(\varepsilon_{\text{col}}\)）
- 无 grease：`greaseThickness=0`, `bottomAirGap=10 um`
- 顶/侧空气层：`topAirGap=100 um`, `sideGap=100 um`
- 事件数：`N=10000`

### 结果（关键三点）

| sigma\_alpha (rad) | Photon Hit si / N | 解释 |
|---:|---:|---|
| 0.0 | 0.339 | 镜面困光明显（baseline，与此前 no-grease sanity 一致） |
| 0.1 | 0.840 | 微粗糙显著打破困光，几何到达率大幅提升 |
| 0.2 | 0.835 | 与 0.1 同量级，提示可能进入“平台/最优区间附近” |

### 结论（给后续实验的操作性结论）

- **微粗糙是决定性因素**：在当前几何下，sigma\_alpha 从 0 增加到 0.1 rad，可把 \(\varepsilon_{\text{col}}\) 从 ~0.34 拉到 ~0.84。
- 后续所有“贴合比例/grease 厚度”等扫描，建议以一个更现实的微粗糙基线（例如 sigma\_alpha=0.1 rad）开展；同时保留 sigma\_alpha=0 的曲线作为“困光极限对照”。

---

## 2025-12-21（补充3）：侧面贴合比例（probabilistic boundary）快速扫描（基线 sigma\_alpha=0.1）

### 目的

在已经确认“微粗糙打破困光”的前提下，开始研究 **侧面贴合比例 \(p\)** 对几何到达率的影响。

### 运行条件（固定）

- `pdetMode=0`（纯几何到达率）
- 无 grease：`greaseThickness=0`, `bottomAirGap=10 um`
- 顶/侧空气层：`topAirGap=100 um`, `sideGap=100 um`
- 粗糙度：`crystalSigmaAlpha=0.1 rad`
- PTFE 反射率：按论文基准 97.5%
- 事件数：`N=10000`

### 结果（p 从 0 到 1）

| sideContactRatio p | Photon Hit si / N | Escaped_PTFE | Escaped_Other | 备注 |
|---:|---:|---:|---:|---|
| 0.00 | 0.776 | 0 | 777 | 基线（无贴合补丁） |
| 0.25 | 0.617 | 377 | 2536 | 贴合补丁增加与 PTFE 交互，吸收与“其他逃逸”显著增加 |
| 0.50 | 0.547 | 525 | 3021 | 进一步下降 |
| 0.75 | 0.510 | 746 | 3587 | 进一步下降 |
| 1.00 | 0.474 | 1035 | 3767 | 下降到 ~47% |

### 初步结论

- 在“已有微粗糙（sigma\_alpha=0.1）”的条件下，**侧面贴合比例越大，几何到达率越低**（本模型/本几何/本反射率条件下）。
- 下降的直接原因（从通道统计可见）：**与 PTFE 的等效吸收（Escaped_PTFE）和其他逃逸（Escaped_Other）显著增加**。
- 下一步需要做的验证：
  - 在 **有 grease** 的条件下重复同一扫描（看趋势是否一致）。
  - 把 PTFE 反射率作为参数（例如 0.96/0.975/0.99）做交叉检查，确认结论对反射率敏感性。

