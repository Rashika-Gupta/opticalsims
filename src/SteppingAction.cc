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
#include "corecel/io/Logger.hh"
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

    // Record optical photon creation time (before transport)
    if (aTrack->GetCurrentStepNumber() == 1)
    {
        auto *vol = step->GetPreStepPoint()->GetPhysicalVolume();
        G4double energy = aTrack->GetKineticEnergy(); // MeV
        auto *mpt = aTrack->GetMaterial()->GetMaterialPropertiesTable();
        auto *rindex_table = mpt->GetProperty(kRINDEX);
        auto *groupvel_table = mpt->GetProperty(kGROUPVEL);

        G4double momentum = aTrack->GetDynamicParticle()->GetTotalMomentum();

        G4double n = rindex_table ? rindex_table->Value(energy) : -1;
        G4double vg_by_E = groupvel_table ? groupvel_table->Value(energy) : -1;
        G4double vg_by_p = groupvel_table ? groupvel_table->Value(momentum) : -1;
        G4double vg_track = aTrack->GetVelocity();
        G4double v_phase = (n > 0) ? CLHEP::c_light / n : -1;

        G4double steplen = step->GetStepLength() / CLHEP::mm;
        G4double dt = step->GetDeltaTime() / CLHEP::ns;
        G4double t_global = aTrack->GetGlobalTime() / CLHEP::ns;
        // ── Write one JSONL record per step ──────────────────────────────────
        // static std::ofstream out("group-vel-g4-steppingaction.jsonl");
        // std::cout << "Event RINDEX " << n << " vg_Table value : " << vg_by_E << " vg_by_p " << vg_by_p << " vg_track " << vg_track << std::endl;
        // nlohmann::json rec;
        // rec["E_MeV"] = energy;
        // rec["n"] = n;
        // rec["vg"] = vg_track;
        // rec["vg_track_over_c"] = vg_track / CLHEP::c_light;
        // rec["vg_gdml_over_c"] = vg_by_E / CLHEP::c_light;
        // rec["vphase_over_c"] = 1.0 / n;
        //
        // out << rec.dump() << "\n";

        // ──
        //   if (vol && vol->GetLogicalVolume() && vol->GetLogicalVolume()->GetMaterial() && vol->GetLogicalVolume()->GetMaterial()->GetName() == "lAr")
        //  {
        G4String PostdetectName = step->GetPostStepPoint()->GetPhysicalVolume()->GetName();

        // std::map<G4String, G4int> *fDetectIds = anaHelper->GetDetectIds();
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

        /* Check if there are any boundary processes */

        /* dump a json file with all the position and timing information*/
        /* nlohmann::json j;

         j["track_id"] = track_id;
         j["process"] = procid == 0   ? "Scintillation"
                        : procid == 1 ? "Cerenkov"
                                      : "Unknown";
         // add geometry process

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
        */

        ArapucaHit hit(
            procid,
            0, // dummy detector id
            "LAr",
            wavelength,
            time,
            pos,
            dir,
            pol,
            track_id,
            aTrack->GetCurrentStepNumber(),
            step->GetStepLength());
        anaHelper->AddG4Hits(hit);
        // }
    }

    return;
}

// G4OpBoundaryProcess *boundary = nullptr;
//
// if (!boundary)
// { // the pointer is not defined yet
//     // Get the list of processes defined for the optical photon
//     // and loop through it to find the optical boundary process.
//     G4ProcessVector *pv = pdef->GetProcessManager()->GetProcessList();
//     for (size_t i = 0; i < pv->size(); i++)
//     {
//         if ((*pv)[i]->GetProcessName() == "OpBoundary")
//         {
//             boundary = (G4OpBoundaryProcess *)(*pv)[i];
//             break;
//         }
//     }
// }
// else
// {
//     return;
// }
/* // For Testing Purposes
if (boundary) {
    G4OpBoundaryProcessStatus status = boundary->GetStatus();

    switch (status) {
    case Detection:
        G4cout << "Photon detected at boundary" << G4endl;
        break;
    case Absorption:
        G4cout << "Photon absorbed at boundary" << G4endl;
        break;
    case FresnelReflection:
        G4cout << "Photon reflected (Fresnel)" << G4endl;
        break;
    case TotalInternalReflection:
        G4cout << "Photon totally internally reflected" << G4endl;
        break;
    case Transmission:
        G4cout << "Photon transmitted through boundary" << G4endl;
        break;
    default:

        break;
    }
} */

// Process the hits
// Only Optical Photons

// G4OpBoundaryProcessStatus status = boundary->GetStatus();
//// std::cout << "Status " <<  boundary->GetStatus() << std::endl;
// if (status == Detection and pdef == G4OpticalPhoton::Definition())
//{
//     G4String PredetectName = step->GetPreStepPoint()->GetPhysicalVolume()->GetName();
//     G4String PostdetectName = step->GetPostStepPoint()->GetPhysicalVolume()->GetName();
//     // std::cout << "Pre Detector Name " << PredetectName << std::endl;
//     //// std::cout << "Post Detector Name " << PostdetectName << std::endl;
//
//    G4ThreeVector PPosition = aTrack->GetPosition();
//    G4ThreeVector PMomentDir = aTrack->GetMomentumDirection();
//    G4ThreeVector PPolar = aTrack->GetPolarization();
//    G4double time = aTrack->GetGlobalTime();
//
//    G4double Wavelength = EtoWavelength(aTrack->GetTotalEnergy() / CLHEP::eV);
//    const G4VProcess *proc = aTrack->GetCreatorProcess();
//    G4String processName;
//    G4int Procid = -1;
//    G4int Sid = -1;
//    G4int TrackID = aTrack->GetTrackID();
//    G4int NumSteps = aTrack->GetCurrentStepNumber();
//
//    G4double step_length = step->GetStepLength();
//    std::map<G4String, G4int> *fDetectIds = anaHelper->GetDetectIds();
//    // G4Material * mt=step->GetPostStepPoint()->GetMaterial();
//
//    auto it = fDetectIds->find(PostdetectName);
//    if (it != fDetectIds->end())
//    {
//        Sid = it->second;
//    }
//
//    else
//    {
//        /*
//      std::cout << "Status " <<  boundary->GetStatus() << std::endl;
//      std::cout << "Pre Detector Name " << PredetectName << std::endl;
//      std::cout << "Post Detector Name " << PostdetectName << std::endl;
//      std::cout << "Material " << mt->GetName() << std::endl;
//
//      auto pr  = step->GetPostStepPoint()->GetProcessDefinedStep();
//        G4cout << "Proc: " << pr->GetProcessName()
//               << " | TrackStatus: " << aTrack->GetTrackStatus()
//               << " | StepStatus: " << step->GetPostStepPoint()->GetStepStatus()
//               << " | BoundaryStatus: "<<boundary->GetStatus()
//               << " | PredetectName: "<< PredetectName
//               << " | PostdetectName: "<< PostdetectName
//               << G4endl;*/
//        // G4Exception("SteppingAction::UserSteppingAction","Sid==-1",JustWarning,"Cant Find the Detector");
//
//        return;
//    }
//
//    if (proc != NULL)
//        processName = proc->GetProcessName();
//    else
//        processName = "None";
//    if (processName.compare("Scintillation") == 0)
//        Procid = 0;
//    else if (processName.compare("Cerenkov") == 0)
//        Procid = 1;
//
//    ArapucaHit Hit = ArapucaHit(Procid, Sid, PostdetectName, Wavelength, time, PPosition, PMomentDir, PPolar, TrackID, NumSteps, step_length);
//    anaHelper->AddG4Hits(Hit);
//}
//}
