//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//
/// \file persistency/gdml//include/PrimaryGeneratorAction.hh
/// \brief Definition of the PrimaryGeneratorAction class
//
//
//
//

#ifndef _G04PRIMARYGENERATORACTION_H_
#define _G04PRIMARYGENERATORACTION_H_

#include "G4VUserPrimaryGeneratorAction.hh"
// #include "G4GeneralParticleSource.hh"
#include "G4ParticleGun.hh"
#include "G4GenericMessenger.hh"
#include "globals.hh"

#include "include/config.h"
#ifdef With_Opticks
#include "sphoton.h"
#include "OpticksPhoton.h"
#include "SEvt.hh"
#include "Opticks/OpticksHitHandler.hh"
#endif
class G4Event;
// class G4GeneralParticleSource;

/// Primary generator action for GDML sensitive detector example

class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction
{
public:
  PrimaryGeneratorAction();
  ~PrimaryGeneratorAction();
  void GeneratePrimaries(G4Event *anEvent) override;
#ifdef With_Opticks
  void setPhotons(std::vector<sphoton> sphotons);
#endif
private:
  // G4GeneralParticleSource *fParticleGun;
  G4ParticleGun *fParticleGun;

  G4GenericMessenger *fmsg;
  G4String fFileName;
  G4String finitParticleType;
  G4int fAmount;
#ifdef With_Opticks
  std::vector<sphoton> sphotons;
#endif
};

#endif
