/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

/// Based on bbbb code from @author Frederic Renner

#include "BaselineVarsSemiResolvedAlg.h"
#include "TopologyUtils.h"
#include "AthContainers/AuxElement.h"
#include "xAODBase/IParticleHelpers.h"
#include "xAODJet/JetContainer.h"         // for xAOD::Jet
#include "TLorentzVector.h"
#include "FourMomUtils/xAODP4Helpers.h"

namespace HHH6B

{
  BaselineVarsSemiResolvedAlg ::BaselineVarsSemiResolvedAlg(const std::string &name,
                                                  ISvcLocator *pSvcLocator)
      : AthHistogramAlgorithm(name, pSvcLocator)
  {
  }

  StatusCode BaselineVarsSemiResolvedAlg::initialize()
  {
    ATH_MSG_INFO("*********************************\n");
    ATH_MSG_INFO("      BaselineVarsSemiResolvedAlg     \n");
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

  StatusCode BaselineVarsSemiResolvedAlg ::execute()
  {
    ATH_MSG_DEBUG("BaselineVarsSemiResolvedAlg::execute() called");
    for (const auto& sys : m_systematicsList.systematicsVector()) {
      // container we read in
      const xAOD::EventInfo *eventInfo = nullptr;
      ANA_CHECK (m_eventHandle.retrieve (eventInfo, sys));

      const xAOD::JetContainer *largeRjets = nullptr;
      ANA_CHECK (m_LargeRJetHandle.retrieve (largeRjets, sys));
      std::size_t n_largeRjets = largeRjets->size();

      const xAOD::JetContainer *jets = nullptr;
      ANA_CHECK (m_jetHandle.retrieve (jets, sys));


      // set defaults
      for (const std::string& var : m_Fvars) {
        m_Fdecos.at(var).set(*eventInfo, -99, sys);
      }

      // large jet sector
      ATH_MSG_DEBUG("Processing large-R jets: size = " << largeRjets->size());
      if (n_largeRjets == 1 && jets->size() >= 4)
      {
        TLorentzVector h1_v4 = largeRjets->at(0)->p4();

        m_Fdecos.at("semiresolved_h1_m").set(*eventInfo, h1_v4.M(), sys);
        m_Fdecos.at("semiresolved_h1_pt").set(*eventInfo, h1_v4.Pt(), sys);
        m_Fdecos.at("semiresolved_h1_eta").set(*eventInfo, h1_v4.Eta(), sys);
        m_Fdecos.at("semiresolved_h1_phi").set(*eventInfo, h1_v4.Phi(), sys);
        m_Fdecos.at("semiresolved_h1_E").set(*eventInfo, h1_v4.E(), sys);


        TLorentzVector tlv1 = jets->at(0)->p4();
        TLorentzVector tlv2 = jets->at(1)->p4();  
        TLorentzVector h2_v4 = tlv1 + tlv2;

        TLorentzVector tlv3 = jets->at(2)->p4();
        TLorentzVector tlv4 = jets->at(3)->p4();  
        TLorentzVector h3_v4 = tlv3 + tlv4;

        m_Fdecos.at("semiresolved_h2_Jet34Mass").set(*eventInfo, (tlv1 + tlv2).M(), sys);
        m_Fdecos.at("semiresolved_h2_DeltaEtaJet3Jet4").set(*eventInfo, std::abs(tlv1.Eta() - tlv2.Eta()), sys);
        m_Fdecos.at("semiresolved_h2_Jet34Pt").set(*eventInfo, (tlv1 + tlv2).Pt(), sys);
        m_Fdecos.at("semiresolved_h2_DeltaR34").set(*eventInfo, xAOD::P4Helpers::deltaR(jets->at(0),jets->at(1)), sys);
        m_Fdecos.at("semiresolved_h2_DeltaPhiJet3Jet4").set(*eventInfo, std::abs((jets->at(0)->p4()).DeltaPhi(jets->at(1)->p4())), sys);

        m_Fdecos.at("semiresolved_h3_Jet56Mass").set(*eventInfo, (tlv3 + tlv4).M(), sys);
        m_Fdecos.at("semiresolved_h3_DeltaEtaJet5Jet6").set(*eventInfo, std::abs(tlv3.Eta() - tlv4.Eta()), sys);
        m_Fdecos.at("semiresolved_h3_Jet56Pt").set(*eventInfo, (tlv3 + tlv4).Pt(), sys);
        m_Fdecos.at("semiresolved_h3_DeltaR56").set(*eventInfo, xAOD::P4Helpers::deltaR(jets->at(2),jets->at(3)), sys);
        m_Fdecos.at("semiresolved_h3_DeltaPhiJet5Jet6").set(*eventInfo, std::abs((jets->at(2)->p4()).DeltaPhi(jets->at(3)->p4())), sys);

        TLorentzVector hhh_v4 = h1_v4 + h2_v4 + h3_v4;
        m_Fdecos.at("semiresolved_hhh_m").set(*eventInfo, hhh_v4.M(), sys);
        m_Fdecos.at("semiresolved_hhh_pt").set(*eventInfo, hhh_v4.Pt(), sys);
        
        m_Fdecos.at("semiresolved_h1h2_delta_eta").set(*eventInfo, std::abs(h1_v4.Eta() - h2_v4.Eta()), sys);
        m_Fdecos.at("semiresolved_h1h3_delta_eta").set(*eventInfo, std::abs(h1_v4.Eta() - h3_v4.Eta()), sys);
        m_Fdecos.at("semiresolved_h2h3_delta_eta").set(*eventInfo, std::abs(h2_v4.Eta() - h3_v4.Eta()), sys);
        m_Fdecos.at("semiresolved_h1h2_delta_phi").set(*eventInfo, std::abs(h1_v4.DeltaPhi(h2_v4)), sys);
        m_Fdecos.at("semiresolved_h1h3_delta_phi").set(*eventInfo, std::abs(h1_v4.DeltaPhi(h3_v4)), sys);
        m_Fdecos.at("semiresolved_h2h3_delta_phi").set(*eventInfo, std::abs(h2_v4.DeltaPhi(h3_v4)), sys);

        std::string prefix = "semiresolved_h1";
        if (m_isMC){
            int truthLabel0 = m_R10TruthLabel.get(*largeRjets->at(0), sys);
            m_Fdecos.at(prefix+"_truthLabel").set(*eventInfo, truthLabel0, sys);

            static const SG::ConstAccessor<ULong64_t> parentMaskAcc("parentHiggsParentsMask");
            ULong64_t mask1 = parentMaskAcc(*jets->at(0));
            ULong64_t mask2 = parentMaskAcc(*jets->at(1)); 
            ULong64_t mask3 = parentMaskAcc(*jets->at(2));
            ULong64_t mask4 = parentMaskAcc(*jets->at(3)); 
            m_Fdecos.at("semiresolved_h2_Jet3_parentHiggsParentsMask").set(*eventInfo, mask1, sys);
            m_Fdecos.at("semiresolved_h2_Jet4_parentHiggsParentsMask").set(*eventInfo, mask2, sys);
            m_Fdecos.at("semiresolved_h3_Jet5_parentHiggsParentsMask").set(*eventInfo, mask3, sys);
            m_Fdecos.at("semiresolved_h3_Jet6_parentHiggsParentsMask").set(*eventInfo, mask4, sys);    
        }

        // Topology variables
        std::vector<TLorentzVector> jetMomenta;
        float totalEnergy = 0.0;
        TLorentzVector jetMom = TLorentzVector(largeRjets->at(0)->px(), largeRjets->at(0)->py(), 
                                               largeRjets->at(0)->pz(), largeRjets->at(0)->e());
        jetMomenta.push_back(jetMom);
        totalEnergy += jetMom.E();

        for (int i = 0; i < 4; ++i) {
          TLorentzVector jetMom = TLorentzVector(jets->at(i)->px(), jets->at(i)->py(), 
                                                 jets->at(i)->pz(), jets->at(i)->e());
          jetMomenta.push_back(jetMom);
          totalEnergy += jetMom.E();
        }

        m_Fdecos.at("semiresolved_Thrust").set(*eventInfo, HHH6B::calculateThrust(jetMomenta, totalEnergy), sys);
        m_Fdecos.at("semiresolved_Sphericity").set(*eventInfo, HHH6B::calculateSphericity(jetMomenta), sys);
        m_Fdecos.at("semiresolved_TransverseSphericity").set(*eventInfo, HHH6B::calculateTransverseSphericity(jetMomenta), sys);
        m_Fdecos.at("semiresolved_Aplanarity").set(*eventInfo, HHH6B::calculateAplanarity(jetMomenta), sys);
        m_Fdecos.at("semiresolved_Planarity").set(*eventInfo, HHH6B::calculatePlanarity(jetMomenta), sys);

        m_Fdecos.at("semiresolved_h1Jet3_deltaR").set(*eventInfo, xAOD::P4Helpers::deltaR(largeRjets->at(0),jets->at(0)), sys);
        m_Fdecos.at("semiresolved_h1Jet4_deltaR").set(*eventInfo, xAOD::P4Helpers::deltaR(largeRjets->at(0),jets->at(1)), sys);
        m_Fdecos.at("semiresolved_h1Jet5_deltaR").set(*eventInfo, xAOD::P4Helpers::deltaR(largeRjets->at(0),jets->at(2)), sys);
        m_Fdecos.at("semiresolved_h1Jet6_deltaR").set(*eventInfo, xAOD::P4Helpers::deltaR(largeRjets->at(0),jets->at(3)), sys);

        // Mass differences between Higgs canditates
        m_Fdecos.at("semiresolved_h1h2_MassDiff").set(*eventInfo, std::abs(h1_v4.M() - h2_v4.M()), sys);
        m_Fdecos.at("semiresolved_h1h3_MassDiff").set(*eventInfo, std::abs(h1_v4.M() - h3_v4.M()), sys);
        m_Fdecos.at("semiresolved_h2h3_MassDiff").set(*eventInfo, std::abs(h2_v4.M() - h3_v4.M()), sys);
        
      }
    }
    return StatusCode::SUCCESS;
  }
}
