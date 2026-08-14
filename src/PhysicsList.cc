//
// Created by ilker on 6/14/25.
//

#include "PhysicsList.hh"
#include "G4DecayPhysics.hh"
#include "G4EmStandardPhysics.hh"
#include "G4ProcessManager.hh"
#include "G4ParticleTable.hh"
#include "G4ParticleDefinition.hh"
#include "G4ProcessVector.hh"
#include "G4ScintillationOpticks.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4RadioactiveDecayPhysics.hh"
#include "G4HadronPhysicsFTFP_BERT_HP.hh"
#include "G4IonPhysics.hh"
#include "G4StoppingPhysics.hh"
#include "G4EmExtraPhysics.hh"
#include "G4Electron.hh"
#include "G4Positron.hh"
#include "G4OpticalPhysics.hh"
#include "G4OpticalParameters.hh"
#include <accel/SharedParams.hh>
// Celertias offload
#include "accel/gen/CherenkovOffload.hh"
#include "accel/gen/ScintillationOffload.hh"

PhysicsList::PhysicsList(G4String const &offloadMode) : FTFP_BERT_HP()
{
    // check if celeritas is enabled or not

    if (offloadMode == "optical-distribution" && celeritas::SharedParams::GetMode() == celeritas::OffloadMode::enabled)
    {

        RegisterPhysics(new celeritas::SupportedOpticalPhysics(physics_options()));
    }
    else
    {
        auto *params = G4OpticalParameters::Instance();
        params->SetProcessActivation("Cerenkov", false);
        params->SetProcessActivation("Scintillation", true);
        params->SetProcessActivation("OpAbsorption", true);
        params->SetProcessActivation("OpRayleigh", false);
        params->SetProcessActivation("OpBoundary", true);
        params->SetProcessActivation("OpWLS", true);
        params->SetProcessActivation("OpMieHG", false);
        RegisterPhysics(new G4OpticalPhysics());
    }
}
PhysicsList::~PhysicsList() noexcept {};
void PhysicsList::ConstructProcess()
{
    FTFP_BERT_HP::ConstructProcess();

    for (G4ParticleDefinition *particle :
         {static_cast<G4ParticleDefinition *>(G4Electron::Definition()),
          static_cast<G4ParticleDefinition *>(G4Positron::Definition())})
    {
        auto *pm = particle->GetProcessManager();

        for (int i = pm->GetProcessListLength() - 1; i >= 0; --i)
        {
            auto *proc = (*pm->GetProcessList())[i];

            if (proc->GetProcessName() == "CoulombScat")
            {
                G4cout << "Removing CoulombScat from "
                       << particle->GetParticleName()
                       << G4endl;

                pm->RemoveProcess(proc);
            }
        }
    }
}
celeritas::GeantOpticalPhysicsOptions PhysicsList::optical_options() const
{
    celeritas::GeantOpticalPhysicsOptions optical;

    optical.cherenkov.emplace();
    optical.cherenkov->custom_cherenkov = []
    {
        return std::make_unique<celeritas::CherenkovOffload>();
    };
    optical.cherenkov->stack_photons = false; // don't create G4 photon tracks

    optical.scintillation.emplace();
    optical.scintillation->custom_scintillation = []
    {
        return std::make_unique<celeritas::ScintillationOffload>();
    };
    optical.scintillation->stack_photons = false;

    optical.boundary->invoke_sd = false; // no G4 SD callback for optical photons
    optical.absorption = true;
    optical.rayleigh_scattering = false;

    return optical;
}

celeritas::GeantPhysicsOptions PhysicsList::physics_options() const
{
    celeritas::GeantPhysicsOptions opts;
    opts.optical = this->optical_options();
    return opts;
}
