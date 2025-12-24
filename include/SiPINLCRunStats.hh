// SiPINLCRunStats.hh
// Thread-safe run-level statistics via Geant4 Accumulables.
//
// Motivation:
// - In MT mode, reading run/event counts from `G4Run` in master can be misleading.
// - RTTI may be disabled in this project, so we avoid dynamic_cast between user actions.
// - EventAction accumulates per-event numbers into these accumulables; RunAction merges & writes CSV.

#ifndef SiPINLCRunStats_h
#define SiPINLCRunStats_h 1

#include "G4Accumulable.hh"
#include "globals.hh"

class SiPINLCRunStats
{
public:
  static SiPINLCRunStats& Instance();

  // Called from RunAction constructor (each thread) to register accumulables.
  void RegisterAccumulables();
  // Called from BeginOfRunAction (each thread) to reset.
  void Reset();
  // Called from EndOfRunAction (master) to merge thread-local values.
  void Merge();

  // Called from EventAction (each event) to accumulate event-level contributions.
  void AccumulateEventStats(
    G4int siHitsDetected,
    G4int scintGenerated,
    G4int chGenerated,
    G4int escCrystal,
    G4int escTop,
    G4int escSide,
    G4int escPTFE,
    G4int escGrease,
    G4int escWorld,
    G4double thetaSumDeg,
    G4int thetaCount,
    G4double hitCountSum,
    G4int hitCountN);

  // Getters (valid after Merge on master; in single-thread also OK)
  G4int NEvents() const { return fAccNEvents.GetValue(); }
  G4int SiHits() const { return fAccSiHits.GetValue(); }
  G4int ScintGenerated() const { return fAccScintGenerated.GetValue(); }
  G4int CherenkovGenerated() const { return fAccCherenkovGenerated.GetValue(); }
  G4int EscCrystal() const { return fAccEscCrystal.GetValue(); }
  G4int EscTop() const { return fAccEscTop.GetValue(); }
  G4int EscSide() const { return fAccEscSide.GetValue(); }
  G4int EscPTFE() const { return fAccEscPTFE.GetValue(); }
  G4int EscGrease() const { return fAccEscGrease.GetValue(); }
  G4int EscWorld() const { return fAccEscWorld.GetValue(); }
  G4double ThetaSumDeg() const { return fAccThetaSumDeg.GetValue(); }
  G4int ThetaCount() const { return fAccThetaCount.GetValue(); }
  G4double HitCountSum() const { return fAccHitCountSum.GetValue(); }
  G4int HitCountN() const { return fAccHitCountN.GetValue(); }

private:
  SiPINLCRunStats() = default;

  G4Accumulable<G4int> fAccNEvents{0};
  G4Accumulable<G4int> fAccSiHits{0};
  G4Accumulable<G4int> fAccScintGenerated{0};
  G4Accumulable<G4int> fAccCherenkovGenerated{0};
  G4Accumulable<G4int> fAccEscCrystal{0};
  G4Accumulable<G4int> fAccEscTop{0};
  G4Accumulable<G4int> fAccEscSide{0};
  G4Accumulable<G4int> fAccEscPTFE{0};
  G4Accumulable<G4int> fAccEscGrease{0};
  G4Accumulable<G4int> fAccEscWorld{0};
  G4Accumulable<G4double> fAccThetaSumDeg{0.0};
  G4Accumulable<G4int> fAccThetaCount{0};
  G4Accumulable<G4double> fAccHitCountSum{0.0};
  G4Accumulable<G4int> fAccHitCountN{0};
};

#endif


