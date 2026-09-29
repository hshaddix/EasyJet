# Declare the package
atlas_subdir(PhSVJAnalysis)

# Find an external package (i.e. ROOT)
find_package(ROOT COMPONENTS Core Tree Hist REQUIRED)

# We don't want any warnings in compilation
add_compile_options(-Werror)

# Build the Athena component library
atlas_add_component(PhSVJAnalysis
  src/.cxx
  src/BaselineVarsPhSVJAlg.cxx
  src/PhSVJSelectorAlg.cxx
  src/components/PhSVJ_entries.cxx

  LINK_LIBRARIES
  AthenaBaseComps
  AsgTools
  AthContainers
  xAODEventInfo
  xAODEgamma
  xAODMuon
  xAODJet
  xAODPFlow
  xAODTracking
  SystematicsHandlesLib
  FourMomUtils
  TruthUtils
  EasyjetHubLib
  TriggerMatchingToolLib
)

# Install python modules, joboptions, and share content
atlas_install_scripts(
  bin/phsvj-ntupler
)
atlas_install_python_modules(
  python/PhSVJ_config.py
)
atlas_install_runtime( scripts/*.py POST_BUILD_CMD ${ATLAS_FLAKE8} )
atlas_install_data(
  share/*
)
