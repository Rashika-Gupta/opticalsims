//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file offload-template/src/MakeCelerOptions.cc
//---------------------------------------------------------------------------//

#include <G4OpticalPhoton.hh>
#include "G4RunManager.hh"
#include "AnalysisManagerHelper.hh"
#include "G4AnalysisManager.hh"
#include "G4Electron.hh"

#include <accel/AlongStepFactory.hh>
#include <accel/SetupOptions.hh>
#include "celeritas/optical/DetectorData.hh"
#include <celeritas/inp/OpticalPhysics.hh>
#include <celeritas/phys/PDGNumber.hh>
#include <corecel/io/Logger.hh>
#include <corecel/Assert.hh>
#include <corecel/sys/Environment.hh>
#include <celeritas/ext/GeantOpticalPhysicsOptions.hh>
#include <celeritas/optical/detail/OpticalUtils.hh>
#include <G4Event.hh>
#include <G4EventManager.hh>
#include <string>
#include <vector>
#include "geocel/GeantGeoParams.hh"
#include "geocel/g4/Convert.hh"
#include "G4VPhysicalVolume.hh"
#include <celeritas/Quantities.hh>

struct OffloadConfiguration
{
    celeritas::SetupOptions::VecG4PD particles;
    celeritas::inp::OpticalGenerator generator;
};

inline void RecordOpticalHits(
    celeritas::Span<
        celeritas::optical::DetectorHit const>
        hits);

inline OffloadConfiguration MakeOffloadConfiguration(
    std::string const &mode);

inline celeritas::SetupOptions MakeCelerOptions(
    std::string const &mode)
{

    celeritas::SetupOptions opts;
    opts.sd.enabled = false;
    opts.make_along_step =
        celeritas::UniformAlongStepFactory();
    opts.output_file = "celeritas.out.json";

    constexpr celeritas::size_type num_tracks = 50650;

    celeritas::OpticalSetupOptions optical;

    optical.capacity.tracks = num_tracks;
    optical.capacity.primaries = 8 * num_tracks;
    optical.capacity.generators = 2 * num_tracks;
    optical.detectors.callback = RecordOpticalHits;
    auto config = MakeOffloadConfiguration(mode);

    opts.offload_particles = std::move(config.particles);
    optical.generator = std::move(config.generator);
    opts.optical = std::move(optical);

    return opts;
}
//---------------------------------------------------------------------------//
/*!
 * Record optical hits callback.
 */
inline void RecordOpticalHits(
    celeritas::Span<
        celeritas::optical::DetectorHit const>
        hits)
{
    using celeritas::real_type;
    using celeritas::value_as;
    using celeritas::units::MevEnergy;
    auto const *event =
        G4EventManager::GetEventManager()
            ->GetConstCurrentEvent();
    CELER_EXPECT(event);
    auto geant_geo = celeritas::global_geant_geo().lock();
    CELER_VALIDATE(geant_geo, << "Geant4 geometry mapping is unavailable");

    // auto *sensor_ids =
    //     AnalysisManagerHelper::getInstance()->GetDetectIds();

    std::vector<CelerOpticalHit> celer_hits;
    celer_hits.reserve(hits.size());

    for (auto const &hit : hits)
    {
        CelerOpticalHit h{};
        h.celer_detector_id = hit.detector
                                  ? static_cast<int>(hit.detector.unchecked_get())
                                  : -1;

        h.primary_id = hit.primary
                           ? static_cast<int>(hit.primary.unchecked_get())
                           : -1;

        h.volume_instance_id = hit.volume_instance
                                   ? static_cast<int>(hit.volume_instance.unchecked_get())
                                   : -1;

        h.unique_instance_id = hit.unique_instance
                                   ? hit.unique_instance.unchecked_get()
                                   : 0;
        h.event_id = event->GetEventID();
        h.x = static_cast<float>(hit.position[0]);
        h.y = static_cast<float>(hit.position[1]);
        h.z = static_cast<float>(hit.position[2]);
        h.t = static_cast<float>(hit.time);

        h.energy_mev = static_cast<float>(value_as<MevEnergy>(hit.energy));
        // Convert MeV to nm: E[eV] = 1239.8 / lambda[nm]
        float energy_ev = h.energy_mev * 1e6f;
        h.wavelength_nm = (energy_ev > 0) ? (1239.8f / energy_ev) : -1.f;
        auto const *physical =
            geant_geo->id_to_geant(hit.volume_instance);

        h.sensor_name = physical->GetName();

        // auto sid = sensor_id->find(h.sensor_name);
        // if (sid == sensor_id->end())
        //     continue; // match G4's "discard unknown volume" behavior
        // h.sensor_id = sid->second;
        celer_hits.push_back(h);
    }
    if (anaHelper)
    {
        CELER_LOG(debug)
            << "Adding " << celer_hits.size()
            << " Celeritas optical hits to analysis manager";
        anaHelper->AddCelerHits(celer_hits);
    }

    CELER_LOG(debug)
        << "Received " << hits.size()
        << " Celeritas optical hits";
}

//---------------------------------------------------------------------------//
/*!
 * Make offload configuration for the given mode.
 * There are different mechanisms and path for offloading optical photon to celeritas.
 * The offload mode is set through the environment variable OPTICALSIMS_CELERITAS_MODE.
 * Supported values for OPTICALSIMS_CELERITAS_MODE are:
 * - electron-photon: Offload both electron and optical photon to celeritas.
 * - optical-gun: Generate optical photons of fixed energy and transport it in celeritas.
 * - optical-track: Geant4 creates optical photons and tracks and handles this track in celeritas.
 * This is like ray-tracing of optical photons in celeritas.
 * - optical-distribution: Offload the optical photon generation data specifically the scintillation
 * and cherenkov distribution generation to celeritas. Celeritas then will sample the optical photons and transport them in celeritas.
 */
inline OffloadConfiguration MakeOffloadConfiguration(
    std::string const &mode)
{

    if (mode == "optical-gun" || mode == "optical-track")
    {
        return {
            {G4OpticalPhoton::Definition()},
            celeritas::inp::OpticalDirectGenerator{}};
    }

    if (mode == "optical-distribution")
    {
        return {
            {},
            celeritas::inp::OpticalOffloadGenerator{}};
    }

    if (mode == "electron-photon")
    {
        return {
            {G4Electron::Definition()},
            celeritas::inp::OpticalEmGenerator{}};
    }

    CELER_VALIDATE(
        false,
        << "invalid optical offload mode '"
        << mode << "'");
    CELER_ASSERT_UNREACHABLE();
}