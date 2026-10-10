#include "CelerOpticalPrimaryRunner.hh"

#include <memory>
#include <utility>

#include <G4Threading.hh>

#include <corecel/Assert.hh>
#include <corecel/data/AuxStateVec.hh>
#include <corecel/sys/Device.hh>
#include <geocel/GeantUtils.hh>
#include <celeritas/Types.hh>
#include <celeritas/optical/CoreParams.hh>
#include <celeritas/optical/CoreState.hh>
#include <celeritas/optical/Transporter.hh>
#include <celeritas/optical/gen/PrimaryGeneratorAction.hh>

namespace
{
struct LocalOpticalPrimaryState
{
    std::shared_ptr<celeritas::optical::CoreStateBase> state;
    std::shared_ptr<celeritas::optical::Transporter> transporter;
    std::shared_ptr<celeritas::optical::PrimaryGeneratorAction const> generator;
};

thread_local LocalOpticalPrimaryState local_state;
}

CelerOpticalPrimaryRunner& CelerOpticalPrimaryRunner::Instance()
{
    static CelerOpticalPrimaryRunner result;
    return result;
}

void CelerOpticalPrimaryRunner::SetOptionsFactory(OptionsFactory factory)
{
    CELER_VALIDATE(!options_factory_,
                   << "Celeritas optical-primary options factory was already set");
    CELER_VALIDATE(factory,
                   << "invalid Celeritas optical-primary options factory");
    options_factory_ = std::move(factory);
}

void CelerOpticalPrimaryRunner::BeginOfRunAction()
{
    CELER_VALIDATE(options_factory_,
                   << "Celeritas optical-primary options factory has not been set");

    if (celeritas::SharedParams::GetMode()
        != celeritas::OffloadMode::enabled)
    {
        return;
    }

    if (G4Threading::IsMasterThread())
    {
        // Evaluate after macro commands have been processed so every run uses
        // the latest optical-gun configuration.
        options_ = options_factory_();
        shared_.Initialize(options_);
    }
    else
    {
        shared_.InitializeWorker(options_);
    }

    // An MT master owns shared parameters but does not process events.
    if (G4Threading::IsMultithreadedApplication()
        && G4Threading::IsMasterThread())
    {
        return;
    }

    auto const& problem = shared_.optical_problem_loaded();
    local_state.transporter = problem.transporter;
    local_state.generator = std::dynamic_pointer_cast<
        celeritas::optical::PrimaryGeneratorAction const>(problem.generator);

    CELER_VALIDATE(local_state.generator,
                   << "expected a Celeritas optical primary generator");
    CELER_ASSERT(local_state.transporter);
    CELER_ASSERT(local_state.transporter->params());

    auto const& params = *local_state.transporter->params();
    celeritas::validate_geant_threading(params.sizes().streams);

    auto const stream_id = celeritas::id_cast<celeritas::StreamId>(
        celeritas::get_geant_thread_id());
    auto const memspace = celeritas::device()
                              ? celeritas::MemSpace::device
                              : celeritas::MemSpace::host;

    if (memspace == celeritas::MemSpace::device)
    {
        local_state.state = std::make_shared<
            celeritas::optical::CoreState<celeritas::MemSpace::device>>(
            params, stream_id, params.sizes().tracks);
    }
    else
    {
        local_state.state = std::make_shared<
            celeritas::optical::CoreState<celeritas::MemSpace::host>>(
            params, stream_id, params.sizes().tracks);
    }

    if (params.aux_reg())
    {
        local_state.state->aux()
            = std::make_shared<celeritas::AuxStateVec>(*params.aux_reg(),
                                                       memspace,
                                                       stream_id,
                                                       params.sizes().tracks);
    }
}

void CelerOpticalPrimaryRunner::GenerateAndTransport(int event_id)
{
    CELER_EXPECT(event_id >= 0);
    CELER_VALIDATE(local_state.state && local_state.transporter
                       && local_state.generator,
                   << "Celeritas optical-primary state is not initialized");

    local_state.state->reseed(
        local_state.transporter->params()->rng(),
        celeritas::id_cast<celeritas::UniqueEventId>(event_id));

    // Celeritas samples the configured position, energy, direction and
    // polarization, then transports every generated photon to completion.
    local_state.generator->insert(*local_state.state);
    (*local_state.transporter)(*local_state.state);
}

void CelerOpticalPrimaryRunner::EndOfRunAction()
{
    if (celeritas::SharedParams::GetMode()
        != celeritas::OffloadMode::enabled)
    {
        return;
    }

    // Release stream-local state before the master destroys shared data.
    if (!G4Threading::IsMultithreadedApplication()
        || G4Threading::IsWorkerThread())
    {
        local_state = {};
    }

    if (G4Threading::IsMasterThread())
    {
        shared_.Finalize();
    }
}
