//
// Created by ilker on 10/22/25.
//

#ifndef GDMLOPTICKS_EVENTACTION_HH
#define GDMLOPTICKS_EVENTACTION_HH

#include "G4UserEventAction.hh"
#include "globals.hh"
#include <string>
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
    chrono::time_point<chrono::high_resolution_clock> startTime;
    std::string celer_offload_mode_;
};

#endif // GDMLOPTICKS_EVENTACTION_HH