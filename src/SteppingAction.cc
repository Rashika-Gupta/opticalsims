//
// Created by ilker on 10/22/25.
//
#include "G4OpticalPhoton.hh"
#include "G4OpBoundaryProcess.hh"
#include "G4ProcessManager.hh"
#include "SteppingAction.hh"
#include "ArapucaHit.hh"
#include "G4Exception.hh"
#include "include/config.h"
SteppingAction::SteppingAction() : G4UserSteppingAction()
{
    fDetectIds = anaHelper->GetDetectIds();
}

SteppingAction::~SteppingAction()
{
}

void SteppingAction::UserSteppingAction(const G4Step *step)
{
    auto aTrack = step->GetTrack();
    G4ParticleDefinition *pdef = aTrack->GetDefinition();

#ifdef With_DEBUG
    // Collect the steps
    anaHelper->SaveParticleSteps(step);
#endif

    // Early exit for non-optical photons
    if (pdef != G4OpticalPhoton::Definition())
        return;

    // Get boundary process (cached)
    G4OpBoundaryProcess *boundary = GetOpticalBoundaryProcess();

    if (!boundary)
        return;

    G4OpBoundaryProcessStatus status = boundary->GetStatus();

    if (status == Detection)
    {
        G4int Procid = -1;
        const G4VProcess *proc = aTrack->GetCreatorProcess();

        if (proc != nullptr)
        {
            G4String processName = proc->GetProcessName();
            if (processName == "Scintillation")
                Procid = 0;
            else if (processName == "Cerenkov")
                Procid = 1;
            // add more if needed
        }
        const G4StepPoint *postPoint = step->GetPostStepPoint();
        G4String volName = postPoint->GetPhysicalVolume()->GetName();

        auto it = fDetectIds.find(volName);
        if (it == fDetectIds.end())
            return;

        int Sid = it->second;

        // Collect hit data
        ArapucaHit Hit(
            Procid,
            Sid,
            volName,
            EtoWavelength(aTrack->GetTotalEnergy() / CLHEP::eV),
            aTrack->GetGlobalTime(),
            aTrack->GetPosition(),
            aTrack->GetMomentumDirection(),
            aTrack->GetPolarization());

        anaHelper->AddG4Hits(Hit);
    }
}

// Helper method
G4OpBoundaryProcess *SteppingAction::GetOpticalBoundaryProcess()
{
    G4OpBoundaryProcess *boundary = nullptr;
    if (!boundary)
    {
        G4ProcessVector *pv = G4OpticalPhoton::Definition()
                                  ->GetProcessManager()
                                  ->GetProcessList();
        for (size_t i = 0; i < pv->size(); ++i)
        {
            if ((*pv)[i]->GetProcessName() == "OpBoundary")
            {
                boundary = dynamic_cast<G4OpBoundaryProcess *>((*pv)[i]);
                break;
            }
        }
    }
    return boundary;
}