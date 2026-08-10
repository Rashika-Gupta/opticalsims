//
// Created by ilker on 10/22/25.
//

#ifndef GDMLOPTICKS_RUNACTION_HH
#define GDMLOPTICKS_RUNACTION_HH

#include "G4UserRunAction.hh"
#include "chrono"
#include "vector"
#include "G4GenericMessenger.hh"

#include <accel/ExceptionConverter.hh>
#include <accel/TrackingManagerIntegration.hh>
#include <accel/detail/IntegrationSingleton.hh>
#include <celeritas/global/CoreParams.hh>
#include <celeritas/global/CoreState.hh>
#include <celeritas/optical/CoreState.hh>
#include <celeritas/optical/OpticalCollector.hh>
#include <corecel/io/Logger.hh>
#include <corecel/io/OutputRegistry.hh>
#include <nlohmann/json.hpp>
#include <accel/UserActionIntegration.hh>
#include <corecel/sys/Environment.hh>
using namespace std;
#include <string>
class RunAction : public G4UserRunAction
{
public:
  // Construct
  RunAction(std::string celer_offload_mode);
  // Destruct
  ~RunAction();
  void BeginOfRunAction(const G4Run *) override;
  void EndOfRunAction(const G4Run *) override;
  vector<double> RunTimes;
  vector<double> NPhotons;
  double RunTime;
  chrono::time_point<chrono::high_resolution_clock> startTime;
  G4GenericMessenger *fmsg;
  G4String fFileName;

private:
  std::string celer_offload_mode_;
};

#endif // GDMLOPTICKS_RUNACTION_HH