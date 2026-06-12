//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file offload-template/src/MakeCelerOptions.cc
//---------------------------------------------------------------------------//

#include <G4Electron.hh>
#include <G4Gamma.hh>
#include <G4MuonMinus.hh>
#include <G4MuonPlus.hh>
#include <G4Neutron.hh>
#include <G4OpticalPhoton.hh>
#include <G4Positron.hh>
#include "AnalysisManagerHelper.hh"
#include </Users/r1i/Desktop/project/forked/celeritas/src/accel/AlongStepFactory.hh>
#include </Users/r1i/Desktop/project/forked/celeritas/src/accel/SetupOptions.hh>
#include "celeritas/optical/DetectorData.hh"
#include <celeritas/inp/OpticalPhysics.hh>
#include "G4AnalysisManager.hh"
#include "G4RunManager.hh"
#include <celeritas/phys/PDGNumber.hh>
#include <corecel/io/Logger.hh>
//---------------------------------------------------------------------------/
/*!
 * Load vector of \c G4ParticleDefinition from list of PDGs.
 */
celeritas::SetupOptions::VecG4PD from_pdgs(std::vector<int> input)
{
  using celeritas::PDGNumber;
  static std::unordered_map<PDGNumber, G4ParticleDefinition *> supported = {
      {celeritas::pdg::gamma(), G4Gamma::Definition()},
      {celeritas::pdg::electron(), G4Electron::Definition()},
      {celeritas::pdg::positron(), G4Positron::Definition()},
      {celeritas::pdg::mu_minus(), G4MuonMinus::Definition()},
      {celeritas::pdg::mu_plus(), G4MuonPlus::Definition()},

      {PDGNumber{-22}, G4OpticalPhoton::OpticalPhotonDefinition()},
  };

  CELER_VALIDATE(!input.empty(),
                 << "Celeritas \"offload_particles\" option is present but "
                    "empty. Specify PDGs or remove it to use the Celeritas "
                    "default list.");
  celeritas::SetupOptions::VecG4PD result;
  for (auto pdg : input)
  {
    auto it = supported.find(PDGNumber{pdg});
    CELER_VALIDATE(it != supported.end(),
                   << "PDG '" << pdg << "' not available");
    result.push_back(it->second);
  }
  return result;
}

//---------------------------------------------------------------------------/
/*!
 * Celeritas runtime options.
 */
celeritas::SetupOptions MakeCelerOptions()
{

  using PDG = G4int;
  using VecPDG = std::vector<PDG>;
  celeritas::SetupOptions opts;

  // Offload particles
  opts.offload_particles = from_pdgs({G4Electron::Definition()->GetPDGEncoding(), G4OpticalPhoton::Definition()->GetPDGEncoding()}); // electron and optical photon

  opts.geometry_output_file = "/Users/r1i/Desktop/OpticalSims-upstream/dune-rice-celer.gdml";
  // No Geant4 SD callback from Celeritas — hits come back via optical callback
  opts.sd.enabled = false;

  // Configure optical physics
  opts.optical = []
  {
    celeritas::OpticalSetupOptions opt;
    opt.capacity.tracks = 4096;
    opt.capacity.primaries = 8 * opt.capacity.tracks;
    opt.capacity.generators = 2 * opt.capacity.tracks;

    return opt;
  }();

  opts.make_along_step = celeritas::UniformAlongStepFactory();
  opts.output_file = "celeritas.out.json";
  opts.ignore_processes = {"CoulombScat"};
  static size_t total_celer_optical = 0;
  opts.optical->detectors.callback =
      [](celeritas::Span<celeritas::optical::DetectorHit const> hits)
  {
    using celeritas::value_as;
    using celeritas::units::MevEnergy;
    int event_id = G4EventManager::GetEventManager()
                       ->GetConstCurrentEvent()
                       ->GetEventID();
    std::vector<CelerOpticalHit> celer_hits;
    celer_hits.reserve(hits.size());

    for (auto const &hit : hits)
    {
      CelerOpticalHit h;
      h.detector_id = hit.detector
                          ? static_cast<int>(hit.detector.unchecked_get())
                          : -1;
      h.event_id = event_id;
      h.x = static_cast<float>(hit.position[0]);
      h.y = static_cast<float>(hit.position[1]);
      h.z = static_cast<float>(hit.position[2]);
      h.t = static_cast<float>(hit.time);
      h.energy_mev = static_cast<float>(value_as<MevEnergy>(hit.energy));
      // Convert MeV to nm: E[eV] = 1239.8 / lambda[nm]
      float energy_ev = h.energy_mev * 1e6f;
      h.wavelength_nm = (energy_ev > 0) ? (1239.8f / energy_ev) : -1.f;
      celer_hits.push_back(h);
    }

    AnalysisManagerHelper::getInstance()->AddCelerHits(celer_hits);
    total_celer_optical += hits.size();
    CELER_LOG(debug) << "[Celeritas] optical hits this flush: " << hits.size()
                     << " | total: " << total_celer_optical << "\n";
  };

  return opts;
}
