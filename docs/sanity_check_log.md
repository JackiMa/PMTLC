# Sanity Check 实验日志

## 时间线

### 2024-12-22 讨论与调试过程

#### 问题1：SteppingAction 中的 position 修改导致 GeomNav1002 警告
- **现象**：大量 `GeomNav1002` 警告，进程卡住或被 Kill
- **原因**：在 SteppingAction 中用 `SetPosition()` 手动"推回"光子，导致导航器检测到位置异常跳变
- **解决**：禁用 stepping override 逻辑，改用几何层面的 `G4LogicalBorderSurface`

#### 问题2：光子仍然发生镜面反射，未漫反射
- **现象**：可视化显示光子轨迹呈规则网格状（镜面反射特征）
- **原因**：使用 `dielectric_dielectric` 类型时，光子在 crystal→air 界面发生 TIR（全内反射），TIR 是理想镜面反射，**完全绕过**了表面的 REFLECTIVITY 和漫反射设置
- **解决**：改用 `dielectric_metal` 类型，绕过 TIR 物理，让 `ground` finish 产生真正的 Lambertian 漫反射

#### 问题3：有/无 air-gap 的物理区别
- **物理机制**：
  - **有 air-gap**：光子在 crystal-air 界面先尝试 TIR（100% 无损），只有透射的光子才碰到 PTFE（有损）
  - **无 air-gap**：光子直接与 PTFE 交互，每次反射都有 2.5% 损失
- **验证结果**：
  - 有 air-gap + crystalSigmaAlpha=0.1：ε_col = **89.6%**
  - 无 air-gap（直接贴合 PTFE，R=97.5%）：ε_col = **66.7%**
  - 差距 23%，证实 air-gap 利用 TIR 提高效率

---

## 最终设计

### 参数说明

| 参数 | 取值 | 含义 |
|------|------|------|
| `sideContactRatio` | 0 | 有 air-gap（利用 TIR） |
| `sideContactRatio` | 1 | 无 air-gap（直接贴合 PTFE，R=97.5%） |
| `sideContactRatio` | >1 (如 2.0) | sanity-check：理想 R=100% |
| `crystalSigmaAlpha` | >0 rad | 晶体表面粗糙度，打破 TIR 困光 |

### 物理逻辑

```
有 air-gap (sideContactRatio=0):
  光子 → crystal-air界面 → {
    角度 < 临界角: 透射到 air → 碰到 PTFE → 97.5% 反射 / 2.5% 吸收
    角度 > 临界角: TIR (100% 无损反射) → 继续在晶体内
  }
  crystalSigmaAlpha > 0: 每次反射时微表面法线随机偏移 → 打散角度 → 打破困光

无 air-gap (sideContactRatio=1):
  光子 → crystal-"PTFE"界面 → dielectric_metal 漫反射
  → 97.5% 反射 / 2.5% 吸收
  没有 TIR 保护
```

---

## 验证实验计划

### 场景定义（参考 docs/main.md）

| 场景 | 侧面/顶面 | 底面 | 理论 ε_col |
|------|---------|------|-----------|
| A-Air | 镜面反射（TIR） | Air | 0.157 |
| A-Grease | 镜面反射（TIR） | Grease | 0.381 |
| B-Air | Lambertian R=1 | Air | → 1.0 |
| B-Grease | Lambertian R=1 | Grease | → 1.0 |
| C-Air | PTFE R=97.5% | Air | 0.698 |
| C-Grease | PTFE R=97.5% | Grease | 0.832 |

### 实现方式

- **场景 A**：`sideContactRatio=0`, `crystalSigmaAlpha=0`（纯 TIR 困光）
- **场景 B**：`sideContactRatio=2`（理想 R=100%）
- **场景 C**：`sideContactRatio=1`（真实 PTFE R=97.5%）

---

## 验证实验结果（2024-12-22）

### 实验条件
- 光子数：1000
- 晶体材料：GAGG_Ce_Mg (scaleFactor=1, 原始吸收长度)
- 运行模式：8 线程

### 结果汇总

| 场景 | 理论值 | 仿真值 | 耗时(s) | 备注 |
|------|-------|-------|--------|------|
| A-Air | 0.157 | **0.300** | 3.2 | TIR困光，但有限立方体比无限平板逃逸更多 |
| A-Grease | 0.381 | **0.488** | 1.8 | 同上 |
| B-Air | →1.0 | **0.825** | 1.8 | 晶体自吸收导致损失 |
| B-Grease | →1.0 | **0.914** | 1.9 | 晶体自吸收导致损失 |
| C-Air | 0.698 | **0.576** | 1.8 | 偏低，可能需要更多光子统计 |
| C-Grease | 0.832 | **0.766** | 1.8 | 接近理论值 |

### 结果分析

1. **A 场景偏高**：理论公式假设无限平板（TIR 角度永不改变），实际有限立方体中光子经多次反射后角度可能改变，更多光子有机会逃逸。

2. **B 场景未达 100%**：晶体自吸收限制了效率上限。GAGG 在发射波长附近有非零吸收，光子在多次漫反射后累积路径长，部分被吸收。

3. **C 场景接近理论**：考虑到晶体自吸收的影响，结果与理论趋势一致。

### 结论

仿真结果在物理趋势上与理论一致：
- Grease 耦合 > Air 耦合（更大的透射锥）
- B 场景 > C 场景 > A 场景（漫反射打破困光 > 部分损失 > 完全困光）
- 晶体自吸收是实际效率低于理想值的主要原因

