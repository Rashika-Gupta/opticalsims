# Celeritas–Geant4 OpticalSims

This project integrates Celeritas with OpticalSims to transport optical
photons in the DUNE geometry.

## Build

```bash
cmake -S . -B build
cmake --build build -j
```

Available modes are `optical-track`, `optical-distribution`,
`electron-photon`, and `optical-gun`. Select a mode by configuring
`OPTICALSIMS_CELERITAS_MODE` with:

```bash
ccmake .
```

## Run

```bash
cd build
./gdml_det GDML/dune10kt_v5_refactored_1x2x6_nowires_NoField.gdml ../macros/g04-photon.mac
```

Set `CELER_DISABLE=0` to enable Celeritas offloading, or
`CELER_DISABLE=1` to use Geant4 transport only. For example:

```bash
CELER_DISABLE=0 ./gdml_det <geometry.gdml> <macro.mac>
```

Simulation results are written to a ROOT file. Celeritas diagnostics are
written to `celeritas.out.json`.
