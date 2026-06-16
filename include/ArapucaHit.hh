//
// Created by ilker on 10/27/25.
//

#ifndef GDMLOPTICKS_ARAPUCAHIT_HH
#define GDMLOPTICKS_ARAPUCAHIT_HH
#include "G4VHit.hh"
#include "G4THitsCollection.hh"
#include "G4Allocator.hh"
#include "G4ThreeVector.hh"

class G4VHit;

class ArapucaHit : public G4VHit
{
public:
    ArapucaHit();
    ~ArapucaHit();
    ArapucaHit(G4int ipid, G4int isid, G4String iname, G4double iwave, G4double itime,
               G4ThreeVector ipos, G4ThreeVector idire, G4ThreeVector ipol,
               G4int itrack_id,
               G4int inum_steps,
               G4double istep_length);
    ArapucaHit(const ArapucaHit &);
    const ArapucaHit &operator=(const ArapucaHit &);
    G4bool operator==(const ArapucaHit &) const;
    inline void *operator new(size_t);
    inline void operator delete(void *);
    void Draw() final;
    void Print();
    G4int GetPid();
    G4int GetSid();
    G4String GetDetName();
    G4double GetWave();
    G4double GetTime();
    G4ThreeVector GetPos();
    G4ThreeVector GetDir();
    G4ThreeVector GetPol();

    G4int GetTrackID() const { return ftrack_id; }
    G4int GetNumSteps() const { return fnum_steps; }

    G4double GetStepLength() const { return fstep_length; }

private:
    G4int fpid{0};
    G4int fsid{0};
    G4String fname{"None"};
    G4double fwave{0};
    G4double ft{0};
    G4ThreeVector fpos{0, 0, 0};
    G4ThreeVector fdir{0, 0, 0};
    G4ThreeVector fpol{0, 0, 0};
    G4int ftrack_id{0};
    G4int fnum_steps{0};
    G4double fstep_length{0};
};

using ArapucaHitsCollection = G4THitsCollection<ArapucaHit>;
extern G4ThreadLocal G4Allocator<ArapucaHit> *ArapucaHitA;

inline void *ArapucaHit::operator new(size_t)
{
    if (!ArapucaHitA)
    {
        ArapucaHitA = new G4Allocator<ArapucaHit>;
    }
    return (void *)ArapucaHitA->MallocSingle();
}

inline void ArapucaHit::operator delete(void *aHit)
{
    ArapucaHitA->FreeSingle((ArapucaHit *)aHit);
}

inline G4int ArapucaHit::GetPid() { return fpid; }
inline G4int ArapucaHit::GetSid() { return fsid; }
inline G4String ArapucaHit::GetDetName() { return fname; }
inline G4double ArapucaHit::GetWave() { return fwave; }
inline G4double ArapucaHit::GetTime() { return ft; }
inline G4ThreeVector ArapucaHit::GetPos() { return fpos; }
inline G4ThreeVector ArapucaHit::GetDir() { return fdir; }
inline G4ThreeVector ArapucaHit::GetPol() { return fpol; }

#endif // GDMLOPTICKS_ARAPUCAHIT_HH