#include "OpticalGunConfig.hh"

OpticalGunConfig& OpticalGunConfig::Instance()
{
    static OpticalGunConfig result;
    return result;
}

OpticalGunConfig::OpticalGunConfig()
    : messenger_(this, "/opticalGun/", "Optical primary gun controls")
{
    messenger_.DeclareProperty(
        "photons",
        parameters_.num_photons,
        "Number of optical photons generated per event");

    messenger_.DeclarePropertyWithUnit(
        "position",
        "mm",
        parameters_.position,
        "Optical photon source position");

    messenger_.DeclarePropertyWithUnit(
        "meanEnergy",
        "eV",
        parameters_.mean_energy,
        "Mean of the optical photon Gaussian energy distribution");

    messenger_.DeclarePropertyWithUnit(
        "sigmaEnergy",
        "eV",
        parameters_.sigma_energy,
        "Standard deviation of the Gaussian energy distribution");

    messenger_.DeclarePropertyWithUnit(
        "minEnergy",
        "eV",
        parameters_.min_energy,
        "Lower bound of the truncated Gaussian distribution");

    messenger_.DeclarePropertyWithUnit(
        "maxEnergy",
        "eV",
        parameters_.max_energy,
        "Upper bound of the truncated Gaussian distribution");
}
