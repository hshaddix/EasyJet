# Declare the package
atlas_subdir(HZyAnalysis)

# Find an external package (i.e. ROOT)
find_package(ROOT COMPONENTS Core REQUIRED)

# We don't want any warnings in compilation
add_compile_options(-Werror)

# Build the Athena component library
atlas_add_component(HZyAnalysis
  src/*.cxx
  src/components/HZyAnalysis_entries.cxx
  LINK_LIBRARIES
  AthenaBaseComps
  SystematicsHandlesLib
  xAODJet
  xAODMuon
  xAODEgamma
  xAODEventInfo
  xAODTruth
  TruthUtils
  TriggerMatchingToolLib
  EventBookkeeperToolsLib
  EasyjetHubLib
  ZMassConstraintLib
  FsrUtilsLib
  MuonMomentumCorrectionsLib
)

# Install python modules, joboptions, and share content
atlas_install_scripts(
  bin/HZy-ntupler
  scripts/*
  )
#atlas_install_runtime( scripts/*.py POST_BUILD_CMD ${ATLAS_FLAKE8} )

atlas_install_python_modules(
  python/HZy_config.py
)
atlas_install_data(
  data/*
  share/*
  datasets/*
)

# You can access your data from code using path resolver, e.g.
# PathResolverFindCalibFile("JetMETCommon/file.txt")
