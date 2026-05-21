/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

/// Based on bbbb code from @author Frederic Renner

#include "BaselineVarsBoostedAlg.h"
#include "TopologyUtils.h"
#include "AthContainers/AuxElement.h"

namespace HHH6B
{
  BaselineVarsBoostedAlg ::BaselineVarsBoostedAlg(const std::string &name,
                                                  ISvcLocator *pSvcLocator)
      : AthHistogramAlgorithm(name, pSvcLocator)
  {
  }

  StatusCode BaselineVarsBoostedAlg ::initialize()
  {
    ATH_MSG_INFO("*********************************\n");
    ATH_MSG_INFO("      BaselineVarsBoostedAlg     \n");
    ATH_MSG_INFO("*********************************\n");

    ATH_CHECK (m_LargeRJetHandle.initialize(m_systematicsList));
    ATH_CHECK (m_eventHandle.initialize(m_systematicsList));

    ATH_CHECK(m_R10TruthLabel.initialize(m_systematicsList, m_LargeRJetHandle));
    ATH_CHECK(m_Tau2_wta.initialize(m_systematicsList, m_LargeRJetHandle));
    ATH_CHECK(m_Tau3_wta.initialize(m_systematicsList, m_LargeRJetHandle));

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

  StatusCode BaselineVarsBoostedAlg ::execute()
  {
    ATH_MSG_DEBUG("BaselineVarsBoostedAlg::execute() called");
    for (const auto& sys : m_systematicsList.systematicsVector()) {
      // container we read in
      const xAOD::EventInfo *eventInfo = nullptr;
      ANA_CHECK (m_eventHandle.retrieve (eventInfo, sys));

      const xAOD::JetContainer *largeRjets = nullptr;
      ANA_CHECK (m_LargeRJetHandle.retrieve (largeRjets, sys));
      std::size_t n_largeRjets = largeRjets->size();

      // set defaults
      for (const std::string& var : m_Fvars) {
        m_Fdecos.at(var).set(*eventInfo, -99, sys);
      }

      // large jet sector
      ATH_MSG_DEBUG("Processing large-R jets: size = " << largeRjets->size());
      if (n_largeRjets >= 3)
      {
        TLorentzVector h1_v4 = largeRjets->at(0)->p4();
        TLorentzVector h2_v4 = largeRjets->at(1)->p4();
        TLorentzVector h3_v4 = largeRjets->at(2)->p4();

        // Properties of first Higgs candidate
        m_Fdecos.at("boosted_h1_m").set(*eventInfo, h1_v4.M(), sys);
        m_Fdecos.at("boosted_h1_pt").set(*eventInfo, h1_v4.Pt(), sys);
        m_Fdecos.at("boosted_h1_eta").set(*eventInfo, h1_v4.Eta(), sys);
        m_Fdecos.at("boosted_h1_phi").set(*eventInfo, h1_v4.Phi(), sys);
        m_Fdecos.at("boosted_h1_E").set(*eventInfo, h1_v4.E(), sys);

        // Properties of second Higgs candidate
        m_Fdecos.at("boosted_h2_m").set(*eventInfo, h2_v4.M(), sys);
        m_Fdecos.at("boosted_h2_pt").set(*eventInfo, h2_v4.Pt(), sys);
        m_Fdecos.at("boosted_h2_eta").set(*eventInfo, h2_v4.Eta(), sys);
        m_Fdecos.at("boosted_h2_phi").set(*eventInfo, h2_v4.Phi(), sys);
        m_Fdecos.at("boosted_h2_E").set(*eventInfo, h2_v4.E(), sys);

        // Properties of third Higgs candidate
        m_Fdecos.at("boosted_h3_m").set(*eventInfo, h3_v4.M(), sys);
        m_Fdecos.at("boosted_h3_pt").set(*eventInfo, h3_v4.Pt(), sys);
        m_Fdecos.at("boosted_h3_eta").set(*eventInfo, h3_v4.Eta(), sys);
        m_Fdecos.at("boosted_h3_phi").set(*eventInfo, h3_v4.Phi(), sys);
        m_Fdecos.at("boosted_h3_E").set(*eventInfo, h3_v4.E(), sys);

        // Properties of the combined 3-Higgs system
        TLorentzVector hhh_v4 = h1_v4 + h2_v4 + h3_v4;
        m_Fdecos.at("boosted_hhh_m").set(*eventInfo, hhh_v4.M(), sys);
        m_Fdecos.at("boosted_hhh_pt").set(*eventInfo, hhh_v4.Pt(), sys);
        
        // Geometric differences between Higgs candidates
        m_Fdecos.at("boosted_h1h2_delta_eta").set(*eventInfo, std::abs(h1_v4.Eta() - h2_v4.Eta()), sys);
        m_Fdecos.at("boosted_h1h3_delta_eta").set(*eventInfo, std::abs(h1_v4.Eta() - h3_v4.Eta()), sys);
        m_Fdecos.at("boosted_h2h3_delta_eta").set(*eventInfo, std::abs(h2_v4.Eta() - h3_v4.Eta()), sys);
        m_Fdecos.at("boosted_h1h2_delta_phi").set(*eventInfo, std::abs(h1_v4.DeltaPhi(h2_v4)), sys);
        m_Fdecos.at("boosted_h1h3_delta_phi").set(*eventInfo, std::abs(h1_v4.DeltaPhi(h3_v4)), sys);
        m_Fdecos.at("boosted_h2h3_delta_phi").set(*eventInfo, std::abs(h2_v4.DeltaPhi(h3_v4)), sys);


        // Mass differences between Higgs canditates
        m_Fdecos.at("boosted_h1h2_MassDiff").set(*eventInfo, std::abs(h1_v4.M() - h2_v4.M()), sys);
        m_Fdecos.at("boosted_h1h3_MassDiff").set(*eventInfo, std::abs(h1_v4.M() - h3_v4.M()), sys);
        m_Fdecos.at("boosted_h2h3_MassDiff").set(*eventInfo, std::abs(h2_v4.M() - h3_v4.M()), sys);

        // Topology variables
        std::vector<TLorentzVector> jetMomenta;
        float totalEnergy = 0.0;
        for (int i = 0; i < 3; ++i) {
          TLorentzVector jetMom = TLorentzVector(largeRjets->at(i)->px(), largeRjets->at(i)->py(), 
                                                 largeRjets->at(i)->pz(), largeRjets->at(i)->e());
          jetMomenta.push_back(jetMom);
          totalEnergy += jetMom.E();
        }

        m_Fdecos.at("boosted_Thrust").set(*eventInfo, HHH6B::calculateThrust(jetMomenta, totalEnergy), sys);
        m_Fdecos.at("boosted_Sphericity").set(*eventInfo, HHH6B::calculateSphericity(jetMomenta), sys);
        m_Fdecos.at("boosted_TransverseSphericity").set(*eventInfo, HHH6B::calculateTransverseSphericity(jetMomenta), sys);
        m_Fdecos.at("boosted_Aplanarity").set(*eventInfo, HHH6B::calculateAplanarity(jetMomenta), sys);
        m_Fdecos.at("boosted_Planarity").set(*eventInfo, HHH6B::calculatePlanarity(jetMomenta), sys);

        for (std::size_t i=0; i<3; i++){

          std::string prefix = "boosted_h"+std::to_string(i+1);

          float tau32 = m_Tau3_wta.get(*largeRjets->at(i),sys) /
	    m_Tau2_wta.get(*largeRjets->at(i),sys);
          m_Fdecos.at(prefix+"_Tau32_wta").set(*eventInfo, tau32, sys);

          if (m_isMC){
            int truthLabel_i = m_R10TruthLabel.get(*largeRjets->at(i), sys);
            m_Fdecos.at(prefix+"_truthLabel").set(*eventInfo, truthLabel_i, sys);
          }
        }
      }
    }
    return StatusCode::SUCCESS;
  }
}
