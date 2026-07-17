//
// Created by ilker on 10/22/25.
//
#include "G4OpticalPhoton.hh"
#include "G4OpBoundaryProcess.hh"
#include "G4ProcessManager.hh"
#include "SteppingAction.hh"
#include "ArapucaHit.hh"
#include "G4Exception.hh"
#include "nlohmann/json.hpp"
#include <fstream>
#include <unistd.h>
SteppingAction::SteppingAction() : G4UserSteppingAction(), anaHelper(AnalysisManagerHelper::getInstance())
{
}

SteppingAction::~SteppingAction()
{
}

void SteppingAction::UserSteppingAction(const G4Step *step)
{

    auto aTrack = step->GetTrack();
    G4ParticleDefinition *pdef = step->GetTrack()->GetDefinition();
    if (pdef != G4OpticalPhoton::Definition())
    {
        return;
    }

    G4OpBoundaryProcess *boundary = nullptr;

    if (!boundary)
    {
        // the pointer is not defined yet
        // Get the list of processes defined for the optical photon
        // and loop through it to find the optical boundary process.
        G4ProcessVector *pv = pdef->GetProcessManager()->GetProcessList();
        for (size_t i = 0; i < pv->size(); i++)
        {
            if ((*pv)[i]->GetProcessName() == "OpBoundary")
            {
                boundary = (G4OpBoundaryProcess *)(*pv)[i];
                break;
            }
        }
    }
    else
    {
        return;
    }

    G4OpBoundaryProcessStatus status = boundary->GetStatus();

    // Recording the optical photon only if it is detected by the detector
    // could be changed by changing the status.

    if (pdef == G4OpticalPhoton::Definition())
    {
        G4String PredetectName = step->GetPreStepPoint()->GetPhysicalVolume()->GetName();
        G4String PostdetectName = step->GetPostStepPoint()->GetPhysicalVolume()->GetName();

        G4ThreeVector PPosition = aTrack->GetPosition();
        G4ThreeVector PMomentDir = aTrack->GetMomentumDirection();
        G4ThreeVector PPolar = aTrack->GetPolarization();
        G4double time = aTrack->GetGlobalTime();

        G4double Wavelength = EtoWavelength(aTrack->GetTotalEnergy() / CLHEP::eV);
        const G4VProcess *proc = aTrack->GetCreatorProcess();
        G4String processName;
        G4int Procid = -1;
        G4int Sid = -1;
        G4int TrackID = aTrack->GetTrackID();
        G4int NumSteps = aTrack->GetCurrentStepNumber();

        G4double step_length = step->GetStepLength();
        std::map<G4String, G4int> *fDetectIds = anaHelper->GetDetectIds();

        auto it = fDetectIds->find(PostdetectName);
        if (it != fDetectIds->end())
        {
            Sid = it->second;
        }

        else
        {

            return;
        }

        if (proc != NULL)
            processName = proc->GetProcessName();
        else
            processName = "None";
        if (processName.compare("Scintillation") == 0)
            Procid = 0;
        else if (processName.compare("Cerenkov") == 0)
            Procid = 1;

        ArapucaHit Hit = ArapucaHit(Procid, Sid, PostdetectName, Wavelength, time, PPosition, PMomentDir, PPolar, TrackID, NumSteps, step_length);
        anaHelper->AddG4Hits(Hit);
    }
}

//-----------------------------------------//
//  OPTICAL SIMS
//-----------------------------------------//

//         std::cout << "Status " <<  boundary->GetStatus() << std::endl;
//         std::cout << "Pre Detector Name " << PredetectName << std::endl;
//         std::cout << "Post Detector Name " << PostdetectName << std::endl;
//         std::cout << "Material " << mt->GetName() << std::endl;
//
//         auto pr  = step->GetPostStepPoint()->GetProcessDefinedStep();
//           G4cout << "Proc: " << pr->GetProcessName()
//                  << " | TrackStatus: " << aTrack->GetTrackStatus()
//                  << " | StepStatus: " << step->GetPostStepPoint()->GetStepStatus()
//                  << " | BoundaryStatus: "<<boundary->GetStatus()
//                  << " | PredetectName: "<< PredetectName
//                  << " | PostdetectName: "<< PostdetectName
//                  << G4endl;
// G4Exception("SteppingAction::UserSteppingAction","Sid==-1",JustWarning,"Cant Find the Detector");
//
// For Testing Purposes
//    if (boundary) {
//        G4OpBoundaryProcessStatus status = boundary->GetStatus();
//
//        switch (status) {
//        case Detection:
//            G4cout << "Photon detected at boundary" << G4endl;
//            break;
//        case Absorption:
//            G4cout << "Photon absorbed at boundary" << G4endl;
//            break;
//        case FresnelReflection:
//            G4cout << "Photon reflected (Fresnel)" << G4endl;
//            break;
//        case TotalInternalReflection:
//            G4cout << "Photon totally internally reflected" << G4endl;
//            break;
//        case Transmission:
//            G4cout << "Photon transmitted through boundary" << G4endl;
//            break;
//        default:
//            if (step->GetPostStepPoint()->GetMaterial()->GetName() != "LAr")
//            {
//                G4cout << " Material Name " <<step->GetPostStepPoint()->GetMaterial()->GetName() << " Photon Status " << status  << G4endl;
//
//            }
//            break;
//        }
//    }