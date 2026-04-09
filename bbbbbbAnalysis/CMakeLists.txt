# Declare the package
atlas_subdir(bbbbbbAnalysis)

# Find an external package (i.e. ROOT)
find_package(ROOT COMPONENTS Core Tree Hist REQUIRED)

find_package( Boost )

# We don't want any warnings in compilation
add_compile_options(-Werror)

# MR1: selector + factory only (other algorithms added in MR2)
atlas_add_component(bbbbbbAnalysis
  src/bbbbbbSelectorAlg.cxx
  src/components/bbbbbbAnalysis_entries.cxx
  LINK_LIBRARIES
  AthenaBaseComps
  AsgTools
  AthContainers
  xAODEventInfo
  xAODJet
  xAODTruth
  SystematicsHandlesLib
  FourMomUtils
  TruthUtils
  EasyjetHubLib
)

atlas_install_data( share/* )
