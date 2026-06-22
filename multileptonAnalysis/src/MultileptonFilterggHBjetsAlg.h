/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

// Always protect against multiple includes!

#ifndef SELECTIONFILTERGGHBJETSMultileptonALG_H
#define SELECTIONFILTERGGHBJETSMultileptonALG_H

#include <AsgDataHandles/ReadHandleKey.h>

#include <AthenaBaseComps/AthHistogramAlgorithm.h>
#include <xAODTruth/TruthParticleContainer.h>


#include <EventBookkeeperTools/FilterReporterParams.h>

namespace MULTILEPTON
{
  
  /// \brief An algorithm for counting containers
  class MultileptonFilterggHBjetsAlg final : public AthHistogramAlgorithm {

    public:
      MultileptonFilterggHBjetsAlg(const std::string &name, ISvcLocator *pSvcLocator);

      /// \brief Initialisation method, for setting up tools and other persistent
      /// configs
      StatusCode initialize() override;
      /// \brief Execute method, for actions to be taken in the event loop
      StatusCode execute(const EventContext& ctx) override;
      /// \brief This is the mirror of initialize() and is called after all events are processed.
      StatusCode finalize() override; ///I added this to write the cutflow histogram.

    private :
      SG::ReadHandleKey<xAOD::TruthParticleContainer> m_truthBottomInKey{
        this, "TruthBottomInKey", "TruthBottom",
        "the truth bottom quarks container to run on"};

      FilterReporterParams m_filterParams {this, "noAdditionalBjets" ,"no Additional Bjets selection"};

  };

}

#endif // SELECTIONFILTERGGHBJETSMultileptonALG_H
