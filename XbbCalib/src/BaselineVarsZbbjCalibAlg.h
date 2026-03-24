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
#include <xAODEgamma/ElectronContainer.h>
#include <xAODMuon/MuonContainer.h>
#include <xAODMissingET/MissingETContainer.h>

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

    /// \brief Initialisation method, for setting up tools and other persistent
    /// configs
    StatusCode initialize() override;
    /// \brief Execute method, for actions to be taken in the event loop
    StatusCode execute() override;
    /// We use default finalize() -- this is for cleanup, and we don't do any


private:

    CP::SysListHandle m_systematicsList {this};

    CP::SysReadHandle<xAOD::JetContainer>
    m_lrjetHandle{ this, "largeRJets", "",   "Large-R jet container to read" };

    CP::SysReadHandle<xAOD::EventInfo>
    m_eventHandle{ this, "event", "EventInfo",   "EventInfo container to read" };

    CP::SysWriteDecorHandle<int>
      m_lrjets_n{this, "lrjetsNDecoration", "lrjets_n_%SYS%",
                 "number of selected large-R jets"};

    std::unique_ptr<FourVectorOutBlock> m_z_candidate_4vec;
  };
}

#endif
