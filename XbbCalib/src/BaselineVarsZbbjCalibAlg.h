/*
  Copyright (C) 2002-2024 CERN for the benefit of the ATLAS collaboration
*/

// Always protect against multiple includes!
#ifndef ZBBJCALIB_FINALVARSXBBCALIBALG
#define ZBBJCALIB_FINALVARSXBBCALIBALG

#include <AthenaBaseComps/AthHistogramAlgorithm.h>

#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysListHandle.h>
#include <SystematicsHandles/SysWriteDecorHandle.h>
#include <SystematicsHandles/SysReadDecorHandle.h>
#include <xAODEventInfo/EventInfo.h>
#include <xAODJet/JetContainer.h>
#include "SystVariableCopier.h"
#include "FourVectorOutBlock.h" 
#include <memory>


namespace XBBCALIB
{

  /// \brief An algorithm for counting containers
  class BaselineVarsZbbjCalibAlg final : public AthHistogramAlgorithm
  {
    /// \brief The standard constructor
public:
    BaselineVarsZbbjCalibAlg(const std::string &name, ISvcLocator *pSvcLocator);
        
    ~BaselineVarsZbbjCalibAlg();
    ///  \\brief Initialisation method, for setting up tools and other persistent
    /// configs
    StatusCode initialize() override;
    /// \brief Execute method, for actions to be taken in the event loop
    StatusCode execute(const EventContext& ctx) override;
    /// We use default finalize() -- this is for cleanup, and we don't do any


private:

    CP::SysListHandle m_systematicsList {this};

    CP::SysReadHandle<xAOD::JetContainer>
    m_lrjetHandle{ this, "largeRJets", "",   "Large-R jet container to read" };

    CP::SysReadHandle<xAOD::EventInfo>
    m_eventHandle{ this, "event", "EventInfo",   "EventInfo container to read" };
    
     // writing 

    //jets
    CP::SysWriteDecorHandle<int> m_lrjets_n{"lrjets_n_%SYS%", this};

    //copying

    Gaudi::Property<std::string> m_copied_variable_prefix0{
      this, "copiedVariablePrefix0", "", "prefix for copied variables"
    };

    Gaudi::Property<std::string> m_copied_variable_prefix1{
      this, "copiedVariablePrefix1", "", "prefix for copied variables"
    };

    // floats
    Gaudi::Property<std::vector<std::string>> m_floats_to_copy{
      this, "floatsToCopy", {}, "floats to copy to eventinfo"};
    std::unique_ptr<SystVariableCopier<float>> m0_float_copier;
    std::unique_ptr<SystVariableCopier<float>> m1_float_copier;
    // ints
    Gaudi::Property<std::vector<std::string>> m_ints_to_copy{
      this, "intsToCopy", {}, "intss to copy to eventinfo"};
    std::unique_ptr<SystVariableCopier<int>> m0_int_copier;
    std::unique_ptr<SystVariableCopier<int>> m1_int_copier;

    /// \brief Setup sys-aware output decorations
  
    std::unique_ptr<FourVectorOutBlock> m0_z_candidate_4vec;
    std::unique_ptr<FourVectorOutBlock> m1_z_candidate_4vec;
  
  };
}

#endif
