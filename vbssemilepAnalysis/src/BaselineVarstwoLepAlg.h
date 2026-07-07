/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/


#ifndef VBSSEMILEPANALYSIS_BASELINEVARFULLLEPALG_H
#define VBSSEMILEPANALYSIS_BASELINEVARFULLLEPALG_H
#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysListHandle.h>
#include <SystematicsHandles/SysWriteDecorHandle.h>
#include <SystematicsHandles/SysReadDecorHandle.h>

#include <AthContainers/ConstDataVector.h>

#include <xAODEventInfo/EventInfo.h>
#include <xAODJet/JetContainer.h>
#include <xAODMuon/MuonContainer.h>
#include <xAODEgamma/ElectronContainer.h>
#include <xAODMissingET/MissingETContainer.h>

namespace VBSSEMILEP{
    /// \brief An algorithm for counting containers
    class BaselineVarstwoLepAlg final : public AthHistogramAlgorithm {
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

        CP::SysReadHandle<xAOD::JetContainer> m_smalljetHandle{ this, "smalljets", "vbssemilepAnalysisJets_%SYS%", "Base small-R jets to check for b-tags" };
        
        CP::SysReadHandle<xAOD::JetContainer> m_signaljetHandle{ this, "signaljets", "vbssemilepAnalysisSignalJets_%SYS%", "Small R jets Jet container to read"};

        CP::SysReadHandle<xAOD::JetContainer> m_vbsLRJetHandle{ this, "vbsLRJets", "vbssemilepAnalysisLargeJets_%SYS%", "Large R Jet container to read"};

        CP::SysReadHandle<xAOD::JetContainer> m_vbsJetHandle{ this, "vbsJets", "vbssemilepAnalysisVBSJets_%SYS%",   "VBS Jet container to read" };
        
        CP::SysReadHandle<xAOD::JetContainer> m_RNNjetBoostedHandle{ this, "RNNJets_boosted", "RNNJets_boostedCategory_%SYS%", "input RNN jets container for boosted 20 GeV" };

        CP::SysReadHandle<xAOD::JetContainer> m_RNNjetResolvedHandle{ this, "RNNJets_resolved", "RNNJets_resolvedCategory_%SYS%", "input RNN jets container for resolved 20 GeV" };
        
        CP::SysReadHandle<xAOD::ElectronContainer> m_vbsElectronHandle{ this, "vbsElectrons", "vbssemilepAnalysisElectrons_%SYS%",   "Electron container to read" };
        CP::SysReadHandle<xAOD::ElectronContainer> m_electronHandle{ this, "electrons", "AnalysisElectrons_%SYS%", "Original electron container to read" };

        CP::SysReadHandle<xAOD::MuonContainer> m_vbsMuonHandle{ this, "vbsMuons", "vbssemilepAnalysisMuons_%SYS%",   "Muon container to read" };
        CP::SysReadHandle<xAOD::MuonContainer> m_muonHandle{ this, "muons", "AnalysisMuons_%SYS%", "Original muon container to read" };

        CP::SysReadHandle<xAOD::EventInfo>
        m_eventHandle{ this, "event", "EventInfo",   "EventInfo container to read" };

        Gaudi::Property<bool> m_isMC { this, "isMC", false, "Is this simulation?" };

        Gaudi::Property<std::string> m_eleWPName { this, "eleWP", "","Electron ID + Iso working point" };
        CP::SysReadDecorHandle<float> m_ele_SF{"", this};

        // electron truth information
        CP::SysReadDecorHandle<int> m_ele_truthOrigin{"truthOrigin", this};
        CP::SysReadDecorHandle<int> m_ele_truthType{"truthType", this};
        CP::SysReadDecorHandle<int> m_ele_firstEgMotherTruthType{"firstEgMotherTruthType", this};
        CP::SysReadDecorHandle<int> m_ele_firstEgMotherTruthOrigin{"firstEgMotherTruthOrigin", this};
        CP::SysReadDecorHandle<int> m_ele_firstEgMotherPdgId{"firstEgMotherPdgId", this};

        Gaudi::Property<std::string> m_muWPName { this, "muonWP", "","Muon ID + Iso working point" };
        CP::SysReadDecorHandle<float> m_mu_SF{"", this};

        // muon truth information
        CP::SysReadDecorHandle<int> m_mu_truthOrigin{"truthOrigin", this};
        CP::SysReadDecorHandle<int> m_mu_truthType{"truthType", this};
      
        CP::SysReadDecorHandle<char>  m_isBtag {this, "bTagWPDecorName", "", "Name of input dectorator for b-tagging"};

        CP::SysReadDecorHandle<int> m_PCBT {this, "PCBTDecorName", "", "Name of pseudo-continuous b-tagging decorator"};

        CP::SysReadDecorHandle<int> m_truthFlav { this, "truthFlav", "HadronConeExclTruthLabelID", "Jet truth flavor" };

        CP::SysReadDecorHandle<char> m_eleECIDS { this, "ElectronsECIDS", "DFCommonElectronsECIDS", "Charge ID Selector" };

        CP::SysReadDecorHandle<float> m_GN3Xv01_pqcdcx = {this, "pqcdcx", "GN3XPV01_pqcdcx", "GN3XPV01_pqcdcx"};
        CP::SysReadDecorHandle<float> m_GN3Xv01_pWqq = {this, "pWqq", "GN3XPV01_pWqq", "GN3XPV01_pWqq"};
        CP::SysReadDecorHandle<float> m_GN3Xv01_pqcdbx = {this, "pqcdbx", "GN3XPV01_pqcdbx", "GN3XPV01_pqcdbx"};
        CP::SysReadDecorHandle<float> m_GN3Xv01_ptop = {this, "ptop", "GN3XPV01_ptop", "GN3XPV01_ptop"};
        CP::SysReadDecorHandle<int> m_truth_label = {this, "TruthLabel", "R10TruthLabel_R22v1", "TruthLabel"};
        
        // W-tagger
        Gaudi::Property<std::string> m_WTag_Type { this, "wtag_type", "", "WTagger type"};
        Gaudi::Property<std::string> m_WTag_WP { this, "wtag_wp", "", "WTagger wp"};
        CP::SysReadDecorHandle<float> m_WTag_score{"", this};
        CP::SysReadDecorHandle<char> m_Pass_WTag{"", this};

        // do resolved category
        Gaudi::Property<bool> m_doResolved { this, "do_resolved", false, "Run Resolved category" };
        
        Gaudi::Property<std::vector<std::string>> m_floatVariables {this, "floatVariableList", {}, "Name list of floating variables"};
        Gaudi::Property<std::vector<std::string>> m_intVariables {this, "intVariableList", {}, "Name list of integer variables"};

        /// \brief Setup sys-aware output decorations
        std::unordered_map<std::string, CP::SysWriteDecorHandle<float>> m_Fbranches;
        std::unordered_map<std::string, CP::SysWriteDecorHandle<int>> m_Ibranches;

        Gaudi::Property<bool> m_UseVBFRNN { this, "UseVBFRNN", false, "Not use VBS tagging jets or yes (i.e. use VBF-RNN jets or not)" };
        

    };
}
#endif
