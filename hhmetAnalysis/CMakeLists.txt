# Declare the package
atlas_subdir(hhmetAnalysis)

# Find an external package (i.e. ROOT)
find_package(ROOT COMPONENTS Core Tree Hist REQUIRED)

find_package( Boost )

find_package(HDF5 1.10.1 COMPONENTS HL C CXX)

# We don't want any warnings in compilation
add_compile_options(-Werror)

# Build the Athena component library
atlas_add_component(hhmetAnalysis
  src/BaselineVarsCommonAlg.cxx
  src/BaselineVarsResolvedAlg.cxx
  src/BaselineVarsBoostedAlg.cxx
  src/hhmetSelectorAlg.cxx
  src/components/hhmetAnalysis_entries.cxx
  LINK_LIBRARIES
  AthenaBaseComps
  AsgTools
  AthContainers
  xAODEventInfo
  xAODEgamma
  xAODJet
  xAODMuon
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
  TriggerMatchingToolLib
)

# Install python modules, joboptions, and share content
atlas_install_scripts(
  bin/hhmet-ntupler
)

atlas_install_python_modules(
  python/*.py
)

atlas_install_data(
  share/*.yaml
)

# You can access your data from code using path resolver, e.g.
# PathResolverFindCalibFile("JetMETCommon/file.txt")
