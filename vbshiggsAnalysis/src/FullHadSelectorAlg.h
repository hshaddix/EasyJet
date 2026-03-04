/*
  Copyright (C) 2002-2024 CERN for the benefit of the ATLAS collaboration
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
    CP::SysReadHandle<xAOD::JetContainer> m_smallJetHandle{this, "smallJets", "SmallJets", "Signal small jets"};
    CP::SysReadHandle<xAOD::JetContainer> m_signaljetHandle{ this, "signaljets", "vbshiggsAnalysisSignalJets_%SYS%", "Signal Jet container to read" };
    CP::SysReadHandle<xAOD::JetContainer> m_HCandHandle{ this, "higgsCandidates", "vbshiggsAnalysisHJets_%SYS%", "Higgs Candidates container to read"};
    CP::SysReadDecorHandle<char>  m_isBtag {this, "bTagWPDecorName", "", "Name of input dectorator for b-tagging"};
    CP::SysReadHandle<xAOD::JetContainer> m_vbsjetHandle{ this, "vbsjets", "vbshiggsAnalysisVBSJets_%SYS%", "VBS Jet container to read" };

    CP::SysReadHandle<xAOD::JetContainer> m_largeJetHandle{this, "largeJets", "LargeJets", "Large boosted jets"};
    CP::SysReadHandle<xAOD::ElectronContainer> m_electronHandle{this, "electrons", "Electrons", "Electron container"};
    CP::SysReadHandle<xAOD::MuonContainer> m_muonHandle{this, "muons", "Muons", "Muon container"};
    CP::SysReadHandle<xAOD::MissingETContainer> m_metHandle{this, "met", "MET", "Missing ET container" };
    CP::SysFilterReporterParams m_filterParams {this, "vbshiggs selection"};
    std::map<Booleans, bool> m_bools;
    std::map<Booleans, std::string> m_boolnames{
        {PASS_EXACTLY_TWO_SMALL_JETS, "PASS_EXACTLY_TWO_SMALL_JETS"},
        {PASS_TWO_LARGE_JETS, "PASS_TWO_LARGE_JETS"},
        {PASS_ALL_CUTS, "PASS_ALL_CUTS"}
    };

    Gaudi::Property<bool> m_UseVBFRNN { this, "UseVBFRNN", false, "Not use VBS tagging jets or yes (i.e. use VBF-RNN jets or not)" };
   CP::SysReadDecorHandle<unsigned int> m_year {this, "year", "dataTakingYear", ""};

    CutManager m_vbshiggsCuts;
    unsigned int m_totalEvents{0};
    bool m_doBoosted{true};
    bool m_doResolved{true};

    StatusCode initialiseCutflow();
};

} 
