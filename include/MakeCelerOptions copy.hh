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
#include "G4RunManager.hh"
#include "AnalysisManagerHelper.hh"

#include <accel/AlongStepFactory.hh>
#include <accel/SetupOptions.hh>
#include "celeritas/optical/DetectorData.hh"
#include <celeritas/inp/OpticalPhysics.hh>
#include "G4AnalysisManager.hh"
#include <celeritas/phys/PDGNumber.hh>
#include <corecel/io/Logger.hh>
#include <corecel/Assert.hh>
#include <corecel/sys/Environment.hh>
#include <celeritas/ext/GeantOpticalPhysicsOptions.hh>

//---------------------------------------------------------------------------//
enum class OpticalOffloadMode
{
  electron_photon,
  optical_gun,
  optical_track,
  optical_generation
};

//---------------------------------------------------------------------------//
/*!
 * Read the optical offload mode from the environment.
 *
 * Supported values for OPTICALSIMS_CELERITAS_MODE are:
 * - electron-photon
 * - optical-gun
 * - optical-track
 * - optical-generation
 */

inline OpticalOffloadMode GetOpticalOffloadMode()
{
  static OpticalOffloadMode const result = []
  {
    std::string const &value = celeritas::getenv("OPTICALSIMS_CELERITAS_MODE");

    CELER_VALIDATE(
        !value.empty(),
        << "OPTICALSIMS_CELERITAS_MODE is not set: expected "
           "'electron-photon', 'optical-gun', 'optical-track', or "
           "'optical-generation'");

    if (value == "electron-photon")
    {
      return OpticalOffloadMode::electron_photon;
    }
    if (value == "optical-gun")
    {
      return OpticalOffloadMode::optical_gun;
    }
    if (value == "optical-track")
    {
      return OpticalOffloadMode::optical_track;
    }
    if (value == "optical-generation")
    {
      return OpticalOffloadMode::optical_generation;
    }

    CELER_VALIDATE(
        false,
        << "invalid OPTICALSIMS_CELERITAS_MODE='" << value
        << "': expected 'electron-photon', 'optical-gun', 'optical-track', or "
           "'optical-generation'");
    CELER_ASSERT_UNREACHABLE();
  }();

  return result;
}

//---------------------------------------------------------------------------/
/*!
 * Load vector of \c G4ParticleDefinition from list of PDGs.
 */
inline celeritas::SetupOptions::VecG4PD from_pdgs(std::vector<int> input)
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

//---------------------------------------------------------------------------//
/*!
 * Offload electrons and Geant4 optical-photon tracks.
 */
inline void ConfigureElectronPhotonOffload(
    celeritas::SetupOptions &opts)
{
  CELER_EXPECT(opts.optical);

  opts.offload_particles = from_pdgs(
      {G4Electron::Definition()->GetPDGEncoding(),
       G4OpticalPhoton::Definition()->GetPDGEncoding()});
  // opts.optical->generator = celeritas::inp::OpticalDirectGenerator{};
}

//---------------------------------------------------------------------------//
/*!
 * Offload only Geant4 optical-photon tracks.
 */
inline void ConfigureOpticalTrackOffload(
    celeritas::SetupOptions &opts)
{
  CELER_EXPECT(opts.optical);

  opts.offload_particles = from_pdgs(
      {G4OpticalPhoton::Definition()->GetPDGEncoding()});
  opts.optical->generator = celeritas::inp::OpticalDirectGenerator{};
}

//---------------------------------------------------------------------------//
/*!
 * Offload optical generation data and generate photons in Celeritas.
 */
inline void ConfigureOpticalGenerationOffload(
    celeritas::SetupOptions &opts)
{
  CELER_EXPECT(opts.optical);

  opts.offload_particles = {};
  opts.optical->generator = celeritas::inp::OpticalOffloadGenerator{};
}

//---------------------------------------------------------------------------//
/*!
 * Offload optical photons directly by the particle gun
 */
inline void ConfigureOpticalGunOffload(
    celeritas::SetupOptions &opts)
{
  CELER_EXPECT(opts.optical);

  opts.offload_particles = from_pdgs(
      {G4OpticalPhoton::Definition()->GetPDGEncoding()});
  opts.optical->generator = celeritas::inp::OpticalDirectGenerator{};
}

//---------------------------------------------------------------------------/
/*!
 * Celeritas runtime options.
 */
inline celeritas::SetupOptions MakeCelerOptions()
{

  celeritas::SetupOptions opts;

  opts.geometry_output_file = "/Users/r1i/Desktop/OpticalSims-upstream/lar-celer_test_derviate_changed.gdml";
  CELER_LOG(status) << "Using geometry output: " << opts.geometry_output_file;
  // No Geant4 SD callback from Celeritas — hits come back via optical callback
  opts.sd.enabled = false;

  // Configure optical physics
  opts.optical = []
  {
    celeritas::OpticalSetupOptions opt;
    constexpr celeritas::size_type num_tracks = 50650;

    opt.capacity.tracks = num_tracks;
    opt.capacity.primaries = 8 * num_tracks;
    opt.capacity.generators = 2 * num_tracks;
    // opt.generator = celeritas::inp::OpticalDirectGenerator{};

    // ── Disable optical physics processes ──────────────────────────────────
    return opt;
  }();

  OpticalOffloadMode const mode = GetOpticalOffloadMode();
  switch (mode)
  {
  case OpticalOffloadMode::optical_gun:
    ConfigureOpticalGunOffload(opts);
    CELER_LOG(status)
        << "Offloading optical photons directly by the particle gun";
    break;
  case OpticalOffloadMode::electron_photon:
    ConfigureElectronPhotonOffload(opts);
    CELER_LOG(status)
        << "Offloading electrons and optical photons";
    break;

  case OpticalOffloadMode::optical_track:
    ConfigureOpticalTrackOffload(opts);
    CELER_LOG(status)
        << "Offloading Geant4 optical-photon tracks";
    break;

  case OpticalOffloadMode::optical_generation:
    ConfigureOpticalGenerationOffload(opts);
    CELER_LOG(status)
        << "Offloading optical generation data";
    break;
  }

  opts.make_along_step = celeritas::UniformAlongStepFactory();
  opts.output_file = "celeritas.out.json";
  // opts.ignore_processes = {"CoulombScat"};
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
      CelerOpticalHit h{};
      h.detector_id = hit.detector
                          ? static_cast<int>(hit.detector.unchecked_get())
                          : -1;
      h.event_id = event_id;
      h.x = static_cast<float>(hit.position[0]);
      h.y = static_cast<float>(hit.position[1]);
      h.z = static_cast<float>(hit.position[2]);
      h.t = static_cast<float>(hit.time);
      h.track_id = hit.primary
                       ? static_cast<int>(hit.primary.unchecked_get())
                       : -1;

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
