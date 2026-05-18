# Declare the package
atlas_subdir(vbssemilepAnalysis)

# We don't want any warnings in compilation
add_compile_options(-Werror)

# Build the Athena component library
atlas_add_component(vbssemilepAnalysis
  src/*.cxx
  src/components/vbssemilepAnalysis_entries.cxx
  LINK_LIBRARIES
  AthenaBaseComps
  AsgTools
  AthContainers
  xAODEventInfo
  xAODEgamma
  xAODMuon
  xAODTau
  xAODJet
  xAODTracking
  xAODTruth
  SystematicsHandlesLib
  FourMomUtils
  TruthUtils
  DiTauMassToolsLib
  TriggerMatchingToolLib
  EventBookkeeperToolsLib
  EasyjetHubLib
  VBFTaggerLib
  BoostedJetTaggersLib
)

# Install python modules, joboptions, and share content
#atlas_install_scripts(
#  bin/vbssemilep-ntupler
#)

#atlas_install_python_modules(
#  python/*.py
#)
atlas_install_data(
  share/*.yaml
  )

# atlas_install_data( data/* )
# You can access your data from code using path resolver, e.g.
# PathResolverFindCalibFile("JetMETCommon/file.txt")
