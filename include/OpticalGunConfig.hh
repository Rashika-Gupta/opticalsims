#ifndef OPTICALSIMS_OPTICALGUNCONFIG_HH
#define OPTICALSIMS_OPTICALGUNCONFIG_HH

#include "G4GenericMessenger.hh"
#include "G4SystemOfUnits.hh"
#include "G4ThreeVector.hh"
#include "globals.hh"

struct OpticalGunParameters
{
    G4int num_photons{1000};

    G4double mean_energy{11.01 * eV};
    G4double sigma_energy{0.1866666667 * eV};
    G4double min_energy{10.45 * eV};
    G4double max_energy{11.57 * eV};

    G4ThreeVector position{
        20.0 * mm,
        -5915.6875 * mm,
        351.1875 * mm};
};

class OpticalGunConfig
{
  public:
    static OpticalGunConfig& Instance();

    OpticalGunParameters const& parameters() const
    {
        return parameters_;
    }

  private:
    OpticalGunConfig();

    OpticalGunParameters parameters_;
    G4GenericMessenger messenger_;
};

#endif
