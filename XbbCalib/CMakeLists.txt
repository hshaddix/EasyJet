# Declare the package
atlas_subdir(XbbCalib)

# Find an external package (i.e. ROOT)
find_package(ROOT COMPONENTS Core Tree Hist REQUIRED)

find_package( Boost COMPONENTS json )

find_package(HDF5 1.10.1 COMPONENTS C CXX)

# We don't want any warnings in compilation
add_compile_options(-Werror)

# Build the Athena component library
atlas_add_component(XbbCalib
  src/DataEnrichmentAlg.cxx
  src/FourVectorOutBlock.cxx
  src/BaselineVarsttCalibAlg.cxx
  src/BaselineVarsZbbjCalibAlg.cxx
  src/BaselineVarsZlljCalibAlg.cxx
  src/BaselineVarsZllyCalibAlg.cxx
  src/BaselineVarsZtautauCalibAlg.cxx
  src/ZbbjCalibSelectorAlg.cxx
  src/ZlljCalibSelectorAlg.cxx
  src/ZllyCalibSelectorAlg.cxx
  src/ZtautauCalibSelectorAlg.cxx
  src/ttCalibSelectorAlg.cxx
  src/components/XbbCalib_entries.cxx
  src/ZbbyCalibSelectorAlg.cxx
  src/BaselineVarsZbbyCalibAlg.cxx
  src/ZcandSelectorAlg.cxx
  src/EventInfoHistsAlg.cxx
  src/FinalSelectionAlg.cxx
  src/FinalWeightAlg.cxx
  src/ZbbHistogramAlg.cxx
  src/ZDecayTruthLabelAlg.cxx

  LINK_LIBRARIES
  AthenaBaseComps
  AsgTools
  AthContainers
  xAODEventInfo
  xAODTruth
  xAODEgamma
  xAODMuon
  xAODJet
  SystematicsHandlesLib
  FourMomUtils
  TruthUtils
  EasyjetHubLib
  TriggerMatchingToolLib
  IH5GroupSvc
  HDF5Utils
  ${HDF5_LIBRARIES}
  Boost::json
)

# Install python modules, joboptions, and share content
atlas_install_scripts(
  bin/xbbcalib-enrich-data
  bin/xbbcalib-ntupler
  bin/xbbcalib-hists
  scripts/xbbcalib-enrich-submit
  scripts/XbbCalib-submitGrid.py
)
atlas_install_python_modules(
  python/ttCalib_config.py
  python/ZbbyCalib_config.py
  python/ZbbjCalib_config.py
  python/ZlljCalib_config.py
  python/ZllyCalib_config.py
  python/ZtautauCalib_config.py
  python/vars_to_copy.py
  python/zdecay_label_config.py
  python/hists_output_config.py
  python/tag_meta_cfg.py
)
atlas_install_runtime( scripts/*.py POST_BUILD_CMD ${ATLAS_FLAKE8} )
atlas_install_data(
  share/*
  datasets/*
)
