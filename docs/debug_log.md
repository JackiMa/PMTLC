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

