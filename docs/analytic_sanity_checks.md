### 目的：用“可手算”的简化光学场景，验证 Geant4 光子追踪与统计口径的正确性

本项目的核心输出之一是光收集效率
\[
\varepsilon_{\text{col}} = P(\text{photon hits Si})
\]
在代码里对应 `run_data.csv` 的 `lightCollectionEfficiency`（在 `pdetMode=0` 且发射 primary optical photon 的 sanity 模式下，它等价于 `Photon Hit si / N_events`）。

真实几何下（PTFE 包覆 + 顶/侧空气层 + 底部 grease/空气薄层）很难直接解析计算。因此我们设计一组“理论可解”的极简边界条件，让 Monte Carlo 必须落在可预测的结果上，从而为后续复杂场景建立信心。

---

### 简化模型开关（本仓库已实现）

新增 MAC 控制参数（见 `include/config.hh` 和 `src/SiPINLCSteppingAction.cc`）：

- `/SiPINLC/sanity/wallModel`
  - 0：关闭（使用真实边界与既有逻辑）
  - 1：晶体**侧面+顶面**强制为**理想镜面反射**（specular），反射率 `R` 可设
  - 2：晶体**侧面+顶面**强制为**朗伯漫反射**（Lambertian），反射率 `R` 可设
- `/SiPINLC/sanity/wallReflectivity`：墙面反射率 \(R\in[0,1]\)
- `/SiPINLC/sanity/wallApplySide`、`/SiPINLC/sanity/wallApplyTop`：是否作用于侧面/顶面

注意：
- **底面永远不覆盖**（保持真实的 bottom coupling：空气薄层或 grease slab），这样解析结果只依赖底面临界角/TIR。

对应的宏文件：
- `mac/analytic_sanity_mirror_air.mac`
- `mac/analytic_sanity_mirror_grease.mac`
- `mac/analytic_sanity_lambert_air_R1.mac`
- `mac/analytic_sanity_lambert_air_R0975.mac`

---

### 场景 S1：侧/顶为理想镜面墙（R=1），底部为介质界面（空气或 grease）

#### 假设
- 晶体为均匀介质，折射率 \(n_1\)（GAGG，具体值取 `MyMaterials` 在发射波长处的 \(n(\lambda)\)）
- 底部耦合介质折射率 \(n_2\)（空气 \(n_2\approx 1\)；grease \(n_2\approx 1.46\)）
- 晶体侧面与顶面为**完美镜面反射**（不吸收，且镜面反射不改变入射角的极角）
- 光子在晶体内各向同性发射，方向分布均匀

在这个场景中，光子能否最终到达 Si 的唯一“永久阻挡机制”是：它在底面入射角超过临界角导致 **TIR（全反射）**，从而永远无法进入底部耦合介质。

一旦某个方向满足“非 TIR”，即使每次到底面还有 Fresnel 反射，只要透射概率 \(T(\theta)>0\)，在无吸收条件下重复多次尝试的最终透射概率为 1。  
因此 \(\varepsilon_{\text{col}}\) 只由 “是否发生 TIR” 决定，而与 Fresnel 系数大小无关。

#### 手算推导（闭式）
设光子在晶体内相对底面法线（\(-z\) 或 \(+z\)）的夹角为 \(\theta\in[0,\pi/2]\)。  
TIR 条件为
\[
\sin\theta > \frac{n_2}{n_1}.
\]
定义 \(\mu=\cos\theta\in[0,1]\)。各向同性方向满足 \(\mu\) 在 \([0,1]\) 上均匀分布。  
令
\[
\mu_0 = \sqrt{1-\left(\frac{n_2}{n_1}\right)^2}.
\]
则“发生 TIR（永远不可能透射到底部）”对应 \(\mu<\mu_0\)，其概率为 \(\mu_0\)。  
因此
\[
\boxed{
\varepsilon_{\text{col}}^{\text{mirror}}(n_1,n_2)=P(\mu\ge \mu_0)=1-\sqrt{1-\left(\frac{n_2}{n_1}\right)^2}
}
\]

#### 代入一个典型数值（仅作数量级参考）
若取 \(n_1=1.86\)：
- 空气 \(n_2=1\)：\(\varepsilon_{\text{col}}\approx 1-\sqrt{1-(1/1.86)^2}\approx 0.157\)
- grease \(n_2=1.46\)：\(\varepsilon_{\text{col}}\approx 1-\sqrt{1-(1.46/1.86)^2}\approx 0.380\)

你应该用 `MyMaterials` 中的实际折射率（对应你的单色光子能量/波长）替换 \(n_1,n_2\) 再算一次，作为“理论预测值”。

#### 对应仿真宏
- `mac/analytic_sanity_mirror_air.mac`
- `mac/analytic_sanity_mirror_grease.mac`

对照方式：
- 用 `run_data.csv` 的 `lightCollectionEfficiency` 与上述闭式公式对比（统计误差 \(\sim \sqrt{p(1-p)/N}\)）。

---

### 场景 S2：侧/顶为理想朗伯漫反射墙（R=1），底部为空气耦合

#### 直觉与结论
只要侧/顶的反射会**随机化方向**（漫反射），则光子会不断“重抽”入射角。  
在无吸收、无步长上限导致的数值截断的理想条件下，总会以概率 1 在某一次到达底面时满足 \(\theta<\theta_c\)，从而最终进入底部并到达 Si。

因此
\[
\boxed{\varepsilon_{\text{col}}^{\text{lambert}}(R=1)\to 1}
\]

#### 对应仿真宏
- `mac/analytic_sanity_lambert_air_R1.mac`

---

### 场景 S3：侧/顶为 PTFE-like 漫反射墙（R=0.975），底部为空气耦合（半解析检验）

此时的损失机制主要来自“每次撞墙有 \(1-R\) 的吸收概率”。解析严格解需要知道在“最终到达底部之前”会撞墙多少次的随机分布。

一个很强的半解析校验方法是两步走：

1) **先跑 R=1 的几何**（不吸收），统计每个光子到达 Si 之前经历的墙面交互次数 \(N_{\text{wall}}\) 的分布 \(P(N_{\text{wall}})\)  
   - 该分布在 ROOT 里是 `WallHitCount` 直方图

2) 由“每次撞墙独立存活概率为 R”的假设，可预测
\[
\boxed{
\varepsilon_{\text{col}}^{\text{pred}}(R)=\mathbb{E}\left[R^{N_{\text{wall}}}\right] = \sum_k P(N_{\text{wall}}=k)\,R^k
}
\]
再与直接跑 `R=0.975` 的 Monte Carlo 得到的 \(\varepsilon_{\text{col}}^{\text{direct}}(R)\) 对比。

若两者一致（在统计误差内），说明：
- “墙面吸收/反射判决”实现正确
- 逃逸通道/计数口径（尤其是 `Escaped_PTFE`）一致
- `WallHitCount` 的统计方式与几何逻辑一致

对应宏：
- `mac/analytic_sanity_lambert_air_R1.mac`（拿 \(P(N_{\text{wall}})\)）
- `mac/analytic_sanity_lambert_air_R0975.mac`（直接 MC）

---

### 你关心的三个“简化场景”如何映射到本套 sanity check

1) **“晶体表面绝对镜面反射；下表面空气/grease 耦合”**  
   - 直接对应：S1（镜面墙）+ `bottomAirGap` 或 `greaseThickness`

2) **“晶体侧面和顶面被 teflon 100% 贴合，teflon 反射率 97.5%”**  
   - 直接对应：S3（Lambertian wall，R=0.975，作用于 side+top）
   - 说明：这里把 PTFE 的复杂反射近似为“朗伯漫反射 + 常数反射率”，用于 sanity 对照

3) **“别的一些简化场景”**  
   - 推荐顺序：先用 S1/S2/S3 把“角度拓扑 / 反射随机化 / 吸收判决”三件事分别验收通过，再回到真实 PTFE wrapper + airgap + roughness 的组合场景


