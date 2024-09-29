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
/// \file PMTLC/src/PMTLCRunAction.cc
/// \brief Implementation of the PMTLCRunAction class
//
//
//
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
#include "PMTLCRunAction.hh"
#include "PMTLCPrimaryGeneratorAction.hh"
#include "PMTLCRun.hh"
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

#include "config.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
PMTLCRunAction::PMTLCRunAction(PMTLCPrimaryGeneratorAction *prim)
    : G4UserRunAction(), fRun(nullptr), fPrimary(prim)
{

  auto analysisManager = G4AnalysisManager::Instance();
  analysisManager->SetVerboseLevel(1);
  analysisManager->SetNtupleMerging(true);


  // Creating histograms for the spectra
  gID_H1_sc_wl = 0;
  analysisManager->CreateH1("ScintillationWavelength", "Scintillation Wavelength in Crystal", 600, 200.0, 800.0);
  
  gID_H1_ch_wl = 1;
  analysisManager->CreateH1("CherenkovLightWavelength", "CherenkovLight Wavelength in Crystal", 600, 200.0, 800.0);
  
  gID_H1_PMT_wl = 2;
  analysisManager->CreateH1("PhotonHitatPhotoncathodeWavelength", "Wavelength of Light Hit at Photoncathode", 600, 200.0, 800.0);
  
  gID_H1_sc_ed = 3;
  analysisManager->CreateH1("EnergyDepositionInScintillator", "Energy Deposition in Scintillator", 1000, 0.0, 1);

  gID_H1_PMT_LC = 4;
  analysisManager->CreateH1("LightCollectionAtPMT", "Light Collectiopn", 500, 0.0, 1000);
  
  gID_H1_photon_posY0 = 5;
  analysisManager->CreateH1("PhotonPosition_Y=0", "Photon Position Y in [-0.5, 0.5", 100, -0.5 * g_worldX, 0.5 * g_worldX);

  gID_H1_photon_posYX = 6;
  analysisManager->CreateH1("PhotonPosition_Y=X", "Photon Position X=Y in [-0.5, 0.5]", 100, -sqrt(2)*0.5 * g_worldX, sqrt(2)*0.5 * g_worldX);

  gID_H2_source_pos = 0;
  analysisManager->CreateH2("SourcePosition", "Source Position", 100, -0.5 * g_worldX, 0.5 * g_worldX, 100, -0.5 * g_worldY, 0.5 * g_worldY);
  
  gID_H2_photon_pos = 1;
  analysisManager->CreateH2("PhotonPosition", "Photon Position", 100, -0.5 * g_worldX, 0.5 * g_worldX, 100, -0.5 * g_worldY, 0.5 * g_worldY);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
PMTLCRunAction::~PMTLCRunAction() {}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
G4Run *PMTLCRunAction::GenerateRun()
{
  fRun = new PMTLCRun();
  return fRun;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
void PMTLCRunAction::BeginOfRunAction(const G4Run *)
{
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
void PMTLCRunAction::EndOfRunAction(const G4Run * run)
{

  G4int runID = run->GetRunID();

  auto analysisManager = G4AnalysisManager::Instance();


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
              << "Photon Hit Photocathode,"
              << "lightCollectionEfficiency" << "\n";
    }

    // 统计光子产生与收集
    G4int scintillationPhotonCount = analysisManager->GetH1(gID_H1_sc_wl)->entries();
    G4int cherenkovPhotonCount = analysisManager->GetH1(gID_H1_ch_wl)->entries();
    G4int PhotocathodeCounts = analysisManager->GetH1(gID_H1_PMT_wl)->entries();
    G4double lightCollectionEfficiency = (G4double)PhotocathodeCounts / (scintillationPhotonCount + cherenkovPhotonCount);

    // 写入数据
    outFile << runID << ","
            << scintillationPhotonCount << ","
            << cherenkovPhotonCount << ","
            << PhotocathodeCounts <<","
            << lightCollectionEfficiency <<"\n";

    // 关闭文件
    outFile.close();
  }
  analysisManager->Write();
  analysisManager->CloseFile();

  G4cout << "Data written and file closed." << G4endl;
}

bool PMTLCRunAction::fileExists(const G4String &fileName)
{
  std::ifstream file(fileName.c_str());
  return file.good();
}

G4String PMTLCRunAction::getNewfileName(G4String baseFileName)
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
