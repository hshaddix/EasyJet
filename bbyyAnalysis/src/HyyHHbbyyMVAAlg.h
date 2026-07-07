/*
  Copyright (C) 2002-2024 CERN for the benefit of the ATLAS collaboration
*/

// Always protect against multiple includes!
#ifndef HHBBYYANALYSIS_MVAALG
#define HHBBYYANALYSIS_MVAALG

#include <AthenaBaseComps/AthHistogramAlgorithm.h>
#include <SystematicsHandles/SysListHandle.h>
#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysReadDecorHandle.h>
#include <SystematicsHandles/SysWriteDecorHandle.h>

#include <xAODEventInfo/EventInfo.h>
#include <xAODJet/JetContainer.h>
#include <xAODMuon/MuonContainer.h>
#include <xAODEgamma/ElectronContainer.h>
#include <xAODEgamma/PhotonContainer.h>
#include <xAODEgamma/PhotonContainer.h>
#include <xAODMissingET/MissingETContainer.h>

#include "MVAUtils/BDT.h"

namespace HHBBYY
{

  struct HyyMultiBDTResults
  {
    std::vector<float> multi_response;
    int                multiclass_index;
    int                binary_index;
    float              binary_score;
  };

  /// \brief An algorithm for counting containers
  class HyyHHbbyyMVAAlg final : public AthHistogramAlgorithm
  {
    /// \brief The standard constructor
  public:
    using AthHistogramAlgorithm::AthHistogramAlgorithm;

    /// \brief Initialisation method, for setting up tools and other persistent 
    /// configs
    StatusCode initialize() override;

    /// \brief Execute method, for actions to be taken in the event loop
    StatusCode execute(const EventContext& ctx) override;


  private:
    // ToolHandle<whatever> handle {this, "pythonName", "defaultValue", "someInfo"};

    // Systematics list
    CP::SysListHandle m_systematicsList {this};

    // Sys-aware input container handles
    CP::SysReadHandle<xAOD::JetContainer>
      m_jetHandle { this, "jets", "bbyyAnalysisJets_%SYS%", "Jet container to read" };

    CP::SysReadHandle<xAOD::PhotonContainer>
      m_photonHandle { this, "photons", "bbyyAnalysisPhotons_%SYS%", "Photon container to read" };

    CP::SysReadHandle<xAOD::EventInfo>
      m_eventHandle { this, "event", "EventInfo", "EventInfo container to read" };

    CP::SysReadHandle<xAOD::MissingETContainer>
      m_metHandle { this, "met", "AnalysisMET_%SYS%", "MET container to read" };

    // B-tagging decorator
    CP::SysReadDecorHandle<char>
      m_isBtag { this, "bTagWPDecorName", "", "Name of input decorator for b-tagging" };


    CP::SysReadDecorHandle<int> m_nCentralJets
      { this, "nCentralJets", "nCentralJets_%SYS%", "Number of Central Jets decoration" };

    CP::SysReadDecorHandle<int> m_nJets
      { this, "nJets", "nJets_%SYS%", "Number of Jets decoration" };

    CP::SysReadDecorHandle<int> m_nBJets
      { this, "nBJets", "nBJets_%SYS%", "Number of B Jets decoration" };

    // MET significance
    CP::SysReadDecorHandle<float>
      m_met_sig { this, "METSignificance", "significance_%SYS%", "Met Significance" };

    // Declare the BDTs
    std::vector<std::unique_ptr<MVAUtils::BDT>> m_bdts;

    // HyyMultiBDT helper methods
    void HyyMultiBDT_init(const std::string &folder);
    void HyyMultiBDT_createVariableMap(const xAOD::EventInfo *event,
                                       const auto &sys,
                                       const xAOD::MissingET *met,
                                       const int n_photons,
                                       const TLorentzVector &ph1,
                                       const TLorentzVector &ph2,
                                       const int &nCentralJets,
                                       const int &nJets,
                                       const int &nBJets);
    HyyMultiBDTResults HyyMultiBDT_perform();

    template<typename T>
    T var_or_nan(T value, T default_value);

    std::vector<float> get_HyyHHbbyyMVAInput(
      const std::vector<std::string> &variables
    );

    // Configuration: Hyy multiclass
    Gaudi::Property<bool> m_doHyy_multiclass
      { this, "doHyy_multiclass", false, "Do H-yy Couplings multiclass BDT?" };

    Gaudi::Property<std::string> m_HyyMultiBDT_folder
      { this, "HyyMultiBDT_folder", "", "Path to H-yy Couplings multiclass BDT" };

    Gaudi::Property<std::string> m_MulticlassBDT_model_name
      { this, "MulticlassBDT_model_name", "", "MulticlassBDT model name" };

    Gaudi::Property<std::map<int, std::string>> m_MulticlassBDT_bins
      { this, "MulticlassBDT_bins", {}, "MulticlassBDT output index and its corresponding bin name" };
      
    Gaudi::Property<std::vector<std::string>> m_MulticlassBDT_variables
      { this, "MulticlassBDT_variables", {}, "Input variables for MulticlassBDT" };

    Gaudi::Property<std::vector<std::string>> m_BinaryBDT_model_names
      { this, "BinaryBDT_model_names", {}, "BinaryBDT models names" };

    Gaudi::Property<std::map<std::string, std::vector<std::string>>> m_BinaryBDT_variables
      { this, "BinaryBDT_variables", {}, "BinaryBDT bin names and corresponding input variables" };

    Gaudi::Property<std::map<std::string, std::vector<std::string>>> m_BinaryBDT_boundaries
      { this, "BinaryBDT_boundaries", {}, "BinaryBDT bin names and corresponding boundaries" };

    Gaudi::Property<std::map<std::string, std::string>> m_BinaryBDT_weights
      { this, "BinaryBDT_weights", {}, "BinaryBDT bin names and corresponding weights" };

    // Dynamic decoration variable lists: photon-jets
    Gaudi::Property<std::vector<std::string>> m_intVariables_BDTInput
      { this, "intVariableList_BDTInput", {}, "Name list of integer variables for BDTInput" };

    Gaudi::Property<std::vector<std::string>> m_floatVariables_BDTInput
      { this, "floatVariableList_BDTInput", {}, "Name list of float variables for BDTInput" };

    /// \brief Setup sys-aware read decorations photonjets
    std::unordered_map<std::string, CP::SysReadDecorHandle<int>>   m_Ibranches_BDTInput;
    std::unordered_map<std::string, CP::SysReadDecorHandle<float>> m_Fbranches_BDTInput;

    // Declare HyyMultiBDT
    std::unique_ptr<MVAUtils::BDT>              HyyMultiBDT_multiclass;
    std::vector<std::unique_ptr<MVAUtils::BDT>> HyyMultiBDT_binaries;
    std::vector<int>                            HyyMultiBDT_offset_cat;
    std::unordered_map<std::string, float>      HyyMultiBDT_VariableMap;

    // Systematics control
    Gaudi::Property<bool> m_doSystematics
      { this, "doSystematics", false, "Run on all systematics" };

    // // Generic int/float variable lists for output decorations
    Gaudi::Property<std::vector<std::string>> m_intVariables
      { this, "intVariableList", {}, "Name list of integer variables" };

    Gaudi::Property<std::vector<std::string>> m_floatVariables
      { this, "floatVariableList", {}, "Name list of integer variables" };
    

    /// \brief Setup sys-aware output decorations
    std::unordered_map<std::string, CP::SysWriteDecorHandle<int>>   m_Ibranches;
    std::unordered_map<std::string, CP::SysWriteDecorHandle<float>> m_Fbranches;
  };

} // namespace HHBBYY

#endif // HHBBYYANALYSIS_MVAALG

