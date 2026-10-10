//
// Created by ilker on 10/22/25.
//

#ifndef GDMLOPTICKS_EVENTACTION_HH
#define GDMLOPTICKS_EVENTACTION_HH

#include "G4UserEventAction.hh"
#include "globals.hh"
#include <cstddef>
#include <string>
#include <chrono>
class G4Event;

using namespace std;
class EventAction : public G4UserEventAction
{
public:
    EventAction(std::string celer_offload_mode);
    ~EventAction();
    void BeginOfEventAction(const G4Event *) override;
    void EndOfEventAction(const G4Event *) override;

private:
    using Clock = std::chrono::steady_clock;
    Clock::time_point startTime;
    std::string celer_offload_mode_;
    std::size_t celer_optical_tracks_at_event_start_{0};
};

#endif // GDMLOPTICKS_EVENTACTION_HH
