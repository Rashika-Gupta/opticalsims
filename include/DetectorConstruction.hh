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
/// \file persistency/gdml//include/DetectorConstruction.hh
/// \brief Definition of the DetectorConstruction class
//
//
//
//

#ifndef _DETECTORCONSTRUCTION_H_
#define _DETECTORCONSTRUCTION_H_

#include "G4VUserDetectorConstruction.hh"
#include "SensitiveDetector.hh"

class G4GDMLParser;

/// Detector construction for laoding GDML geometry

class DetectorConstruction : public G4VUserDetectorConstruction
{
public:
  DetectorConstruction(const G4GDMLParser *parser);

  virtual G4VPhysicalVolume *Construct();
  virtual void ConstructSDandField();
  std::vector<std::string_view> Split(const std::string_view &s, char del);

private:
  const G4GDMLParser *fParser;
  G4VPhysicalVolume *fDetector;
  std::map<G4String, G4int> fDetectIds;
};
class GdmlSensitiveDetector final : public G4VSensitiveDetector
{
public:
  GdmlSensitiveDetector(std::string const &name)
      : G4VSensitiveDetector{name} {}
  void Initialize(G4HCofThisEvent *) final {}
  bool ProcessHits(G4Step *, G4TouchableHistory *) final { return false; }
};

#endif
