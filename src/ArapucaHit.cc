//
// Created by ilker on 10/27/25.
//

#include "../include/ArapucaHit.hh"
#include "G4VisManager.hh"
#include "G4Circle.hh"
template <class type>
class G4Allocator;
G4ThreadLocal G4Allocator<ArapucaHit> *ArapucaHitA = nullptr;

ArapucaHit::ArapucaHit()
    : G4VHit()
{
}

ArapucaHit::~ArapucaHit()
{
}

ArapucaHit::ArapucaHit(G4int ipid, G4int isid, G4String iname, G4double iwave,
                       G4double itime, G4ThreeVector ipos,
                       G4ThreeVector idir, G4ThreeVector ipol, G4int itrack_id, G4int inum_steps, G4double istep_length)
    : G4VHit()
{
    fpid = ipid;
    fsid = isid;
    fname = iname;
    fwave = iwave;
    ft = itime;
    fpos = ipos;
    fdir = idir;
    fpol = ipol;
    ftrack_id = itrack_id;
    fnum_steps = inum_steps;
    fstep_length = istep_length;
}
ArapucaHit::ArapucaHit(const ArapucaHit &p)
    : G4VHit()
{
    fpid = p.fpid;
    fsid = p.fsid;
    fname = p.fname;
    fwave = p.fwave;
    ft = p.ft;
    fpos = p.fpos;
    fdir = p.fdir;
    fpol = p.fpol;
    ftrack_id = p.ftrack_id;
    fnum_steps = p.fnum_steps;
    fstep_length = p.fstep_length;
}

const ArapucaHit &ArapucaHit::operator=(const ArapucaHit &p)
{
    fpid = p.fpid;
    fsid = p.fsid;
    fname = p.fname;
    fwave = p.fwave;
    ft = p.ft;
    fpos = p.fpos;
    fdir = p.fdir;
    fpol = p.fpol;
    ftrack_id = p.ftrack_id;
    fnum_steps = p.fnum_steps;
    fstep_length = p.fstep_length;
    return *this;
}

G4bool ArapucaHit::operator==(const ArapucaHit &p) const
{
    return (this == &p) ? true : false;
}

void ArapucaHit::Draw()
{
    G4VVisManager *visMan = G4VVisManager::GetConcreteInstance();
    if (visMan)
    {
        G4Circle circle(fpos);
        circle.SetScreenSize(2.);
        circle.SetFillStyle(G4Circle::filled);
        G4Colour colour(0., 0., 1);
        G4VisAttributes attribs(colour);
        circle.SetVisAttributes(attribs);
        visMan->Draw(circle);
    }
}

void ArapucaHit::Print()
{
    G4cout << "------ Printing Hit Info ------" << G4endl;
    G4cout << "Process ID : " << fpid << G4endl;
    G4cout << "Detector Name : " << fname << G4endl;
    G4cout << "Detector ID : " << fsid << G4endl;
    G4cout << "Hit Position : " << fpos.getX() << " " << fpos.getY() << " " << fpos.getZ() << G4endl;
    G4cout << "Hit Time : " << ft << G4endl;
    G4cout << "Hit Wavelength : " << fwave << G4endl;
    G4cout << "Hit Polarization : " << fpol.getX() << " " << fpol.getY() << " " << fpol.getZ() << G4endl;
    G4cout << "Hit Direction : " << fdir.getX() << " " << fdir.getY() << " " << fdir.getZ() << G4endl;
    G4cout << "Track ID : " << ftrack_id << G4endl;
    G4cout << "Number of Steps : " << fnum_steps << G4endl;
    G4cout << "Track Length : " << fstep_length << G4endl;
}