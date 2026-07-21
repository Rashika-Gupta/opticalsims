//
// Created by ilker on 6/14/25.
//

#ifndef G4_PHYSICSLIST_HH
#define G4_PHYSICSLIST_HH

#include "G4VModularPhysicsList.hh"
#include "FTFP_BERT_HP.hh"
class PhysicsList : public FTFP_BERT_HP
{
public:
    PhysicsList();
    virtual ~PhysicsList();
    void ConstructProcess() override;
};

#endif // G4_PHYSICSLIST_HH
