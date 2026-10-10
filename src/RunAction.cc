//
// Created by ilker on 10/22/25.
//
#include "RunAction.hh"
#include "G4Run.hh"
#include "G4AnalysisManager.hh"
#include "G4GeneralParticleSourceData.hh"
#include "G4Material.hh"
#include "G4SingleParticleSource.hh"
#include "G4SPSEneDistribution.hh"
#include "G4SPSPosDistribution.hh"
#include "G4ios.hh"
#include <fstream>
#include <iomanip>
#include <G4Threading.hh>
#include <G4SystemOfUnits.hh>
#include <mutex>
#include <sstream>
#include <utility>
#include <vector>
#include "CelerOpticalPrimaryRunner.hh"
#include "OpticalGunConfig.hh"

RunAction::RunAction(std::string celer_offload_mode) : G4UserRunAction(), fmsg(nullptr), fFileName("out.csv"), celer_offload_mode_(std::move(celer_offload_mode))
{
    // G4int n_particle = 1;

    fmsg = new G4GenericMessenger(this, "/RunAction/output/", "");
    fmsg->DeclareProperty("file", fFileName, "File Name to Save");
}
namespace
{
    using TimePoint = std::chrono::steady_clock::time_point;

    double Seconds(TimePoint begin, TimePoint end)
    {
        return std::chrono::duration<double>(end - begin).count();
    }

    // Buffer timing records during the run; write JSON after application teardown.
    struct RunTiming
    {
        int run_id;
        int thread;
        int events_processed;
        bool is_master;
        double celer_hit_callback;
        std::size_t celer_hits_received;
        double run_action_total;
    };

    struct EventTiming
    {
        int run_id;
        int thread;
        int event_id;
        double event_elapsed;
    };

    std::mutex timing_mutex;
    std::vector<RunTiming> run_timings;
    std::vector<EventTiming> event_timings;
    thread_local std::vector<EventTiming> local_event_timings;
    thread_local int current_run_id = -1;
    thread_local double local_hit_callback_seconds = 0.0;
    thread_local std::size_t local_hits_received = 0;

    std::string FilenameValue(double value)
    {
        std::ostringstream os;
        os << std::fixed << std::setprecision(4) << value;
        std::string result = os.str();

        while (!result.empty() && result.back() == '0')
        {
            result.pop_back();
        }
        if (!result.empty() && result.back() == '.')
        {
            result.pop_back();
        }
        if (result == "-0")
        {
            result = "0";
        }
        return result;
    }
}

RunAction::~RunAction()
{
    delete fmsg;
}

void RunAction::BeginOfRunAction(const G4Run *run)
{
    startTime = std::chrono::steady_clock::now();
    current_run_id = run->GetRunID();
    local_event_timings.clear();
    local_hit_callback_seconds = 0.0;
    local_hits_received = 0;

    if (celer_offload_mode_ == "optical-gun")
    {
        if (celeritas::SharedParams::GetMode()
            == celeritas::OffloadMode::enabled)
        {
            CelerOpticalPrimaryRunner::Instance().BeginOfRunAction();
        }
    }
    else if (celer_offload_mode_ == "optical-distribution")
    {
        celeritas::UserActionIntegration::Instance()
            .BeginOfRunAction(run);
    }
    else
    {
        celeritas::TrackingManagerIntegration::Instance()
            .BeginOfRunAction(run);
    }
    // ─────────────────────────────────────────────────────────────────────

    // Get the analysis manager
    auto *analysisManager = G4AnalysisManager::Instance();
    analysisManager->SetNtupleMerging(true); // 🔴 REQUIRED for MT merging
    analysisManager->SetFileName(fFileName); // base name, no _t0 etc.

    // Open an output file
    std::string transport = celeritas::getenv("CELER_DISABLE") == "1"
                                ? "geant4"
                                : "celeritas";

    std::string output_file = transport + "-" + celer_offload_mode_;
    if (celer_offload_mode_ == "optical-gun")
    {
        auto const &gun = OpticalGunConfig::Instance().parameters();
        output_file += "-E" + FilenameValue(gun.mean_energy / eV) + "eV";
        output_file += "-pos_" + FilenameValue(gun.position.x() / mm);
        output_file += "_" + FilenameValue(gun.position.y() / mm);
        output_file += "_" + FilenameValue(gun.position.z() / mm) + "mm";
        output_file += "-events"
                       + std::to_string(run->GetNumberOfEventToBeProcessed());
    }
    else if (celer_offload_mode_ == "electron-photon"
             || celer_offload_mode_ == "optical-track"
             || celer_offload_mode_ == "optical-distribution")
    {
        // GPS distribution data are shared by Geant4 worker threads, so this
        // reads the values set by the /gps/* macro commands.
        auto *source
            = G4GeneralParticleSourceData::Instance()->GetCurrentSource();
        CELER_VALIDATE(source, << "Geant4 GPS source is unavailable");

        auto const energy = source->GetEneDist()->GetMonoEnergy();
        auto const &position = source->GetPosDist()->GetCentreCoords();

        output_file += "-E" + FilenameValue(energy / MeV) + "MeV";
        output_file += "-pos_" + FilenameValue(position.x() / mm);
        output_file += "_" + FilenameValue(position.y() / mm);
        output_file += "_" + FilenameValue(position.z() / mm) + "mm";
        output_file += "-events"
                       + std::to_string(run->GetNumberOfEventToBeProcessed());
    }
    output_file += "-" + std::string(fFileName);
    if (analysisManager)
        analysisManager->OpenFile(output_file);
    cout << "Generating " << output_file << G4endl;
    // Main Particle
    analysisManager->CreateNtuple("generator", "Particle Generator Info");
    analysisManager->CreateNtupleSColumn("name");
    analysisManager->CreateNtupleIColumn("pdg");
    analysisManager->CreateNtupleDColumn("energy");
    analysisManager->CreateNtupleDColumn("ix");
    analysisManager->CreateNtupleDColumn("iy");
    analysisManager->CreateNtupleDColumn("iz");
    analysisManager->CreateNtupleDColumn("it");
    analysisManager->CreateNtupleDColumn("mx");
    analysisManager->CreateNtupleDColumn("my");
    analysisManager->CreateNtupleDColumn("mz");
    analysisManager->CreateNtupleIColumn("evtID");
    analysisManager->FinishNtuple();

    // Opticks Hits
    analysisManager->CreateNtuple("OpticksHits", "Opticks Hits");
    analysisManager->CreateNtupleIColumn("evtID");
    analysisManager->CreateNtupleIColumn("hit_Id");
    analysisManager->CreateNtupleIColumn("SensorID");
    analysisManager->CreateNtupleFColumn("x");
    analysisManager->CreateNtupleFColumn("y");
    analysisManager->CreateNtupleFColumn("z");
    analysisManager->CreateNtupleFColumn("t");
    analysisManager->CreateNtupleFColumn("wavelength");
    analysisManager->CreateNtupleIColumn("boundary");
    analysisManager->FinishNtuple();

    // Geant4 Hits
    analysisManager->CreateNtuple("Geant4Hits", "Geant4 Hits");
    analysisManager->CreateNtupleIColumn("evtID");
    analysisManager->CreateNtupleIColumn("SensorID");
    analysisManager->CreateNtupleSColumn("SensorName");
    analysisManager->CreateNtupleDColumn("x");
    analysisManager->CreateNtupleDColumn("y");
    analysisManager->CreateNtupleDColumn("z");
    analysisManager->CreateNtupleDColumn("t");
    analysisManager->CreateNtupleDColumn("wavelength");
    analysisManager->CreateNtupleIColumn("ProcessID");
    analysisManager->FinishNtuple();

    // Celeritas Hits
    analysisManager->CreateNtuple("CeleritasHits", "Celeritas Hits");
    analysisManager->CreateNtupleIColumn("evtID");
    analysisManager->CreateNtupleIColumn("SensorID");
    analysisManager->CreateNtupleSColumn("SensorName");
    analysisManager->CreateNtupleDColumn("x");
    analysisManager->CreateNtupleDColumn("y");
    analysisManager->CreateNtupleDColumn("z");
    analysisManager->CreateNtupleDColumn("t");
    analysisManager->CreateNtupleDColumn("wavelength");
    analysisManager->CreateNtupleIColumn("trackID");
    analysisManager->CreateNtupleIColumn("numSteps");
    analysisManager->CreateNtupleDColumn("pathLength");
    analysisManager->FinishNtuple();

    // Hits recorded by SensitiveDetector::ProcessHits
    analysisManager->CreateNtuple(
        "Geant4SensitiveDetectorHits",
        "Geant4 SensitiveDetector Hits");
    analysisManager->CreateNtupleIColumn("evtID");
    analysisManager->CreateNtupleIColumn("SensorID");
    analysisManager->CreateNtupleSColumn("SensorName");
    analysisManager->CreateNtupleDColumn("x");
    analysisManager->CreateNtupleDColumn("y");
    analysisManager->CreateNtupleDColumn("z");
    analysisManager->CreateNtupleDColumn("t");
    analysisManager->CreateNtupleDColumn("wavelength");
    analysisManager->CreateNtupleIColumn("ProcessID");
    analysisManager->FinishNtuple();

    // PhotonInfo
    analysisManager->CreateNtuple("PhotonInfo", "PhotonInfo");
    analysisManager->CreateNtupleIColumn("G4ScintPhotons");
    analysisManager->CreateNtupleIColumn("G4CernPhotons");
    analysisManager->CreateNtupleIColumn("OScintPhotons");
    analysisManager->CreateNtupleIColumn("OCerenkovPhotons");
    analysisManager->CreateNtupleDColumn("Time");
    analysisManager->CreateNtupleIColumn("eventID");
    analysisManager->CreateNtupleIColumn("BatchID");
    analysisManager->FinishNtuple();
#ifdef With_DEBUG
    // ParticleStep
    analysisManager->CreateNtuple("step", "particle steps");
    analysisManager->CreateNtupleIColumn("eventID");
    analysisManager->CreateNtupleIColumn("trackID");
    analysisManager->CreateNtupleDColumn("x");
    analysisManager->CreateNtupleDColumn("y");
    analysisManager->CreateNtupleDColumn("z");
    analysisManager->CreateNtupleDColumn("t");
    analysisManager->CreateNtupleSColumn("volume1");
    analysisManager->CreateNtupleSColumn("volume2");
    analysisManager->CreateNtupleIColumn("status");
    analysisManager->CreateNtupleDColumn("mx");
    analysisManager->CreateNtupleDColumn("my");
    analysisManager->CreateNtupleDColumn("mz");
    analysisManager->CreateNtupleDColumn("px");
    analysisManager->CreateNtupleDColumn("py");
    analysisManager->CreateNtupleDColumn("pz");
    analysisManager->CreateNtupleDColumn("energy");
    analysisManager->FinishNtuple();
#endif
    G4cout << "### Run started ###" << G4endl;
}

void RunAction::EndOfRunAction(const G4Run *run)
{
    using Mode = celeritas::OffloadMode;

    if (celer_offload_mode_ == "electron-photon"
        && (G4Threading::IsWorkerThread()
            || !G4Threading::IsMultithreadedApplication()))
    {
        auto &tmi = celeritas::TrackingManagerIntegration::Instance();
        if (tmi.GetMode() == Mode::enabled)
        {
            auto &integration = celeritas::detail::IntegrationSingleton::instance();
            auto &local = dynamic_cast<celeritas::LocalTransporter &>(
                integration.local_offload());

            auto const &optical_collector = integration.shared_params().problem_loaded().optical_collector;

            if (optical_collector)
            {
                G4cout << "nEvents: " << run->GetNumberOfEvent() << "\n";
                auto counter_stats = optical_collector->exchange_counters(local.GetState().aux());
                size_t total_photons_generated = 0;
                for (auto const &gen_counters : counter_stats.generators)
                {
                    total_photons_generated += gen_counters.num_generated;
                }
                G4cout << "Celeritas generated " << total_photons_generated << " optical photons " << "\n";
            }
            // Write Celeritas diagnostics to ROOT file
            std::ostringstream diagnostics;
            tmi.GetParams().output_reg()->output(&diagnostics);
        }
    }
    // Write and Close File
    auto analysisManager = G4AnalysisManager::Instance();
    if (analysisManager)
    {
        if (G4Threading::IsMasterThread())
            cout << "Saving Events to " << analysisManager->GetFileName() << " root file .." << G4endl;

        analysisManager->Write();
        analysisManager->CloseFile();
    }
    // Return Celeritas to an invalid state
    if (celer_offload_mode_ == "optical-gun")
    {
        if (celeritas::SharedParams::GetMode()
            == celeritas::OffloadMode::enabled)
        {
            CelerOpticalPrimaryRunner::Instance().EndOfRunAction();
        }
    }
    else if (celer_offload_mode_ == "optical-distribution")
    {
        celeritas::UserActionIntegration::Instance()
            .EndOfRunAction(run);
    }
    else
    {
        celeritas::TrackingManagerIntegration::Instance()
            .EndOfRunAction(run);
    }
    const auto runEnd = std::chrono::steady_clock::now();
    RecordRunTiming(run, runEnd);
}

void RunAction::RecordEventTiming(int event_id, double elapsed_s)
{
    // Buffer event records per thread; merge once when that thread ends its run.
    local_event_timings.push_back({current_run_id,
                                   G4Threading::G4GetThreadId(),
                                   event_id,
                                   elapsed_s});
}

void RunAction::RecordCelerHitCallbackTiming(double elapsed_s, std::size_t hits)
{
    // The callback receives host hits; GPU-to-host transfer happened earlier.
    local_hit_callback_seconds += elapsed_s;
    local_hits_received += hits;
}

void RunAction::RecordRunTiming(const G4Run *run, TimePoint run_end) const
{
    // One record per run/thread. Run times from different threads overlap.
    RunTiming entry{
        run->GetRunID(),
        G4Threading::G4GetThreadId(),
        run->GetNumberOfEvent(),
        !G4Threading::IsWorkerThread(),
        local_hit_callback_seconds,
        local_hits_received,
        Seconds(startTime, run_end)};

    std::lock_guard<std::mutex> lock(timing_mutex);
    run_timings.push_back(entry);
    event_timings.insert(event_timings.end(),
                         local_event_timings.begin(),
                         local_event_timings.end());
    local_event_timings.clear();
}

void RunAction::WriteTimingJson(const ProcessTiming &t,
                                const std::string &offload_mode,
                                bool celeritas_enabled)
{
    const double gdml = Seconds(t.gdml_start, t.gdml_end);
    const double initialize = Seconds(t.init_start, t.init_end);
    nlohmann::json report;
    report["system"] = {
        {"backend", celeritas_enabled ? "celeritas" : "geant4"},
        {"offload_mode", offload_mode}};

    auto &time = report["result"]["time"];
    time["_units"] = "s";
    time["process"] = {
        {"gdml_read", gdml},
        {"g4_init", initialize}};

    time["runs"] = nlohmann::json::array();
    std::size_t events_total = 0;
    {
        std::lock_guard<std::mutex> lock(timing_mutex);
        for (const auto &r : run_timings)
        {
            // Master counts already include all worker events.
            if (r.is_master)
                events_total += r.events_processed;
            time["runs"].push_back({{"run_id", r.run_id},
                                    {"thread", r.thread},
                                    {"events_processed", r.events_processed},
                                    {"celer_hits_received", r.celer_hits_received},
                                    {"time", {{"celer_hit_callback", r.celer_hit_callback}, {"run_action_total", r.run_action_total}}}});
        }
        time["events"] = nlohmann::json::array();
        for (const auto &e : event_timings)
        {
            time["events"].push_back({{"run_id", e.run_id},
                                      {"thread", e.thread},
                                      {"event_id", e.event_id},
                                      {"time", {{"event_elapsed", e.event_elapsed}}}});
        }
    }
    report["system"]["events_total"] = events_total;
    // Keep application timings separate from Celeritas's own output JSON.
    const char *filename = celeritas_enabled
                               ? "timing_celeritas.json"
                               : "timing_geant4.json";
    std::ofstream output(filename);
    if (!output)
    {
        G4cerr << "Could not write " << filename << G4endl;
        return;
    }
    output << report.dump(2) << '\n';
}
