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

    Gaudi::Property<std::vector<std::string>> m_floatVariables
          {this, "floatVariableList", {}, "Name list of floating variables"};
  
    Gaudi::Property<std::vector<std::string>> m_intVariables
          {this, "intVariableList", {}, "Name list of integer variables"};

    // matching tool
    Gaudi::Property<std::map<std::string, std::vector<std::string>>> m_triggerlist
          {this, "triggerlist", {}, "Trigger list for each year"};

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

    void getTriggers(int year, const std::basic_string<char>& key_word, std::vector<std::string>& triggers) const;

  };
}

#endif
