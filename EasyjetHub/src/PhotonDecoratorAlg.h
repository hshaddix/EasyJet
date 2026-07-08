/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration

  PhotonDecoratorAlg:
  An alg that copies photon information to aux decorations so can be
  output branch.
*/

// Always protect against multiple includes!
#ifndef EASYJET_PHOTONDECORATORALG
#define EASYJET_PHOTONDECORATORALG

#include <AsgDataHandles/WriteDecorHandleKey.h>
#include <AsgDataHandles/ReadDecorHandleKey.h>
#include <AthenaBaseComps/AthReentrantAlgorithm.h>

#include <xAODEgamma/PhotonContainer.h>
#include <xAODTracking/VertexContainer.h>
#include <xAODTruth/xAODTruthHelpers.h>


namespace Easyjet
{

  /// \brief An algorithm for counting containers
  class PhotonDecoratorAlg final : public AthReentrantAlgorithm
  {
    /// \brief The standard constructor
  public:
    using AthReentrantAlgorithm::AthReentrantAlgorithm;

    /// \brief Initialisation method, for setting up tools and other persistent
    /// configs
    StatusCode initialize() override;
    /// \brief Execute method, for actions to be taken in the event loop
    StatusCode execute(const EventContext& ctx) const override;
    /// We use default finalize() -- this is for cleanup, and we don't do any
    


  private:
    // Members for configurable properties
    Gaudi::Property<bool> m_isMC
      { this, "isMC", false, "Is this simulation?" };
    Gaudi::Property<bool> m_isRun3
      { this, "isRun3", false, "Is this Run3 / mc23?" };
    Gaudi::Property<bool> m_doRetrieveConversionVariables
      {this, "doRetrieveConversionVariables", false, "retrieve conversion information for photons" };
    Gaudi::Property<bool> m_doRetrieveExtendedTruthInfos
      {this, "doRetrieveExtendedTruthInfos", false, "retrieve extended truth-matching information for photons" };

    // Photons
    SG::ReadHandleKey<xAOD::PhotonContainer> m_photonsInKey{
      this, "photonsIn", "", "reco photon container"
    };

    SG::WriteDecorHandleKey<xAOD::PhotonContainer> m_convRadiusDecorKey;
    SG::WriteDecorHandleKey<xAOD::PhotonContainer> m_isConvDecorKey;
    SG::WriteDecorHandleKey<xAOD::PhotonContainer> m_convTypeDecorKey;

    // standard truth-information
    SG::WriteDecorHandleKey<xAOD::PhotonContainer> m_isTruthMatchedDecorKey;
    SG::WriteDecorHandleKey<xAOD::PhotonContainer> m_pdgIdDecorKey;
    SG::WriteDecorHandleKey<xAOD::PhotonContainer> m_parentPdgIdDecorKey;

      // extended truth-matching information
    SG::WriteDecorHandleKey<xAOD::PhotonContainer> m_truthPtDecorKey;
    SG::WriteDecorHandleKey<xAOD::PhotonContainer> m_truthEDecorKey;
    SG::WriteDecorHandleKey<xAOD::PhotonContainer> m_truthEtaDecorKey;
    SG::WriteDecorHandleKey<xAOD::PhotonContainer> m_truthPhiDecorKey;
    SG::WriteDecorHandleKey<xAOD::PhotonContainer> m_truthConvRadiusDecorKey;
 
  };

}

#endif
