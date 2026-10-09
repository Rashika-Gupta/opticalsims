//
// Created by ilker on 10/22/25.
//

#include "EventAction.hh"
#include "RunAction.hh"

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
#include <accel/detail/IntegrationSingleton.hh>
#include <accel/LocalTransporter.hh>
#include <celeritas/optical/OpticalCollector.hh>
#include <celeritas/optical/CoreState.hh>
#include <celeritas/global/CoreState.hh>
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
    startTime = Clock::now();
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
}

void EventAction::EndOfEventAction(const G4Event *event)
{
#ifdef With_Opticks
    G4int evtID = event->GetEventID();
#endif
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
    // Save Opticks Hits
#ifdef With_Opticks
    hitHandler->SaveHits();
#endif

    if (celer_offload_mode_ == "optical-distribution")
    {
        celeritas::UserActionIntegration::Instance()
            .EndOfEventAction(event);
    }

    // Include the Celeritas end-of-event flush in PhotonInfo.Time.
    const double eventTime =
        std::chrono::duration<double>(Clock::now() - startTime).count();
    anaHelper->SetDuration(eventTime);
    anaHelper->SavePhotonInfotoFile();

    if (anaHelper->HasCelerHits())
    {
        anaHelper->SaveCelerHitsToFile();
    }
    /////// GEANT4 HITS ///////
    else
    {
        anaHelper->SaveG4HitsToFile();
    }
    // Include the end-of-event flush and hit saving in the JSON event time.
    RunAction::RecordEventTiming(
        event->GetEventID(),
        std::chrono::duration<double>(Clock::now() - startTime).count());
}
