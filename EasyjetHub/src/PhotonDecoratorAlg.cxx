/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/


#include "PhotonDecoratorAlg.h"
#include <AsgDataHandles/ReadDecorHandle.h>
#include <AsgDataHandles/WriteDecorHandle.h>

#include <AthenaKernel/Units.h>

#include <xAODEgamma/PhotonxAODHelpers.h>
#include <xAODEgamma/EgammaxAODHelpers.h>
#include <xAODTruth/xAODTruthHelpers.h>
#include <xAODTruth/TruthVertex.h>
#include <TLorentzVector.h>
#include <cmath>


namespace Easyjet
{
  StatusCode PhotonDecoratorAlg ::initialize()
  {
    ATH_CHECK (m_photonsInKey.initialize());

    // Conversion variables
    m_convRadiusDecorKey = m_photonsInKey.key() + ".convRadius";
    m_isConvDecorKey = m_photonsInKey.key() + ".isConv";
    m_convTypeDecorKey = m_photonsInKey.key() + ".convType";
    ATH_CHECK (m_convRadiusDecorKey.initialize(m_doRetrieveConversionVariables));
    ATH_CHECK (m_isConvDecorKey.initialize(m_doRetrieveConversionVariables));
    ATH_CHECK (m_convTypeDecorKey.initialize(m_doRetrieveConversionVariables));

    // Standard truth variables
    m_isTruthMatchedDecorKey = m_photonsInKey.key() + ".is_truth_matched";
    m_pdgIdDecorKey = m_photonsInKey.key() + ".truth_pdgId";
    m_parentPdgIdDecorKey = m_photonsInKey.key() + ".truth_parent_pdgId";
    ATH_CHECK (m_isTruthMatchedDecorKey.initialize(m_isMC));
    ATH_CHECK (m_pdgIdDecorKey.initialize(m_isMC));
    ATH_CHECK (m_parentPdgIdDecorKey.initialize(m_isMC));
    
    // Extended truth-matching information
    m_truthPtDecorKey  = m_photonsInKey.key() + ".truth_pt";
    m_truthEDecorKey   = m_photonsInKey.key() + ".truth_E";
    m_truthEtaDecorKey = m_photonsInKey.key() + ".truth_eta";
    m_truthPhiDecorKey = m_photonsInKey.key() + ".truth_phi";
    m_truthConvRadiusDecorKey = m_photonsInKey.key() + ".truth_convRadius";
    ATH_CHECK (m_truthPtDecorKey.initialize(m_doRetrieveExtendedTruthInfos));
    ATH_CHECK (m_truthEDecorKey.initialize(m_doRetrieveExtendedTruthInfos));
    ATH_CHECK (m_truthEtaDecorKey.initialize(m_doRetrieveExtendedTruthInfos));
    ATH_CHECK (m_truthPhiDecorKey.initialize(m_doRetrieveExtendedTruthInfos));
    ATH_CHECK (m_truthConvRadiusDecorKey.initialize(m_doRetrieveExtendedTruthInfos));

    return StatusCode::SUCCESS;
  }

  StatusCode PhotonDecoratorAlg ::execute(const EventContext& ctx) const
  {
    // input handles
    SG::ReadHandle<xAOD::PhotonContainer> photonsIn(m_photonsInKey,ctx);
    ATH_CHECK (photonsIn.isValid());

    if (m_doRetrieveConversionVariables){

      SG::WriteDecorHandle<xAOD::PhotonContainer, float> convRadiusDecorHandle(m_convRadiusDecorKey, ctx);
      SG::WriteDecorHandle<xAOD::PhotonContainer, int> isConvDecorHandle(m_isConvDecorKey, ctx);
      SG::WriteDecorHandle<xAOD::PhotonContainer, int> convTypeDecorHandle(m_convTypeDecorKey, ctx);

      // loop over photons
      for(const xAOD::Photon* photon : *photonsIn) {
        // conversion radius from EGammaMVAFunctions
        convRadiusDecorHandle(*photon) = xAOD::EgammaHelpers::conversionRadius(photon);

        // conversion type from EGammaHelpers
        // for mc23 and data Run3, the TRT-converted photons (convType 2 and 4) are treated as unconverted by EGamma
        // conversionType!=0 is therefore an incorrect way to identify unconverted photons
        // see https://twiki.cern.ch/twiki/bin/view/AtlasProtected/EGammaIdentificationRun2#TRT_conversion_definition_for_Ru
        bool excludeTRT = m_isRun3;
        isConvDecorHandle(*photon) = xAOD::EgammaHelpers::isConvertedPhoton(photon, excludeTRT);
        convTypeDecorHandle(*photon) = photon->conversionType();
      }
    }

    if(m_isMC){

      // truth-matching information
      SG::WriteDecorHandle<xAOD::PhotonContainer, bool> isTruthMatchedDecorHandle(m_isTruthMatchedDecorKey, ctx);
      SG::WriteDecorHandle<xAOD::PhotonContainer, int> pdgIdDecorHandle(m_pdgIdDecorKey, ctx);
      SG::WriteDecorHandle<xAOD::PhotonContainer, int> parentPdgIdDecorHandle(m_parentPdgIdDecorKey, ctx);

      // loop over photons
      for(const xAOD::Photon* photon : *photonsIn) {
          const xAOD::TruthParticle* truthPart = xAOD::TruthHelpers::getTruthParticle(*photon);

          isTruthMatchedDecorHandle(*photon) = false;
          pdgIdDecorHandle(*photon) = 0;
          parentPdgIdDecorHandle(*photon) = 0;
          
          if(truthPart){
              isTruthMatchedDecorHandle(*photon) = true;
              pdgIdDecorHandle(*photon) = truthPart->pdgId();
              parentPdgIdDecorHandle(*photon) = (truthPart->nParents() > 0 && truthPart->parent(0)) ? truthPart->parent(0)->pdgId() : 0;
          }
      }
      // second loop to set the extended truth-matching information
      if (m_doRetrieveExtendedTruthInfos){
        // extended truth-matching information
        SG::WriteDecorHandle<xAOD::PhotonContainer, float> truthPtDecorHandle(m_truthPtDecorKey, ctx);
        SG::WriteDecorHandle<xAOD::PhotonContainer, float> truthEDecorHandle(m_truthEDecorKey, ctx);
        SG::WriteDecorHandle<xAOD::PhotonContainer, float> truthEtaDecorHandle(m_truthEtaDecorKey, ctx);
        SG::WriteDecorHandle<xAOD::PhotonContainer, float> truthPhiDecorHandle(m_truthPhiDecorKey, ctx);
        SG::WriteDecorHandle<xAOD::PhotonContainer, float> truthConvRadiusDecorHandle(m_truthConvRadiusDecorKey, ctx);
        // loop over photons
        for(const xAOD::Photon* photon : *photonsIn) {
          const xAOD::TruthParticle* truthPart = xAOD::TruthHelpers::getTruthParticle(*photon);

          truthPtDecorHandle(*photon) = -99.;
          truthEDecorHandle(*photon) = -99.;
          truthEtaDecorHandle(*photon) = -99.;
          truthPhiDecorHandle(*photon) = -99.;
          truthConvRadiusDecorHandle(*photon) = -99.;
          
          if(truthPart){
            truthPtDecorHandle(*photon) = truthPart->pt();
            truthEDecorHandle(*photon) = truthPart->e();
            truthEtaDecorHandle(*photon) = truthPart->eta();
            truthPhiDecorHandle(*photon) = truthPart->phi();
            truthConvRadiusDecorHandle(*photon) = (truthPart->pdgId() == 22) ? (truthPart->hasDecayVtx() ? truthPart->decayVtx()->perp() : 9999.) : -99;
          } 
        }
      }
    }
    return StatusCode::SUCCESS;
  }

}
