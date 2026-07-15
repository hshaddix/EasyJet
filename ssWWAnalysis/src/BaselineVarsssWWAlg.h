/*
  Copyright (C) 2002-2025 CERN for the benefit of the ATLAS collaboration
*/

// Always protect against multiple includes!
#ifndef SSWWANALYSIS_FINALVARSSSWWALG
#define SSWWANALYSIS_FINALVARSSSWWALG

#include <AthenaBaseComps/AthHistogramAlgorithm.h>

#include "AsgTools/ToolHandle.h"
#include "PMGAnalysisInterfaces/IPMGTruthWeightTool.h"

#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysListHandle.h>
#include <SystematicsHandles/SysWriteDecorHandle.h>
#include <SystematicsHandles/SysReadDecorHandle.h>

#include <xAODEventInfo/EventInfo.h>
#include <xAODJet/JetContainer.h>
#include <xAODMuon/MuonContainer.h>
#include <xAODEgamma/ElectronContainer.h>
#include <xAODMissingET/MissingETContainer.h>


namespace ssWWVBS
{

  /// \brief An algorithm for counting containers
  class BaselineVarsssWWAlg final : public AthHistogramAlgorithm
  {
    /// \brief The standard constructor
public:
    using AthHistogramAlgorithm::AthHistogramAlgorithm;

    /// \brief Initialisation method, for setting up tools and other persistent
    /// configs
    StatusCode initialize() override;
    /// \brief Execute method, for actions to be taken in the event loop
    StatusCode execute(const EventContext& ctx) override;
    /// We use default finalize() -- this is for cleanup, and we don't do any

private:
    /// \brief Setup syst-aware input container handles
    CP::SysListHandle m_systematicsList {this};

    CP::SysReadHandle<xAOD::JetContainer>
      m_ssWWJetHandle{ this, "ssWWJets", "ssWWAnalysisJets_%SYS%", "Jet container to read" };
    
    CP::SysReadHandle<xAOD::ElectronContainer>
      m_ssWWElectronHandle{ this, "ssWWElectrons", "ssWWAnalysisElectrons_%SYS%", "Electron container to read" };
    CP::SysReadHandle<xAOD::ElectronContainer>
      m_electronHandle{ this, "electrons", "AnalysisElectrons_%SYS%", "Original electron container to read" };

    CP::SysReadHandle<xAOD::MuonContainer>
      m_ssWWMuonHandle{ this, "ssWWMuons", "ssWWAnalysisMuons_%SYS%", "Muon container to read" };
    CP::SysReadHandle<xAOD::MuonContainer>
      m_muonHandle{ this, "muons", "AnalysisMuons_%SYS%", "Original muon container to read" };

    CP::SysReadHandle<xAOD::MissingETContainer>
    m_metHandle{ this, "met", "AnalysisMET_%SYS%", "MET container to read" };

    CP::SysReadHandle<xAOD::EventInfo>
    m_eventHandle{ this, "event", "EventInfo", "EventInfo container to read" };

    Gaudi::Property<bool> m_isMC
      { this, "isMC", false, "Is this simulation?" };

    ToolHandle<PMGTools::IPMGTruthWeightTool> m_weightTool
      { this, "weightTool", "",
        "PMG truth weight tool for named MC weights" };

    Gaudi::Property<std::string> m_expandedEwWeightName
      { this, "expandedEwWeightName", "MUR1_MUF1_PDF303200_EXPASSEW",
        "Named PMG truth weight for the expanded EW variation" };

    Gaudi::Property<std::string> m_additionalEwWeightName
      { this, "additionalEwWeightName", "MUR1_MUF1_PDF303200_ASSEW",
        "Named PMG truth weight for the additive EW variation" };

    Gaudi::Property<std::string> m_multiplicativeEwWeightName
      { this, "multiplicativeEwWeightName", "MUR1_MUF1_PDF303200_MULTIASSEW",
        "Named PMG truth weight for the multiplicative EW variation" };

    Gaudi::Property<unsigned int> m_specialWeightsDsid
      { this, "specialWeightsDsid", 701045,
        "Only retrieve the special PMG truth weights for this DSID" };

    Gaudi::Property<bool> m_isRun3
      { this, "isRun3", true, "Is this Run 3?" };
    Gaudi::Property<std::string> m_eleWPName
      { this, "eleWP", "","Electron ID + Iso working point" };
    CP::SysReadDecorHandle<float> m_ele_SF{"", this};
    CP::SysReadDecorHandle<char> m_muonWPDecorHandle{"", this};

    CP::SysReadDecorHandle<int> m_ele_truthOrigin{"truthOrigin", this};
    CP::SysReadDecorHandle<int> m_ele_truthType{"truthType", this};
    CP::SysReadDecorHandle<char>
    m_eleECIDS { this, "ElectronsECIDS", "DFCommonElectronsECIDS", "Charge ID Selector" };

    Gaudi::Property<std::string> m_muWPName
      { this, "muonWP", "","Muon ID + Iso working point" };
    CP::SysReadDecorHandle<float> m_mu_SF{"", this};
    CP::SysReadDecorHandle<char> m_eleWPDecorHandle{"", this};
    CP::SysReadDecorHandle<int> m_mu_truthOrigin{"truthOrigin", this};
    CP::SysReadDecorHandle<int> m_mu_truthType{"truthType", this};

    CP::SysReadDecorHandle<char> m_ele_selected{"ele_is_selected_%SYS%", this};
    CP::SysReadDecorHandle<char> m_mu_selected{"mu_is_selected_%SYS%", this};
    CP::SysReadDecorHandle<char> m_ele_TrigMatching{"ele_TrigMatchingDecor_%SYS%", this};
    CP::SysReadDecorHandle<char> m_mu_TrigMatching{"mu_TrigMatchingDecor_%SYS%", this};

    CP::SysReadDecorHandle<char> 
    m_isBtag {this, "bTagWPDecorName", "", "Name of input dectorator for b-tagging"};

    CP::SysReadDecorHandle<int> m_truthFlav{ this, "truthFlav", "HadronConeExclTruthLabelID", "Jet truth flavor" };

    Gaudi::Property<std::vector<std::string>> m_floatVariables
          {this, "floatVariableList", {}, "Name list of floating variables"};

    Gaudi::Property<std::vector<std::string>> m_intVariables
          {this, "intVariableList", {}, "Name list of integer variables"};

    Gaudi::Property<std::vector<std::string>> m_boolVectorVariables
          {this, "boolVectorVariableList", {}, "Name list of bool vector-like variables"};


    /// \brief Setup sys-aware output decorations
    std::unordered_map<std::string, CP::SysWriteDecorHandle<float>>
        m_Fbranches;

    std::unordered_map<std::string, CP::SysWriteDecorHandle<int>> m_Ibranches;

    std::unordered_map<std::string, CP::SysWriteDecorHandle<std::vector<bool>>> 
	m_bool_vectorlike_branches;

    std::unordered_map<std::string, std::vector<bool>>  m_vectorBool;

 };
}
#endif
