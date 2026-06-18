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
#include "G4OpticalPhysicsOpticks.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4RadioactiveDecayPhysics.hh"
#include "G4HadronPhysicsFTFP_BERT_HP.hh"
#include "G4IonPhysics.hh"
#include "G4StoppingPhysics.hh"
#include "G4EmExtraPhysics.hh"
#include "G4Electron.hh"
#include "G4Positron.hh"
PhysicsList::PhysicsList() : FTFP_BERT_HP()
{
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
