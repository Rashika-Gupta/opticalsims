//
// Created by ilker on 6/14/25.
//

#ifndef G4_PHYSICSLIST_HH
#define G4_PHYSICSLIST_HH

#include <celeritas/ext/GeantPhysicsOptions.hh>
#include <celeritas/g4/SupportedOpticalPhysics.hh>
#include "G4VModularPhysicsList.hh"
#include "FTFP_BERT_HP.hh"
class PhysicsList : public FTFP_BERT_HP
{
public:
    explicit PhysicsList(G4String const &offloadMode);

    virtual ~PhysicsList() noexcept;

private:
    celeritas::GeantOpticalPhysicsOptions optical_options() const;
    celeritas::GeantPhysicsOptions physics_options() const;
};

#endif // G4_PHYSICSLIST_HH
