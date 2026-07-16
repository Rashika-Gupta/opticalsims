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

    //------------------------------------------------------------------//
    // Record optical photon creation time
    //(before transport)in LAr
    //------------------------------------------------------------------//

    if (aTrack->GetCurrentStepNumber() == 1)
    {
        auto *vol = step->GetPreStepPoint()->GetPhysicalVolume();

        if (vol && vol->GetLogicalVolume() && vol->GetLogicalVolume()->GetMaterial() && vol->GetLogicalVolume()->GetMaterial()->GetName() == "LAr")
        {
            G4ThreeVector pos = aTrack->GetPosition();
            G4ThreeVector dir = aTrack->GetMomentumDirection();
            G4ThreeVector pol = aTrack->GetPolarization();

            G4double time = aTrack->GetGlobalTime();
            G4double wavelength = EtoWavelength(aTrack->GetTotalEnergy() / CLHEP::eV);

            G4int track_id = aTrack->GetTrackID();

            G4int procid = -1;
            if (auto const *proc = aTrack->GetCreatorProcess())
            {
                if (proc->GetProcessName() == "Scintillation")
                    procid = 0;
                else if (proc->GetProcessName() == "Cerenkov")
                    procid = 1;
            }

            //------------------------------------------------------------------//
            // Dump Json file to compare group velocity
            // optical photon's first step in LAr
            //------------------------------------------------------------------//
            auto *vol = step->GetPreStepPoint()->GetPhysicalVolume();
            G4double energy = aTrack->GetKineticEnergy(); // MeV
            G4double n = aTrack->GetMaterial()
                             ->GetMaterialPropertiesTable()
                             ->GetProperty(kRINDEX)
                             ->Value(energy);
            G4double v_group = aTrack->GetVelocity(); // mm/ns internally
            G4double v_phase = CLHEP::c_light / n;    // mm/ns
            G4double t = aTrack->GetGlobalTime() / CLHEP::ns;
            G4double steplen = step->GetStepLength() / CLHEP::mm;
            G4double dt = step->GetDeltaTime() / CLHEP::ns; // time this step
            auto *groupvel_table = aTrack->GetMaterial()
                                       ->GetMaterialPropertiesTable()
                                       ->GetProperty(kGROUPVEL);
            G4double vg_direct = groupvel_table->Value(energy);
            auto *mpt = track->GetMaterial()->GetMaterialPropertiesTable();
            auto *rindex_table = mpt->GetProperty(kRINDEX);
            auto *groupvel_table = mpt->GetProperty(kGROUPVEL);

            G4double energy = track->GetKineticEnergy();
            G4double momentum = track->GetDynamicParticle()->GetTotalMomentum();

            G4double n_direct = rindex_table->Value(energy);
            G4double vg_table = groupvel_table ? groupvel_table->Value(momentum) : -1;
            G4double vg_track = track->GetVelocity();

            // -----------------------------------------------
            // dump a json file with velocity information
            // -----------------------------------------------

            nlohmann::json j
                j["track_id"] = track_id;
            j["process"] = procid == 0   ? "Scintillation"
                           : procid == 1 ? "Cerenkov"
                                         : "Unknown";

            j["time_ns"] = time / CLHEP::ns;
            j["wavelength_nm"] = wavelength;

            j["position_cm"] = {
                pos.x() / CLHEP::cm,
                pos.y() / CLHEP::cm,
                pos.z() / CLHEP::cm};

            j["energy_eV"] = aTrack->GetTotalEnergy() / CLHEP::eV;

            j["volume"] = vol->GetName();
            j["post_volume"] = PostdetectName;
            j["step_number"] = aTrack->GetCurrentStepNumber();
            j["step_length_cm"] = step->GetStepLength() / CLHEP::cm;
            static std::ofstream dump("optical_births.jsonl",
                                      std::ios::app);

            dump << j.dump() << '\n';

            // --------------------------------------------------------------//
            // Record Step information for LAr only
            // --------------------------------------------------------------//
            ArapucaHit hit(procid, 0, // dummy detector id
                           "LAr",     // filling with LAr
                           wavelength, time, pos, dir, pol, track_id, aTrack->GetCurrentStepNumber(), step->GetStepLength());
            anaHelper->AddG4Hits(hit);
        }
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