// #include <G4Neutron.hh>
// #include <G4Electron.hh>
// #include <G4VSensitiveDetector.hh>
// #include <G4THitsMap.hh>
// #include <G4Step.hh>
// #include <G4HCofThisEvent.hh>
// #include <G4TouchableHistory.hh>
// #include <G4AnalysisManager.hh>

// class LayerSensitiveDetector : public G4VSensitiveDetector {
// public:
//     LayerSensitiveDetector(const G4String& name, const G4String& hitsCollectionName);
//     virtual ~LayerSensitiveDetector();

//     virtual void Initialize(G4HCofThisEvent* hce) override;
//     virtual G4bool ProcessHits(G4Step* step, G4TouchableHistory* history) override;
//     virtual void EndOfEvent(G4HCofThisEvent* hce) override;

// private:
//     G4THitsMap<G4double>* fHitsCollection;
//     G4int fEnergyNtupleId;
//     G4int fNeutronNtupleId;
//     G4int fElectronNtupleId;
// };
