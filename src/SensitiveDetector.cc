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
/// \file persistency/gdml//src/SensitiveDetector.cc
/// \brief Implementation of the SensitiveDetector class
//
//
//

#include "SensitiveDetector.hh"
#include "G4HCofThisEvent.hh"
#include "G4Step.hh"
#include "G4ThreeVector.hh"
#include "G4SDManager.hh"
#include "G4ios.hh"
#include "G4AnalysisManager.hh"
#include "G4OpticalPhoton.hh"
#include "G4Track.hh"
#include "G4VHit.hh"
#include "G4VProcess.hh"
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

SensitiveDetector::SensitiveDetector(const G4String &name)
    : G4VSensitiveDetector(name)
{
    G4String name_HC = name + "_HitCollection";
    collectionName.insert(name_HC);
    G4cout << collectionName.size() << " Detector name:  " << name
           << " collection Name: " << name_HC << G4endl;
    fHCid = -1;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

SensitiveDetector::~SensitiveDetector()
{
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void SensitiveDetector::Initialize(G4HCofThisEvent *G4hc)
{
    fArapucaHitsCollection = new ArapucaHitsCollection(SensitiveDetectorName, collectionName[0]);
    if (fHCid < 0)
    {
        fHCid = G4SDManager::GetSDMpointer()->GetCollectionID(collectionName[0]);
    }
    G4hc->AddHitsCollection(fHCid, fArapucaHitsCollection);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4bool SensitiveDetector::ProcessHits(G4Step *aStep, G4TouchableHistory *th)
{

    // Only Optical Photons
    auto aTrack = aStep->GetTrack();

    if (aTrack->GetParticleDefinition() != G4OpticalPhoton::OpticalPhoton())
        return false;
    // auto analysisManager = G4AnalysisManager::Instance();

    G4String detectName = aStep->GetPreStepPoint()->GetPhysicalVolume()->GetName();
    G4ThreeVector PPosition = aTrack->GetPosition();
    G4ThreeVector PMomentDir = aTrack->GetMomentumDirection();
    G4ThreeVector PPolar = aTrack->GetPolarization();
    G4double time = aTrack->GetGlobalTime();

    G4double Wavelength = EtoWavelengthNM(aTrack->GetTotalEnergy() / CLHEP::eV);
    G4String processName;
    G4int Procid = -1;
    G4int Sid = -1;
    auto it = fDetectIds->find(detectName);
    if (it != fDetectIds->end())
    {
        Sid = it->second;
    }
    assert(Sid != -1);
    const G4VProcess *proc = aTrack->GetCreatorProcess();

    if (proc != NULL)
        processName = proc->GetProcessName();
    else
        processName = "None";
    if (processName.compare("Scintillation") == 0)
        Procid = 0;
    else if (processName.compare("Cerenkov") == 0)
        Procid = 1;

    ArapucaHit *Hit = new ArapucaHit(Procid, Sid, detectName, Wavelength, time, PPosition, PMomentDir, PPolar);
    fArapucaHitsCollection->insert(Hit);
    aTrack->SetTrackStatus(fStopAndKill);

    return false;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void SensitiveDetector::EndOfEvent(G4HCofThisEvent *)
{
}
