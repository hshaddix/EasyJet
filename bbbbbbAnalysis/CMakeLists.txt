# Declare the package
atlas_subdir(bbbbbbAnalysis)

# Find an external package (i.e. ROOT)
find_package(ROOT COMPONENTS Core Tree Hist REQUIRED)

find_package( Boost )

find_package(HDF5 1.10.1 COMPONENTS HL C CXX)

# We don't want any warnings in compilation
add_compile_options(-Werror)

# Build the Athena component library
atlas_add_component(bbbbbbAnalysis
  src/BaselineVarsResolvedAlg.cxx
  src/bbbbbbSelectorAlg.cxx
  src/SmallRJetTriggerSFAlg.cxx
  src/TriggerDecoratorAlg.cxx
  src/JetPairingAlg.cxx
  src/components/bbbbbbAnalysis_entries.cxx
  LINK_LIBRARIES
  AthenaBaseComps
  AsgTools
  AthContainers
  xAODEventInfo
  xAODEgamma
  xAODJet
  xAODTracking
  xAODTruth
  SystematicsHandlesLib
  FourMomUtils
  TruthUtils
  MVAUtils
  IH5GroupSvc
  HDF5Utils
  ${HDF5_LIBRARIES} ${HDF5_HL_LIBRARIES}
  TrigCompositeUtilsLib
  PathResolver
  EasyjetHubLib
  VBFTaggerLib
  TriggerMatchingToolLib
)

atlas_install_data( share/* )
