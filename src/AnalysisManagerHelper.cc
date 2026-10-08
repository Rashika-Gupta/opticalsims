//
// Created by ilker on 11/5/25.
//

#include "AnalysisManagerHelper.hh"
#include "G4AnalysisManager.hh"
#include "G4RunManager.hh"
#include "G4Threading.hh"
#include "G4AutoLock.hh"
namespace
{
    G4Mutex FileMutex = G4MUTEX_INITIALIZER;
}
thread_local std::unique_ptr<AnalysisManagerHelper> anaHelper = nullptr;
AnalysisManagerHelper::AnalysisManagerHelper()
{
    Reset();
}
AnalysisManagerHelper::~AnalysisManagerHelper()
{
}

G4int AnalysisManagerHelper::GetG4ScintPhotons()
{
    return G4ScintPhotons;
}

G4int AnalysisManagerHelper::GetOpticksScintPhotons()
{
    return OpticksScintPhotons;
}

G4int AnalysisManagerHelper::GetG4CerenkovPhotons()
{
    return G4CerenkovPhotons;
}

G4int AnalysisManagerHelper::GetOpticksCerenkovPhotons()
{
    return OpticksCerenkovPhotons;
}

G4int AnalysisManagerHelper::GetDuration()
{
    return Duration;
}

void AnalysisManagerHelper::AddG4ScintPhotons(G4int ph)
{
    G4ScintPhotons += ph;
}
void AnalysisManagerHelper::AddG4CerenkovPhotons(G4int ph)
{
    G4CerenkovPhotons += ph;
}

void AnalysisManagerHelper::AddOpticksScintPhotons(G4int ph)
{
    OpticksScintPhotons += ph;
}

void AnalysisManagerHelper::AddOpticksCerenkovPhotons(G4int ph)
{
    OpticksCerenkovPhotons += ph;
}

void AnalysisManagerHelper::SetDuration(G4double dr)
{
    Duration = dr;
}

void AnalysisManagerHelper::Reset()
{
    Duration = 0;
    G4CerenkovPhotons = 0;
    OpticksScintPhotons = 0;
    OpticksCerenkovPhotons = 0;
    OpticksScintPhotons = 0;
    G4CerenkovPhotons = 0;
    G4ScintPhotons = 0;
    fbatchID = 0;
    ArapucaHits.clear();
    ArapucaHits.shrink_to_fit();
    ResetCelerHits();
}
void AnalysisManagerHelper::SavePhotonInfotoFile()
{
    G4AutoLock lock(&FileMutex);
    G4AnalysisManager *AnaMngr = G4AnalysisManager::Instance();
    auto run = G4RunManager::GetRunManager();
    G4int eventID = run->GetCurrentEvent()->GetEventID();
    AnaMngr->FillNtupleIColumn(5, 0, G4ScintPhotons);
    AnaMngr->FillNtupleIColumn(5, 1, G4CerenkovPhotons);
    AnaMngr->FillNtupleIColumn(5, 2, OpticksScintPhotons);
    AnaMngr->FillNtupleIColumn(5, 3, OpticksCerenkovPhotons);
    AnaMngr->FillNtupleDColumn(5, 4, Duration);
    AnaMngr->FillNtupleIColumn(5, 5, eventID);
    AnaMngr->FillNtupleIColumn(5, 6, fbatchID);
    AnaMngr->AddNtupleRow(5);
}

void AnalysisManagerHelper::SaveG4HitsToFile()
{
    G4AutoLock lock(&FileMutex);
    G4AnalysisManager *AnaMngr = G4AnalysisManager::Instance();
    auto run = G4RunManager::GetRunManager();
    for (auto hit : ArapucaHits)
    {
        AnaMngr->FillNtupleIColumn(2, 0, run->GetCurrentEvent()->GetEventID());
        AnaMngr->FillNtupleIColumn(2, 1, hit.GetSid());
        AnaMngr->FillNtupleSColumn(2, 2, hit.GetDetName());
        AnaMngr->FillNtupleDColumn(2, 3, hit.GetPos().getX());
        AnaMngr->FillNtupleDColumn(2, 4, hit.GetPos().getY());
        AnaMngr->FillNtupleDColumn(2, 5, hit.GetPos().getZ());
        AnaMngr->FillNtupleDColumn(2, 6, hit.GetTime());
        AnaMngr->FillNtupleDColumn(2, 7, hit.GetWave());
        AnaMngr->FillNtupleIColumn(2, 8, hit.GetPid());
        AnaMngr->AddNtupleRow(2);
    }
    // std::cout << "G4Sim Event ID "<< run->GetCurrentEvent()->GetEventID() << " Saved " << ArapucaHits.size() << " hits to file" << std::endl;
    ArapucaHits.clear();
    ArapucaHits.shrink_to_fit();
}

void AnalysisManagerHelper::SaveG4SensitiveDetectorHitToFile(
    ArapucaHit &hit)
{
    G4AutoLock lock(&FileMutex);

    auto *analysisManager = G4AnalysisManager::Instance();
    auto *runManager = G4RunManager::GetRunManager();

    constexpr G4int ntupleId = 4;

    analysisManager->FillNtupleIColumn(
        ntupleId, 0,
        runManager->GetCurrentEvent()->GetEventID());
    analysisManager->FillNtupleIColumn(ntupleId, 1, hit.GetSid());
    analysisManager->FillNtupleSColumn(ntupleId, 2, hit.GetDetName());
    analysisManager->FillNtupleDColumn(ntupleId, 3, hit.GetPos().x());
    analysisManager->FillNtupleDColumn(ntupleId, 4, hit.GetPos().y());
    analysisManager->FillNtupleDColumn(ntupleId, 5, hit.GetPos().z());
    analysisManager->FillNtupleDColumn(ntupleId, 6, hit.GetTime());
    analysisManager->FillNtupleDColumn(ntupleId, 7, hit.GetWave());
    analysisManager->FillNtupleIColumn(ntupleId, 8, hit.GetPid());
    analysisManager->AddNtupleRow(ntupleId);
}

void AnalysisManagerHelper::SaveParticleSteps(const G4Step *step)
{
    G4AutoLock lock(&FileMutex);
    G4AnalysisManager *AnaMngr = G4AnalysisManager::Instance();
    auto run = G4RunManager::GetRunManager();
    G4int eventID = run->GetCurrentEvent()->GetEventID();
    int id = 5;
    AnaMngr->FillNtupleIColumn(id, 0, eventID);
    AnaMngr->FillNtupleIColumn(id, 1, step->GetTrack()->GetTrackID());
    AnaMngr->FillNtupleDColumn(id, 2, step->GetTrack()->GetPosition().getX());
    AnaMngr->FillNtupleDColumn(id, 3, step->GetTrack()->GetPosition().getY());
    AnaMngr->FillNtupleDColumn(id, 4, step->GetTrack()->GetPosition().getZ());
    AnaMngr->FillNtupleDColumn(id, 5, step->GetTrack()->GetGlobalTime());
    AnaMngr->FillNtupleSColumn(id, 6, step->GetPreStepPoint()->GetPhysicalVolume()->GetName());
    AnaMngr->FillNtupleSColumn(id, 7, step->GetPostStepPoint()->GetPhysicalVolume()->GetName());
    AnaMngr->FillNtupleIColumn(id, 8, step->GetTrack()->GetTrackStatus());
    AnaMngr->FillNtupleDColumn(id, 9, step->GetTrack()->GetMomentumDirection().getX());
    AnaMngr->FillNtupleDColumn(id, 10, step->GetTrack()->GetMomentumDirection().getY());
    AnaMngr->FillNtupleDColumn(id, 11, step->GetTrack()->GetMomentumDirection().getZ());
    AnaMngr->FillNtupleDColumn(id, 12, step->GetTrack()->GetPolarization().getX());
    AnaMngr->FillNtupleDColumn(id, 13, step->GetTrack()->GetPolarization().getY());
    AnaMngr->FillNtupleDColumn(id, 14, step->GetTrack()->GetPolarization().getZ());
    AnaMngr->FillNtupleDColumn(id, 15, step->GetTrack()->GetKineticEnergy());
    AnaMngr->AddNtupleRow(id);
}
// Save Celeritas hits
void AnalysisManagerHelper::SaveCelerHitsToFile()
{
    G4AnalysisManager *AnaMngr = G4AnalysisManager::Instance();
    G4cout << "Saving " << fCelerHits.size() << " Celeritas hits to file..." << G4endl;
    for (auto const &hit : fCelerHits)
    {

        AnaMngr->FillNtupleIColumn(3, 0, hit.event_id);
        AnaMngr->FillNtupleIColumn(3, 1, hit.sensor_id);   // SensorID
        AnaMngr->FillNtupleSColumn(3, 2, hit.sensor_name); // SensorName
        AnaMngr->FillNtupleDColumn(3, 3, hit.x);
        AnaMngr->FillNtupleDColumn(3, 4, hit.y);
        AnaMngr->FillNtupleDColumn(3, 5, hit.z);
        AnaMngr->FillNtupleDColumn(3, 6, hit.t);
        AnaMngr->FillNtupleDColumn(3, 7, hit.wavelength_nm);
        //        AnaMngr->FillNtupleIColumn(3, 8, hit.track_id);
        //        AnaMngr->FillNtupleIColumn(3, 9, hit.num_steps);
        //        AnaMngr->FillNtupleDColumn(3, 10, hit.path_length);
        AnaMngr->AddNtupleRow(3);
    }
    ResetCelerHits();
}