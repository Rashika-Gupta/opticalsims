//
// Created by ilker on 10/22/25.
//
#include "RunAction.hh"
#include "G4Run.hh"
#include "G4AnalysisManager.hh"
#include "G4Material.hh"
#include <fstream>
#include <G4Run.hh>
#include <G4Threading.hh>
RunAction::RunAction(std::string celer_offload_mode) : G4UserRunAction(), celer_offload_mode_(std::move(celer_offload_mode)), fmsg(nullptr), fFileName("out.csv")
{
    // G4int n_particle = 1;

    fmsg = new G4GenericMessenger(this, "/RunAction/output/", "");
    fmsg->DeclareProperty("file", fFileName, "File Name to Save");
}

RunAction::~RunAction()
{
    delete fmsg;
}

void RunAction::BeginOfRunAction(const G4Run *run)
{

    if (celer_offload_mode_ == "optical-distribution")
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

    std::string output_file = transport + "-" + celer_offload_mode_ + "-" + fFileName;
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

    startTime = chrono::high_resolution_clock::now();
    RunTime = 0;
    G4cout << "### Run started ###" << G4endl;
}

void RunAction::EndOfRunAction(const G4Run *run)
{
    auto duration = chrono::high_resolution_clock::now() - startTime;
    RunTime = chrono::duration_cast<chrono::duration<double>>(duration).count();
    if (G4Threading::IsMasterThread())
        std::cout << "Run time: " << RunTime << " seconds" << G4endl;

    std::cout << "Run time: " << RunTime << " seconds" << G4endl;
    using Mode = celeritas::OffloadMode;

    auto &tmi = celeritas::TrackingManagerIntegration::Instance();
    if (G4Threading::IsWorkerThread() || !G4Threading::IsMultithreadedApplication())
    {
        if (celer_offload_mode_ == "electron-photon" && tmi.GetMode() == Mode::enabled)
        {
            std::cout << "Entering here" << std::endl;
            auto &integration = celeritas::detail::IntegrationSingleton::instance();
            auto &local = dynamic_cast<celeritas::LocalTransporter &>(
                integration.local_offload());

            auto const &optical_collector = integration.shared_params().problem_loaded().optical_collector;

            if (optical_collector)
            {
                // run->GetNumberOfEvent();
                G4cout << "nEvents: " << run->GetNumberOfEvent() << "\n";

                auto const &accum = optical_collector->optical_state(local.GetState()).accum();

                // /    G4cout << "Celeritas accumulates " << accum.steps
                // /           << " optical photons" << "\n";
            }
            auto counter_stats = optical_collector->exchange_counters(local.GetState().aux());
            size_t total_photons_generated = 0;
            for (auto const &gen_counters : counter_stats.generators)
            {
                total_photons_generated += gen_counters.num_generated;
            }
            G4cout << "Celeritas generated " << total_photons_generated << " optical photons " << "\n";
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

    if (celer_offload_mode_ == "optical-distribution")
    {
        std::cout << "Eding RunAction for optical-distribution mode" << std::endl;
        celeritas::UserActionIntegration::Instance()
            .EndOfRunAction(run);
    }
    else
    {
        celeritas::TrackingManagerIntegration::Instance()
            .EndOfRunAction(run);
    }
}