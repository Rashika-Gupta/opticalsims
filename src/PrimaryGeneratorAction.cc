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
/// \file persistency/gdml//src/PrimaryGeneratorAction.cc
/// \brief Implementation of the PrimaryGeneratorAction class
//
//
//
//

#include "PrimaryGeneratorAction.hh"
#include "G4Event.hh"
#include "G4Electron.hh"
#include "G4Exception.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4AnalysisManager.hh"
#include "G4ParticleDefinition.hh"
#include "G4GeneralParticleSource.hh"
#include "G4SystemOfUnits.hh"
#include "G4GenericMessenger.hh"
#include "G4OpticalPhoton.hh"
#include "G4PrimaryParticle.hh"
#include "G4ParticleGun.hh"
#include "TFile.h"
#include "TTreeReader.h"
#include "TTreeReaderValue.h"
#include <cstdlib>
#include <iostream>
#include <string>

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

PrimaryGeneratorAction::PrimaryGeneratorAction(std::string celer_offload_mode)
    : G4VUserPrimaryGeneratorAction(), celer_offload_mode_(std::move(celer_offload_mode)),
      fParticleGun(0),
      fmsg(nullptr), fFileName(""), finitParticleType("GPS"), fAmount(100)
{
  fParticleGun = new G4ParticleGun(1);
  fParticleGun->SetParticleDefinition(G4OpticalPhoton::Definition());

  fParticleGun->SetParticleDefinition(G4OpticalPhoton::Definition());
  fParticleGun->SetParticlePosition(G4ThreeVector(0., 0., 0.));
  fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0., 0., 1.));

  // Must be perpendicular to momentum
  fParticleGun->SetParticlePolarization(G4ThreeVector(1., 0., 0.));
  // G4int n_particle = 1;
  // fParticleGun = new G4GeneralParticleSource();
  fmsg = new G4GenericMessenger(this, "/PrimaryGenerationAction/input/", "");
  fmsg->DeclareProperty("type", finitParticleType, "Initial Particle Type: LArSoft or GPS (Default)");
  fmsg->DeclareProperty("file", fFileName, "File Name to Read");
  fmsg->DeclareProperty("phamount", fAmount, "Amount of Photons to produce");
  fmsg->DeclareProperty("energyscan", fEnergyScan, "Enable per-event fixed energy scan");
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

PrimaryGeneratorAction::~PrimaryGeneratorAction()
{
  delete fParticleGun;
  delete fmsg;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void PrimaryGeneratorAction::GeneratePrimaries(G4Event *anEvent)
{
  static const std::vector<G4double> optical_energies = {
      // 1.8785e-6 * MeV,
      // 2.88625e-6 * MeV,
      // 2.0e-06 * MeV,
      // 3.21e-06 * MeV,
      // 3.90205e-6 * MeV,
      // 4.95070e-6 * MeV,
      // 1.96760e-6 * MeV,
      // 5.98475e-6 * MeV,
      // 6.9e-6 * MeV,
      // 7.55e-6 * MeV,
      // 7.8e-6 * MeV,
      // 8.0e-6 * MeV,
      // 8.2e-6 * MeV,
      // 8.5e-6 * MeV,
      // 8.7e-6 * MeV,
      // 8.9e-6 * MeV,
      // 9.0e-6 * MeV,
      // 9.1e-6 * MeV,
      // 9.2e-6 * MeV,
      // 9.49745e-6 * MeV,
      // 9.69380e-6 * MeV,
      // 9.85e-6 * MeV,
      // 1.0e-5 * MeV,
      // 1.03e-05 * MeV, 11.4736e-06 * MeV, 11.4849e-06 * MeV, 11.4962e-06 * MeV, 11.5075e-06 * MeV, 11.5188e-06 * MeV, 11.5302e-06 * MeV, 11.5416e-06 * MeV, 11.5530e-06 * MeV, 11.5644e-06 * MeV, 11.5758e-06 * MeV,
      // 1e-06 * MeV, 2e-06 * MeV, 3e-06 * MeV, 4e-06 * MeV, 5e-06 * MeV, 6e-06 * MeV, 2e-07 * MeV
      1.7e-06 * MeV,
      2e-06 * MeV,
      2.4e-06 * MeV,
      2.7e-06 * MeV,
      3.2e-06 * MeV,
      3.4e-06 * MeV,
      3.8e-06 * MeV,
  };
  constexpr G4double electron_energy = 50 * MeV;

  if (celer_offload_mode_ == "optical-gun")
  {
    auto const event_id = static_cast<std::size_t>(anEvent->GetEventID());
    if (event_id >= optical_energies.size())
    {
      return;
    }

    fParticleGun->SetParticleDefinition(G4OpticalPhoton::Definition());
    fParticleGun->SetParticleEnergy(optical_energies[event_id]);
  }
  else if (celer_offload_mode_ == "optical-distribution" || celer_offload_mode_ == "electron-photon" || celer_offload_mode_ == "optical-track")
  {
    // A charged primary is required to create scintillation/Cherenkov
    // distribution data. In electron-photon mode the electron itself is also
    // handed to Celeritas.
    fParticleGun->SetParticleDefinition(G4Electron::Definition());
    fParticleGun->SetParticleEnergy(electron_energy);
    fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0., 0., -1.));
    fParticleGun->SetParticlePosition(G4ThreeVector(200., 300., 1000.));
  }
  else
  {
    G4ExceptionDescription description;
    description << "Invalid OPTICALSIMS_CELERITAS_MODE='" << celer_offload_mode_
                << "': expected 'optical-gun', 'optical-track', "
                   "'optical-distribution', or 'electron-photon'";
    G4Exception("PrimaryGeneratorAction::GeneratePrimaries",
                "OpticalSims002",
                FatalException,
                description);
    return;
  }

  fParticleGun->GeneratePrimaryVertex(anEvent);

  // return;

  if (finitParticleType == "GPS")
  {
    auto analysisManager = G4AnalysisManager::Instance();
    fParticleGun->GeneratePrimaryVertex(anEvent);
    analysisManager->FillNtupleSColumn(0, 0, fParticleGun->GetParticleDefinition()->GetParticleName());
    analysisManager->FillNtupleIColumn(0, 1, fParticleGun->GetParticleDefinition()->GetParticleDefinitionID());
    analysisManager->FillNtupleDColumn(0, 2, fParticleGun->GetParticleEnergy());
    analysisManager->FillNtupleDColumn(0, 3, fParticleGun->GetParticlePosition().x());
    analysisManager->FillNtupleDColumn(0, 4, fParticleGun->GetParticlePosition().y());
    analysisManager->FillNtupleDColumn(0, 5, fParticleGun->GetParticlePosition().z());
    analysisManager->FillNtupleDColumn(0, 6, fParticleGun->GetParticleTime());
    analysisManager->FillNtupleDColumn(0, 7, fParticleGun->GetParticleMomentumDirection().x());
    analysisManager->FillNtupleDColumn(0, 8, fParticleGun->GetParticleMomentumDirection().y());
    analysisManager->FillNtupleDColumn(0, 9, fParticleGun->GetParticleMomentumDirection().z());
    analysisManager->FillNtupleIColumn(0, 10, anEvent->GetEventID());
    analysisManager->AddNtupleRow(0);

    if ((fParticleGun->GetParticleDefinition()->GetParticleName() != "opticalphoton"))
      return;
// Clear the sphotons before the next event
#ifdef With_Opticks
    if (sphotons.size() > 0)
    {
      sphotons.clear();
      sphotons.shrink_to_fit();
    }
#endif

    G4PrimaryVertex *vertex = new G4PrimaryVertex(fParticleGun->GetParticlePosition(), fParticleGun->GetParticleTime());

    for (int i = 0; i < fAmount; i++) // Produce specified amount of photons
    {
      G4PrimaryParticle *particle = new G4PrimaryParticle(G4OpticalPhoton::Definition());
      particle->SetKineticEnergy(fParticleGun->GetParticleEnergy());
      particle->SetMomentumDirection(fParticleGun->GetParticleMomentumDirection());
      particle->SetPolarization(fParticleGun->GetParticlePolarization());
      vertex->SetPrimary(particle);
#ifdef With_Opticks

      if ((SEventConfig::IntegrationMode() == 1) || (SEventConfig::IntegrationMode() == 3))
      {
        // Produce photon on GPU with GPS
        sphoton spht;
        spht.zero();
        spht.zero_flags();
        spht.set_flag(TORCH);
        spht.pos = make_float3(fParticleGun->GetParticlePosition().x(), fParticleGun->GetParticlePosition().y(), fParticleGun->GetParticlePosition().z());
        spht.pol = make_float3(fParticleGun->GetParticlePolarization().x(), fParticleGun->GetParticlePolarization().y(), fParticleGun->GetParticlePolarization().z());
        spht.mom = make_float3(fParticleGun->GetParticleMomentumDirection().x(), fParticleGun->GetParticleMomentumDirection().y(), fParticleGun->GetParticleMomentumDirection().z());
        spht.wavelength = (1240) / (fParticleGun->GetParticleEnergy() / eV); // nm
        spht.time = 0;
        sphotons.push_back(spht);
      }
#endif
    }
    anEvent->AddPrimaryVertex(vertex);
  }
  else if (finitParticleType == "ROOT")
  {

    if (!fFileName)
    {
      std::cout << fFileName << "  is not found" << std::endl;
      return;
    }

    // Loading photon info from a ROOT file
    TFile file(fFileName);
    std::cout << "[PrimaryGeneratorAction::GeneratePrimaries] Using Root for reading photon positions" << std::endl;
    std::cout << "[PrimaryGeneratorAction::GeneratePrimaries] Reading  " << fFileName << " ..." << std::endl;

    TTreeReader reader("photon_gen", &file);
    TTreeReaderValue<int> fevtID(reader, "evtID");
    TTreeReaderValue<double> fx(reader, "x");
    TTreeReaderValue<double> fy(reader, "y");
    TTreeReaderValue<double> fz(reader, "z");
    TTreeReaderValue<double> ft(reader, "t");
    TTreeReaderValue<double> fpx(reader, "px");
    TTreeReaderValue<double> fpy(reader, "py");
    TTreeReaderValue<double> fpz(reader, "pz");
    TTreeReaderValue<double> fmx(reader, "mx");
    TTreeReaderValue<double> fmy(reader, "my");
    TTreeReaderValue<double> fmz(reader, "mz");
    TTreeReaderValue<double> fwave(reader, "wavelength");
    TTreeReaderValue<double> fenergy(reader, "energy");
    G4bool simPhotonCPU = true;
#ifdef With_Opticks
    if ((SEventConfig::IntegrationMode() == 1))
      simPhotonCPU = false;
#endif
    // Clear the sphotons before the next event
    //  if (sphotons.size() > 0)
    //  {
    //    sphotons.clear();
    //    sphotons.shrink_to_fit();
    //  }

    // Produce Photons from a root file
    while (reader.Next())
    {
      // std::cout << "Event ID "<< anEvent->GetEventID() << " Event From File " <<*fevtID << std::endl;
      if (anEvent->GetEventID() != (*fevtID) - 1)
        continue;

      if (simPhotonCPU)
      {
        // std::cout << "[PrimaryGeneratorAction::GeneratePrimaries] Simulating Photons in Geant4 for Event ID "<<*fevtID << std::endl;
        G4PrimaryParticle *particle = new G4PrimaryParticle(G4OpticalPhoton::Definition());
        G4PrimaryVertex *vertex = new G4PrimaryVertex(G4ThreeVector((*fx) * cm, (*fy) * cm, (*fz) * cm), (*ft) * ns);
        particle->SetKineticEnergy((*fenergy) * eV);
        particle->SetMomentumDirection(G4ThreeVector(*fmx, *fmy, *fmz));
        particle->SetPolarization(G4ThreeVector(*fpx, *fpy, *fpz));
        vertex->SetPrimary(particle);
        anEvent->AddPrimaryVertex(vertex);
      }
#ifdef With_Opticks
      if (SEventConfig::IntegrationMode() == 1 || SEventConfig::IntegrationMode() == 3)
      {
        // std::cout << "[PrimaryGeneratorAction::GeneratePrimaries] Simulating Photons in GPU for Event ID "<<*fevtID << std::endl;
        sphoton spht;
        spht.zero();
        spht.zero_flags();
        spht.set_flag(TORCH);
        spht.pos = make_float3((*fx) * 10, (*fy) * 10, (*fz) * 10); // mm
        spht.mom = make_float3(*fmx, *fmy, *fmz);
        spht.pol = make_float3(*fpx, *fpy, *fpz);
        spht.wavelength = *fwave; // nm
        spht.time = *ft;          // ns
        sphotons.push_back(spht);
      }
#endif
    }

    // Add Analysis Manager
  }
#ifdef With_Opticks
  if (sphotons.size() > 0)
  {
    std::cout << "[PrimaryGeneratorAction::GeneratePrimaries]: Amount of Photons to simulate " << sphotons.size() << std::endl;
    OpticksHitHandler *OpticksHandler = OpticksHitHandler::getInstance();
    OpticksHandler->setPrimPhotons(sphotons);
  }
  else
    std::cout << "[PrimaryGeneratorAction::GeneratePrimaries]: No photons to simulate ... " << std::endl;
#endif
}
