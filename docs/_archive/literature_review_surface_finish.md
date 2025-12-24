## Literature review notes: scintillator surface finish / roughness / optical coupling (for PMTLC & SiPINLC)

Last update: 2025-12-21

### 0) Scope & why this matters in *this* project

Your current engineering/physics question is:

- **How crystal surface finish (polished vs rough)** and **packaging interfaces** (side/top airgap vs contact, bottom grease vs airgap) jointly determine:
  - **Light collection** \(\varepsilon_\mathrm{col} = P(\text{hit Si})\)
  - and (later) **effective detection efficiency** \(\alpha_{\text{SiPIN}} = \varepsilon_{\text{col}}\langle EQ_{\text{eff}}\rangle\) with \(P_\mathrm{det}(\lambda,\theta)\).

For the paper, the literature must support:

- Why “polished + airgap” can trap light (waveguiding/TIR modes).
- Why “slightly rough” can break trapping but introduces extra loss (more PTFE hits, absorption accumulation).
- Why “side contact to reflector” is not monotonic better (TIR weakened, more coupling into reflector with <100% reflectance).
- How roughness is *parameterized* in simulation (Geant4 **UNIFIED** model with `sigma_alpha`) and how to map/validate against measurements.
- For long crystals / DOI: why **selective/segmented roughening** (hybrid treatment) can improve position/DOI performance.

### 1) Geant4 surface models: what parameter corresponds to “roughness”?

For a practical and defensible parameter scan, use Geant4 **UNIFIED**:

- **`sigma_alpha` (rad)**: width of the micro-facet normal distribution (effective angular spread per reflection).
- This is **not** Ra (nm). Any mapping Ra → `sigma_alpha` should be justified by measurement/fit (or explicitly stated as a phenomenological parameter scan).

### 2) “Measured reflectance drives simulation” (methodology cornerstone)

These papers are the main methodological support for “don’t guess surfaces; measure reflectance and fit/drive the simulation surface model”:

- **Janecek & Moses (2008)** — measuring reflectance of crystal surfaces (BGO)  
  DOI: `10.1109/TNS.2008.2003253`
- **Janecek & Moses (2010)** — simulating scintillator light collection using measured optical reflectance  
  DOI: `10.1109/TNS.2010.2042731`

How to cite in your writing (Chinese text, figures in English):

- Explain the need for a *measured or calibrated* boundary model; then introduce UNIFIED parameters as a “compact” representation.

### 3) “Roughness vs light collection” (direct trend evidence)

- **Kilimchuk et al. (2010)** — *Study of light collection as a function of scintillator surface roughness*  
  Radiation Measurements 45(3–6), 383–385  
  DOI: `10.1016/j.radmeas.2010.01.027`

This supports the claim: light collection is not necessarily monotonic; there can be an optimum roughness due to the trade-off between de-trapping and added loss.

### 4) DOI / dual-ended readout / “systematic (banded) roughening” (closest to your remembered idea)

These are **very close** to “远端更镜面、近端更粗糙/分区粗糙”的工程思想（在长晶体上用分段粗糙改变光传输，从而改善位置/DOI 解析）：

- **ur-Rehman, Tai, Goertzen (2011, NSS/MIC)** — *Improvement in spatial resolution of dual-ended readout of 100 mm long LYSO crystals through use of systematic crystal surface roughing*  
  Published in: 2011 IEEE Nuclear Science Symposium Conference Record  
  DOI: `10.1109/NSSMIC.2011.6153681`
- **ur-Rehman, Tai, Goertzen (IEEE TNS, 2014 issue / 2013 online)** — *Optical Simulation of Dual-Ended Readout of Axially-Oriented 100 mm Long LYSO Crystals for Use in a Compact PET System*  
  IEEE Transactions on Nuclear Science, Volume 61 Issue 1 (Feb 2014), pp. 3–13  
  DOI: `10.1109/TNS.2013.2283174`
  - Abstract mentions **systematic etched band patterns / roughed surface bands** (patterned/segmented roughness).
 - **ur-Rehman, Tai, Goertzen (Phys. Med. Biol., 2012)** — *Use of systematic surface roughing...*  
  DOI: `10.1088/0031-9155/57/24/N501`  
  - This is a journal version that directly supports the “banded/segmented roughness” narrative.

Recommended “citation narrative” for your paper:

- In long crystals, polished surfaces can guide light with little angular mixing → strong position dependence and/or poor DOI separation.
- Introducing **segmented roughened bands** modulates the transport (mixing/extraction) and improves positioning/DOI metrics.
- This motivates your second-stage design idea (“分区/梯度 sigma_alpha”).

Optional context (not required for your 5 mm cube, but good for framing long-crystal literature):

- (2020, Phys. Med. Biol.) BGO vs LYSO dual-ended DOI performance comparison: `10.1088/1361-6560/abc365`
- (2025, Medical Physics) dual-ended readout + different reflectors: `10.1002/mp.18103`

### 5) A key practical limitation in our current tooling (important for downloads)

In this environment:

- The automated Chrome instance uses a **temporary profile** (see `chrome://version` → “个人资料路径”), not your real `C:\\Users\\GeMaw\\AppData\\Local\\Google\\Chrome\\User Data\\Default`.
- Google Scholar currently times out here (network-level), even when IEEE Xplore works.
- IEEE PDF endpoints are protected (anti-bot), so **automatic PDF downloading is not yet reliable** from this agent.

Workaround:

- I will keep collecting **DOIs + publisher/IEEE pages + abstracts + citation chains**.
- For PDF: you can open each DOI/publisher page in your normal Chrome (with your plugins) and download; we can also build a “download list” file to make this one-click for you.

Additional note (2025-12-21):

- Web of Science can be reached under campus access, but automated searches may trigger **hCaptcha** (“unusual activity from your institution”). When that happens, we switch to **IEEE Xplore citation chains** + **Crossref metadata** for DOI resolution.

### 6) What to do next (for an efficient literature pass)

Suggested next search axes (within IEEE Xplore + NIM A + Medical Physics):

- Keywords:
  - `systematic surface roughing`, `etched band`, `band pattern`
  - `selective roughening`, `partial roughening`, `hybrid surface treatment`
  - `dual-ended readout`, `depth of interaction`, `DOI`, `long crystal`
  - `UNIFIED model`, `sigma alpha`, `micro-facet`

Deliverable target (paper-ready):

- A small table “**Surface model / Roughness parameter / Coupling condition / Outcome metric**”
- A paragraph that connects:
  - (i) TIR + polished surfaces → trapping/waveguiding
  - (ii) roughness breaks trapping but increases boundary hits and loss
  - (iii) segmented roughness can optimize transport in long crystals (DOI)


