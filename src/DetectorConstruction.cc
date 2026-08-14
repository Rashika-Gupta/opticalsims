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
#include "G4PhysicalVolumeStore.hh"
#include "G4OpticalSurface.hh"
#include "SensitiveDetector.hh"
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

G4Material *DetectorConstruction::fGetMaterial(G4String name)
{
  G4Material *material = G4Material::GetMaterial(name);
  if (!material)
  {
    G4cout << "Error: Material " << name << " not found!" << G4endl;
    assert(false);
  }
  return material;
}

void DetectorConstruction::GetOpticalSurfaceFast()
{
  // Prevent filling multiple times
  if (!fOpticalSurfaces.empty())
    return; // already filled

  const G4SurfacePropertyTable *table = G4SurfaceProperty::GetSurfacePropertyTable();
  if (!table)
  {
    G4cout << "Warning: G4SurfacePropertyTable is null!" << G4endl;
    return;
  }

  for (const auto &s : *table)
  {
    if (!s)
      continue;

    G4OpticalSurface *ops = dynamic_cast<G4OpticalSurface *>(s);
    if (!ops || ops->GetName().empty())
      continue;

    fOpticalSurfaces.emplace(ops->GetName(), ops);
  }

  G4cout << "Optical surfaces cache filled: " << fOpticalSurfaces.size()
         << " surfaces loaded." << G4endl;
}

G4VPhysicalVolume *DetectorConstruction::Construct()
{
  auto phyStore = G4PhysicalVolumeStore::GetInstance();
  // ArapucaSurface
  // G4OpticalSurface * ArapucaSurface= new G4OpticalSurface("ArapucaSurface",unified,polished,dielectric_metal);

  // Making sure we have the material
  // G4Material * ArapucaWindowMaterial=fGetMaterial("ArapucaWindowProperties");
  // G4MaterialPropertiesTable * Arapuca_mpt=nullptr;

  /* if (Arapuca_mpt==nullptr)
   {
     Arapuca_mpt=ArapucaWindowMaterial->GetMaterialPropertiesTable();
     ArapucaSurface->SetMaterialPropertiesTable(Arapuca_mpt);
   }
 */
  // Get The optical surfaces from the G4SurfacePropertyTable and cache them ( should be defined in GDML).
  GetOpticalSurfaceFast();

  G4OpticalSurface *surface = nullptr; // direct access

  const G4GDMLAuxMapType *auxmap = fParser->GetAuxMap();
  G4int count = 0;
  G4int sid = 0;
  std::map<G4String, std::vector<G4LogicalVolume *>> sensdet_map;

  // The same as above, but now we are looking for
  // sensitive detectors setting them for the volumes

  for (G4GDMLAuxMapType::const_iterator iter = auxmap->begin();
       iter != auxmap->end(); iter++)
  {

    for (G4GDMLAuxListType::const_iterator vit = (*iter).second.begin();
         vit != (*iter).second.end(); vit++)
    {
      // myvol = (*iter).first;

      // Surfaces
      if ((*vit).type == "Surface")
      {
        surface = fOpticalSurfaces[(*vit).value];
        if (!surface)
        {
          G4cout << "ERROR: Surface '" << (*vit).value << "' not found in GDML!" << G4endl;
          G4Exception("DetectorConstruction", "MissingSurface", FatalException,
                      "The specified surface is required but not defined.");
        }
        // Create Skin surface
        G4cout << "Attaching optical surface " << (*vit).value
               << " to volume " << ((*iter).first)->GetName()
               << G4endl << G4endl;
        G4String volName = ((*iter).first)->GetName();
        if (G4StrUtil::contains((*vit).value, "Border"))
        {
          // === SENSOR VOLUME ===
          G4VPhysicalVolume *phyv1 = phyStore->GetVolume("volCryostat_PV"); // photons leaving
          G4VPhysicalVolume *phyv2 = phyStore->GetVolume(volName + "_PV");  // photons arriving

          if (phyv1 && phyv2)
          {
            G4String surfaceName = volName + "_BorderSurface";
            new G4LogicalBorderSurface(surfaceName, phyv1, phyv2, surface);

            G4cout << "Attached BorderSurface for sensor: " << surfaceName
                   << " (volName=" << volName << ")" << G4endl;
          }
          else
          {
            G4cout << "WARNING: Could not find phyv1 or phyv2 for sensor " << volName << G4endl;
          }
        }
        else
        {
          // === SKIN SURFACE ===
          G4String surfaceName = volName + "_SkinSurface";
          new G4LogicalSkinSurface(surfaceName, (*iter).first, surface);
          G4cout << "Attached SkinSurface to volume: " << volName << G4endl;
        }

        count++;
      }

      if (((*vit).type == "PD" or (*vit).type == "SensDet") and (*vit).value == "PhotonDetector")
      {
        G4cout << "Attaching sensitive detector " << (*vit).value
               << " to volume " << ((*iter).first)->GetName()
               << G4endl << G4endl;
        sensdet_map[(*vit).value].push_back((*iter).first);
        std::string_view name = std::string_view((*iter).first->GetName().c_str(), (*iter).first->GetName().size());
        std::vector<std::string_view> spfirst = Split(name, '_');
        if (spfirst.size() > 1)
        {
          fDetectIds.insert(std::pair<G4String, G4int>((*iter).first->GetName() + "_PV", sid++));
        }
        else
        {
          std::cout << "Warning: Can not generate detector ids from the name" << G4endl;
          std::cout << "Opticks will use the copy number as sensitive detector id" << G4endl;
          fDetectIds.insert(std::pair<G4String, G4int>((*iter).first->GetName() + "_PV", -99));
        }
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
    // aTrackerSD->SetDetectIds(&fDetectIds);
  }

  //------------------------------------------//
  //------ Attaching SDs for Celeritas--------//
  //------------------------------------------//
  G4SDManager *sdman = G4SDManager::GetSDMpointer();
  for (auto &[sd_name, volumes] : sensdet_map)
  {
    auto *sd = new SensitiveDetector(sd_name);
    sdman->AddNewDetector(sd);
    for (auto *lv : volumes)
    {
      lv->SetSensitiveDetector(sd);
    }
  }
  // G4cout<<"Detector construction sensor surface count " << count <<G4endl;
  fDetector = fParser->GetWorldVolume();
  // Pass the World Volume to Opticks
#ifdef With_Opticks
  std::cout << "Setting up detector construction for Opticks" << std::endl;
  MySensorIdentifier *OpticksSensor = new MySensorIdentifier(fDetectIds);

  G4CXOpticks::SetSensorIdentifier(OpticksSensor);
  G4CXOpticks::SetGeometry(fDetector);
#endif

  return fDetector;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DetectorConstruction::ConstructSDandField()
{
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
