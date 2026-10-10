#ifndef OPTICALSIMS_CELEROPTICALPRIMARYRUNNER_HH
#define OPTICALSIMS_CELEROPTICALPRIMARYRUNNER_HH

#include <functional>

#include <accel/SetupOptions.hh>
#include <accel/SharedParams.hh>

// Drive Celeritas' optical-primary generator from the Geant4 event loop.
// This is separate from the Geant4 track-offload integrations because an
// OpticalPrimaryGenerator creates an optical-only Celeritas problem.
class CelerOpticalPrimaryRunner
{
  public:
    using OptionsFactory = std::function<celeritas::SetupOptions()>;

    static CelerOpticalPrimaryRunner& Instance();

    void SetOptionsFactory(OptionsFactory);
    void BeginOfRunAction();
    void GenerateAndTransport(int event_id);
    void EndOfRunAction();

  private:
    CelerOpticalPrimaryRunner() = default;

    OptionsFactory options_factory_;
    celeritas::SetupOptions options_;
    celeritas::SharedParams shared_;
};

#endif
