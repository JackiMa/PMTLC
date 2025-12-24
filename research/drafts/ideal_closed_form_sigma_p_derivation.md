# 理想闭式模型：\(\varepsilon_{\mathrm{col}}(\sigma_\alpha, p)\) 的推导（含 air/grease）

> 写作目的：把 `docs/analytic_check2.md` 的 6 个特例（A/B/C × air/grease）推广到连续参数  
> **侧面粗糙度** \(\sigma_\alpha\) 与 **侧面贴合比例** \(p\) 的“理想闭式模型”，得到类似于
> “LCE vs sigma”、“LCE vs p”的理论曲线，并明确该模型在极限上必须回到已验证的解析结果。

---

## 0. 变量、口径与目标

### 0.1 对照量

本推导仍以 `analytic_check2` 的对照量为基准：

\[
\boxed{\varepsilon_{\mathrm{col}} \equiv P(\text{photon hits Si})}
\]

即“光子至少一次到达/撞击 Si（或 SiPIN 有效面）”的概率；不包含 Si 表面探测概率 \(P_{\text{det}}(\lambda,\theta)\)，也不包含 \(\alpha_{\mathrm{SiPIN}}\)。

### 0.2 固定参数（与 analytic\_check2 一致）

- 晶体折射率：\(n_1\)（默认示例 \(n_1=1.86\)）
- 底面外介质折射率：  
  - air：\(n_2=1.0\)  
  - grease：\(n_2=1.46\)
- 侧/顶“接触区”反射率：\(R_c\)（PTFE-like，常数近似，如 \(R_c=0.975\)）
- 侧面贴合比例（sideContactRatio）：\(p\in[0,1]\)
  - \(p=0\)：侧面全是 air-gap（理想化：无吸收损失）
  - \(p=1\)：侧面全贴合 PTFE（每次撞墙有吸收损失 \(1-R_c\)）

### 0.3 目标：连续曲线 + 极限回归

我们希望得到闭式表达 \(\varepsilon_{\mathrm{col}}(\sigma_\alpha,p)\)，并满足以下极限（必须）：

- **A 场景（镜面、无损）**：\(\sigma_\alpha\to 0\)，且 \(p\) 任意但 \(R_{\mathrm{eff}}=1\)  
  \[
  \varepsilon_{\mathrm{col}} \to 1-\sqrt{1-(n_2/n_1)^2}\qquad(\text{analytic\_check2 A})
  \]
- **B 场景（朗伯、无损）**：\(\sigma_\alpha\to \infty\)（充分混合），且 \(R_{\mathrm{eff}}=1\)  
  \[
  \varepsilon_{\mathrm{col}} \to 1\qquad(\text{analytic\_check2 B})
  \]
- **C 场景（朗伯、墙面吸收）**：\(\sigma_\alpha\to \infty\)，且 \(p=1\Rightarrow R_{\mathrm{eff}}=R_c<1\)  
  \[
  \varepsilon_{\mathrm{col}} \to
  \frac{p_b s}{p_b s+(1-p_b)(1-R_c)},\quad
  p_b=\frac16,\ s=\left(\frac{n_2}{n_1}\right)^2
  \qquad(\text{analytic\_check2 C})
  \]

> 说明：这里的“充分混合”是解析近似（mean-field）。真实 Geant4 中 \(\sigma_\alpha\) 与“混合速度/程度”的关系并非严格等价，但可以用一个单调映射去近似。

---

## 1. 两个关键概率：底面“可出射集合”的概率

令底面入射角 \(\theta\) 相对底面法线，\(\mu\equiv\cos\theta\in[0,1]\)。

临界角：
\[
\sin\theta_c=\frac{n_2}{n_1}\quad(n_1>n_2),
\qquad
\mu_0 \equiv \cos\theta_c = \sqrt{1-(n_2/n_1)^2}.
\]

### 1.1 “镜面困光极限”（对应 analytic\_check2 A）

对晶体内各向同性方向，\(\mu\) 在 \([0,1]\) 上均匀。  
非 TIR（可出射）集合 \(\theta<\theta_c\iff \mu>\mu_0\) 的概率为：
\[
\boxed{s_A = P(\mu>\mu_0)=1-\mu_0
=1-\sqrt{1-(n_2/n_1)^2}}.
\]

### 1.2 “充分混合朗伯极限”（对应 analytic\_check2 B/C）

对“撞到某一表面”的入射角，方向分布为 cosine-weighted：
\[
f(\mu)=2\mu,\quad \mu\in[0,1].
\]

因此非 TIR 的概率为：
\[
\boxed{s_L = \int_{\mu_0}^{1} 2\mu\,d\mu = 1-\mu_0^2 = \left(\frac{n_2}{n_1}\right)^2}.
\]

> 这就是 analytic\_check2 C 中用到的 \(s=(n_2/n_1)^2\)。

---

## 2. 侧面贴合比例 \(p\)：等效墙面反射率

理想化假设：

- 我们在此明确区分两种“可验证的理想极限”，分别输出两套理论曲线：
  - **模型 M1：air-gap 逃逸全回收（上界）**  
    光子一旦从晶体侧/顶面折射进入 air-gap，最终仍被外部反射层/包覆结构完全回收并返回晶体；因此可近似视为“无损边界”（等效 \(R=1\)）。
  - **模型 M2：air-gap 逃逸全丢失（下界，裸晶体极限）**  
    光子一旦折射逃逸进入 air-gap，即永久丢失（不回收）。该模型更贴近“未封装晶体”的实验/仿真条件。

- 侧/顶“非接触区”（air-gap）在 **模型 M1** 中视为**无吸收损失**：反射率 \(R=1\)（等效全回收）。
- 侧/顶“接触区”（贴合 PTFE）具有常数反射率 \(R_c<1\)。

在 **模型 M1** 中，每次撞墙的等效反射率为面积加权：
\[
\boxed{
R_{\mathrm{eff}}(p)= (1-p)\cdot 1 + p\cdot R_c = 1-p(1-R_c).
}
\]

---

## 3. 粗糙度 \(\sigma_\alpha\)：用“混合概率” \(q(\sigma_\alpha)\) 做闭式化

真实的 UNIFIED/\(\sigma_\alpha\) 会改变反射后方向分布；为得到可解析结果，我们做一个最小建模：

- 每次发生“墙面反射且未被吸收”后：
  - 以概率 \(q(\sigma_\alpha)\) 认为方向被“充分混合”（下一次撞到任何表面时入射角服从朗伯统计）
  - 以概率 \(1-q(\sigma_\alpha)\) 认为仍保持“镜面困光式的角度结构”

并要求：
\[
q(0)=0,\qquad q(\sigma_\alpha\to\infty)\to 1.
\]

一个简单且无新尺度的选取是用“晶体-空气”临界角作尺度（避免 \(q\) 随 bottom 介质变化）：
\[
\boxed{
q(\sigma_\alpha)=1-\exp\!\left[-\left(\frac{\sigma_\alpha}{\theta_c}\right)^2\right]
},
\qquad
\theta_c=\arcsin(1/n_1).
\]

> 这不是唯一选取，但满足单调、无量纲、且“\(\sigma\sim\theta_c\) 足以破坏困光”的直觉。

---

## 4. 闭式求解：两态马尔可夫链（U/T）

### 4.1 状态定义

我们只跟踪“底面是否处于可出射集合”的状态：

- \(U\)（untrapped）：下一次到底面时为非 TIR（可出射集合）
- \(T\)（trapped）：下一次到底面时为 TIR（不可出射集合）

### 4.2 事件分解（一次“表面相互作用”）

采用 analytic\_check2 C 同样的 mean-field：

- 以概率 \(p_b=\frac16\) 碰到底面；
- 以概率 \(1-p_b=\frac56\) 碰到侧/顶墙面。

墙面相互作用：

- 以概率 \(1-R_{\mathrm{eff}}(p)\) 被吸收（失败终止）
- 以概率 \(R_{\mathrm{eff}}(p)\) 存活并反射；随后
  - 以概率 \(q\) 进入“充分混合”机制：此时状态被重置为
    \[
    P(U|\text{mix})=s_L,\quad P(T|\text{mix})=1-s_L
    \]
  - 以概率 \(1-q\) 状态保持不变（\(U\to U\)，\(T\to T\)）

底面相互作用：

- 若处于 \(U\)：认为“最终一定能出射并 hit Si”（成功终止）
- 若处于 \(T\)：发生 TIR，仍处于 \(T\) 并继续

### 4.3 写出方程

令 \(P_U\) 表示从状态 \(U\) 出发最终成功（hit Si）的概率；\(P_T\) 类似。

记 \(A\) 为“一次墙面相互作用后仍留在晶体内部并继续传播”的概率（wall-survival）。

- **模型 M1（air-gap 逃逸全回收）**：
\[
\boxed{
A = (1-p_b)\,R_{\mathrm{eff}}(p)=\frac56 R_{\mathrm{eff}}(p)
}.
\]

则

\[
\begin{aligned}
P_U &= p_b\cdot 1
     + A\Big[(1-q)P_U + q\big(s_L P_U+(1-s_L)P_T\big)\Big],\\
P_T &= p_b\cdot P_T
     + A\Big[(1-q)P_T + q\big(s_L P_U+(1-s_L)P_T\big)\Big].
\end{aligned}
\]

整理得线性方程组：

\[
\boxed{
\begin{aligned}
\Big[1-A(1-q+qs_L)\Big]P_U - A q(1-s_L)P_T &= p_b,\\
-A q s_L P_U + \Big[1-p_b-A(1-q s_L)\Big]P_T &= 0.
\end{aligned}}
\]

解该 \(2\times2\) 线性系统即可得到闭式 \(P_U,P_T\)（分母为行列式）。

### 4.4 初始条件（必须保证回到 A）

初始光子在体内各向同性发射，对底面法线的 \(\mu\) 均匀，因此初始处于 \(U\) 的概率应取 **A 极限**：
\[
\boxed{
P(U\ \text{at }t=0)=s_A = 1-\sqrt{1-(n_2/n_1)^2}.
}
\]

因此总体收集效率为：
\[
\boxed{
\varepsilon_{\mathrm{col}}(\sigma_\alpha,p)
= s_A\cdot P_U + (1-s_A)\cdot P_T.
}
\]

> 这一步是“回到 A 特例”的关键：即使后续混合采用朗伯统计，初始状态仍必须是“体内各向同性”对应的 \(s_A\)。

### 4.5 模型 M2：air-gap 逃逸全丢失（新增失败通道）

在模型 M2 中，侧/顶 air-gap 区域的晶体-空气界面允许光子折射逃逸；我们假设一旦逃逸就永久丢失（不回收）。
因此需要一个“逃逸概率” \(e_{\mathrm{eff}}(\sigma_\alpha)\)。

对晶体-空气界面（外侧折射率取 1），临界角对应
\[
\mu_0^{(\mathrm{air})}=\sqrt{1-(1/n_1)^2}.
\]

在本闭式化处理中，逃逸概率同样用 \(q(\sigma_\alpha)\) 在两种入射角统计间插值：

- 体内各向同性（\(\mu\) 均匀）对应
\[
\boxed{
e_A = 1-\mu_0^{(\mathrm{air})}
    = 1-\sqrt{1-(1/n_1)^2}.
}
\]
- 充分混合朗伯（cosine-weighted）对应
\[
\boxed{
e_L = (1/n_1)^2.
}
\]

于是
\[
\boxed{
e_{\mathrm{eff}}(\sigma_\alpha)=(1-q)\,e_A+q\,e_L.
}
\]

对一次墙面相互作用（侧/顶五个面合并为“墙面”）：
- 以概率 \(p\) 命中接触区：存活概率 \(R_c\)，失败（吸收）概率 \(1-R_c\)；
- 以概率 \(1-p\) 命中 air-gap 区：以概率 \(e_{\mathrm{eff}}\) 逃逸并失败；以概率 \(1-e_{\mathrm{eff}}\) 发生 TIR 反射并存活。

因此 **模型 M2 的 wall-survival 概率**为：
\[
\boxed{
A(\sigma_\alpha,p)
 = (1-p_b)\left[p R_c + (1-p)\bigl(1-e_{\mathrm{eff}}(\sigma_\alpha)\bigr)\right].
}
\]

只要在第 4.3 节线性方程组中把 \(A\) 替换为上式，即得到 M2 的闭式曲线。

---

## 5. 极限检验（与 analytic\_check2 对齐）

### 5.1 \(\sigma_\alpha\to 0\)（\(q\to 0\)），且 \(R_{\mathrm{eff}}=1\)

此时墙面不吸收、也不混合，只有初始处于 \(U\) 的光子能成功，困光态永远困光：
\[
\varepsilon_{\mathrm{col}}\to s_A
=1-\sqrt{1-(n_2/n_1)^2},
\]
与 analytic\_check2 A 一致。

### 5.2 \(\sigma_\alpha\to\infty\)（\(q\to 1\)），且 \(R_{\mathrm{eff}}=1\)

无损且每次墙面都会重置状态，最终必然成功：
\[
\varepsilon_{\mathrm{col}}\to 1,
\]
与 analytic\_check2 B 一致。

### 5.3 \(\sigma_\alpha\to\infty\)（\(q\to 1\)），且 \(p=1\Rightarrow R_{\mathrm{eff}}=R_c<1\)

该极限应回到 analytic\_check2 C。  
直观解释：每次“表面相互作用”要么成功（到底面且非 TIR），要么失败（撞墙被吸收），要么继续。其几何级数求和得到
\[
\varepsilon_{\mathrm{col}}\to \frac{p_b s_L}{p_b s_L+(1-p_b)(1-R_c)}.
\]

> 这一点是我们用“混合后取朗伯统计 \(s_L=(n_2/n_1)^2\)”而不是“均匀 \(\mu\)”的原因；否则无法在极限上回到 C 的解析值。

---

## 6. air/grease 两种底部介质

该模型对底面外介质仅通过 \(n_2\) 进入：

- air：\(n_2=1.0\)
- grease：\(n_2=1.46\)

所有公式保持不变，只需替换 \(n_2\)。

---

## 7. 关于 “sideContactRatio = 0 时是否应为 100%？”

在本闭式模型的**理想极限**里：

- 若 \(p=0\Rightarrow R_{\mathrm{eff}}=1\)（墙面无吸收），且 \(\sigma_\alpha\) 足够大使 \(q>0\)（能不断混合），则
  \[
  \varepsilon_{\mathrm{col}}\to 1.
  \]

这并不矛盾：analytic\_check2 B 就是在“无损 + 持续随机化”的理想条件下得到 \(\to 1\)。

真实仿真/实验中之所以可能明显小于 1，通常来自：
- 晶体自吸收（或你故意不做 \(\times 10\) 的弱化）
- 侧/顶存在逃逸到 world 的通道（air-gap 不是完美镜子）
- 有步数截断/时间截断/kill
- 统计口径未闭合或终态分类遗漏

因此：**“p=0 不一定 100%”是工程现实；但在我们为推导而设定的理想无损模型里，它可以趋近 100%。**

补充：在 **模型 M2（air-gap 逃逸全丢失）** 下，\(p=0\) 时墙面全为 air-gap，存在非零逃逸概率 \(e_{\mathrm{eff}}>0\)，因此一般不会趋近 100\%。

---

## 8. 画图（闭式采样，不是蒙卡）

对应脚本：`SiPIN_绝对光产额测量/analysis/closed_form_sigma_p.py`  
（按闭式公式密集采样输出两套模型：M1=air-gap 全回收、M2=air-gap 全丢失，并分别对 air/grease 作图。）

---

## 9. 下一步实验与仿真验证规划（把“闭式两极限”落到可测）

> 目标：最终是把光产额测准，因此我们关心的核心量是总探测效率
> \(\alpha_{\mathrm{SiPIN}}\)。
> 但在进入 \(\alpha\) 之前，应先用下列实验/仿真把几何项 \(\varepsilon_{\mathrm{col}}\) 与“air-gap 回收机制”验收清楚。

### 9.1 基础对照：裸晶体（对应模型 M2：air-gap 逃逸全丢失）

- **实验配置**：晶体不包 PTFE/不加外反射层；侧/顶外侧尽量“黑”（吸光背景），使逃逸光子不被回收。
- **底部两工况**：
  - **air 底部**：晶体底面与 SiPIN 之间不涂 grease（保留空气界面）
  - **grease 底部**：晶体底面涂 grease 与 SiPIN 耦合
- **可观测量**：
  - 若只做相对对照：比较两个工况下的信号幅度比（对应 \(\alpha_{\mathrm{SiPIN}}\) 的变化趋势）
  - 若做绝对：配合电荷刻度与能量沉积求 \(\alpha_{\mathrm{SiPIN}}\)（这一步需要后续引入 Si 表面 \(P_{\mathrm{det}}\) 模型）
- **与闭式对照**：重点验证
  - \(\varepsilon_{\mathrm{col}}\) 在 \(p=0\) 下不再趋近 1；
  - \(\sigma_\alpha\) 增大时是否出现“先提升（打破困光）后受逃逸/损失限制”的行为（可由表面处理改变粗糙度来探测趋势）。

### 9.2 上界对照：强回收封装（对应模型 M1：air-gap 逃逸全回收）

- **实验配置**：侧/顶用高反射材料充分包覆（例如厚 PTFE/ESR 等），尽量提高外部回收率；让“逃逸到 air-gap”的光尽可能回到晶体。
- **目的**：验证“几何上界”趋势，即在足够混合且无显著吸收/截断条件下，\(\varepsilon_{\mathrm{col}}\) 可显著提高并在极限上逼近 1。

### 9.3 侧面接触比例扫描（连接实验工艺与参数 \(p\)）

- **实验/仿真方式**：
  - 通过可控工艺改变侧面贴合程度（或用可替换的包覆结构），形成不同等效 \(p\)
  - 在仿真中可直接扫 \(p\)（并输出逃逸/吸收通道分解）
- **与闭式对照**：比较 \(\varepsilon_{\mathrm{col}}\) 随 \(p\) 的单调性与曲线形状在 M1/M2 下的差异，从而判定真实系统更接近哪一侧（全回收 vs 全丢失）。

### 9.4 表面粗糙度扫描（\(\sigma_\alpha\)）：验证“最优粗糙度/峰值”是否真实

- **背景**：在 M2 下，\(\sigma_\alpha\) 同时会
  - 打破困光（提升到达底面的机会）
  - 增加在 air-gap 界面折射逃逸并被丢弃的机会
  两者竞争可导致 \(\varepsilon_{\mathrm{col}}(\sigma_\alpha)\) 出现峰值（“最优粗糙度”）。
- **验证方式**：
  - 实验：准备不同表面处理（抛光/轻微磨砂/较粗糙）样品或同一晶体分区处理
  - 仿真：扫 \(\sigma_\alpha\) 并输出逃逸通道与底面 hit 统计

### 9.5 从 \(\varepsilon_{\mathrm{col}}\) 过渡到 \(\alpha_{\mathrm{SiPIN}}\)（最终目标）

一旦几何与回收机制被上述对照锁定，我们再进入总探测效率：
\[
\alpha_{\mathrm{SiPIN}}=\varepsilon_{\mathrm{col}}\cdot\langle EQ_{\mathrm{eff}}\rangle.
\]
其中 \(\langle EQ_{\mathrm{eff}}\rangle\) 需要 Si 的光学常数 \(n,k\) 与 Si\(_3\)N\(_4\) 增透层建模（TMM）才能在 air/grease、宽角入射下正确计算。


