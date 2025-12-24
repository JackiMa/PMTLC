// SiPINLCRunStats.cc

#include "SiPINLCRunStats.hh"

#include "G4AccumulableManager.hh"

SiPINLCRunStats& SiPINLCRunStats::Instance()
{
  static SiPINLCRunStats s;
  return s;
}

void SiPINLCRunStats::RegisterAccumulables()
{
  auto* accMan = G4AccumulableManager::Instance();
  accMan->RegisterAccumulable(fAccNEvents);
  accMan->RegisterAccumulable(fAccSiHits);
  accMan->RegisterAccumulable(fAccScintGenerated);
  accMan->RegisterAccumulable(fAccCherenkovGenerated);
  accMan->RegisterAccumulable(fAccEscCrystal);
  accMan->RegisterAccumulable(fAccEscTop);
  accMan->RegisterAccumulable(fAccEscSide);
  accMan->RegisterAccumulable(fAccEscPTFE);
  accMan->RegisterAccumulable(fAccEscGrease);
  accMan->RegisterAccumulable(fAccEscWorld);
  accMan->RegisterAccumulable(fAccThetaSumDeg);
  accMan->RegisterAccumulable(fAccThetaCount);
  accMan->RegisterAccumulable(fAccHitCountSum);
  accMan->RegisterAccumulable(fAccHitCountN);
}

void SiPINLCRunStats::Reset()
{
  G4AccumulableManager::Instance()->Reset();
}

void SiPINLCRunStats::Merge()
{
  G4AccumulableManager::Instance()->Merge();
}

void SiPINLCRunStats::AccumulateEventStats(
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
  G4int hitCountN)
{
  fAccNEvents += 1;
  fAccSiHits += siHitsDetected;
  fAccScintGenerated += scintGenerated;
  fAccCherenkovGenerated += chGenerated;
  fAccEscCrystal += escCrystal;
  fAccEscTop += escTop;
  fAccEscSide += escSide;
  fAccEscPTFE += escPTFE;
  fAccEscGrease += escGrease;
  fAccEscWorld += escWorld;
  fAccThetaSumDeg += thetaSumDeg;
  fAccThetaCount += thetaCount;
  fAccHitCountSum += hitCountSum;
  fAccHitCountN += hitCountN;
}


