/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

// Always protect against multiple includes!
#ifndef HHMETANALYSIS_FINALVARSCOMMONALG
#define HHMETANALYSIS_FINALVARSCOMMONALG

#include <AthenaBaseComps/AthHistogramAlgorithm.h>

#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysListHandle.h>
#include <SystematicsHandles/SysWriteDecorHandle.h>
#include <SystematicsHandles/SysReadDecorHandle.h>

#include <xAODEventInfo/EventInfo.h>
#include <xAODJet/JetContainer.h>

namespace HHMET
{

  /// \brief An algorithm for counting containers
  class BaselineVarsCommonAlg final : public AthHistogramAlgorithm
  {
    /// \brief The standard constructor
    public:

      BaselineVarsCommonAlg(const std::string &name, ISvcLocator *pSvcLocator);

      /// \brief Initialisation method, for setting up tools and other persistent
      /// configs
      StatusCode initialize() override;
      /// \brief Execute method, for actions to be taken in the event loop
      StatusCode execute(const EventContext& ctx) override;
      /// We use default finalize() -- this is for cleanup, and we don't do any

    private:

      // ToolHandle<whatever> handle {this, "pythonName", "defaultValue",
      // "someInfo"};

      CP::SysListHandle m_systematicsList {this};

      CP::SysReadHandle<xAOD::EventInfo>
      m_eventHandle{ this, "event", "EventInfo", "EventInfo container to read" };

      std::unordered_map<std::string, CP::SysWriteDecorHandle<float> > m_genbranches;

      // clang-format off
      std::vector<std::string> m_genVarList{
          "GenFiltMET",
          "GenFiltHT"
      };

      Gaudi::Property<bool> m_isMC
        { this, "isMC", false, "Is this simulation?" };


      CP::SysReadDecorHandle<float>
          m_genFiltMET{ this, "GenFiltMET", "GenFiltMET", "MC Gen filter MET" };

      CP::SysReadDecorHandle<float>
          m_genFiltHT{ this, "GenFiltHT", "GenFiltHT", "MC Gen filter HT" };

      // clang-format on
  
  };

}

#endif
