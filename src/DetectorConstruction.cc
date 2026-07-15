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
//
/// \file DetectorConstruction.cc
/// \brief Implementation of the DetectorConstruction class

#include "DetectorConstruction.hh"
#include "G4VisAttributes.hh"
#include <G4Color.hh>
#include <G4GDMLParser.hh>
#include <G4GDMLParser.hh>
#include <G4VPhysicalVolume.hh>
#include <G4LogicalVolume.hh>
#include <G4OpticalSurface.hh>
#include <G4PhysicalVolumeStore.hh>
#include <G4Material.hh>
#include "SensitiveDetector.hh"
#include "G4SDManager.hh"
#include "G4GDMLParser.hh"
#include "G4LogicalVolumeStore.hh"

#include <map>
#include "include/config.h"
#include "G4UserLimits.hh"
#include "G4LogicalBorderSurface.hh"
#include "G4LogicalSkinSurface.hh"
#include "AnalysisManagerHelper.hh"
#include "corecel/io/Logger.hh"
// Opticks Related headers
#ifdef With_Opticks
#include "Opticks/MySensorIdentifier.hh"
#include "G4CXOpticks.hh"
#include "U4SensorIdentifier.h"
#endif

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

DetectorConstruction::DetectorConstruction(const G4GDMLParser *parser)
    : G4VUserDetectorConstruction(),
      fParser(parser)
{
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4VPhysicalVolume *DetectorConstruction::Construct()
{
  fDetector = fParser->GetWorldVolume();
  return fDetector;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DetectorConstruction::ConstructSDandField()
{
  AnalysisManagerHelper *anaHelper = AnalysisManagerHelper::getInstance();
  // ArapucaSurface
  G4OpticalSurface *ArapucaSurface = new G4OpticalSurface("ArapucaSurface", unified, polished, dielectric_metal);

  // Making sure we have the material
  G4Material *ArapucaWindowMaterial = G4Material::GetMaterial("ArapucaWindowProperties");
  G4MaterialPropertiesTable *mpt = nullptr;
  if (ArapucaWindowMaterial)
  {
    mpt = ArapucaWindowMaterial->GetMaterialPropertiesTable();
    ArapucaSurface->SetMaterialPropertiesTable(mpt);
  }
  else
  {
    G4cout << "Error, No Material " << G4endl;
    assert(false);
  }

  // G4VPhysicalVolume *vol1,*vol2;
  //  UserLimits
  // G4UserLimits* limits = new G4UserLimits(0.01*CLHEP::mm); // or smaller
  // G4LogicalVolume* myvol;

  //------------------------------------------------
  // Sensitive detectors
  //------------------------------------------------
  /*
  G4SDManager* SDman = G4SDManager::GetSDMpointer();

  G4String trackerChamberSDname = "PhotonSD";
  SensitiveDetector* aTrackerSD =
    new SensitiveDetector(trackerChamberSDname);
  SDman->AddNewDetector( aTrackerSD );
  */
  ///////////////////////////////////////////////////////////////////////
  //
  // Example how to retrieve Auxiliary Information for sensitive detector
  //
  const G4GDMLAuxMapType *auxmap = fParser->GetAuxMap();
  G4int count = 0;
  G4int sid = 0;
  std::map<G4String, std::vector<G4LogicalVolume *>> sensdet_map;

  // The same as above, but now we are looking for
  // sensitive detectors setting them for the volumes

  for (G4GDMLAuxMapType::const_iterator iter = auxmap->begin();
       iter != auxmap->end(); iter++)
  {
    /*G4cout << "Volume " << ((*iter).first)->GetName()
           << " has the following list of auxiliary information: "
           << G4endl << G4endl;
    */

    for (G4GDMLAuxListType::const_iterator vit = (*iter).second.begin();
         vit != (*iter).second.end(); vit++)
    {
      // myvol = (*iter).first;

      // Surfaces
      if ((*vit).type == "Surface")
      {
        // vol1=G4PhysicalVolumeStore::GetInstance()->GetVolume((*vit).value+"_PV");
        // vol2=G4PhysicalVolumeStore::GetInstance()->GetVolume((*iter).first->GetName()+"_PV");
        // new G4LogicalBorderSurface(((*iter).first->GetName()+"_"+(*vit).value+"_"+(*vit).type),vol1,vol2,ArapucaSurface);
        new G4LogicalSkinSurface((*iter).first->GetName() + "_Surface", (*iter).first, ArapucaSurface);
        count++;
      }

      if (((*vit).type == "PD" or (*vit).type == "SensDet") and (*vit).value == "PhotonDetector")
      {
        G4cout << "Attaching sensitive detector " << (*vit).value
               << " to volume " << ((*iter).first)->GetName()
               << G4endl << G4endl;
        sensdet_map[(*vit).value].push_back((*iter).first);

        /* G4VSensitiveDetector* mydet =SDman->FindSensitiveDetector((*vit).value);
         if(mydet)
         { */

        // myvol->SetSensitiveDetector(mydet);
        if (G4Threading::IsMasterThread())
        {

          std::string_view name = std::string_view((*iter).first->GetName().c_str(), (*iter).first->GetName().size());
          std::vector<std::string_view> spfirst = Split(name, '_');
          if (spfirst.size() > 1)
          {
            /*std::vector<std::string_view> spsecond=Split(spfirst[1],'-');
            int first,second,third;
            first=std::stoi(std::string(spsecond[2]));
            second=std::stoi(std::string(spsecond[1]));
            third=std::stoi(std::string(spsecond[0]));
            sid=third*(10*4)+second*4+first;
            */
            fDetectIds.insert(std::pair<G4String, G4int>((*iter).first->GetName() + "_PV", sid++));
          }
          else
          {
            std::cout << "Warning: Can not generate detector ids from the name" << G4endl;
            std::cout << "Opticks will use the copy number as sensitive detector id" << G4endl;
            fDetectIds.insert(std::pair<G4String, G4int>((*iter).first->GetName() + "_PV", -99));
          }
        }
        /*
          }
          else
          {
            G4cout << (*vit).value << " detector not found" << G4endl;
          }
          */
      }
      else if ((*vit).type == "Solid")
      {
        /*
        if((*vit).value == "True")
        {
          G4VisAttributes* visatt = new G4VisAttributes(
            ((*iter).first)->GetVisAttributes()->GetColour());
          visatt->SetVisibility(true);
          visatt->SetForceSolid(true);
          visatt->SetForceAuxEdgeVisible(true);
          ((*iter).first)->SetVisAttributes(visatt);

          //((*iter).first)->SetUserLimits(limits);

         }*/
      }
    }
  }
  // aTrackerSD->SetDetectIds(&fDetectIds);
  // --------------------------------------------------------------------------
  // Attach  sensitive detectors
  // --------------------------------------------------------------------------
  // Map: SensDet name -> list of logical volumes

  G4SDManager *sdman = G4SDManager::GetSDMpointer();
  for (auto &[sd_name, volumes] : sensdet_map)
  {
    auto *sd = new GdmlSensitiveDetector(sd_name);
    sdman->AddNewDetector(sd);
    for (auto *lv : volumes)
    {
      CELER_LOG(debug) << "Attaching SD: " << sd_name << " to LV: " << lv->GetName();
      lv->SetSensitiveDetector(sd);
    }
  }

  anaHelper->SetDetectIds(&fDetectIds);
  // G4cout<<"Detector construction sensor surface count " << count <<G4endl;

  // Pass the World Volume to Opticks
#ifdef With_Opticks
  // run it only in master thread
  if (fDetector and G4Threading::IsMasterThread())
  {
    std::cout << "Setting up detector construction for Opticks" << std::endl;
    MySensorIdentifier *OpticksSensor = new MySensorIdentifier(fDetectIds);

    G4CXOpticks::SetSensorIdentifier(OpticksSensor);
    G4CXOpticks::SetGeometry(fDetector);
  }

#endif
}

std::vector<std::string_view> DetectorConstruction::Split(const std::string_view &s, char del)
{
  std::vector<std::string_view> result;
  size_t start = 0;
  while (true)
  {
    size_t pos = s.find(del, start);
    if (pos == std::string::npos)
    {
      result.emplace_back(s.substr(start));
      break;
    }
    result.emplace_back(s.substr(start, pos - start));
    start = pos + 1;
  }
  return result;
}
