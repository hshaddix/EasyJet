/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

/// Based on bbbb code from @author Frederic Renner

#include "BaselineVarsSemiBoostedAlg.h"
#include "TopologyUtils.h"
#include "AthContainers/AuxElement.h"
#include "xAODBase/IParticleHelpers.h"
#include "xAODJet/JetContainer.h"         // for xAOD::Jet
#include "TLorentzVector.h"
#include "FourMomUtils/xAODP4Helpers.h"

namespace HHH6B

{
  BaselineVarsSemiBoostedAlg ::BaselineVarsSemiBoostedAlg(const std::string &name,
                                                  ISvcLocator *pSvcLocator)
      : AthHistogramAlgorithm(name, pSvcLocator)
  {
  }

  StatusCode BaselineVarsSemiBoostedAlg::initialize()
  {
    ATH_MSG_INFO("*********************************\n");
    ATH_MSG_INFO("      BaselineVarsSemiBoostedAlg     \n");
    ATH_MSG_INFO("*********************************\n");


    ATH_CHECK (m_LargeRJetHandle.initialize(m_systematicsList));
    ATH_CHECK (m_jetHandle.initialize(m_systematicsList));
    ATH_CHECK (m_eventHandle.initialize(m_systematicsList));
    ATH_CHECK(m_R10TruthLabel.initialize(m_systematicsList, m_LargeRJetHandle));

    // make decorators
    for (const std::string &string_var: m_Fvars) {
      CP::SysWriteDecorHandle<float> var {string_var+"_%SYS%", this};
      m_Fdecos.emplace(string_var, var);
      ATH_CHECK (m_Fdecos.at(string_var).initialize(m_systematicsList, m_eventHandle));
    }

    // Initialise syst list (must come after all syst-aware inputs and outputs)
    ATH_CHECK (m_systematicsList.initialize());

    return StatusCode::SUCCESS;

  }

  StatusCode BaselineVarsSemiBoostedAlg ::execute(const EventContext& ctx)
  {
    ATH_MSG_DEBUG("BaselineVarsSemiBoostedAlg::execute() called");
    for (const auto& sys : m_systematicsList.systematicsVector()) {
      // container we read in
      const xAOD::EventInfo *eventInfo = nullptr;
      ANA_CHECK (m_eventHandle.retrieve (eventInfo, sys, ctx));

      const xAOD::JetContainer *largeRjets = nullptr;
      ANA_CHECK (m_LargeRJetHandle.retrieve (largeRjets, sys, ctx));
      std::size_t n_largeRjets = largeRjets->size();

      const xAOD::JetContainer *jets = nullptr;
      ANA_CHECK (m_jetHandle.retrieve (jets, sys, ctx));
    

      // set defaults
      for (const std::string& var : m_Fvars) {
        m_Fdecos.at(var).set(*eventInfo, -99, sys);
      }


      // large jet sector
      ATH_MSG_DEBUG("Processing large-R jets: size = " << largeRjets->size());
      if (n_largeRjets == 2 && jets->size() >= 2) 
      {
        TLorentzVector h1_v4 = largeRjets->at(0)->p4();
        TLorentzVector h2_v4 = largeRjets->at(1)->p4();

        m_Fdecos.at("semiboosted_h1_m").set(*eventInfo, h1_v4.M(), sys);
        m_Fdecos.at("semiboosted_h1_pt").set(*eventInfo, h1_v4.Pt(), sys);
        m_Fdecos.at("semiboosted_h1_eta").set(*eventInfo, h1_v4.Eta(), sys);
        m_Fdecos.at("semiboosted_h1_phi").set(*eventInfo, h1_v4.Phi(), sys);
        m_Fdecos.at("semiboosted_h1_E").set(*eventInfo, h1_v4.E(), sys);

        m_Fdecos.at("semiboosted_h2_m").set(*eventInfo, h2_v4.M(), sys);
        m_Fdecos.at("semiboosted_h2_pt").set(*eventInfo, h2_v4.Pt(), sys);
        m_Fdecos.at("semiboosted_h2_eta").set(*eventInfo, h2_v4.Eta(), sys);
        m_Fdecos.at("semiboosted_h2_phi").set(*eventInfo, h2_v4.Phi(), sys);
        m_Fdecos.at("semiboosted_h2_E").set(*eventInfo, h2_v4.E(), sys);
        

        TLorentzVector tlv1 = jets->at(0)->p4();
        TLorentzVector tlv2 = jets->at(1)->p4();
        TLorentzVector h3_v4 = tlv1 + tlv2;


        m_Fdecos.at("semiboosted_h3_Jet56Mass").set(*eventInfo, (tlv1 + tlv2).M(), sys);
        m_Fdecos.at("semiboosted_h3_DeltaEtaJet5Jet6").set(*eventInfo, std::abs(tlv1.Eta() - tlv2.Eta()), sys);
        m_Fdecos.at("semiboosted_h3_Jet56Pt").set(*eventInfo, (tlv1 + tlv2).Pt(), sys);
        m_Fdecos.at("semiboosted_h3_DeltaR56").set(*eventInfo, xAOD::P4Helpers::deltaR(jets->at(0),jets->at(1)), sys);
        m_Fdecos.at("semiboosted_h3_DeltaPhiJet5Jet6").set(*eventInfo, std::abs((jets->at(0)->p4()).DeltaPhi(jets->at(1)->p4())), sys);

        TLorentzVector hhh_v4 = h1_v4 + h2_v4 + h3_v4;
        m_Fdecos.at("semiboosted_hhh_m").set(*eventInfo, hhh_v4.M(), sys);
        m_Fdecos.at("semiboosted_hhh_pt").set(*eventInfo, hhh_v4.Pt(), sys);
        
        m_Fdecos.at("semiboosted_h1h2_delta_eta").set(*eventInfo, std::abs(h1_v4.Eta() - h2_v4.Eta()), sys);
        m_Fdecos.at("semiboosted_h1h3_delta_eta").set(*eventInfo, std::abs(h1_v4.Eta() - h3_v4.Eta()), sys);
        m_Fdecos.at("semiboosted_h2h3_delta_eta").set(*eventInfo, std::abs(h2_v4.Eta() - h3_v4.Eta()), sys);
        m_Fdecos.at("semiboosted_h1h2_delta_phi").set(*eventInfo, std::abs(h1_v4.DeltaPhi(h2_v4)), sys);
        m_Fdecos.at("semiboosted_h1h3_delta_phi").set(*eventInfo, std::abs(h1_v4.DeltaPhi(h3_v4)), sys);
        m_Fdecos.at("semiboosted_h2h3_delta_phi").set(*eventInfo, std::abs(h2_v4.DeltaPhi(h3_v4)), sys);

        // Mass differences between Higgs canditates
        m_Fdecos.at("semiboosted_h1h2_MassDiff").set(*eventInfo, std::abs(h1_v4.M() - h2_v4.M()), sys);
        m_Fdecos.at("semiboosted_h1h3_MassDiff").set(*eventInfo, std::abs(h1_v4.M() - (tlv1 + tlv2).M()), sys);
        m_Fdecos.at("semiboosted_h2h3_MassDiff").set(*eventInfo, std::abs(h2_v4.M() - (tlv1 + tlv2).M()), sys);

        // Topology variables
        std::vector<TLorentzVector> jetMomenta;
        float totalEnergy = 0.0;
        for (int i = 0; i < 2; ++i) {
          TLorentzVector jetMom = TLorentzVector(largeRjets->at(i)->px(), largeRjets->at(i)->py(), 
                                                 largeRjets->at(i)->pz(), largeRjets->at(i)->e());
          jetMomenta.push_back(jetMom);
          totalEnergy += jetMom.E();
        }
        for (int i = 0; i < 2; ++i) {
          TLorentzVector jetMom = TLorentzVector(jets->at(i)->px(), jets->at(i)->py(), 
                                                 jets->at(i)->pz(), jets->at(i)->e());
          jetMomenta.push_back(jetMom);
          totalEnergy += jetMom.E();
        }

        m_Fdecos.at("semiboosted_Thrust").set(*eventInfo, HHH6B::calculateThrust(jetMomenta, totalEnergy), sys);
        m_Fdecos.at("semiboosted_Sphericity").set(*eventInfo, HHH6B::calculateSphericity(jetMomenta), sys);
        m_Fdecos.at("semiboosted_TransverseSphericity").set(*eventInfo, HHH6B::calculateTransverseSphericity(jetMomenta), sys);
        m_Fdecos.at("semiboosted_Aplanarity").set(*eventInfo, HHH6B::calculateAplanarity(jetMomenta), sys);
        m_Fdecos.at("semiboosted_Planarity").set(*eventInfo, HHH6B::calculatePlanarity(jetMomenta), sys);
        

        for (std::size_t i=0; i<2; i++){

          std::string prefix = "semiboosted_h"+std::to_string(i+1);

          if (m_isMC){
            int truthLabel_i = m_R10TruthLabel.get(*largeRjets->at(i), sys);
            m_Fdecos.at(prefix+"_truthLabel").set(*eventInfo, truthLabel_i, sys);
          }
        }
  

        if (m_isMC){
            static const SG::ConstAccessor<ULong64_t> parentMaskAcc("parentHiggsParentsMask");
            ULong64_t mask1 = parentMaskAcc(*jets->at(0));
            ULong64_t mask2 = parentMaskAcc(*jets->at(1)); 
            m_Fdecos.at("semiboosted_h3_Jet5_parentHiggsParentsMask").set(*eventInfo, mask1, sys);
            m_Fdecos.at("semiboosted_h3_Jet6_parentHiggsParentsMask").set(*eventInfo, mask2, sys);  
        }
        
        m_Fdecos.at("semiboosted_h1Jet5_deltaR").set(*eventInfo, xAOD::P4Helpers::deltaR(largeRjets->at(0),jets->at(0)), sys);
        m_Fdecos.at("semiboosted_h1Jet6_deltaR").set(*eventInfo, xAOD::P4Helpers::deltaR(largeRjets->at(0),jets->at(1)), sys);
        m_Fdecos.at("semiboosted_h2Jet5_deltaR").set(*eventInfo, xAOD::P4Helpers::deltaR(largeRjets->at(1),jets->at(0)), sys);
        m_Fdecos.at("semiboosted_h2Jet6_deltaR").set(*eventInfo, xAOD::P4Helpers::deltaR(largeRjets->at(1),jets->at(1)), sys);
        
      
      }
    }
    return StatusCode::SUCCESS;
  }
}
