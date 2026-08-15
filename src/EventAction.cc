//
// Created by ilker on 10/22/25.
//

#include "EventAction.hh"

#include <G4AnalysisManager.hh>
#include <G4AutoLock.hh>
#include "G4Event.hh"
#include "include/config.h"
#include "globals.hh"
#include "G4Threading.hh"
#include "SensitiveDetector.hh"
#include "ArapucaHit.hh"
#include "AnalysisManagerHelper.hh"

#include "include/config.h"
#include <accel/UserActionIntegration.hh>
#include <corecel/sys/Environment.hh>

#ifdef With_Opticks
#include "SEvt.hh"
#include "G4CXOpticks.hh"
#include "Opticks/OpticksHitHandler.hh"
namespace
{
    G4Mutex opticks_mt = G4MUTEX_INITIALIZER;
}
#endif

EventAction::EventAction(std::string celer_offload_mode) : G4UserEventAction(), celer_offload_mode_(std::move(celer_offload_mode)) {}
EventAction::~EventAction() {}

void EventAction::BeginOfEventAction(const G4Event *event)
{
    if (!anaHelper)
    {
        anaHelper = std::make_unique<AnalysisManagerHelper>();
    }

    anaHelper->Reset();

    if (celer_offload_mode_ == "optical-distribution")
    {
        celeritas::UserActionIntegration::Instance()
            .BeginOfEventAction(event);
    }
    startTime = std::chrono::high_resolution_clock::now();
    cout << "Begin event " << event->GetEventID() << endl;
}

void EventAction::EndOfEventAction(const G4Event *event)
{
    G4int evtID = event->GetEventID();
    // auto analysisManager = G4AnalysisManager::Instance();

#ifdef With_Opticks
    // Force Single Thread
    G4AutoLock lock(&opticks_mt);
    OpticksHitHandler *hitHandler = OpticksHitHandler::getInstance();

    // Adding here the photon production
    int numSPhotons = hitHandler->GetSphotons().size();

    // Simulate the Primary photons in GPU
    if (numSPhotons > 0)
        hitHandler->PrimPhotonBatcher(evtID);

    // Get event id and number of gensteps
    G4int ngenstep = SEvt::GetNumGenstepFromGenstep(0);

    if (ngenstep > 0)
    {
        std::cout << "Number of GenStep: " << ngenstep << std::endl;
        std::cout << "Number of Photons: " << SEvt::GetNumPhotonCollected(0) << std::endl;
        hitHandler->Simulate(evtID);
    }
#endif
    // Instance for AnalysisHelper
    auto duration = chrono::high_resolution_clock::now() - startTime;
    auto EventTime = chrono::duration_cast<chrono::duration<double>>(duration).count();

    // Save Opticks Hits
#ifdef With_Opticks
    hitHandler->SaveHits();
#endif

    // Save Photon Computation Time
    anaHelper->SetDuration(EventTime);

    // Save Photon info
    anaHelper->SavePhotonInfotoFile();

    if (anaHelper->HasCelerHits())
    {
        G4cout << "Saving Celeritas hits" << G4endl;
        anaHelper->SaveCelerHitsToFile();
    }
    /////// GEANT4 HITS ///////
    else
    {
        G4cout << "Calling G4 Hits" << G4endl;
        anaHelper->SaveG4HitsToFile();
    }

    if (celer_offload_mode_ == "optical-distribution")
    {
        celeritas::UserActionIntegration::Instance()
            .EndOfEventAction(event);
    }

    G4cout << "Event " << evtID << ", End Time " << EventTime << " seconds" << G4endl;
}
