# 参数扫描仿真设计文档

## 一、Grease 自吸收的添加

### 1.1 数据来源
- **文件**: `EJ550_Transmission_0.1mm.csv`
- **厚度**: 0.1 mm
- **折射率**: 1.46
- **波长范围**: 281.75 ~ 497 nm

### 1.2 透过率 → 吸收长度转换

透过率与吸收长度的关系：
$$T = e^{-d/L_{abs}}$$

因此：
$$L_{abs} = -\frac{d}{\ln(T)} = -\frac{0.1\ \text{mm}}{\ln(T/100)}$$

**示例计算**（取几个代表点）：

| 波长 (nm) | 透过率 (%) | $\ln(T/100)$ | $L_{abs}$ (mm) |
|-----------|-----------|--------------|----------------|
| 282       | 62.5      | -0.470       | 0.213          |
| 300       | 88.3      | -0.124       | 0.806          |
| 350       | 97.6      | -0.0243      | 4.12           |
| 400       | 98.1      | -0.0192      | 5.21           |
| 450       | 98.8      | -0.0121      | 8.26           |
| 500-700   | 98.923 (假设) | -0.0108   | 9.26           |

### 1.3 波长外推策略
- **281.75 nm 以下**：Geant4 通常不需要，因为 GAGG 发射谱在 450-650 nm
- **497 nm 以上**：按 98.923% 透过率假设，$L_{abs} \approx 9.26$ mm

### 1.4 需要确认的问题

**Q1**: 0.1 mm 厚度对应的是 grease 层的厚度，我们仿真中的 grease 厚度是 50 μm (0.05 mm)，吸收长度是材料固有属性，与厚度无关，这个理解对吗？

**A**: 是的，吸收长度 $L_{abs}$ 是材料固有属性，与实际使用的厚度无关。实际透过率 = $e^{-d_{actual}/L_{abs}}$

---

## 二、MAC 控制的参数扫描设计

### 2.1 Geant4 Messenger 机制

需要创建新的 Messenger 类来暴露可调参数。建议的命令结构：

```
/SiPINLC/geometry/greaseThickness 0.05 mm
/SiPINLC/geometry/topAirGap 0.1 mm
/SiPINLC/geometry/sideContactRatio 0.5

/SiPINLC/material/absorptionScale 1.0    # 晶体自吸收缩放系数
/SiPINLC/material/greaseRindex 1.46

/SiPINLC/physics/enableCherenkov false
```

### 2.2 需要实现的 Messenger

1. **SiPINLCGeometryMessenger**：控制几何参数
2. **SiPINLCMaterialMessenger**：控制材料参数

---

## 三、侧面贴合比例的仿真方法

### 3.1 当前状态
目前代码中有 `g_side_contact_ratio = 0.5` 参数，但**尚未实现物理效果**。

### 3.2 两种实现方案

#### 方案 A：概率边界法（推荐）

在 `SteppingAction` 中检测光子撞击晶体侧面时：
- 以概率 $p_{contact}$ 使用"晶体-PTFE"边界（直接贴合，有 Fresnel 反射）
- 以概率 $1-p_{contact}$ 使用"晶体-空气-PTFE"边界（全反射 + 漫反射）

**优点**：统计等效，实现简单，不需要复杂几何
**缺点**：需要在 stepping 中动态修改边界属性

#### 方案 B：几何分片法

将晶体侧面分成多个小面元，随机指定部分为"贴合"，部分为"有空气层"

**优点**：物理上更真实
**缺点**：几何复杂，效率低

### 3.3 方案 C：统计等效法（推荐）

**原理**：跑两组仿真，结果按比例加权

1. **全贴合仿真**：侧面无空气层，晶体直接接触 PTFE
2. **全空气层仿真**：侧面有空气间隙

加权公式：
$$\varepsilon_{col}(p) = p \cdot \varepsilon_{col}^{contact} + (1-p) \cdot \varepsilon_{col}^{airgap}$$

**优点**：
- 不需要修改核心光学边界代码
- 几何简单，只需要切换两种配置
- Bug 风险最低

**校验方法**：
- $\varepsilon_{col}^{contact}$ 应该高于 $\varepsilon_{col}^{airgap}$（贴合时更多光子被反射回晶体）
- 两个极限情况 (p=0, p=1) 应该符合物理预期

---

## 四、晶体自吸收系数的扫描设计

### 4.1 问题分析

你提到想用"发射光谱/自吸收光谱的加权比值"来表征不同材料。这个思路很好，但需要明确：

#### 4.1.1 物理意义
对于不同的闪烁晶体：
- **发射谱** $S(\lambda)$：材料固有，决定发光波长分布
- **自吸收谱** $\alpha(\lambda)$ 或 $L_{abs}(\lambda)$：与发射谱重叠程度决定自吸收严重性

#### 4.1.2 有效自吸收系数

可以定义一个**有效平均吸收长度**：
$$\langle L_{abs} \rangle = \frac{\int S(\lambda) \cdot L_{abs}(\lambda) \, d\lambda}{\int S(\lambda) \, d\lambda}$$

或者定义**自吸收强度因子**：
$$\eta_{abs} = \frac{\int S(\lambda) / L_{abs}(\lambda) \, d\lambda}{\int S(\lambda) \, d\lambda}$$

### 4.2 扫描方式

**方案 1**：直接缩放吸收长度
```cpp
// 在 Messenger 中设置缩放系数
/SiPINLC/material/absorptionScale 0.5   // L_abs 减半，自吸收加倍
/SiPINLC/material/absorptionScale 2.0   // L_abs 加倍，自吸收减半
```

**方案 2**：扫描等效吸收长度
```cpp
/SiPINLC/material/effectiveAbsLength 5 mm
/SiPINLC/material/effectiveAbsLength 10 mm
/SiPINLC/material/effectiveAbsLength 20 mm
```

**Q3**: 你更倾向于哪种方式？方案 1 保持光谱形状，只缩放强度；方案 2 使用平坦的吸收长度，更简单但不够物理。

### 4.3 输出校正曲线

最终输出：
$$C_{abs}(\langle L_{abs} \rangle) = \frac{\varepsilon_{col}(L_{abs} \to \infty)}{\varepsilon_{col}(\langle L_{abs} \rangle)}$$

这条曲线可供其他材料/几何参考。

---

## 五、参数灵敏度分析

### 5.1 需要扫描的参数列表

| 参数 | 符号 | 基准值 | 扫描范围 | 步长 |
|------|------|--------|----------|------|
| Grease 厚度 | $d_{grease}$ | 50 μm | 0 - 200 μm | 20 μm |
| 侧面贴合比例 | $p_{contact}$ | 50% | 0% - 100% | 10% |
| 顶面空气层 | $d_{top}$ | 100 μm | 0 - 500 μm | 50 μm |
| PTFE 反射率 | $\rho$ | 97.5% | 95% - 99% | 1% |
| 晶体吸收缩放 | $k_{abs}$ | 1.0 | 0.5 - 2.0 | 0.25 |
| Grease 折射率 | $n_{grease}$ | 1.46 | 1.44 - 1.50 | 0.02 |

### 5.2 输出格式

建议 CSV 格式：
```
run_id, grease_thickness, contact_ratio, top_airgap, reflectivity, abs_scale, 
photon_generated, photon_detected, collection_efficiency, mean_theta, mean_pathlength
```

### 5.3 扫描脚本示例 (gamma_scan.mac)

```
# === Grease 厚度扫描 ===
/SiPINLC/geometry/greaseThickness 0.00 mm
/run/beamOn 10000

/SiPINLC/geometry/greaseThickness 0.02 mm
/run/beamOn 10000

/SiPINLC/geometry/greaseThickness 0.04 mm
/run/beamOn 10000
# ... 继续
```

---

## 六、实现步骤建议

### Phase 1：基础设施
1. [ ] 创建 `SiPINLCGeometryMessenger` 类
2. [ ] 创建 `SiPINLCMaterialMessenger` 类
3. [ ] 修改 `DetectorConstruction` 支持动态参数更新

### Phase 2：Grease 自吸收
4. [ ] 在 `MyMaterials::OpticalGrease()` 中添加吸收长度数据
5. [ ] 验证透过率计算正确性

### Phase 3：侧面贴合
6. [ ] 实现概率边界法（或几何分片法）
7. [ ] 添加 MAC 命令控制

### Phase 4：扫描与分析
8. [ ] 编写扫描 MAC 脚本
9. [ ] 编写结果分析 Python 脚本
10. [ ] 生成校正曲线

---

## 七、待确认问题汇总

1. **Q1**: 吸收长度是材料固有属性，与样品厚度无关，确认理解正确？

2. **Q2**: 侧面贴合仿真，倾向于概率边界法还是几何分片法？

3. **Q3**: 晶体自吸收扫描，倾向于缩放系数还是等效吸收长度？

4. **Q4**: 扫描时是否需要每个参数组合都跑？还是先做单参数扫描，再做关键参数的交叉扫描？

5. **Q5**: 每个参数点需要多少事件数？建议至少 10000 以保证统计精度。

6. **Q6**: SiPIN 的 AR 涂层建模（论文 4.3 节提到的迭代方法），是否在这次实现，还是后续再做？

---

## 八、时间线建议

| 阶段 | 内容 | 预计时间 |
|------|------|----------|
| Phase 1 | Messenger 基础设施 | 1 天 |
| Phase 2 | Grease 自吸收 | 0.5 天 |
| Phase 3 | 侧面贴合 | 1-2 天 |
| Phase 4 | 扫描脚本 + 分析 | 1 天 |
| **总计** | | **3.5-4.5 天** |

---

## 九、已实现功能清单

### ✅ 已完成

1. **Grease 自吸收**
   - 文件：`src/MyMaterials.cc` 的 `OpticalGrease()` 函数
   - 添加了基于 EJ-550 透过率数据转换的吸收长度
   - 波长范围：300-700 nm，外推到 GAGG 发射谱范围

2. **Parameter Messenger**
   - 文件：`include/SiPINLCParameterMessenger.hh`, `src/SiPINLCParameterMessenger.cc`
   - 支持的 MAC 命令（在 `/run/initialize` **之前**执行）：
     - `/SiPINLC/geometry/greaseThickness` - Grease 层厚度
     - `/SiPINLC/geometry/topAirGap` - 顶面空气层厚度
     - `/SiPINLC/geometry/sideGap` - 侧面空气层厚度
     - `/SiPINLC/geometry/sideContact` - 侧面贴合模式 (true/false)
     - `/SiPINLC/material/absorptionScale` - 吸收长度缩放系数
     - `/SiPINLC/material/effectiveAbsLength` - 等效吸收长度
     - `/SiPINLC/material/ptfeReflectivity` - PTFE 反射率

3. **Shell 扫描脚本**（推荐使用，避免几何重建问题）
   - `scripts/run_grease_scan.sh` - Grease 厚度扫描
   - `scripts/run_side_contact_scan.sh` - 侧面贴合扫描（统计等效法）
   - `scripts/run_top_airgap_scan.sh` - 顶面空气层扫描

4. **侧面贴合处理**
   - 使用统计等效法：分别跑全贴合和全空气层两种配置
   - 加权公式：$\varepsilon_{col}(p) = p \cdot \varepsilon_{col}^{contact} + (1-p) \cdot \varepsilon_{col}^{airgap}$

### 🔲 待实现

5. **等效吸收长度**：需要修改晶体材料定义以支持动态吸收长度
6. **Python 分析脚本**：用于处理扫描结果和绘图

---

## 十、使用说明

### 运行扫描（推荐使用 Shell 脚本）

```bash
cd /home/wsl2/myGeant4/SiPINLC

# Grease 厚度扫描 (默认 1e6 事件)
./scripts/run_grease_scan.sh

# 使用自定义事件数
./scripts/run_grease_scan.sh 100000

# 侧面贴合扫描
./scripts/run_side_contact_scan.sh

# 顶面空气层扫描
./scripts/run_top_airgap_scan.sh
```

### 单次运行测试

```bash
cd /home/wsl2/myGeant4/SiPINLC/build
./SiPINLC -m ../mac/test_single_run.mac
```

### 结果分析

结果保存在 `build/run_data.csv`，包含以下列：
- `runID` - 运行编号
- `Scintillation Photon Count` - 产生的闪烁光子数
- `Photon Hit si` - 到达 SiPIN 的光子数
- `lightCollectionEfficiency` - 光收集效率
- `Escaped_CrystalAbsorbed` - 被晶体吸收的光子数
- `Escaped_TopAir` - 从顶面逃逸的光子数
- `Escaped_SideAir` - 从侧面逃逸的光子数
- `Escaped_PTFE` - 被 PTFE 吸收的光子数
- `Escaped_Other` - 其他损失
- `Mean_IncidenceAngle_deg` - 平均入射角（度）
- `Mean_HitCount` - 平均撞击次数


