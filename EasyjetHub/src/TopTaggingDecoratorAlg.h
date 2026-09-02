/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration

  TopTaggingDecoratorAlg:
  DNN Top tagger to UFO jets
*/



// Always protect against multiple includes!
#ifndef EASYJET_TOPTAGGINGDECORATORALG
#define EASYJET_TOPTAGGINGDECORATORALG

#include <vector>
#include <utility>

#include <AthenaBaseComps/AthAlgorithm.h>
#include <xAODJet/JetContainer.h>
#include "BoostedJetTaggers/JSSTaggerBase.h"

namespace Easyjet
{

  /// \brief An algorithm for counting containers
  class TopTaggingDecoratorAlg final : public AthAlgorithm
  {
    /// \brief The standard constructor
public:
    using AthAlgorithm::AthAlgorithm;

    /// \brief Initialisation method, for setting up tools and other persistent
    /// configs
    StatusCode initialize() override;
    /// \brief Execute method, for actions to be taken in the event loop
    StatusCode execute(const EventContext& ctx) override;
    /// We use default finalize() -- this is for cleanup, and we don't do any

private:

    // Members for configurable properties
    SG::ReadHandleKey<xAOD::JetContainer> m_jetsInKey{
      this, "jetsIn", "", "containerName to read"
    };

    ToolHandle<JSSTaggerBase> m_Toptagger{this, "Toptagger", "", "Top-tagging tool"};

  };
}

#endif
