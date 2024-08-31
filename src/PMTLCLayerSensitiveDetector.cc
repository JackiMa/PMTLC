// #include "PMTLCLayerSensitiveDetector.hh"

// LayerSensitiveDetector::LayerSensitiveDetector(const G4String& name, const G4String& hitsCollectionName)
//     : G4VSensitiveDetector(name), fHitsCollection(nullptr) {
//     auto analysisManager = G4AnalysisManager::Instance();

//     fEnergyNtupleId = analysisManager->CreateNtuple(name + "_EnergySpectrum", "Energy Spectrum");
//     analysisManager->CreateNtupleDColumn(fEnergyNtupleId, "Energy");
//     analysisManager->FinishNtuple(fEnergyNtupleId);

//     fNeutronNtupleId = analysisManager->CreateNtuple(name + "_NeutronSpectrum", "Neutron Spectrum");
//     analysisManager->CreateNtupleDColumn(fNeutronNtupleId, "Energy");
//     analysisManager->FinishNtuple(fNeutronNtupleId);

//     fElectronNtupleId = analysisManager->CreateNtuple(name + "_ElectronSpectrum", "Electron Spectrum");
//     analysisManager->CreateNtupleDColumn(fElectronNtupleId, "Energy");
//     analysisManager->FinishNtuple(fElectronNtupleId);
// }

// LayerSensitiveDetector::~LayerSensitiveDetector() {}

// void LayerSensitiveDetector::Initialize(G4HCofThisEvent* hce) {
//     fHitsCollection = new G4THitsMap<G4double>(SensitiveDetectorName, "HitsCollection");
//     G4int hcID = GetCollectionID(0);
//     hce->AddHitsCollection(hcID, fHitsCollection);
// }

// G4bool LayerSensitiveDetector::ProcessHits(G4Step* aStep, G4TouchableHistory*)
// {
//     auto analysisManager = G4AnalysisManager::Instance();

//     G4double edep = aStep->GetTotalEnergyDeposit();
//     G4Track* track = aStep->GetTrack();
//     G4ParticleDefinition* particle = track->GetDefinition();
//     G4String particleName = particle->GetParticleName();
//     G4int layerID = aStep->GetPreStepPoint()->GetTouchableHandle()->GetCopyNumber();

//     if (edep > 0 || particleName == "neutron" || particleName == "e-") {
//         analysisManager->FillNtupleDColumn(0, layerID);
//         analysisManager->FillNtupleDColumn(1, edep);

//         if (particleName == "neutron" && track->GetParentID() > 0) {
//             analysisManager->FillNtupleDColumn(2, track->GetKineticEnergy());
//         } else {
//             analysisManager->FillNtupleDColumn(2, 0); // 填充默认值
//         }

//         if (particleName == "e-" && track->GetParentID() > 0) {
//             analysisManager->FillNtupleDColumn(3, track->GetKineticEnergy());
//         } else {
//             analysisManager->FillNtupleDColumn(3, 0); // 填充默认值
//         }

//         analysisManager->AddNtupleRow(); // 确保一次性填充所有列后再添加行
//     }

//     return true;
// }

// void LayerSensitiveDetector::EndOfEvent(G4HCofThisEvent* hce) {
//     // Any end of event processing can be done here
// }