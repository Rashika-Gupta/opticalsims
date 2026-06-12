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
struct CelerOpticalHit
{
    int detector_id;
    int event_id;
    float x, y, z, t;
    float energy_mev;
    float wavelength_nm;
};

class AnalysisManagerHelper
{
public:
    static AnalysisManagerHelper *getInstance()
    {
        G4AutoLock lock(&mtx);
        if (instance == nullptr)
        {
            instance = new AnalysisManagerHelper();
        }
        return instance;
    }

    G4int GetG4ScintPhotons();
    G4int GetOpticksScintPhotons();
    G4int GetG4CerenkovPhotons();
    G4int GetOpticksCerenkovPhotons();
    G4int GetDuration();
    std::map<G4String, G4int> *GetDetectIds();

    void AddG4ScintPhotons(G4int ph);
    void AddOpticksScintPhotons(G4int ph);
    void AddG4CerenkovPhotons(G4int ph);
    void AddOpticksCerenkovPhotons(G4int ph);
    void SetDuration(G4double dr);
    void SavePhotonInfotoFile();
    void SaveG4HitsToFile();
    void SetDetectIds(std::map<G4String, G4int> *fIDs);
    void AddG4Hits(ArapucaHit hit);

    void Reset();
    // Celeritas hits management
    void AddCelerHits(std::vector<CelerOpticalHit> const &hits)
    {
        G4AutoLock lock(&mtx);
        fCelerHits.insert(fCelerHits.end(), hits.begin(), hits.end());
    }
    void SaveCelerHitsToFile();
    bool HasCelerHits() const { return !fCelerHits.empty(); }
    void ResetCelerHits()
    {
        fCelerHits.clear();
        fCelerHits.shrink_to_fit();
    }

private:
    AnalysisManagerHelper() {};
    static G4Mutex mtx;
    static AnalysisManagerHelper *instance;
    G4int G4CerenkovPhotons{0};
    G4int OpticksCerenkovPhotons{0};
    G4int G4ScintPhotons{0};
    G4int OpticksScintPhotons{0};
    G4double Duration{0};
    std::map<G4String, G4int> *fDetectIds{nullptr};
    std::vector<ArapucaHit> ArapucaHits{};
    std::vector<CelerOpticalHit> fCelerHits;
};

inline void AnalysisManagerHelper::SetDetectIds(std::map<G4String, G4int> *fIDs)
{
    fDetectIds = fIDs;
}
inline void AnalysisManagerHelper::AddG4Hits(ArapucaHit hit)
{
    ArapucaHits.push_back(hit);
}

inline std::map<G4String, G4int> *AnalysisManagerHelper::GetDetectIds()
{
    return fDetectIds;
}
#endif // GDMLOPTICKS_ANALYSISMANAGERHELPER_HH