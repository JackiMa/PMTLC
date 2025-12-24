# 闭式理论参考 + Geant4 对比说明（给做仿真的同事）

> 目的：用 `docs/analytic_check2.md` 的 6 个特例（A/B/C × air/grease）为锚点，构造一个**可闭式计算**的理想模型  
> \(\varepsilon_{\mathrm{col}}(\sigma_\alpha,p)\)，并给出两种可检验极限：  
> **M1：air-gap 逃逸全回收（上界）**、**M2：air-gap 逃逸全丢失（下界/裸晶体）**。  
> 你们用 Geant4 做 \((\sigma_\alpha,p)\) 扫描后，可以直接与本文给出的曲线/表格对照，判断仿真是否落在合理区间，并定位偏差来源。

---

## 0) 对照量与总目标（不要混淆）

- 本文这里对照的是几何量：
\[
\boxed{\varepsilon_{\mathrm{col}}\equiv P(\text{photon hits Si})}
\]
即“光子至少一次到达/撞击 Si（或 SiPIN 有效面）”的概率。

- 最终我们真正关心的是光产额标定用到的总探测效率：
\[
\boxed{\alpha_{\mathrm{SiPIN}}=\varepsilon_{\mathrm{col}}\cdot \langle EQ_{\mathrm{eff}}\rangle}
\]
其中 \(\langle EQ_{\mathrm{eff}}\rangle\) 需要 Si 的 \(n,k\)、Si\(_3\)N\(_4\) 增透层、TMM 与入射角分布等。  
本文件只处理 \(\varepsilon_{\mathrm{col}}\) 的“几何与边界”部分，用于 Gate/对照与理解趋势。

---

## 1) 固定输入与符号

- 晶体折射率：\(n_1=1.86\)
- 底面外介质（两种）：
  - air：\(n_2=1.0\)
  - grease：\(n_2=1.46\)
- 侧/顶贴合 PTFE-like 区域反射率：\(R_c=0.975\)
- 侧面贴合比例：\(p\in[0,1]\)（sideContactRatio）
- 立方体 mean-field：底面命中概率 \(p_b=1/6\)，墙面（侧+顶）命中概率 \(1-p_b=5/6\)
- 粗糙度参数：\(\sigma_\alpha\)（rad）；在闭式模型中只通过“混合概率” \(q(\sigma_\alpha)\) 进入

定义：
\[
\mu=\cos\theta,\qquad
\mu_0(n_1,n_2)=\sqrt{1-(n_2/n_1)^2}.
\]

底面“可出射集合”概率：
- 体内各向同性（对应 analytic\_check2-A）：
\[
s_A(n_2)=1-\mu_0=1-\sqrt{1-(n_2/n_1)^2}.
\]
- 充分混合朗伯（对应 analytic\_check2-B/C）：
\[
s_L(n_2)=\left(\frac{n_2}{n_1}\right)^2.
\]

混合概率（无新参数、用晶体-空气临界角作尺度）：
\[
\boxed{
q(\sigma_\alpha)=1-\exp\!\left[-\left(\frac{\sigma_\alpha}{\theta_c}\right)^2\right]},\quad
\theta_c=\arcsin(1/n_1).
\]

---

## 2) 两套 air-gap 处理假设（两套理论曲线）

### M1：air-gap 逃逸全回收（上界）

air-gap 区域视为“无损回收”，等效墙面反射率：
\[
R_{\mathrm{eff}}(p)= (1-p)\cdot 1 + p\cdot R_c = 1-p(1-R_c).
\]
墙面存活概率（一次墙面相互作用后仍留在晶体内）：
\[
A = (1-p_b)\,R_{\mathrm{eff}}(p).
\]

### M2：air-gap 逃逸全丢失（下界/裸晶体）

air-gap 区域晶体-空气界面允许折射逃逸；一旦逃逸就丢失。  
定义晶体-空气界面逃逸概率（插值）：
\[
e_A = 1-\sqrt{1-(1/n_1)^2},\qquad
e_L = (1/n_1)^2,
\]
\[
e_{\mathrm{eff}}(\sigma_\alpha)=(1-q)\,e_A+q\,e_L.
\]

墙面存活概率：
\[
\boxed{
A(\sigma_\alpha,p)=(1-p_b)\left[pR_c+(1-p)\bigl(1-e_{\mathrm{eff}}(\sigma_\alpha)\bigr)\right].}
\]

---

## 3) 闭式求解：两态马尔可夫链（U/T）

状态：
- \(U\)：下次到底面为非 TIR（到底面即成功）
- \(T\)：下次到底面为 TIR（到底面不成功）

令 \(P_U,P_T\) 为从对应状态出发最终成功的概率。  
墙面存活概率用上节的 \(A\)（M1）或 \(A(\sigma,p)\)（M2）。

线性方程组：
\[
\begin{aligned}
\Big[1-A(1-q+qs_L)\Big]P_U - A q(1-s_L)P_T &= p_b,\\
-A q s_L P_U + \Big[1-p_b-A(1-q s_L)\Big]P_T &= 0.
\end{aligned}
\]

初始条件必须匹配 analytic\_check2-A（体内各向同性）：
\[
P(U\text{ at }t=0)=s_A,\quad P(T)=1-s_A.
\]

最终：
\[
\boxed{\varepsilon_{\mathrm{col}}= s_A P_U + (1-s_A)P_T.}
\]

---

## 4) 极限回归（对齐 analytic\_check2 的 6 个特例）

- \(\sigma\to0\)（\(q\to0\)）+ 无损：\(\varepsilon_{\mathrm{col}}\to s_A\)（A 场景）
- \(\sigma\to\infty\)（\(q\to1\)）+ 无损：\(\varepsilon_{\mathrm{col}}\to 1\)（B 场景）
- \(\sigma\to\infty\)（\(q\to1\)）+ \(p=1,R_c<1\)：\(\varepsilon_{\mathrm{col}}\to \frac{p_b s_L}{p_b s_L+(1-p_b)(1-R_c)}\)（C 场景）
- M2 下 \(p=0\) 不会趋近 1（因为存在逃逸丢失）

---

## 5) 数值参考（你们跑完可以直接对照）

以下取 \(n_1=1.86\)、\(R_c=0.975\)、\(p_b=1/6\)、固定 \(p=0.8\)，列出若干 \(\sigma_\alpha\) 点的 \(\varepsilon_{\mathrm{col}}\)：

### grease（\(n_2=1.46\)）

- **M1（collect\_all，上界）**
  - \(\sigma=\) 0.0 / 0.1 / 0.2 / 0.3 / 0.4 / 0.469 / 0.6 / 1.0  
  - \(\varepsilon_{\mathrm{col}}=\) 0.3459 / 0.5998 / 0.7515 / 0.8017 / 0.8222 / 0.8297 / 0.8375 / 0.8440

- **M2（discard\_escaped，下界）**
  - \(\sigma=\) 0.0 / 0.1 / 0.2 / 0.3 / 0.4 / 0.469 / 0.6 / 1.0  
  - \(\varepsilon_{\mathrm{col}}=\) 0.3027 / 0.4134 / 0.5302 / 0.5789 / 0.5941 / 0.5961 / 0.5926 / 0.5799  
  - **注意：此曲线在 \(\sigma\approx0.469\) 达到峰值**（“最优粗糙度”来自“困光打破”与“逃逸丢失”竞争）。

### air（\(n_2=1.0\)）

- **M1（collect\_all，上界）**
  - \(\sigma=\) 0.0 / 0.1 / 0.2 / 0.3 / 0.4 / 0.469 / 0.6 / 1.0  
  - \(\varepsilon_{\mathrm{col}}=\) 0.1426 / 0.3504 / 0.5444 / 0.6280 / 0.6658 / 0.6803 / 0.6955 / 0.7088

- **M2（discard\_escaped，下界）**
  - \(\sigma=\) 0.0 / 0.1 / 0.2 / 0.3 / 0.4 / 0.469 / 0.6 / 1.0  
  - \(\varepsilon_{\mathrm{col}}=\) 0.1248 / 0.2026 / 0.3082 / 0.3640 / 0.3858 / 0.3909 / 0.3911 / 0.3820

---

## 6) 需要 Geant4 输出哪些量（最小集）

### 6.1 必须输出（用于直接对比曲线）

对每个 \((\sigma_\alpha,p)\) 点、每种底部介质（air/grease）、每种外部回收配置（对应 M1/M2 的两套几何设置）：

- **\(\varepsilon_{\mathrm{col}}=\) hitSi / N**（与你们当前统计一致）
- **终态通道分解**（至少区分）：
  - hitSi
  - Escaped\_SideAir / Escaped\_TopAir（air-gap 逃逸）
  - Escaped\_PTFE（贴合区吸收）
  - CrystalAbsorbed / OtherKill（若存在）

### 6.2 强烈建议输出（用于解释偏差/做半解析拟合）

- **底面命中频率**：每个光子平均到底面次数、或“表面交互中落在底面的比例”  
  用于检验 mean-field \(p_b=1/6\) 是否成立。
- **air-gap 逃逸概率的统计**：在侧/顶 air-gap 区，发生折射逃逸的比例随 \(\sigma\) 的变化  
  用于检验/拟合 \(e_{\mathrm{eff}}(\sigma)\)。
- **撞墙次数分布 \(P(N_{\text{wall}})\)**：用于判断是否有“截断/kill”影响。

---

## 7) 怎么和理论对比（一步一步）

1. **先做 Gate：统计闭合**  
   必须保证各终态计数求和等于 N，否则不要谈曲线对照。

2. **两条极限夹逼**  
   同一 \((\sigma,p)\) 点的 Geant4 \(\varepsilon_{\mathrm{col}}\) 应落在理论 M2（下界）与 M1（上界）之间。  
   若明显超出：说明仿真中还有额外损失/额外回收机制未在理论里表达，或统计口径不一致。

3. **看“峰值是否存在”来判断系统更接近 M1 还是 M2**  
   - 若看到明显“最优粗糙度/峰值”：系统更接近 M2（逃逸损失不可忽略）。  
   - 若基本单调上升并趋近较高值：系统更接近 M1（外部回收更强）。

4. **半解析改进（可选）**  
   若 Geant4 输出了 \(p_b(\sigma,p)\) 与 escape fraction，可把它们替换进闭式 \(A(\sigma,p)\)，得到更贴近仿真的“半解析曲线”，但仍保留闭式结构与可解释性。

---

## 8) 现成理论曲线文件位置

闭式脚本会输出四类图（air/grease × M1/M2），位置：

`/home/wsl2/myGeant4/20251220/PMTLC/results/ideal_theory_sigma_p/`

文件名中：
- `collect_all` = M1（上界）
- `discard_escaped` = M2（下界）


