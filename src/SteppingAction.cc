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
        return;

    // Celeritas excludes photons already killed by absorption, etc.
    if (aTrack->GetTrackStatus() != fAlive)
        return;

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
    // if (pdef == G4OpticalPhoton::Definition())
    //{
    //
    //    auto *vol = step->GetPreStepPoint()->GetPhysicalVolume();
    //    // if (aTrack->GetCurrentStepNumber() == 1 && vol->GetLogicalVolume()->GetMaterial()->GetName() == "LAr")
    //    //{
    //    //   Celeritas rejects photons killed during optical physics,
    //    //   including photons killed by absorption.
    //    //  if (aTrack->GetTrackStatus() == fAlive)
    //    //{
    //    //   auto *vol = step->GetPostStepPoint()->GetPhysicalVolume();
    //    //   // A reflected photon remains in the pre-step volume even
    //    //   // though the geometrical post-step point is at a boundary.
    //    //   bool const reflected = status == FresnelReflection || status == TotalInternalReflection || status == LambertianReflection || status == LobeReflection || status == SpikeReflection || status == BackScattering || status == CoatedDielectricReflection;
    //    //
    //    //   if (step->GetPostStepPoint()->GetStepStatus() == fGeomBoundary && reflected)
    //    //   {
    //    //       vol = step->GetPreStepPoint()->GetPhysicalVolume();
    //    //   }
    //
    //    auto *logical = vol ? vol->GetLogicalVolume() : nullptr;
    //    auto *material = logical ? logical->GetMaterial() : nullptr;
    //
    //    // This reproduces the LAr-volume selection used when
    //    // constructing the Celeritas detector mapping.
    //    // if (material && material->GetName() == "LAr")
    //    // {
    //    G4ThreeVector pos = aTrack->GetPosition();
    //    G4ThreeVector dir = aTrack->GetMomentumDirection();
    //    G4ThreeVector pol = aTrack->GetPolarization();
    //
    //    G4double time = aTrack->GetGlobalTime();
    //    G4double wavelength = EtoWavelength(aTrack->GetTotalEnergy() / CLHEP::eV);
    //
    //    G4int track_id = aTrack->GetTrackID();
    //
    //    G4int procid = -1;
    //    if (auto const *proc = aTrack->GetCreatorProcess())
    //    {
    //        if (proc->GetProcessName() == "Scintillation")
    //            procid = 0;
    //        else if (proc->GetProcessName() == "Cerenkov")
    //            procid = 1;
    //    }
    //
    //    // --------------------------------------------------------------//
    //    // Record Step information for LAr only
    //    // --------------------------------------------------------------//
    //    ArapucaHit hit(procid, 0, // dummy detector id
    //                   "LAr",     // filling with LAr
    //                   wavelength, time, pos, dir, pol, track_id, aTrack->GetCurrentStepNumber(), step->GetStepLength());
    //    anaHelper->AddG4Hits(hit);
    //
    //    // Celeritas kills an optical track after scoring a detector hit.
    //    //  aTrack->SetTrackStatus(fStopAndKill);
    //    //}
    //    // }
    //}
}
//------------------------------------------------------------------//
// Dump Json file to compare group velocity
// optical photon's first step in LAr
//------------------------------------------------------------------//
// auto *vol = step->GetPreStepPoint()->GetPhysicalVolume();
// G4double energy = aTrack->GetKineticEnergy(); // MeV
// G4double n = aTrack->GetMaterial()
//                 ->GetMaterialPropertiesTable()
//                 ->GetProperty(kRINDEX)
//                 ->Value(energy);
// G4double v_group = aTrack->GetVelocity(); // mm/ns internally
// G4double v_phase = CLHEP::c_light / n;    // mm/ns
// G4double t = aTrack->GetGlobalTime() / CLHEP::ns;
// G4double steplen = step->GetStepLength() / CLHEP::mm;
// G4double dt = step->GetDeltaTime() / CLHEP::ns; // time this step
// auto *groupvel_table = aTrack->GetMaterial()
//                           ->GetMaterialPropertiesTable()
//                           ->GetProperty(kGROUPVEL);
// G4double vg_direct = groupvel_table->Value(energy);
// auto *mpt = aTrack->GetMaterial()->GetMaterialPropertiesTable();
// auto *rindex_table = mpt->GetProperty(kRINDEX);
//
// G4double momentum = aTrack->GetDynamicParticle()->GetTotalMomentum();
//
// G4double n_direct = rindex_table->Value(energy);
// G4double vg_table = groupvel_table ? groupvel_table->Value(momentum) : -1;
// G4double vg_track = aTrack->GetVelocity();
//
// dump a json file with velocity information
//
// nlohmann::json j;
// j["energy"] = energy;
// j["rindex"] = n_direct;
// j["v_phase"] = v_phase;
// j["t"] = t;
// j["steplen"] = steplen;
// j["vg_direct"] = vg_direct;
// j["vg_track_over_c"] = vg_track / CLHEP::c_light;
// j["vg_track"] = vg_track;
// static std::ofstream dump("group-vel-g4-steppingaction.jsonl",
//                           std::ios::app);
//
// dump << j.dump() << '\n';

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
