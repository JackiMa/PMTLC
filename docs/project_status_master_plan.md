## PMTLC / SiPINLC：现状梳理与后续计划（写论文用的总纲文档）

更新时间：2025-12-21  
维护原则：**这份文档是“唯一权威总纲”**；更细节的过程日志写在 `docs/debug_log.md`，设计细节写在 `docs/*_design.md`。

---

### 0) 你现在在讨论什么（当前讨论主题一览）

你当前的讨论围绕同一条主线：**晶体表面处理（镜面/粗糙）与封装界面（空气层/贴合/grease）如何共同决定光收集与最终探测效率**，并把这套物理讨论落实到可复现的 Geant4 仿真与论文写作中。

具体讨论点包括：

- **“镜面 + 空气层”导致困光/像光纤**：高折射率晶体（GAGG）→空气界面临界角小，镜面反射保持角度，容易形成长寿命导波轨迹，导致“各向同性发射≠均匀到达读出面”。
- **“轻微粗糙”打破困光但增加损耗通道**：粗糙让角度逐步随机化，更容易最终到达底面；但也可能增加与 PTFE/边界的碰撞次数，从而累计吸收/漏光，往往存在最优粗糙度。
- **“侧面贴合 PTFE / 无空气层”并非单调更好**：贴合改变界面折射率条件（TIR 减弱），更多光耦合到包覆中；PTFE 高反射但非 100%，多次散射会累积损耗，效果需要仿真+实验约束。
- **论文要写清的“损耗通道”**：晶体吸收、PTFE 吸收、顶/侧空气层逃逸、其他漏光等（工程输出中已有对应统计列）。
- **工程核心**：用 Geant4 得到 \(\varepsilon_{\text{col}}\) 与 \(\langle EQ_{\text{eff}}\rangle\)（含多次撞击增强），最终用于 SiPIN 绝对光产额校正论文（`SiPIN_绝对光产额测量/latex/content.tex`）。

> 讨论要点原始素材：  
> - `SiPIN_绝对光产额测量/drafts/表面处理_镜面vs粗糙_讨论要点.md`  
> - `docs/surface_finish_light_transport.md`

---

### 1) 项目最终目标（你交给我的“交付物”）

**总目标**（见 `docs/current_state.md`, `docs/roadmap_requirements_and_git.md`）：

- 用 Geant4 计算 SiPINLC 的总探测效率（可拆为）
  - **光收集效率**：\(\varepsilon_{\text{col}} = P(\text{hit Si})\)
  - **有效量子效率**：\(\langle EQ_{\text{eff}}\rangle = P(\text{Detected} \mid \text{hit Si})\)
- 在 **PTFE 包覆 + 顶/侧空气层 + 底部 grease 耦合**的实验几何下，得到并解释“**90%+（最终给出 90 多少）**”的结果。
- 输出可用于论文的曲线/图表：粗糙度扫描、贴合比例扫描、grease 厚度扫描、自吸收校正曲线、入射角/撞击次数/损耗通道分解等。

论文目标（`SiPIN_绝对光产额测量/latex/content.tex`）：

- 完成“理论框架 + Geant4 方法学 + 系统不确定度 + 应用到 GAGG 测量”的闭环写作，核心是把仿真从“几何到达率”提升到“带角度/波长依赖响应的真实探测概率”。

---

### 2) 当前现状（已完成 / 已证伪 / 已确认的工程事实）

#### 2.1 已完成（功能与基础设施）

来自 `docs/current_state.md` 与 `docs/status_and_debug_notes.md` 的一致结论：

- **移除 SiPIN window**：界面简化为 `grease(or airgap) → Si`，符合论文建模需求。
- **P_det hook 已跑通（至少到接口层面）**：
  - `pdetMode=1`（常数）、`pdetMode=2`（CSV 查表）逻辑已具备；
  - 判决规则：`U<p_det` 则“探测到”并 kill；否则反射继续传播，且设有 `maxInterfaceHits` 防死循环。
- **调试源/验收手段已具备**：
  - primary optical photon（单色）用于几何/边界 sanity check；
  - 基准仍可回到 “662 keV 电子激发闪烁谱”。
- **统计量与输出（用于论文）已扩展**：
  - 入射角分布、撞击次数、损耗通道、事件光子数、晶体内路径长度（自吸收相关）等；
  - `run_data.csv` 已扩展列，便于参数扫描后分析。

#### 2.2 已确认的关键修复结论（非常重要）

来自 `docs/roadmap_requirements_and_git.md` 与 `docs/surface_finish_light_transport.md`：

- 修复了会破坏边界拓扑的实现细节：**`sc_gap` 不应缩小 `gap_eps`**，否则会形成“世界空气缝隙”，导致 PTFE 反射与底部界面逻辑失效。
- 在 `pdetMode=0` 的纯几何到达率验收中（已恢复趋势）：
  - 无 grease（底部 10 µm 空气层）：`Photon Hit si ≈ 0.3391`
  - 有 grease（50 µm）：`Photon Hit si ≈ 0.5467`
  - 结论：**底部通道与“有 grease > 无 grease”趋势已恢复**。

#### 2.3 当前最核心、最高优先级的问题（仍未完全收敛）

来自 `docs/current_state.md` 与 `docs/debug_log.md` 的最新定位：

- **问题 A（最高优先级）**：PTFE wrapper 的 **skin surface** 作用范围可能仍在某些几何组织方式下“误封底部通道”，使光子难以真正进入 `sipin_si`（历史上无 grease 可能达到 ~80% 的线索很关键）。
  - 典型根因模式：`sc_gap` 被建为 wrapper 子体积时，光子离开 `sc_gap` 底面接触到 wrapper 的底面 skin surface，从而被“当成 PTFE 反射回去”。
  - 修复方向：wrapper 必须是“侧壁+顶盖、底部开口”的壳体（`G4SubtractionSolid`），或至少保证底面不受 skin surface 影响。
- **问题 B**：`g_side_contact_ratio`（侧面贴合比例）尚未真正进入几何/光学逻辑，导致“贴合比例扫描”无法推进到论文需要的曲线层面。
- **问题 C（论文关键但可后置）**：`P_det(λ,θ)` 的集成需要做到“边界过程层面一致”，避免与 `G4OpBoundaryProcess` 重复生效导致方向/计数被二次修改（论文 4.3 节也强调了这一点）。

---

### 3) 当前工作目标的“最小闭环”（近期验收标准）

为了避免同时改太多导致无法定位，近期所有工作按“最小闭环”推进（见 `docs/current_state.md` 的验收计划）：

- **验收 1：无 grease（纯几何）**
  - 用各向同性 primary optical photons；
  - 指标：`Photon Hit si / N` 应回到“合理高”的量级，并与历史版本一致性更高。
- **验收 2：有 grease（纯几何）**
  - 同样条件下，有 grease 应显著高于无 grease（临界角增大）。
- **验收 3：P_det hook 回归（功能正确性）**
  - `pdetMode=1`：p=0/0.5/1 的结果应可预期；
  - `pdetMode=2`：接入真实 CSV 表后做小统计（例如 1e4）只看趋势与稳定性，不追求最终数值。
- **验收 4：切回闪烁基准源**
  - 在几何与边界逻辑正确的前提下，再推进到 662 keV e- 的完整统计与“90%+”目标。

---

### 4) 未来要做什么（计划清单：代码/仿真/文献/写作）

> 本节把“要做什么”拆成可执行的任务列表；每个任务都要有：预期 → 实验/修改 → 验收数据 → 记录到 `docs/debug_log.md` → git commit。

#### 4.1 需要调试/修复的 bug（按优先级）

- **P0：PTFE 几何/skin surface 误封底部通道的风险彻底消除**
  - 目标：wrapper 真正只覆盖侧面+顶面，底部完全开口；
  - 验收：无 grease 的各向同性几何到达率回归到历史合理值；并且“有 grease > 无 grease”趋势稳健。
- **P1：逃逸通道归类的正确性**
  - `Escaped_CrystalAbsorbed` 等通道必须严格对应“物理吸收/出界”等原因，避免把“人为 kill（如探测判决）”误归类为晶体吸收（`docs/debug_log.md` 曾提示过这个风险）。
- **P1：边界判决与 Geant4 默认边界过程的去重**
  - 避免同一步同时被自定义逻辑与 `G4OpBoundaryProcess` 修改，从而出现重复反射或重复计数。

#### 4.2 需要完善的功能（为了论文必须具备）

- **F1：粗糙度参数化与扫描（B 类）**
  - 采用 UNIFIED 的 `sigma_alpha` 表达“角度扩散强度”（见 `docs/roughness_scan_design.md`）；
  - 输出曲线：`sigma_alpha → ε_col`、损耗通道分解、入射角/撞击次数变化；
  - 第二阶段扩展：分区/梯度粗糙（远端镜面、近端粗糙）——更面向长条晶体。
- **F2：侧面贴合比例扫描（C 类/贴合比例）**
  - 先用“统计等效法”两极限（全贴合 vs 全空气层）跑通，再按比例加权；
  - 或实现概率边界法（更工程化，但风险更高）。
- **F3：SiPIN 角度-波长依赖响应（`P_det(λ,θ)`）的可复现接入**
  - 需要：CSV 规范、插值、clamp、版本管理；
  - 同时要能输出：\(\langle EQ_{\text{eff}}\rangle\)、多次撞击增强后的总探测概率统计（与 `content.tex` 第 2.3 节一致）。

#### 4.3 需要做的 Geant4 实验（按阶段）

- **E0（快速验收）**：primary optical photon（单色，iso），只看几何与边界的方向性趋势。
- **E1（几何扫描）**：grease 厚度 / 顶面 airgap / 侧面 airgap / 侧面贴合（极限）在 `pdetMode=0` 下出 \(\varepsilon_{\text{col}}\)。
- **E2（响应接入）**：引入 `P_det(λ,θ)` 后输出总探测效率 \(\alpha_{\text{SiPIN}} = \varepsilon_{\text{col}}\langle EQ_{\text{eff}}\rangle\)。
- **E3（不确定度评估）**：按论文 6 节框架做参数敏感性（PTFE 反射率、贴合比例、吸收长度缩放、grease n 与厚度、粗糙度、AR 薄膜拟合误差等）。

#### 4.4 需要调研/补齐的文献（写综述/讨论必须）

已确认主线（见 `docs/surface_finish_light_transport.md` 与 `SiPIN_绝对光产额测量/ref/auto/README.md`）：

- Janecek & Moses（2008/2010）：测量表面反射并驱动仿真（与 UNIFIED/GLISUR 强相关）
- Kilimchuk et al.（2010）：粗糙度 vs 光收集
- Bircher & Shao（2012）：LYSO 表面 finish/几何对 SiPM 双端 DOI 的影响
- ur‑Rehman et al.（2011, NSS/MIC）：100 mm 长晶体 + systematic surface roughing（非常接近“分区/系统性粗糙”的线索）

仍需定位（优先级高）：

- **你记忆中的那篇：远离读出端更镜面、靠近读出端更粗糙**（分区/选择性粗糙、hybrid surface treatment）。  
  建议关键词：`partial roughening`, `selective roughening`, `hybrid surface treatment`, `dual-end readout`, `DOI`, `long crystal`.

#### 4.5 论文写作边界（`content.tex`：写什么 / 不写什么）

你的要求是：论文重点是 **(1) 建立正确的模拟程序框架**，以及 **(2) 用该框架对感兴趣问题做仿真实验**。因此写作建议如下：

- **不写（或只一句带过）**：
  - 我们在实现/调试过程中遇到的具体 bug、回归、临时 workaround、以及排查过程细节。
  - 这些内容只保留在仓库的 `docs/`（例如 `docs/debug_log.md`）用于复现与维护，不进入论文主体。

- **必须写清（论文主体要交代清楚）**：
  - **框架定义与统计口径**：\(\varepsilon_{\text{col}}\)、\(\langle EQ_{\text{eff}}\rangle\)、\(\alpha_{\text{SiPIN}}\) 的定义，以及仿真中如何统计得到（对应到输出量）。
  - **几何与边界模型**：顶/侧空气层、底部 grease/薄空气层、PTFE 覆盖范围与表面模型（UNIFIED/粗糙度参数化），以及侧面“贴合比例”的建模方法（统计等效或概率边界）。
  - **SiPIN 响应模型**：\(P_{\text{det}}(\lambda,\theta)\) 的来源（TMM 预计算）与在 Geant4 中的使用方式（在线查表判决），并明确多次撞击增强已自然包含在 Monte Carlo 里。
  - **验证思路（只写原则，不写踩坑）**：用简单可控 case 做 sanity check（例如 p=0/1，或几何极限），证明框架“按物理预期工作”。

- **Results/Experiments（论文真正要展示的）**：
  - 你关心的扫描结果（至少包含）：
    - **表面粗糙度**（`sigma_alpha` 或等效参数）扫描；
    - **侧面贴合比例**扫描；
    - **grease 厚度**扫描；
    - **有效衰减率 / 自吸收强度**（吸收长度缩放或等效吸收长度）扫描；
  - 每类扫描都要配套：
    - 总指标：\(\varepsilon_{\text{col}}\)、\(\langle EQ_{\text{eff}}\rangle\)、\(\alpha_{\text{SiPIN}}\)（看论文章节需要选择展示哪个为主）；
    - 解释性统计：入射角分布、撞击次数分布、损耗通道分解（用于解释趋势，而不是解释 bug）。


---

### 5) 你对我的要求（协作约定，写进文档以便长期执行）

你明确要求我（以及我们）：

- **多写笔记，多写文档**：
  - 每次只验证一个假设：改动 → 预期 → 跑数 → 结论；
  - 结论必须写进 `docs/debug_log.md` 或本总纲，并附关键数值与运行条件；
  - 重要讨论（镜面/粗糙/贴合的物理解释与审稿回应话术）要沉淀到 `docs/surface_finish_light_transport.md` 与 `SiPIN_绝对光产额测量/drafts/`。
- **多用 git 管理代码**：
  - 每个闭环都要 commit；
  - commit message 必须包含：**改了什么 / 为什么改 / 验证结果**；
  - 大改动走分支，合并时写清“最终得到什么可复现结果”。

补充说明（与版本管理相关）：

- `SiPIN_绝对光产额测量/...` 目录在你的环境里可能是符号链接到外部位置；若出现“Git 无法跟踪链接外部文件”的情况，建议在仓库内另建可追踪镜像目录（例如 `research/`）并双写关键文档/文献条目，保证可回溯性。

---

### 6) 文档索引（你要快速回顾时看什么）

- **总纲（这份）**：`docs/project_status_master_plan.md`
- **阶段报告（现状总结）**：`docs/current_state.md`
- **调试过程日志（按时间线）**：`docs/debug_log.md`
- **需求与 git 约定**：`docs/roadmap_requirements_and_git.md`
- **粗糙度扫描设计（B 类）**：`docs/roughness_scan_design.md`
- **参数扫描设计（更细节）**：`docs/parameter_scan_design.md`
- **物理讨论与论文话术底稿**：`docs/surface_finish_light_transport.md`、`SiPIN_绝对光产额测量/drafts/表面处理_镜面vs粗糙_讨论要点.md`


---

### 7) 本周里程碑（把计划落到“每天做什么/产出什么”）

> 目标：把工程从“几何到达率验收”推进到“可写入论文的最终结果图表”。  
> 原则：每天只推进 1–2 个闭环，每个闭环都要：运行条件 + 数值结果 + 结论 + commit + 记录到 `docs/debug_log.md`。

#### Day 1：几何与边界拓扑最终验收（P0）

- **要做的事**：
  - 确认 PTFE wrapper 只覆盖侧面+顶面，底部完全开口；skin surface 不影响底部通道。
  - 用 primary optical photon（iso）重复验收：无 grease / 有 grease。
- **验收输出**：
  - `Photon Hit si`（无 grease、有 grease）两点数值 + 运行宏/参数（可复现）。
  - 逃逸通道占比的 sanity check（别把人为 kill 算成晶体吸收）。

#### Day 2：贴合比例与粗糙度的“最小可用扫描”（F1/F2 的第一步）

- **要做的事**：
  - 贴合比例：至少跑通两极限（全空气层 vs 全贴合），先在 `pdetMode=0` 下输出 \(\varepsilon_{\text{col}}\)。
  - 粗糙度：按 `sigma_alpha ∈ {0, 0.1, 0.3}` 做三点扫描（先出趋势，再决定是否加密）。
- **验收输出**：
  - 两张最小曲线（3 点也可以）：`sigma_alpha → Photon Hit si`；`contact_mode → Photon Hit si`。
  - 同时输出损耗通道分解（便于解释趋势）。

> 进展记录（已完成）：我们已验证“晶体表面微粗糙（UNIFIED `sigma_alpha`）能显著打破困光并提升几何到达率”。  
> 在 no-grease（bottomAirGap=10 µm）条件下：`sigma_alpha=0` 时 \(\varepsilon_{\text{col}}\approx 0.339\)，`sigma_alpha=0.1 rad` 时 \(\varepsilon_{\text{col}}\approx 0.84\)。  
> 这一步已写入 `docs/debug_log.md`，后续贴合比例扫描以 `sigma_alpha≈0.1 rad` 作为更现实的基线。

#### Day 3：`P_det(λ,θ)` 接入回归（F3 的功能正确性）

- **要做的事**：
  - 用合成表/常数模式先验证统计正确性：`p=0/0.5/1`。
  - 再切换到真实 CSV 表做小统计（1e4 级别）看趋势与稳定性。
- **验收输出**：
  - 输出：\(\langle EQ_{\text{eff}}\rangle\) 的定义口径（统计方式）与一个基准数值（哪怕只是“数量级合理”）。
  - 证明“不会重复反射/重复计数”（去重策略写清楚）。

#### Day 4：回到 662 keV e- 基准源，给出“90%+ 的第一版数值”

- **要做的事**：
  - 固定一套“论文基准几何”：顶面 airgap、侧面 airgap、grease 厚度、PTFE 反射率。
  - 开启 `P_det(λ,θ)` 后给出总探测效率 \(\alpha_{\text{SiPIN}}\)。
- **验收输出**：
  - 一份“基准配置结果卡片”：\(\varepsilon_{\text{col}}\)、\(\langle EQ_{\text{eff}}\rangle\)、\(\alpha_{\text{SiPIN}}\) 与误差条（至少统计误差）。

#### Day 5：不确定度与论文出图清单对齐（E3 + 写作推进）

- **要做的事**：
  - 选 3–5 个主导参数做小范围扰动，生成 Tornado 图所需数据。
  - 同步把 `content.tex` 中的 `XX` 空位替换为“我们已经有数据/图的占位引用”（先占坑，后续补最终数值）。
- **验收输出**：
  - Tornado 图数据表（CSV）+ 论文里能引用的图编号/表编号草案。

---

### 8) 最终交付物清单（论文需要的图/表/数据）

> 约定：**图用英文标注（plt in English）**，正文叙述为中文；所有图的输入数据文件必须可追溯到一次或一组 Geant4 运行。

#### 8.1 图（Figures）

- **Fig-A：几何示意图**
  - 内容：晶体、顶/侧 airgap、PTFE 包覆、底部 grease/薄空气层、SiPIN 的相对位置（强调“PTFE 底部开口”）。
- **Fig-B：入射角分布 \(P(θ)\)**（到达 SiPIN 的光子）
- **Fig-C：撞击次数分布 \(P(N_{\text{hit}})\)**（多次撞击增强的证据）
- **Fig-D：损耗通道分解**（bar/pie：PTFE 吸收、顶面逃逸、侧面逃逸、晶体吸收、其他）
- **Fig-E：grease 厚度扫描曲线**（\(\varepsilon_{\text{col}}\)、\(\alpha_{\text{SiPIN}}\) 至少其一）
- **Fig-F：侧面贴合比例扫描曲线**（用统计等效法或概率边界法）
- **Fig-G：粗糙度扫描曲线**（`sigma_alpha` vs 指标 + 损耗通道变化）
- **Fig-H：SiPIN 响应：空气入射 vs grease 入射（正入射）**（由 TMM 得到的对比曲线）
- **Fig-I：SiPIN 响应：角度依赖 \(P_{\text{det}}(λ,θ)\)**（二维热图或多条角度曲线）
- **Fig-J：不确定度 Tornado 图**

#### 8.2 表（Tables）

- **Tab-1：基准配置参数表**（几何、材料、反射率、粗糙度、贴合比例等）
- **Tab-2：典型封装配置对比表**（几种配置的 \(\varepsilon_{\text{col}}\)、\(\alpha_{\text{SiPIN}}\)）
- **Tab-3：自吸收校正查找表**（若做了吸收长度扫描）
- **Tab-4：系统不确定度预算表**（对应 `content.tex` 第 6 节模板）

#### 8.3 数据文件（建议的落盘规范）

- **Geant4 原始汇总**：每次运行输出的 `run_data.csv`（建议按运行目录归档，别覆盖）
- **扫描汇总表**：`results/scans/*.csv`
  - 示例：`results/scans/scan_grease_thickness.csv`
  - 每行包含：参数值 + \(\varepsilon_{\text{col}}\) + \(\langle EQ_{\text{eff}}\rangle\) + \(\alpha_{\text{SiPIN}}\) + 通道分解 + 统计误差
- **作图输入表**：与论文图一一对应的 `results/fig_data/*.csv`
- **SiPIN 二维表**：`analysis/out/sipin_pdet_*.csv`（写清 n0、膜厚拟合版本、网格步长）

---

### 9) 运行与命名规范（避免“跑了但找不到是哪次跑的”）

#### 9.1 运行目录建议

建议每次跑把输出放到独立目录（哪怕只是拷贝 `run_data.csv`），例如：

- `results/runs/2025-12-21T1530_iso550nm_pdet0_grease50um/`
- `results/runs/2025-12-22T1010_scint662keV_pdet2_baseConfig/`

目录内至少包含：

- `run_data.csv`
- `run.mac`（或等效记录：实际使用的宏/关键参数）
- `notes.md`（一段话：预期/结果/结论/下一步）

#### 9.2 扫描的 run_id 规则

在 `run_data.csv` 里建议增加/确保至少有：

- `run_id`：可读字符串（包含扫描参数）
- `seed`：随机数种子或种子策略标识
- `n_events`：事件数
- `pdet_mode`：0/1/2

---

### 10) Git 工作流模板（你要求的“多用 git 管理”落地）

#### 10.1 分支约定

- **主线**：`main`（只放“已验收通过”的闭环结果）
- **开发**：`wip/<topic>`（例如 `wip/pdet-table`, `wip/ptfe-wrapper-open-bottom`）
- **实验**：`exp/<scan>`（例如 `exp/roughness-scan-v1`，跑完即合并或归档）

#### 10.2 Commit message 模板（强制包含验证结果）

建议格式：

- `fix(geometry): open wrapper bottom to avoid PTFE skin blocking`
  - Why: `sc_gap as child of wrapper triggers skin surface on bottom`
  - Verify: `iso opticalphoton: hitSi 0.34→0.78 (no grease), 0.55→0.90 (grease50um)`（举例，填真实数）

#### 10.3 每次闭环必写的最小记录

每次闭环要在 `docs/debug_log.md` 追加一段，包含：

- **假设**：我在验证什么
- **改动**：改了哪些文件/参数
- **运行条件**：源、事件数、关键几何参数、pdetMode
- **结果**：关键数值（至少 2–3 个）
- **结论**：下一步做什么


