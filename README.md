## 统计信息
### 获取的统计信息
#### Energy Spectrum
- Source Spectrum: 放射源抽样的能谱
- PassingEnergy: 从上到下穿透所有屏蔽层的总能谱，包含初级粒子和次级粒子
- Edep in Crystal: 在闪烁体中沉积能量的总能谱
#### Shield_layer_#
- energyDeposit: 在当前层中沉积的总能量
- PassingEnergy: 从上到下穿透该层的总能量
- HEphotonEnergy: 该层产生，并且离开该层的总次级 xray/gamma 能谱
- NeutronEnergy: 该层产生，并且离开该层的总次级 中子 能谱
说明：PassingEnergy可能小于HEphotonEnergy，因为并不一定所有的次级射线都从下表面离开
#### Spectrum
- ScintillationWavelength: 闪烁体中产生的闪烁光光谱
- CherenkovLightWavelength: 闪烁体中产生的切伦科夫光光谱
- FiberNumericalAperturelength: 从闪烁体进入到光纤数值孔径的光谱
- ScintillationWavelength: 从闪烁体进入到光纤的光谱
#### SourcePosition
- 所抽样的放射源位置分布
