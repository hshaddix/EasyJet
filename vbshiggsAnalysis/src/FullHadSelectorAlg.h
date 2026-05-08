/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include <memory>

#include "AnaAlgorithm/AnaAlgorithm.h"
#include <AsgDataHandles/ReadDecorHandleKey.h>

#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysListHandle.h>
#include <SystematicsHandles/SysWriteDecorHandle.h>
#include <SystematicsHandles/SysReadDecorHandle.h>
#include <SystematicsHandles/SysFilterReporterParams.h>


#include <xAODEventInfo/EventInfo.h>
#include <xAODJet/JetContainer.h>
#include <xAODMuon/MuonContainer.h>
#include <xAODEgamma/ElectronContainer.h>
#include <xAODMissingET/MissingETContainer.h>

#include "TriggerMatchingTool/IMatchingTool.h"
#include <EasyjetHub/CutManager.h>
#include "vbshiggsEnums.h"

namespace VBSHIGGS {

class FullHadSelectorAlg : public EL::AnaAlgorithm {
public:
    FullHadSelectorAlg(const std::string& name, ISvcLocator* pSvcLocator);

    StatusCode initialize() override;
    StatusCode execute() override;
    StatusCode finalize() override;

private:
    CP::SysListHandle m_systematicsList {this};
    CP::SysReadHandle<xAOD::EventInfo> m_eventHandle{this, "event", "EventInfo", "Event info container to read"};
    CP::SysReadHandle<xAOD::JetContainer> m_smallJetHandle{this, "smallJets", "vbshiggsAnalysisJets_%SYS%", "Signal small jets"}; 
    CP::SysReadHandle<xAOD::JetContainer> m_signaljetHandle{ this, "signaljets", "vbshiggsAnalysisSignalJets_%SYS%", "Signal Jet container to read" };
    CP::SysReadHandle<xAOD::JetContainer> m_HCandHandle{ this, "higgsCandidates", "vbshiggsAnalysisHJets_%SYS%", "Higgs Candidates container to read"};
    CP::SysReadDecorHandle<char>  m_isBtag {this, "bTagWPDecorName", "", "Name of input dectorator for b-tagging"};
    CP::SysReadHandle<xAOD::JetContainer> m_vbsjetHandle{ this, "vbsjets", "vbshiggsAnalysisVBSJets_%SYS%", "VBS Jet container to read" };

    CP::SysReadHandle<xAOD::JetContainer> m_largeJetHandle{this, "largeJets", "vbshiggsAnalysisLargeJets_%SYS%", "Large boosted jets"}; 
    CP::SysReadHandle<xAOD::ElectronContainer> m_electronHandle{ this, "electrons", "AnalysisElectrons_%SYS%", "Original electron container to read" };
     CP::SysReadHandle<xAOD::MuonContainer> m_muonHandle{ this, "muons", "AnalysisMuons_%SYS%", "Original muon container to read" };
    CP::SysReadDecorHandle<char> m_passTriggerJET {this, "passTriggerJET", "pass_trigger_JET_%SYS%", "events pass any jet triggers"};

    
    CP::SysFilterReporterParams m_filterParams {this, "vbshiggs selection"};

    Gaudi::Property<std::vector<std::string>> m_inputCutList{this, "cutList", {}};
    std::vector<VBSHIGGS::Booleans> m_inputCutKeys;
    Gaudi::Property<bool> m_saveCutFlow{this, "saveCutFlow", false};

    std::unordered_map<VBSHIGGS::Booleans, CP::SysWriteDecorHandle<char>> m_Bbranches;
    CP::SysWriteDecorHandle<char> m_passallcuts{"PassAllCuts_%SYS%", this};



    std::map<Booleans, bool> m_bools;
    std::map<Booleans, std::string> m_boolnames{
        {PASS_EXACTLY_TWO_LARGE_JETS, "PASS_EXACTLY_TWO_LARGE_JETS"},
        {PASS_EXACTLY_TWO_SMALL_JETS, "PASS_EXACTLY_TWO_SMALL_JETS"},
        {PASS_MORE_THAN_TWO_LARGE_JETS, "PASS_MORE_THAN_TWO_LARGE_JETS"},
	    {PASS_LEPTON_VETO, "PASS_LEPTON_VETO"},
        {PASS_EXACTLY_ONE_LARGE_JET, "PASS_EXACTLY_ONE_LARGE_JET"},
        {PASS_TRIGGER, "PASS_TRIGGER"},
        {PASS_ALL_CUTS, "PASS_ALL_CUTS"}
    };

   Gaudi::Property<bool> m_UseVBFRNN { this, "UseVBFRNN", false, "Not use VBS tagging jets or yes (i.e. use VBF-RNN jets or not)" };
   CP::SysReadDecorHandle<unsigned int> m_year {this, "year", "dataTakingYear", ""};
   Gaudi::Property<bool> m_bypass{ this, "bypass", false, "Run selector algorithm in pass-through mode" };


    CutManager m_vbshiggsCuts;
    unsigned int m_totalEvents{0};
    bool m_doBoosted{true};
    bool m_doResolved{true};

    Gaudi::Property<std::vector<std::string>> m_triggers 
    { this, "triggerLists", {}, "Name list of trigger" };

    // Gaudi::Property<bool> m_doResolved { this, "do_resolved", false, "Run Resolved category" };


    StatusCode initialiseCutflow();
};

} 
