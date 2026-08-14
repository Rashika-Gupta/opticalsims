//
// Created by ilker on 11/5/25.
//

#include <ArapucaHit.hh>

#include "G4Threading.hh"
#include "G4AutoLock.hh"
#include "G4ThreeVector.hh"
#include "celeritas/optical/DetectorData.hh"
#ifndef GDMLOPTICKS_ANALYSISMANAGERHELPER_HH
#define GDMLOPTICKS_ANALYSISMANAGERHELPER_HH

#pragma once

class AnalysisManagerHelper;
extern thread_local std::unique_ptr<AnalysisManagerHelper> anaHelper;
class G4Step;
struct CelerOpticalHit
{
    int celer_detector_id;
    int sensor_id;
    int event_id;
    int primary_id;

    int volume_instance_id;
    std::uint64_t unique_instance_id;
    std::string sensor_name;

    double x, y, z, t;
    double energy_mev;
    double wavelength_nm;
};

class AnalysisManagerHelper
{
public:
    AnalysisManagerHelper();
    ~AnalysisManagerHelper();

    G4int GetG4ScintPhotons();
    G4int GetOpticksScintPhotons();
    G4int GetG4CerenkovPhotons();
    G4int GetOpticksCerenkovPhotons();
    G4int GetDuration();
    const std::map<G4String, G4int> &GetDetectIds();

    void AddG4ScintPhotons(G4int ph);
    void AddOpticksScintPhotons(G4int ph);
    void AddG4CerenkovPhotons(G4int ph);
    void AddOpticksCerenkovPhotons(G4int ph);
    void SetDuration(G4double dr);
    void SavePhotonInfotoFile();
    void SaveG4HitsToFile();
    void SetDetectIds(const std::map<G4String, G4int> &fIDs);
    void SetBatchID(G4int id) { fbatchID = id; };
    void AddG4Hits(ArapucaHit &hit);
    void SaveParticleSteps(const G4Step *step);
    // void SetStartTime(std::chrono::high_resolution_clock::time_point time){fStartTime=time;};
    // std::chrono::high_resolution_clock::time_point GetStartTime(){return fStartTime;};
    void Reset();
    // Celeritas hits management
    void AddCelerHits(std::vector<CelerOpticalHit> const &hits);

    void SaveCelerHitsToFile();
    bool HasCelerHits() const { return !fCelerHits.empty(); }
    void ResetCelerHits();

private:
    G4int G4CerenkovPhotons{0};
    G4int OpticksCerenkovPhotons{0};
    G4int G4ScintPhotons{0};
    G4int OpticksScintPhotons{0};
    G4double Duration{0};
    std::map<G4String, G4int> fDetectIds;
    std::vector<ArapucaHit> ArapucaHits{};
    std::vector<CelerOpticalHit> fCelerHits;
    std::chrono::high_resolution_clock::time_point fStartTime;
    G4int fbatchID;
};

inline void AnalysisManagerHelper::SetDetectIds(const std::map<G4String, G4int> &fIDs)
{
    fDetectIds = fIDs;
}
inline void AnalysisManagerHelper::AddG4Hits(ArapucaHit &hit)
{
    ArapucaHits.push_back(hit);
}
// Celeritas hits management
inline void AnalysisManagerHelper::AddCelerHits(std::vector<CelerOpticalHit> const &hits)
{

    fCelerHits.insert(fCelerHits.end(), hits.begin(), hits.end());
}
inline void AnalysisManagerHelper::ResetCelerHits()
{

    fCelerHits.clear();
}
inline const std::map<G4String, G4int> &AnalysisManagerHelper::GetDetectIds()
{
    return fDetectIds;
}
#endif // GDMLOPTICKS_ANALYSISMANAGERHELPER_HH