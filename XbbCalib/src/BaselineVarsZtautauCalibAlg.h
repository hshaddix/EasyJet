/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

// Always protect against multiple includes!
#ifndef ZTAUTAUCALIB_FINALVARSXBBCALIBALG
#define ZTAUTAUCALIB_FINALVARSXBBCALIBALG

#include <AthenaBaseComps/AthReentrantAlgorithm.h>

#include <AsgDataHandles/ReadDecorHandle.h>

#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysListHandle.h>
#include <SystematicsHandles/SysWriteDecorHandle.h>
#include <SystematicsHandles/SysReadDecorHandle.h>

#include <string>
#include <xAODEventInfo/EventInfo.h>
#include <xAODJet/JetContainer.h>
#include <xAODTau/TauJetContainer.h>
#include "SystVariableCopier.h"
#include "FourVectorOutBlock.h" 
#include <memory>

#include "TriggerMatchingTool/IMatchingTool.h"

namespace XBBCALIB
{

  /// \brief An algorithm for counting containers
  class BaselineVarsZtautauCalibAlg final : public AthHistogramAlgorithm
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
    // ToolHandle<whatever> handle {this, "pythonName", "defaultValue",
    // "someInfo"};

    /// \brief Setup syst-aware input container handles
    CP::SysListHandle m_systematicsList {this};

    CP::SysReadHandle<xAOD::JetContainer>
    m_lRjetHandle{ this, "lrjets", "XbbCalibLRJets_%SYS%",   "Large-R jet container to read" };

    CP::SysReadHandle<xAOD::TauJetContainer>
    m_tauHandle{ this, "taus", "XbbCalibTaus_%SYS%",   "Tau container to read" };

    CP::SysReadHandle<xAOD::TauJetContainer>
    m_allTauHandle{ this, "allTaus", "AnalysisTauJets_%SYS%", "Full calibrated tau container to decorate with trigger matching" };

    /// \brief Ordered container produced by ZtautauCalibSelectorAlg:
    /// index 0 = Z candidate (smallest |Delta Phi| w.r.t. MET), index 1 = recoiling jet
    CP::SysReadHandle<xAOD::JetContainer>
    m_zcandJetHandle{ this, "zcandJets", "XbbCalibLRJetsOrdered_%SYS%", "Ordered large-R jets: [0]=Z candidate, [1]=recoil" };

    /// \brief Zcand mass corrected with MET in the collinear approximation.
    /// Written by ZtautauCalibSelectorAlg as a per-systematic decoration
    CP::SysReadDecorHandle<float> m_zcandMetMassDecor
      { this, "zcandMetMassDecor", "met_mass_%SYS%", "MET-corrected Z candidate mass" };

    /// \brief Full 4-momentum of the truth large-R jet closest in DeltaR to
    /// the Z-candidate jet (MC only), decorated as a single JetFourMom_t
    /// block directly onto the Zcand jet
    CP::SysReadDecorHandle<xAOD::JetFourMom_t>
    m_zcandTruthJetDecor{this,"zcandTruthJetDecor","TruthLRJet_%SYS%","Truth matched Z candidate jet"};

    CP::SysReadHandle<xAOD::EventInfo>
    m_eventHandle{ this, "event", "EventInfo",   "EventInfo container to read" };

    Gaudi::Property<bool> m_isMC
      { this, "isMC", false, "Is this simulation?" };

    /// \brief Analysis-level selections of the two trigger paths (LJT = large-R
    /// jet trigger, STT = single-tau trigger), on top of the baseline selection
    /// of ZtautauCalibSelectorAlg. They define the pass_LJT / pass_STT flags and
    /// the Zcand0_ljt_* / Zcand0_stt_* histogram variables.
    Gaudi::Property<float> m_minSTTTauPt
      {this, "minSTTTauPt", 170.e3, "Minimum pT of the STT-matched leading tau [MeV]"};
    Gaudi::Property<float> m_maxSTTTauZcandDeltaR
      {this, "maxSTTTauZcandDeltaR", 1.0, "Maximum DeltaR between the STT-matched tau and the Z-candidate jet"};

    /// \brief Tau-ID selection decoration (e.g. RNNLoose) required on tau0 for
    /// the STT path, read from the full calibrated tau container
    CP::SysReadDecorHandle<char> m_tau0IDDecor
      { this, "tau0IDSelectionDecor", "baselineSelection_RNNLoose_%SYS%", "Tau ID selection decoration checked on tau0" };

    Gaudi::Property<std::vector<std::string>> m_floatVariables
          {this, "floatVariableList", {}, "Name list of floating variables"};
  
    Gaudi::Property<std::vector<std::string>> m_intVariables
          {this, "intVariableList", {}, "Name list of integer variables"};

    // matching tool
    Gaudi::Property<std::map<std::string, std::vector<std::string>>> m_triggerlist
          {this, "triggerlist", {}, "Trigger list for each year"};

    Gaudi::Property<std::map<std::string, std::vector<std::string>>> m_singleTauTriggers
          {this, "singleTauTriggerList", {}, "Single-tau (STT) trigger chains for each year"};

    Gaudi::Property<std::map<std::string, std::vector<std::string>>> m_largeRJetTriggers
          {this, "largeRJetTriggerList", {}, "Large-R jet (LJT) trigger chains for each year"};

    ToolHandle<Trig::IMatchingTool> m_matchingTool { this, "trigMatchingTool", "", "Trigger matching tool"};
    CP::SysReadDecorHandle<unsigned int> m_year{this, "year", "dataTakingYear", ""};

    //copying
    Gaudi::Property<std::string> m_copied_variable_prefix0{
      this, "copiedVariablePrefix0", "", "prefix for copied variables"
    };
    // floats
    Gaudi::Property<std::vector<std::string>> m_floats_to_copy{
      this, "floatsToCopy", {}, "floats to copy to eventinfo"};
    std::unique_ptr<SystVariableCopier<float>> m0_float_copier;
    // ints
    Gaudi::Property<std::vector<std::string>> m_ints_to_copy{
      this, "intsToCopy", {}, "intss to copy to eventinfo"};
    std::unique_ptr<SystVariableCopier<int>> m0_int_copier;
  
    /// \brief Setup sys-aware output decorations
    CP::SysWriteDecorHandle<int> m_nLRJetsHandle{"lrjets_n_%SYS%", this};
    std::unordered_map<std::string, CP::SysWriteDecorHandle<float>> m_Fbranches;
    std::unordered_map<std::string, CP::SysWriteDecorHandle<int>> m_Ibranches;
    std::unique_ptr<FourVectorOutBlock> m0_z_candidate_4vec;

    /// \brief Per-tau trigger-match variables ("tau_trigMatch_<sanitized chain>")
    std::vector<std::string> m_tauTrigMatchVars;

    void getTriggers(int year, const std::basic_string<char>& key_word, std::vector<std::string>& triggers) const;

    /// \brief Chain name -> decoration name token ('-' -> '_', '.' -> 'p'),
    /// same convention as the trigPassed_* decorations
    static std::string sanitizeChainName(std::string chain);

    /// \brief Look up the chains of the given year in a year->chains map
    static std::vector<std::string> triggersForYear(
      const std::map<std::string, std::vector<std::string>>& trigMap, int year);

    /// \brief Whether the trigPassed_<chain> decoration of the given chain is
    /// available on the event and set (chains not requested from the TDT, e.g.
    /// of another year, count as not fired)
    static bool chainFired(const xAOD::EventInfo& event, const std::string& chain);

  };
}

#endif
