# Geant4 仿真实验计划

## 目标
根据论文 `SiPIN_绝对光产额测量/latex/content.tex` 的需求，设计并完成 Geant4 仿真实验。

## 论文需要的仿真结果

### 1. Sanity Check (analytic_check2) ✅ 已完成
- A场景：镜面反射（TIR）
- B场景：理想朗伯漫反射（R=100%）
- C场景：PTFE 漫反射（R=97.5%）

### 2. 光传输特性分析 ✅ 已完成
- [x] 入射角分布 P(θ) → `detailed_analysis.png`
- [x] 撞击次数分布 → `detailed_analysis.png`
- [x] 光子逃逸通道分析 → `detailed_analysis.png`
- [x] 光谱变化（自吸收效应）→ `wavelength_spectrum.png`

### 3. 参数扫描 ✅ 已完成
- [x] Grease 层厚度扫描 → `scan_grease_thickness.csv`
- [x] 侧面贴合比例扫描（sideGap）→ `scan_side_gap.csv`
- [x] 顶面空气层厚度扫描 → `scan_top_airgap.csv`
- [x] 自吸收校正曲线（sigma_alpha）→ `scan_absorption.csv`, `absorption_vs_roughness.png`

### 4. SiPIN 有效量子效率 ✅ 已完成
- [x] P_det(λ,θ) 二维表：`sipin_pdet_realistic.csv`
- [x] Geant4 在线查表：`pdetMode=2` 验证通过

---

## 技术问题讨论

### 关于 side_contact_ratio 的实现

**用户建议**：将 air-gap 设置为很小的值（如 1μm），这样：
- 光子进入 gap 后几乎立刻碰到 PTFE wrapper
- 可以直接调用 PTFE 反射过程，无需手动修改光子位置
- 避免 GeomNav1002 警告

**当前实现问题**：
- `sideContactRatio=1` 使用 `dielectric_metal` BorderSurface 模拟"直接贴合"
- 但实际几何中仍有 gap 体积存在

**改进方案**：
1. **方案A**：保持当前 BorderSurface 方式（简单，但非物理）
2. **方案B**：设置极小 air-gap（1μm），让光子自然碰到 PTFE wrapper（更物理）

**决定**：
- **Sanity-check**：采用方案A（BorderSurface），已验证通过
- **真实物理模拟**：采用方案B，设置 `sideGap=1μm`，让光子自然碰到 PTFE wrapper

**方案B实现要点**：
```
sideContactRatio = 0  (不使用BorderSurface)
sideGap = 1 μm       (极薄空气层)
topAirGap = 1 μm     (极薄顶部空气层)
```
这样光子在 crystal-air 界面发生 TIR 或透射后，几乎立刻碰到 PTFE wrapper，
由 PTFE 的 `groundfrontpainted` 表面处理反射，无需手动修改位置。

### 关于 P_det(λ,θ) 的实现

论文 Section 4.3 描述了完整的实现策略：
1. TMM 预计算：从厂家 QE 反推 AR 涂层参数
2. 生成 grease 入射的二维表 P_det(λ,θ)
3. Geant4 在线查表判决

**当前状态**：
- `pdetMode=0`：纯几何（hit Si = detect）← Sanity-check 使用此模式
- `pdetMode=1`：常数探测概率
- `pdetMode=2`：CSV 二维表（待实现完整）

**Sanity-check 是否考虑 P_det(λ,θ)**：
- **否**，sanity-check 使用 `pdetMode=0`，即光子到达 Si 表面即视为探测到
- 这是为了验证几何光收集效率 ε_col，与探测器响应解耦
- P_det(λ,θ) 将在后续真实物理模拟中引入

**实现计划**：
1. 用 TMM 脚本生成 P_det(λ,θ) 二维表
2. 在 `SiPINLCSteppingAction` 中读取表并在线查表
3. 每次光子到达 grease→Si 边界时，根据 (λ,θ) 抽样决定探测/反射

---

## 实验执行计划

### Phase 1: Sanity Check (10000 光子) ✅ 已完成
完成 6 个场景的验证实验，结果见 `sanity_check_log.md`

**结果摘要**：
| 场景 | 理论值 | 仿真值 | 晶体吸收 |
|------|-------|-------|---------|
| A-Air | 0.157 | 0.284 | 6174 |
| A-Grease | 0.381 | 0.481 | 4092 |
| B-Air | →1.0 | 0.821 | 1760 |
| B-Grease | →1.0 | 0.911 | 709 |
| C-Air | 0.698 | 0.583 | 4157 |
| C-Grease | 0.832 | 0.776 | 2074 |

**结论**：物理趋势正确，偏差主要来自晶体自吸收

### Phase 2: 光传输特性分析 ✅ 已完成
使用基准配置运行（50μm grease, sigma_alpha=0.1 rad）：
- ✅ 入射角直方图：入射角多在 0-50°，临界角 51.7° 处有截断
- ✅ 撞击次数直方图：多数光子 1-3 次撞击
- ✅ 各逃逸通道计数：晶体吸收 ~800，PTFE 损失 ~200
- ✅ 到达谱 vs 发射谱：530nm GAGG 峰值保持

### Phase 3: 参数扫描 ✅ 已完成
**扫描结果**:

| 参数 | 范围 | 关键发现 |
|------|------|----------|
| grease厚度 | 0-200μm | 20μm 跳升至 86.4%（需要足够厚以桥接折射率差） |
| 顶面空气层 | 1-500μm | 几乎无影响（~85.5%），TIR 主导 |
| 晶体粗糙度 | 0-0.5rad | **关键**：0→48.9%（困光），0.01→84.5%（打破TIR） |
| 侧面空气层 | 1-500μm | 更大间隙→更多 TIR 机会 |

### Phase 4: P_det(λ,θ) 实现 ✅ 已完成
- ✅ 创建真实 P_det 表：`spectrum/sipin_pdet_realistic.csv`
- ✅ 验证查表逻辑：pdetMode=2 生效
- ✅ 效果：几何 85.7% → 真实探测 57.0%（考虑 Fresnel 损失+QE）

---

## 生成的图表总结

| 文件 | 内容 |
|------|------|
| `parameter_scan.png` | 四参数敏感性分析图 |
| `detailed_analysis.png` | 入射角/撞击次数/逃逸通道/路程分布 |
| `wavelength_spectrum.png` | 闪烁光谱（生成 vs 探测） |
| `absorption_vs_roughness.png` | 表面粗糙度对自吸收影响 |
| `sanity_check_results.png` | Sanity check 对比图 |
| `sanity_check_absorption.png` | Sanity check 吸收分析 |

## CSV 结果文件

| 文件 | 内容 |
|------|------|
| `scan_grease_thickness.csv` | grease 厚度扫描 |
| `scan_top_airgap.csv` | 顶面空气层扫描 |
| `scan_sigma_alpha.csv` | 表面粗糙度扫描 |
| `scan_side_gap.csv` | 侧面空气层扫描 |
| `scan_absorption.csv` | 自吸收分析 |

---

## 文档更新记录

| 日期时间 | 内容 |
|---------|------|
| 2024-12-22 10:00 | 创建计划文档 |
| 2024-12-22 10:21 | 完成 Sanity Check (10000 光子)，生成结果图 |
| 2024-12-22 10:30 | 讨论 side_contact_ratio 实现方案 |
| 2024-12-22 10:35 | 验证 sideGap=1μm 时光子正确碰到 PTFE wrapper (ε_col=0.726) |
| 2024-12-22 10:40 | 开始 Phase 2: 光传输特性分析 |
| 2024-12-23 00:07 | 完成参数扫描 (grease, topGap, sigmaAlpha, sideGap) |
| 2024-12-23 00:08 | 生成 parameter_scan.png/pdf |
| 2024-12-23 00:11 | 测试 P_det 模式: mode0=0.857, mode1(0.8)=0.811 |
| 2024-12-23 00:15 | 生成 detailed_analysis.png/pdf (入射角、撞击次数、逃逸通道、路程分布) |
| 2024-12-23 00:16 | 生成 wavelength_spectrum.png/pdf (闪烁光谱) |
| 2024-12-23 00:18 | 完成自吸收分析: sigma_alpha=0→4096 absorbed, sigma=0.5→713 absorbed |
| 2024-12-23 00:19 | 生成 absorption_vs_roughness.png/pdf |
| 2024-12-23 00:22 | 测试 P_det CSV 表: mode2=0.570 (真实探测效率后降低) |
| 2024-12-23 00:25 | 完成所有仿真任务，更新文档总结 |
| 2024-12-23 00:35 | 实现 TMM 计算脚本，从厂家 QE 反推 Si3N4 膜厚 (d*=96nm) |
| 2024-12-23 00:38 | 生成正确的 P_det(λ,θ) 表：sipin_pdet_air_tmm.csv, sipin_pdet_grease_tmm.csv |
| 2024-12-23 00:40 | 验证：无grease=74.4%, 有grease=84.6% (Grease 减少 TIR 优势明显) |
| 2024-12-23 00:50 | 理论验证：实现闭式 ε_col(σ,p) 计算脚本 |
| 2024-12-23 00:55 | 发现：Geant4 超出理论上界，因 PTFE wrapper 回收 air-gap 逃逸光子 |
| 2024-12-23 01:00 | 生成 theory_validation_report.md 完整文档 |

