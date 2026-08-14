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
#include "Randomize.hh"
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
#include "G4PhysicalConstants.hh"
#include "G4RandomTools.hh"
#include "TFile.h"
#include "TTreeReader.h"
#include "TTreeReaderValue.h"
#include <cstdlib>
#include <iostream>
#include "G4RunManager.hh"
#include "G4Threading.hh"
#include "G4MTRunManager.hh"
#include "../include/AnalysisManagerHelper.hh"
#ifdef With_Opticks
#include "srng.h"
#include "storch.h"
#endif

#include <string>

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

PrimaryGeneratorAction::PrimaryGeneratorAction(std::string celer_offload_mode)
    : G4VUserPrimaryGeneratorAction(), celer_offload_mode_(std::move(celer_offload_mode)),
      fParticleGun(0),
      fmsg(nullptr), fFileName(""), finitParticleType("GPS"), fAmount(2), fPosition(G4ThreeVector(-314 * cm, 72 * cm, 290 * cm)), fMom(9.7 * eV), fSigmaMom(0.1 * eV), fPhotonAmount({0.25, 25, 11}), fVerbose(false), fGPUPhotonType("Sphoton")
{
  // if (celer_offload_mode_ != 0)
  // {
  //    fParticleGun = new G4ParticleGun(1);
  // }
  // else
  // {
  fParticleGun = new G4GeneralParticleSource();
  // // }
  fmsg = new G4GenericMessenger(this, "/PrimaryGenerationAction/input/", "");
  fmsg->DeclareProperty("type", finitParticleType, "Initial Particle Type: LArSoft or GPS (Default)");
  fmsg->DeclareProperty("file", fFileName, "File Name to Read");
  fmsg->DeclarePropertyWithUnit("pos", "cm", fPosition, "Position of the particle gun (x,y,z) in mm");
  fmsg->DeclarePropertyWithUnit("momentum", "eV", fMom, "The mean momentum of the photon in ev");
  fmsg->DeclarePropertyWithUnit("sigma_momentum", "eV", fSigmaMom, "Spread of photonMomentum  in ev");
  fmsg->DeclareProperty("PhLinAmount", fPhotonAmount, "Defines the minimum, maximum, and step size for generating photons with linearly spaced energies (in MeV)");
  fmsg->DeclareProperty("repeats", fAmount, "Amount of particles to produce or the amount of repeats for the linearly spaced energies primary photons");
  fmsg->DeclareProperty("verbose", fVerbose, "turn on / off verbose output");
  fmsg->DeclareProperty("GPUPhotonType", fGPUPhotonType, "Photon Sampling GPU: Storch, CPU: Sphoton");
  simPhotonCPU = true;

  anaHelper->Reset();
#ifdef With_Opticks
  if ((SEventConfig::IntegrationMode() == 1))
    simPhotonCPU = false;
  OpticksHitHandler *OpticksHandler = OpticksHitHandler::getInstance();
  OpticksHandler->setVerbose(fVerbose);

#endif
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

PrimaryGeneratorAction::~PrimaryGeneratorAction()
{
  delete fParticleGun;
  delete fmsg;
}

G4double PrimaryGeneratorAction::EnergySigmaToWavelengthSigma(G4double meanEnergy, G4double sigmaEnergy)
{
  // meanEnergy and sigmaEnergy in eV
  const G4double hc = h_Planck * c_light; // Geant4 units

  // derivative: dλ/dE = -hc / E^2
  G4double sigma_lambda = (hc / (meanEnergy * meanEnergy)) * sigmaEnergy;

  return (sigma_lambda / nm); // convert to nm
}
G4double PrimaryGeneratorAction::EnergyToWavelength(G4double energy)
{
  // energy should be in eV (or any Geant4 energy unit)
  G4double wavelength = (h_Planck * c_light) / energy; // in Geant4 length units
  return (wavelength / nm);                            // convert to nm
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
  constexpr G4double electron_energy = 5 * MeV;
  // G4GeneralParticleSource owns a G4SingleParticleSource.
  // Configure that source according to the selected Celeritas mode.
  auto *source = fParticleGun->GetCurrentSource();
  if (celer_offload_mode_ == "optical-gun")
  {
    auto const event_id = static_cast<std::size_t>(anEvent->GetEventID());
    if (event_id >= optical_energies.size())
    {
      return;
    }
    // Generate a primary optical photon. The tracking-manager
    // integration transfers this optical track to Celeritas.
    fParticleGun->SetParticleDefinition(
        G4OpticalPhoton::Definition());
    source->GetEneDist()->SetEnergyDisType("Mono");
    source->GetEneDist()->SetMonoEnergy(
        optical_energies[event_id]);

    source->GetPosDist()->SetPosDisType("Point");
    source->GetPosDist()->SetCentreCoords(
        G4ThreeVector(0., 0., 0.));

    source->GetAngDist()->SetAngDistType("planar");
    source->GetAngDist()->SetParticleMomentumDirection(
        G4ThreeVector(0., 0., 1.));
  }
  else if (celer_offload_mode_ == "optical-distribution" || celer_offload_mode_ == "electron-photon" || celer_offload_mode_ == "optical-track")
  {
    // Generate a charged primary. The selected mode determines
    // whether the electron, optical tracks, or optical-generation
    // distributions are sent to Celeritas.
    std::cout << "Generating primary electron for event " << anEvent->GetEventID();
    fParticleGun->SetParticleDefinition(
        G4Electron::Definition());

    source->GetEneDist()->SetEnergyDisType("Mono");
    source->GetEneDist()->SetMonoEnergy(
        electron_energy);

    source->GetAngDist()->SetAngDistType("planar");
    source->GetAngDist()->SetParticleMomentumDirection(
        G4ThreeVector(0., 0., -1.));

    source->GetPosDist()->SetPosDisType("Point");
    source->GetPosDist()->SetCentreCoords(
        G4ThreeVector(200., 300., 1000.));
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

#ifdef With_Opticks
  int tid = G4Threading::G4GetThreadId();
  if (sphotons.size() > 0)
  {
    sphotons.clear();
  }
  if ((SEventConfig::IntegrationMode() == 1))
    simPhotonCPU = false;

#endif

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
        spht.wavelength = EnergyToWavelength(fParticleGun->GetParticleEnergy()); // nm
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
    if (fVerbose)
    {
      std::cout << "[PrimaryGeneratorAction::GeneratePrimaries] Using Root for reading photon positions" << std::endl;
      std::cout << "[PrimaryGeneratorAction::GeneratePrimaries] Reading  " << fFileName << " ..." << std::endl;
    }
    TTreeReader reader("opticks/photon_gen", &file);
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
    // Explicitly close ROOT file to free memory
    file.Close();
    // Add Analysis Manager
  }
  else if (finitParticleType == "Performance")
  {
    if (fVerbose)
      std::cout << "[PrimaryGeneratorAction::GeneratePrimaries] Generating Photons for Performance Testing ... " << std::endl;
    GeneratePrimaryLinearly(anEvent);
  }

#ifdef With_Opticks

  if (sphotons.size() > 0 && tid == 0)
  {
    if (fVerbose)
      std::cout << "[PrimaryGeneratorAction::GeneratePrimaries]: Amount of Photons to simulate " << sphotons.size() << std::endl;
    OpticksHitHandler *OpticksHandler = OpticksHitHandler::getInstance();
    OpticksHandler->setPrimPhotons(std::move(sphotons));
  }
  else
  {
    if (fGPUPhotonType == "Sphoton")
      std::cout << "[PrimaryGeneratorAction::GenSphotonsPrimary]: No photons to simulate ... " << std::endl;
  }
#endif
  // anaHelper->SetStartTime(std::chrono::high_resolution_clock::now());
}

#ifdef With_Opticks
void PrimaryGeneratorAction::GenStorchPrimaries(unsigned long long N)
{
  int tid = G4Threading::G4GetThreadId();
  if (tid != 0)
    return; // Only generate photons on GPU for the single thread
  storch gs;

  gs.gentype = OpticksGenstep_TORCH;
  gs.numphoton = N;
  anaHelper->AddOpticksScintPhotons(N);
  gs.pos = make_float3(fPosition.x(), fPosition.y(), fPosition.z());
  gs.time = 0.f;

  // isotropic directions
  gs.type = T_MARSAGLIA_GAUSS;
  gs.radius = 0.f;

  // Gaussian energy
  gs.wavelength = EnergyToWavelength(fMom);                  // mean energy (nm)
  gs.weight = EnergySigmaToWavelengthSigma(fMom, fSigmaMom); // sigma (nm)

  // unused
  gs.zenith = make_float2(0.f, 1.f);
  gs.azimuth = make_float2(0.f, 1.f);
  qtorch qt;
  qt.t = gs; // fill the storch view
  SEvt::AddGenstep(qt.q);
  std::cout << "[PrimaryGeneratorAction::GenStorchPrimaries] Generated " << N << " photons on GPU with Storch " << std::endl;
}

// Generates photons 10x slower than GenStorchPrimaries, but allows for more control over the photon properties (position, momentum, polarization, energy)
void PrimaryGeneratorAction::GenSphotonsPrimary(PrimaryPhoton &pht)
{
  int tid = G4Threading::G4GetThreadId();
  if (tid != 0)
    return; // Only generate photons on GPU for the single thread
  if (SEventConfig::IntegrationMode() == 1 || SEventConfig::IntegrationMode() == 3)
  {
    anaHelper->AddOpticksScintPhotons(1);
    // std::cout << "[PrimaryGeneratorAction::GeneratePrimaries] Simulating Photons in GPU for Event ID "<<*fevtID << std::endl;
    sphoton spht;
    spht.zero();
    spht.zero_flags();
    spht.set_flag(TORCH);
    spht.pos = make_float3(fPosition.x(), fPosition.y(), fPosition.z()); // mm
    spht.mom = make_float3(pht.mx, pht.my, pht.mz);
    spht.pol = make_float3(pht.px, pht.py, pht.pz);

    spht.wavelength = EnergyToWavelength(pht.e); // nm
    spht.time = 0;                               // ns
    sphotons.push_back(spht);
  }
  else
  {
    std::cout << "[PrimaryGeneratorAction::GenSphotonsPrimary] Opticks is not enabled, cannot generate photons on GPU" << std::endl;
  }
}
#endif

std::vector<G4double> PrimaryGeneratorAction::linspace(G4double start, G4double end, G4int num, G4int factor)
{
  std::vector<G4double> values;
  values.reserve(num);

  if (num == 1)
  {
    values.push_back(start * factor);
    return values;
  }

  G4double step = (end - start) / (num - 1);

  for (int i = 0; i < num; ++i)
  {
    values.push_back((start + step * i) * factor);
  }
  return values;
}
void PrimaryGeneratorAction::SinglePhotonGenerator(G4PrimaryVertex *vertex, PrimaryPhoton &pht)
{
  if (simPhotonCPU)
  {
    // std::cout << "[PrimaryGeneratorAction::GeneratePrimaries] Simulating Photons in Geant4 for Event ID "<<anEvent->GetEventID() << std::endl;
    anaHelper->AddG4ScintPhotons(1);
    // std::cout << "[PrimaryGeneratorAction::GeneratePrimaries] Simulating Photons in Geant4 for Event ID "<<*fevtID << std::endl;
    G4PrimaryParticle *particle = new G4PrimaryParticle(G4OpticalPhoton::Definition());

    particle->SetKineticEnergy(pht.e);
    particle->SetMomentumDirection(G4ThreeVector(pht.mx, pht.my, pht.mz));
    particle->SetPolarization(G4ThreeVector(pht.px, pht.py, pht.pz));
    vertex->SetPrimary(particle);
  }
#ifdef With_Opticks
  if (fGPUPhotonType == "Sphoton")
    GenSphotonsPrimary(pht);
#endif
}

void PrimaryGeneratorAction::GeneratePrimaryLinearly(G4Event *anEvent)
{

  G4int evtID = anEvent->GetEventID();
  std::vector<G4double> NPhotons = linspace(fPhotonAmount.x(), fPhotonAmount.y(), fPhotonAmount.z(), 1e6); // ToDo passs this from macro .
  // repeat each index fAmount times
  unsigned idx = evtID / fAmount;

  // clamp to last index (10)
  if (idx >= NPhotons.size())
    idx = NPhotons.size() - 1;

  unsigned long long N = NPhotons[idx];
  G4ThreeVector mdir, pdir, n;
  PrimaryPhoton pht;

  // std::cout << "Simulating " << N << " photons for Event ID " << evtID << std::endl;

#ifdef With_Opticks

  if (fGPUPhotonType == "Storch")
    GenStorchPrimaries(N);
  else
    sphotons.reserve(N);

#endif

  int nthreads = G4MTRunManager::GetMasterRunManager()->GetNumberOfThreads();
  if (fVerbose)
    std::cout << "number of threads " << nthreads << std::endl;
  if (nthreads > 1)
  {

    static std::atomic<unsigned long long> gSeq{0};
    unsigned long long seq = gSeq.fetch_add(1);

    unsigned long long repsPerBatch = nthreads * fAmount;
    unsigned batchIdx = seq / repsPerBatch;
    if (batchIdx >= NPhotons.size())
      batchIdx = NPhotons.size() - 1;
    // N = NPhotons[batchIdx]/nthreads;

    unsigned long long total = NPhotons[batchIdx];
    unsigned long long base = total / nthreads;
    unsigned long long rem = total % nthreads;

    unsigned ftid = G4Threading::G4GetThreadId();
    N = base + (ftid < rem ? 1 : 0);

    anaHelper->SetBatchID(batchIdx);
    if (fVerbose)
      std::cout << "Event ID " << evtID << " Batch " << batchIdx << " seq " << seq << " simulating " << N << " photons on thread " << G4Threading::G4GetThreadId() << " G4_Sim " << simPhotonCPU << std::endl;
  }

  G4PrimaryVertex *vertex = nullptr;
  if (simPhotonCPU)
    vertex = new G4PrimaryVertex(fPosition, 0);
  for (unsigned long long i = 0; i < N; ++i)
  {

    mdir = G4RandomDirection(); // isotropic
    // Build a random polarization perpendicular to mdir
    n = mdir.orthogonal(); // any perpendicular vector
    n = n.unit();

    double phi = CLHEP::twopi * G4UniformRand(); // random phase
    pdir = std::cos(phi) * n + std::sin(phi) * (mdir.cross(n)).unit();

    pht.mx = mdir.x();
    pht.my = mdir.y();
    pht.mz = mdir.z();
    pht.px = pdir.x();
    pht.py = pdir.y();
    pht.pz = pdir.z();
    pht.e = G4RandGauss::shoot(fMom, fSigmaMom);
    SinglePhotonGenerator(vertex, pht);
  }
  if (simPhotonCPU)
    anEvent->AddPrimaryVertex(vertex);
}
