#----------------------------------------------------------------------------
# Setup the project
cmake_minimum_required(VERSION 3.16...3.21)
project(GDMLOpticks)

#----------------------------------------------------------------------------
# Find Geant4 package, activating all available Vis drivers by default
# You can set WITH_GEANT4_VIS to OFF via the command line or ccmake/cmake-gui
# to build a batch mode only executable
#
option(WITH_GEANT4_VIS "Build example with Geant4 Vis drivers" ON)
if(WITH_GEANT4_VIS)
  find_package(Geant4 REQUIRED gdml vis_all)
else()
  find_package(Geant4 REQUIRED gdml)
endif()

option (With_Opticks OFF)

option (With_DEBUG OFF)

set(Celeritas_DIR "/Users/r1i/Desktop/project/forked/celeritas/build/lib/cmake/Celeritas")
find_package(Celeritas 0.7 REQUIRED)

if(With_Opticks)
  message(STATUS "WITH_OPTICKS is set ON")
  list(APPEND CMAKE_MODULE_PATH "$ENV{OPTICKS_HOME}/cmake/Modules")
  #list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/Modules")
  set(OPTICKS_PREFIX "$ENV{OPTICKS_PREFIX}")
  ### we need to Make sure this OPTICKS_PREFIX matches with the one on OpticksBuildOptions
  message(STATUS "OPTICKS_PREFIX : $ENV{OPTICKS_PREFIX} ")
  include(OpticksBuildOptions)
  message(STATUS "---- OPTICKS ----")
  message(STATUS "CMAKE_MODULE_PATH --> ${CMAKE_MODULE_PATH}")
  message(STATUS "OPTICKS HOME --> $ENV{OPTICKS_HOME}")
  message(STATUS "---- OPTICKS END ----")
  ## These are needed for opticks
  find_package(G4CX REQUIRED CONFIG)
  find_package(U4 REQUIRED CONFIG)
endif ()

#----------------------------------------------------------------------------
# Setup Geant4 include directories and compile definitions
#
include(${Geant4_USE_FILE})

#----------------------------------------------------------------------------
# Locate sources and headers for this project
#
include_directories(${PROJECT_SOURCE_DIR}/include 
                    ${Geant4_INCLUDE_DIR})
file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)

if (With_Opticks)
  file(GLOB OpRHDRS ${CMAKE_SOURCE_DIR}/include/${DIR}/Opticks/*.h)
  file(GLOB OpSRCS ${CMAKE_SOURCE_DIR}/src/${DIR}/Opticks/*.cc)
endif()

#----------------------------------------------------------------------------
# Add the executable, and link it to the Geant4 libraries
#
add_executable(gdml_det gdml_det.cc ${sources} ${headers})

#target_link_libraries(gdml_det ${Geant4_LIBRARIES} )
if(NOT Celeritas_FOUND)
  message(FATAL_ERROR
    "No Celeritas installation found.\n"
    "Export Celeritas_ROOT=/path/to/celeritas/install or use CMAKE_INSTALL_PREFIX"
  )
endif()
if(NOT CELERITAS_USE_Geant4)
  message(SEND_ERROR "This example requires Geant4 support "
    "to be enabled in Celeritas")
endif()
# Link Opticks Libraries
if (With_Opticks)
  target_link_libraries(gdml_det Opticks::G4CX Opticks::U4)
  target_sources(gdml_det PRIVATE ${SRCS} ${OpSRCS})
endif ()
include_directories(${CMAKE_CURRENT_BINARY_DIR})
configure_file(config.h.in ${CMAKE_BINARY_DIR}/include/config.h)


# Find the ROOT package
#    "REQUIRED" fails the build if ROOT isn't found.
#    "COMPONENTS" allows you to specify specific libraries (optional but good practice).
find_package(ROOT REQUIRED COMPONENTS RIO Net Tree ROOTDataFrame )

# Setup ROOT definitions and include directories

target_link_libraries(gdml_det PRIVATE
Celeritas::celeritas
Celeritas::G4
${Geant4_LIBRARIES}
ROOT::RIO
ROOT::Net
ROOT::Tree
ROOT::ROOTDataFrame
)
#----------------------------------------------------------------------------
# Copy all scripts to the build directory, i.e. the directory in which we
# build G04. This is so that we can run the executable directly because it
# relies on these scripts being in the current working directory.
#
set(G04_SCRIPTS macros/iso.mac macros/test.mac macros/Speed.mac macros/g04.mac vis.mac GDML/test.gdml GDML/dune10kt_v5_refactored_1x2x6_nowires_NoField.gdml)
foreach(_script ${G04_SCRIPTS})
  configure_file(
    ${PROJECT_SOURCE_DIR}/${_script}
    ${PROJECT_BINARY_DIR}/${_script}
    COPYONLY
    )
endforeach()


#----------------------------------------------------------------------------
# Add program to the project targets
# (this avoids the need of typing the program name after make)
#
add_custom_target(G04 DEPENDS gdml_det)

#----------------------------------------------------------------------------
# Install the executable to 'bin' directory under CMAKE_INSTALL_PREFIX
#
install(TARGETS gdml_det DESTINATION bin)

