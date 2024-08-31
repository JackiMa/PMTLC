#include "G4VPrimitiveScorer.hh"
#include "G4Step.hh"
#include "G4THitsMap.hh"

#include "G4ParticleDefinition.hh"
#include "G4ParticleTable.hh"
#include "G4Neutron.hh"
#include "G4Electron.hh"
#include "G4Gamma.hh"
#include "G4OpticalPhoton.hh"

class NeutronScorer : public G4VPrimitiveScorer {
public:
    NeutronScorer(const G4String& name, G4int depth = 0);
    virtual ~NeutronScorer();

    virtual G4bool ProcessHits(G4Step* aStep, G4TouchableHistory*) override;
    virtual void Initialize(G4HCofThisEvent* HCE) override;
    virtual void EndOfEvent(G4HCofThisEvent* HCE) override;

private:
    G4THitsMap<G4double>* fHitsMap;
};

class TotalEnergyScorer : public G4VPrimitiveScorer {
public:
    TotalEnergyScorer(const G4String& name, G4int depth = 0);
    virtual ~TotalEnergyScorer();

    virtual G4bool ProcessHits(G4Step* aStep, G4TouchableHistory*) override;
    virtual void Initialize(G4HCofThisEvent* HCE) override;
    virtual void EndOfEvent(G4HCofThisEvent* HCE) override;

private:
    G4THitsMap<G4double>* fHitsMap;
};

class PassingEnergyScorer : public G4VPrimitiveScorer {
public:
    PassingEnergyScorer(const G4String& name, const G4String& scorer, G4int depth = 0);
    virtual ~PassingEnergyScorer();

    virtual G4bool ProcessHits(G4Step* aStep, G4TouchableHistory*) override;
    virtual void Initialize(G4HCofThisEvent* HCE) override;
    virtual void EndOfEvent(G4HCofThisEvent* HCE) override;

private:
    G4THitsMap<G4double>* fHitsMap;
    G4String scorerName;
};

class PassingEnergyScorer_Secondary : public G4VPrimitiveScorer {
public:
    PassingEnergyScorer_Secondary(const G4String& name,const G4String& scorer, G4int depth = 0);
    virtual ~PassingEnergyScorer_Secondary();

    virtual G4bool ProcessHits(G4Step* aStep, G4TouchableHistory*) override;
    virtual void Initialize(G4HCofThisEvent* HCE) override;
    virtual void EndOfEvent(G4HCofThisEvent* HCE) override;

private:
    G4THitsMap<G4double>* fHitsMap;
    G4String scorerName;
};

class ElectronEnergyScorer : public G4VPrimitiveScorer {
public:
    ElectronEnergyScorer(const G4String& name, G4int depth = 0);
    virtual ~ElectronEnergyScorer();

    virtual G4bool ProcessHits(G4Step* aStep, G4TouchableHistory*) override;
    virtual void Initialize(G4HCofThisEvent* HCE) override;
    virtual void EndOfEvent(G4HCofThisEvent* HCE) override;

private:
    G4THitsMap<G4double>* fHitsMap;
};


class HEPhotonScorer : public G4VPrimitiveScorer {
public:
    HEPhotonScorer(const G4String& name, G4int depth = 0);
    virtual ~HEPhotonScorer();

    virtual G4bool ProcessHits(G4Step* aStep, G4TouchableHistory* touchableHistory) override;
    virtual void Initialize(G4HCofThisEvent* HCE) override;
    virtual void EndOfEvent(G4HCofThisEvent* HCE) override;

private:
    G4THitsMap<G4double>* fHitsMap;
};

class SCLightScorer : public G4VPrimitiveScorer {
public:
    SCLightScorer(const G4String& name, const G4int fillH1Id, G4int depth = 0);
    virtual ~SCLightScorer();

    virtual G4bool ProcessHits(G4Step* aStep, G4TouchableHistory* touchableHistory) override;
    virtual void Initialize(G4HCofThisEvent*) override;
    virtual void EndOfEvent(G4HCofThisEvent*) override;

private:
    G4int fHistogramId;
    std::set<G4int> processedTrackIDs;  // 存储已经处理过的光子的 track ID
};

class CherenkovLightScorer : public G4VPrimitiveScorer {
public:
    CherenkovLightScorer(const G4String& name, const G4int fillH1Id, G4int depth = 0);
    virtual ~CherenkovLightScorer();

    virtual G4bool ProcessHits(G4Step* aStep, G4TouchableHistory* touchableHistory) override;
    virtual void Initialize(G4HCofThisEvent*) override;
    virtual void EndOfEvent(G4HCofThisEvent*) override;

private:
    G4int fHistogramId;
    std::set<G4int> processedTrackIDs;  // 存储已经处理过的光子的 track ID
};


class FiberAcceptanceScorer : public G4VPrimitiveScorer {
public:
    FiberAcceptanceScorer(const G4String& name, const G4int fillH1Id, G4int depth = 0);
    virtual ~FiberAcceptanceScorer();

    virtual G4bool ProcessHits(G4Step* aStep, G4TouchableHistory* touchableHistory) override;
    virtual void Initialize(G4HCofThisEvent*) override;
    virtual void EndOfEvent(G4HCofThisEvent*) override;

    private:
    G4int fHistogramId;
    std::set<G4int> processedTrackIDs;  // 存储已经处理过的光子的 track ID
};


class PhotocathodeScorer : public G4VPrimitiveScorer {
public:
    PhotocathodeScorer(const G4String& name, const G4int fillH1Id,const G4int fillH2Id, G4int depth = 0);
    virtual ~PhotocathodeScorer();
    virtual void EndOfEvent(G4HCofThisEvent*) override;
    virtual G4bool ProcessHits(G4Step* aStep, G4TouchableHistory*);

private:
    G4int fH1Id, fH2Id;
    std::set<G4int> processedTrackIDs;  // 存储已经处理过的光子的 track ID
};


G4bool IsInsideSensitiveVolume(G4Step* aStep, const G4ThreeVector& vertexPosition);
