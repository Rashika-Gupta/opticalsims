//
// Created by ilker on 6/14/25.
//

#ifndef G4_PHYSICSLIST_HH
#define G4_PHYSICSLIST_HH

#include "celeritas/g4/SupportedOpticalPhysics.hh"

#include "G4VModularPhysicsList.hh"
#include "FTFP_BERT_HP.hh"
#include "accel/SharedParams.hh"
class PhysicsList : public FTFP_BERT_HP
{
public:
    PhysicsList(bool use_celeritas = true);
    virtual ~PhysicsList();
    void ConstructProcess() override;
    //
    // PhysicsList(bool use_celeritas = celeritas::SharedParams::GetMode() != celeritas::OffloadMode::disabled);

private:
    celeritas::GeantOpticalPhysicsOptions optical_options() const;
    celeritas::GeantPhysicsOptions physics_options() const;
    bool use_celeritas_;
};

#endif // G4_PHYSICSLIST_HH
