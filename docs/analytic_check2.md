
### 文档：解析 sanity-check 场景（5×5×5 mm GAGG，\(n=1.86\)）用于验证光收集仿真

#### 0) 目的与对照口径（非常重要）
我们希望用“足够简单但仍贴近真实封装物理”的场景，得到**闭式可算**的 \(\varepsilon_{\text{col}}\)（光收集效率）预测值，用来验证 Geant4 光学追踪与统计口径正确。

- **推荐对照量**：  
  \[
  \boxed{\varepsilon_{\text{col}} \equiv P(\text{photon hits Si surface})}
  \]
  即“光子到达/撞击 Si 表面（或 Si 体积）的概率”，不要求被吸收/产生电荷。

- **为什么这样定义**：  
  这能把“几何/边界导致的到达率”与“Si 表面增透膜 + 入射角/波长依赖吸收（\(P_{\text{det}}\)）”分离开，先把几何光学部分验收干净。

- **关于 Si3N4 增透膜是否需要考虑**：  
  - 若你对照的是 \(\varepsilon_{\text{col}}\)（hit Si 即算成功），**Si3N4 不需要进入解析推导**。  
  - 若你对照的是“被探测到”的概率 \(\alpha\)，则需要：
    \[
    \alpha = \varepsilon_{\text{col}}\cdot \langle EQ_{\text{eff}}\rangle
    \]
    其中 \(\langle EQ_{\text{eff}}\rangle\) 才与 Si3N4（折射率、厚度）通过 Fresnel/TMM 相关。  
  **因此本 sanity-check 文档只推 \(\varepsilon_{\text{col}}\)**；你做仿真时建议把“hit Si 的计数”作为对照量（或设置成 hit 即 kill 并计数）。

---

#### 1) 固定参数
- **晶体尺寸**：\(L_x=L_y=L_z=5\,\text{mm}\)（立方体）
- **晶体折射率**：\(n_1=1.86\)
- **底部外介质折射率**：
  - Air：\(n_2=1.00\)
  - Grease：\(n_2=1.46\)

---

## A) 场景 A：侧/顶“绝对镜面反射”，底面为 Air / Grease（最强闭式、最适合第一跑）
### A.1 场景定义（尽量真实的简化）
- 侧面 + 顶面：**完美镜面反射**（Specular，反射率 100%，无吸收）
- 底面：真实的晶体→外介质界面（Air 或 Grease），允许 Fresnel + TIR
- 发射：晶体内部各向同性发射（位置可取中心）

> 这个场景对应真实系统中的“极限上界”：把 PTFE/ESR 的复杂反射近似为无损镜面，从而让几何只受底面临界角限制。

### A.2 关键物理事实
在侧/顶完美镜面下，光线的极角 \(\theta\)（相对底面法线）在传播中保持不变。  
因此是否能从底面离开晶体完全由 **是否发生全反射 TIR** 决定：

- 若 \(\theta>\theta_c\)（超过临界角），则在底面永远 TIR，**永远到不了 Si**。
- 若 \(\theta<\theta_c\)，每次到底面都有非零透射概率 \(T(\theta)>0\)，反复尝试后“最终透射概率”趋于 1。  
  所以 **Fresnel 反射率大小不影响最终能否出射**（只影响出射所需反射次数，不影响最终概率）。

### A.3 手算推导（闭式）
临界角：
\[
\sin\theta_c=\frac{n_2}{n_1}\qquad (n_1>n_2)
\]
令 \(\mu=\cos\theta\in[0,1]\)。对各向同性发射方向，\(\mu\) 在 \([0,1]\) 上均匀。  
TIR 条件 \(\sin\theta>\frac{n_2}{n_1}\iff \mu<\sqrt{1-(n_2/n_1)^2}\equiv \mu_0\)。

因此
\[
\boxed{
\varepsilon_{\text{col}}^{(A)} = 1-\mu_0
=1-\sqrt{1-\left(\frac{n_2}{n_1}\right)^2}
}
\]

### A.4 代入数值（你就拿这个去对照仿真）
- Air：\(\left(\frac{n_2}{n_1}\right)^2=(1/1.86)^2=0.2890\)
\[
\varepsilon_{\text{col}}^{(A,\text{air})}
=1-\sqrt{1-0.2890}
=1-\sqrt{0.7110}
=0.1567
\]
- Grease：\(\left(\frac{n_2}{n_1}\right)^2=(1.46/1.86)^2=0.6169\)
\[
\varepsilon_{\text{col}}^{(A,\text{grease})}
=1-\sqrt{1-0.6169}
=1-\sqrt{0.3831}
=0.3810
\]

**结论（场景 A 预测）：**
- \(\boxed{\varepsilon_{\text{col}}\approx 0.157\ \text{(air)}}\)
- \(\boxed{\varepsilon_{\text{col}}\approx 0.381\ \text{(grease)}}\)

---

## B) 场景 B：侧/顶“完美朗伯漫反射 (R=1)”，底面为 Air / Grease（应趋近 1，用于排查“凭空吞光”）
### B.1 场景定义
- 侧面 + 顶面：**朗伯漫反射**（Lambertian），反射率 \(R=1\)（无吸收）
- 底面：晶体→外介质（Air 或 Grease），保留 TIR + Fresnel
- 发射：晶体内部各向同性

> 这个场景对应真实系统中的另一极限：表面粗糙/漫反射足够强，使方向不断随机化。

### B.2 理论结论（概率上趋于 1）
由于每次撞侧/顶都会把方向重新随机化，光子会无限次“重抽”底面入射角。  
非 TIR 的角度集合概率 > 0，因此在无吸收/无截断的理想极限：
\[
\boxed{\varepsilon_{\text{col}}^{(B)} \to 1}
\]

**这不是为了给一个精确小数，而是为了“验收不丢光”。**  
如果你在这个场景下仍远小于 1（且你已关闭晶体吸收、关闭“逃逸到世界”的通道），说明统计口径或几何拓扑仍有问题。

### B.3（可选）给你一个“平均碰撞次数”的解析量，方便 sanity-check
在“充分混合”的近似下（对立方体很合理），每次表面相互作用：
- 以概率 \(p\) 碰到底面；在底面一次尝试成功概率 \(s\)
- 以概率 \(1-p\) 碰到侧/顶面并被随机化继续

对立方体：所有 6 面等价，底面占一面  
\[
\boxed{p=\frac{1}{6}}
\]
在朗伯入射角分布下，底面一次尝试“非 TIR”的概率为
\[
\boxed{s=\left(\frac{n_2}{n_1}\right)^2}
\]
于是“每次相互作用的成功概率”为 \(p s\)，几何分布给出平均相互作用次数
\[
\boxed{\mathbb{E}[N_{\text{coll}}]=\frac{1}{p s}}
\]
平均“侧/顶墙面”相互作用次数
\[
\boxed{\mathbb{E}[N_{\text{wall}}]=\frac{1-p}{p s}}
\]
代入数值：
- Air：\(s=0.2890\)  
  \(\mathbb{E}[N_{\text{coll}}]=1/( (1/6)\cdot0.289)=20.8\)  
  \(\mathbb{E}[N_{\text{wall}}]= (5/6)/((1/6)\cdot0.289)=17.3\)
- Grease：\(s=0.6169\)  
  \(\mathbb{E}[N_{\text{coll}}]=9.72\)，\(\mathbb{E}[N_{\text{wall}}]=8.10\)

这能帮你检查：仿真里“平均撞墙次数”是否在数量级上合理。

---

## C) 场景 C：侧/顶为“PTFE-like 朗伯漫反射，R=0.975”，底面为 Air / Grease（闭式，可直接对照）
### C.1 场景定义（贴近真实封装、但保持可解析）
- 侧面 + 顶面：朗伯漫反射，**反射率常数** \(R=0.975\)（每次撞墙有 \(1-R=0.025\) 的吸收损失）
- 底面：晶体→外介质（Air 或 Grease），保留 TIR + Fresnel
- 发射：各向同性

> 这就是你说的“侧面与顶面 100% 贴合 teflon，反射率 97.5%”的解析版本：把 PTFE 复杂 BRDF 与波长依赖简化为“朗伯 + 常数 R”，用于验证趋势与数量级。

### C.2 推导思路（两吸收态马尔可夫链，闭式）
把每次表面相互作用看作一次“试验”，有三种结果：

- **成功（到底面且非 TIR，最终会出去并 hit Si）**：概率 \(a=p\,s\)
- **失败（撞侧/顶墙并被吸收）**：概率 \(b=(1-p)(1-R)\)
- **继续（其他情况：撞墙反射、或到底面但 TIR）**：概率 \(1-a-b\)

则总成功概率为几何级数：
\[
\varepsilon=a+(1-a-b)a+(1-a-b)^2a+\cdots=\frac{a}{a+b}
\]
即
\[
\boxed{
\varepsilon_{\text{col}}^{(C)}
=
\frac{p\,s}{p\,s+(1-p)(1-R)}
}
\]
其中（立方体）：
\[
\boxed{p=\frac{1}{6}},\qquad
\boxed{s=\left(\frac{n_2}{n_1}\right)^2}
\]

### C.3 代入数值（R=0.975）
先算公共项：
\[
(1-p)(1-R)=\frac{5}{6}\cdot 0.025 = 0.020833
\]

- Air：\(s=(1/1.86)^2=0.2890\)，\(p s=0.04817\)
\[
\varepsilon_{\text{col}}^{(C,\text{air})}
=\frac{0.04817}{0.04817+0.020833}
=0.698
\]
- Grease：\(s=(1.46/1.86)^2=0.6169\)，\(p s=0.10282\)
\[
\varepsilon_{\text{col}}^{(C,\text{grease})}
=\frac{0.10282}{0.10282+0.020833}
=0.832
\]

**结论（场景 C 预测）：**
- \(\boxed{\varepsilon_{\text{col}}\approx 0.698\ \text{(air)}}\)
- \(\boxed{\varepsilon_{\text{col}}\approx 0.832\ \text{(grease)}}\)

---

## 2) 汇总表（你直接拿去对照）
在 \(5\times5\times5\text{ mm}^3\)、\(n_1=1.86\) 下：

- **场景 A（侧/顶完美镜面）**
  - Air：\(\varepsilon_{\text{col}}=0.157\)
  - Grease：\(\varepsilon_{\text{col}}=0.381\)

- **场景 B（侧/顶朗伯、R=1）**
  - Air：\(\varepsilon_{\text{col}}\to 1\)
  - Grease：\(\varepsilon_{\text{col}}\to 1\)

- **场景 C（侧/顶朗伯、R=0.975）**
  - Air：\(\varepsilon_{\text{col}}=0.698\)
  - Grease：\(\varepsilon_{\text{col}}=0.832\)

---

## 3) 你仿真时的“最小建议设置”（保证能和解析式对上）
为让解析假设成立，建议你在这三组 sanity-check 里尽量做到：

- **只保留一个损失机制**  
  - 场景 A：只允许底面 TIR 困光（侧/顶不吸收）
  - 场景 B：不允许任何吸收（R=1），不允许顶/侧漏到世界
  - 场景 C：只允许“侧/顶每次 2.5% 吸收”这一种损失（不允许额外漏光/晶体吸收）

- **统计口径要对**  
  - 对照 \(\varepsilon_{\text{col}}\)：用 “hit Si surface/volume” 计数，不要把 \(P_{\text{det}}\) 混进去  
  - 若你必须用“被探测到”的口径，那就先把 \(P_{\text{det}}\equiv 1\)（或等价设置）再跑 sanity-check

- **Si3N4 增透膜**  
  - 做 \(\varepsilon_{\text{col}}\) sanity-check 时：不用管（它只影响 \(\langle EQ_{\text{eff}}\rangle\)）

---

## 4) 你跑完给我什么结果，我能帮你判断“是否一致”
你每个场景（A-air、A-grease、B-air、B-grease、C-air、C-grease）给我两列就够：
- \(\varepsilon_{\text{col}}=\) hitSi / N
- 以及你是否确认“顶/侧没有漏到 world / 没有晶体吸收”（用逃逸通道或日志确认）

我就能告诉你：
- 是否与解析值在统计误差内一致
- 若不一致，最可能是哪条假设被破坏（例如：方向没保持/没充分混合、存在额外漏光、hit 口径不是“到达”而是“吸收”、等等）