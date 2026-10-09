//
// Created by ilker on 10/22/25.
//

#ifndef GDMLOPTICKS_RUNACTION_HH
#define GDMLOPTICKS_RUNACTION_HH

#include "G4UserRunAction.hh"
#include <chrono>
#include <cstddef>
#include <vector>
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
  using TimePoint = std::chrono::steady_clock::time_point;

  // Application-level timestamps. The JSON writer computes durations.
  struct ProcessTiming
  {
    TimePoint gdml_start;
    TimePoint gdml_end;
    TimePoint init_start;
    TimePoint init_end;
  };

  static void WriteTimingJson(const ProcessTiming &,
                              const std::string &offload_mode,
                              bool celeritas_enabled);
  static void RecordEventTiming(int event_id, double elapsed_s);
  static void RecordCelerHitCallbackTiming(double elapsed_s, std::size_t hits);

  // Construct
  RunAction(std::string celer_offload_mode);
  // Destruct
  ~RunAction();
  void BeginOfRunAction(const G4Run *) override;
  void EndOfRunAction(const G4Run *) override;
  vector<double> RunTimes;
  vector<double> NPhotons;
  TimePoint startTime;
  G4GenericMessenger *fmsg;
  G4String fFileName;

private:
  std::string celer_offload_mode_;
  void RecordRunTiming(const G4Run *run, TimePoint run_end) const;
};

#endif // GDMLOPTICKS_RUNACTION_HH
