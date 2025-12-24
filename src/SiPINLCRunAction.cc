//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//
/// \file SiPINLC/src/SiPINLCRunAction.cc
/// \brief Implementation of the SiPINLCRunAction class
//
//
//
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
#include "SiPINLCRunAction.hh"
#include "SiPINLCPrimaryGeneratorAction.hh"
#include "SiPINLCRun.hh"
#include "G4ParticleDefinition.hh"
#include "G4Run.hh"
#include <fstream>
#include <filesystem>
#include <sstream>
#include <mutex>
#include "G4UnitsTable.hh"
#include "G4SystemOfUnits.hh"

#include "G4AnalysisManager.hh"
#include "G4AccumulableManager.hh"
#include "G4RunManager.hh"

#include "config.hh"
#include "SiPINLCRunStats.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
SiPINLCRunAction::SiPINLCRunAction(SiPINLCPrimaryGeneratorAction *prim)
    : G4UserRunAction(), fRun(nullptr), fPrimary(prim)
{
  // Register accumulables for MT-safe run summaries (no RTTI required).
  SiPINLCRunStats::Instance().RegisterAccumulables();

  auto analysisManager = G4AnalysisManager::Instance();
  analysisManager->SetVerboseLevel(1);
  analysisManager->SetNtupleMerging(true);


  // Creating histograms for the spectra
  gID_H1_sc_wl = 0;
  analysisManager->CreateH1("ScintillationWavelength", "Scintillation Wavelength in Crystal", 600, 200.0, 800.0);
  
  gID_H1_ch_wl = 1;
  analysisManager->CreateH1("CherenkovLightWavelength", "CherenkovLight Wavelength in Crystal", 600, 200.0, 800.0);
  
  gID_H1_sipin_wl = 2;
  analysisManager->CreateH1("PhotonHitatPhotoncathodeWavelength", "Wavelength of Light Hit at Photoncathode", 600, 200.0, 800.0);
  
  gID_H1_sc_ed = 3;
  analysisManager->CreateH1("EnergyDepositionInScintillator", "Energy Deposition in Scintillator", 1000, 0.0, 1);

  gID_H1_sipin_LC = 4;
  analysisManager->CreateH1("LightCollectionAtsipin", "Light Collectiopn", 500, 0.0, 1000);
  
  gID_H1_photon_posY0 = 5;
  analysisManager->CreateH1("PhotonPosition_Y=0", "Photon Position Y in [-0.5, 0.5", 100, -0.5 * g_worldX, 0.5 * g_worldX);

  gID_H1_photon_posYX = 6;
  analysisManager->CreateH1("PhotonPosition_Y=X", "Photon Position X=Y in [-0.5, 0.5]", 100, -sqrt(2)*0.5 * g_worldX, sqrt(2)*0.5 * g_worldX);

  gID_H2_source_pos = 0;
  analysisManager->CreateH2("SourcePosition", "Source Position", 100, -0.5 * g_worldX, 0.5 * g_worldX, 100, -0.5 * g_worldY, 0.5 * g_worldY);
  
  gID_H2_photon_pos = 1;
  analysisManager->CreateH2("PhotonPosition", "Photon Position", 100, -0.5 * g_worldX, 0.5 * g_worldX, 100, -0.5 * g_worldY, 0.5 * g_worldY);
  
  // === 论文所需的新增直方图 ===
  gID_H1_sipin_theta = 7;
  analysisManager->CreateH1("IncidenceAngle", "Photon Incidence Angle at SiPIN (deg)", 90, 0.0, 90.0);
  
  gID_H1_sipin_hitCount = 8;
  analysisManager->CreateH1("HitCount", "Number of Hits per Photon at SiPIN", 20, 0.5, 20.5);
  
  gID_H1_escape_channel = 9;
  // 通道编码: 0=被晶体吸收, 1=从顶面逃逸, 2=从侧面逃逸, 3=被PTFE吸收, 4=其他
  analysisManager->CreateH1("EscapeChannel", "Photon Escape/Loss Channel", 5, -0.5, 4.5);
  
  gID_H1_photon_generated = 10;
  analysisManager->CreateH1("PhotonGenerated", "Scintillation Photons Generated per Event", 500, 0.0, 5000.0);
  
  gID_H1_photon_pathlength = 11;
  // 光子在晶体内的总路程分布，用于理解自吸收效应
  // 对于5mm晶体，典型路程可能在几mm到几十mm（多次反射）
  analysisManager->CreateH1("PhotonPathInCrystal", "Photon Total Path Length in Crystal (mm)", 200, 0.0, 100.0);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
SiPINLCRunAction::~SiPINLCRunAction() {}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
G4Run *SiPINLCRunAction::GenerateRun()
{
  fRun = new SiPINLCRun();
  return fRun;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
void SiPINLCRunAction::BeginOfRunAction(const G4Run *)
{
  // Reset accumulables (thread-local) at the beginning of each run.
  SiPINLCRunStats::Instance().Reset();

  if (fPrimary)
  {
    G4double energy;
    G4ParticleDefinition *particle;

    fPrimary->isInitialized = false;
    if (fPrimary->GetUseParticleGun())
    {
      particle = fPrimary->GetParticleGun()->GetParticleDefinition();
      energy = fPrimary->GetParticleGun()->GetParticleEnergy();
    }
    else
    {
      particle = fPrimary->GetGPS()->GetParticleDefinition();
      energy = fPrimary->GetGPS()->GetCurrentSource()->GetEneDist()->GetMonoEnergy();
    }

    fRun->SetPrimary(particle, energy);
  }

  // Open an output file
  //
  auto analysisManager = G4AnalysisManager::Instance();

  G4String fileName = getNewfileName("LYsimulations");
  if (!analysisManager->OpenFile(fileName))
  {
    G4cerr << "Error: could not open file " << fileName << G4endl;
  }
  else
  {
    G4cout << "Successfully opened file " << fileName << G4endl;
  }
  G4cout << "Using " << analysisManager->GetType() << G4endl;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
void SiPINLCRunAction::EndOfRunAction(const G4Run * run)
{

  G4int runID = run->GetRunID();

  auto analysisManager = G4AnalysisManager::Instance();

  // Ensure accumulables are merged to master before we write summary CSV.
  // (In many setups this is already handled by RunManager, but calling explicitly is safe.)
  if (isMaster)
  {
    SiPINLCRunStats::Instance().Merge();
  }

  if (isMaster)
  {

    static std::mutex fileMutex;
    std::lock_guard<std::mutex> lock(fileMutex); // 使用互斥锁确保线程安全

    std::ofstream outFile("run_data.csv", std::ios::app); // 以追加模式打开文件

    // 在文件为空时写入标题行
    if (outFile.tellp() == 0)
    {
      outFile << "runID,"
              << "Scintillation Photon Count,"
              << "Cherenkov Photon Count,"
              << "Photon Hit si,"
              << "lightCollectionEfficiency,"
              << "Escaped_CrystalAbsorbed,"
              << "Escaped_TopAir,"
              << "Escaped_SideAir,"
              << "Escaped_PTFE,"
              << "Escaped_Grease,"
              << "Escaped_World,"
              << "Mean_IncidenceAngle_deg,"
              << "Mean_HitCount" << "\n";
    }

    // 统计光子产生与收集（使用 RunStats/Accumulable，保证 MT 下正确）
    const auto& stats = SiPINLCRunStats::Instance();
    const G4int nEvents = stats.NEvents();
    const G4int scintillationPhotonCount = stats.ScintGenerated();
    const G4int cherenkovPhotonCount = stats.CherenkovGenerated();
    const G4int siCounts = stats.SiHits();
    G4double lightCollectionEfficiency = 0.0;
    if ((scintillationPhotonCount + cherenkovPhotonCount) > 0) {
      lightCollectionEfficiency = (G4double)siCounts / (scintillationPhotonCount + cherenkovPhotonCount);
    } else if (nEvents > 0) {
      lightCollectionEfficiency = (G4double)siCounts / nEvents;
    }
    
    // === 论文所需：逃逸通道统计（Accumulable） ===
    const G4int escapeCrystal = stats.EscCrystal();
    const G4int escapeTopAir = stats.EscTop();
    const G4int escapeSideAir = stats.EscSide();
    const G4int escapePTFE = stats.EscPTFE();
    const G4int escapeGrease = stats.EscGrease();
    const G4int escapeWorld = stats.EscWorld();
    
    // === 论文所需：入射角和撞击次数的均值 ===
    const G4int thetaN = stats.ThetaCount();
    const G4double meanTheta = (thetaN > 0) ? (stats.ThetaSumDeg() / thetaN) : 0.0;
    const G4int hitN = stats.HitCountN();
    const G4double meanHitCount = (hitN > 0) ? (stats.HitCountSum() / hitN) : 0.0;

    // 写入数据
    outFile << runID << ","
            << scintillationPhotonCount << ","
            << cherenkovPhotonCount << ","
            << siCounts << ","
            << lightCollectionEfficiency << ","
            << escapeCrystal << ","
            << escapeTopAir << ","
            << escapeSideAir << ","
            << escapePTFE << ","
            << escapeGrease << ","
            << escapeWorld << ","
            << meanTheta << ","
            << meanHitCount << "\n";

    // 关闭文件
    outFile.close();
  }
  analysisManager->Write();
  analysisManager->CloseFile();

  G4cout << "Data written and file closed." << G4endl;
}

bool SiPINLCRunAction::fileExists(const G4String &fileName)
{
  std::ifstream file(fileName.c_str());
  return file.good();
}

G4String SiPINLCRunAction::getNewfileName(G4String baseFileName)
{
  G4String fileExtension = ".root";
  G4String fileName;
  int fileIndex = 0;

  do
  {
    std::stringstream ss;
    ss << baseFileName;
    if (fileIndex > 0)
    {
      ss << "(" << fileIndex << ")";
    }
    ss << fileExtension;
    fileName = ss.str();
    fileIndex++;
  } while (fileExists(fileName));

  return fileName;
}
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
