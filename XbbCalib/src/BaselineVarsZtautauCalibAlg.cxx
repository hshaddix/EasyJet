/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "BaselineVarsZtautauCalibAlg.h"

namespace XBBCALIB
{
  StatusCode BaselineVarsZtautauCalibAlg::initialize()
  {
    ATH_CHECK (m_lRjetHandle.initialize(m_systematicsList));
    ATH_CHECK (m_zcandJetHandle.initialize(m_systematicsList));
    ATH_CHECK (m_zcandMetMassDecor.initialize(m_systematicsList, m_zcandJetHandle));
    ATH_CHECK (m_tauHandle.initialize(m_systematicsList));
    ATH_CHECK (m_eventHandle.initialize(m_systematicsList));
    ATH_CHECK(m_nLRJetsHandle.initialize(m_systematicsList, m_eventHandle));
    ATH_CHECK (m_zcandTruthJetDecor.initialize(m_systematicsList, m_zcandJetHandle));

    ATH_CHECK(m_year.initialize(m_systematicsList, m_eventHandle));

    ATH_CHECK (m_matchingTool.retrieve());

    // Intialise syst-aware output decorators
    for (const std::string &var : m_floatVariables) {
      CP::SysWriteDecorHandle<float> whandle{var+"_%SYS%", this};
      m_Fbranches.emplace(var, whandle);
      ATH_CHECK (m_Fbranches.at(var).initialize(m_systematicsList, m_eventHandle));
    }

    for (const std::string &var : m_intVariables){
      CP::SysWriteDecorHandle<int> whandle{var+"_%SYS%", this};
      m_Ibranches.emplace(var, whandle);
      ATH_CHECK(m_Ibranches.at(var).initialize(m_systematicsList, m_eventHandle));
    };

    m0_z_candidate_4vec = std::make_unique<FourVectorOutBlock>(
      this, m_copied_variable_prefix0, m_systematicsList, m_eventHandle);
    // float copying 
    m0_float_copier = std::make_unique<SystVariableCopier<float>> (
      this,
      m_floats_to_copy,
      m_systematicsList,
      m_zcandJetHandle,
      m_eventHandle,
      m_copied_variable_prefix0);
    // int copying
    m0_int_copier = std::make_unique<SystVariableCopier<int>> (
      this,
      m_ints_to_copy,
      m_systematicsList,
      m_zcandJetHandle,
      m_eventHandle,
      m_copied_variable_prefix0);

    // Intialise syst list (must come after all syst-aware inputs and outputs)
    ATH_CHECK (m_systematicsList.initialize());

    return StatusCode::SUCCESS;
  }

  StatusCode BaselineVarsZtautauCalibAlg::execute(const EventContext& ctx)
  {
    //Loop over all systs
    for (const auto& sys : m_systematicsList.systematicsVector())
    {

      // Initialize variables
      const xAOD::EventInfo *event = nullptr;
      ANA_CHECK (m_eventHandle.retrieve (event, sys, ctx));

      const xAOD::JetContainer *largeRjets = nullptr;
      ANA_CHECK (m_lRjetHandle.retrieve (largeRjets, sys, ctx));

      // Ordered jets from ZtautauCalibSelectorAlg: [0] = Z candidate, [1] = recoiling jet.
      // The Z candidate is already decorated there with its MET-corrected mass
      const xAOD::JetContainer *zcandJets = nullptr;
      ANA_CHECK (m_zcandJetHandle.retrieve (zcandJets, sys, ctx));

      const xAOD::TauJetContainer *taus = nullptr;
      ANA_CHECK (m_tauHandle.retrieve (taus, sys, ctx));

      for (const std::string &string_var: m_floatVariables) {
        m_Fbranches.at(string_var).set(*event, -99., sys);
      }

      for (const auto& var: m_intVariables) {
        m_Ibranches.at(var).set(*event, -99, sys);
      }

      size_t n_lrjets = largeRjets->size();
      m_nLRJetsHandle.set(*event, n_lrjets, sys);
      // The Z candidate (index 0) and recoiling jet (index 1) are selected
      // upstream in ZtautauCalibSelectorAlg::execute, as the large-R jet with
      // the smallest |Delta Phi| w.r.t. MET (mirrors python getZcandLR()).
      if (zcandJets->size() == 2) {
        const xAOD::Jet* zcandJet = zcandJets->at(0);
        m0_z_candidate_4vec->set(*event, *zcandJet, sys);
        m0_float_copier->set(*event, *zcandJet, sys);
        m0_int_copier->set(*event, *zcandJet, sys);

        // Zcand mass corrected with MET in the collinear approximation, read back
        // via the same per-systematic key template as it was written with in
        // ZtautauCalibSelectorAlg::execute (met_mass_%SYS%).
        const float zcand_met_mass = m_zcandMetMassDecor.get(*zcandJet, sys);
        const auto zcand_truth_jet = m_zcandTruthJetDecor.get(*zcandJet, sys);
        m_Fbranches.at("Zcand0_met_mass").set(*event, zcand_met_mass, sys);
        m_Fbranches.at("Zcand0_truth_m").set(*event, zcand_truth_jet.M(), sys);
        m_Fbranches.at("Zcand0_truth_eta").set(*event, zcand_truth_jet.Eta(), sys);
        m_Fbranches.at("Zcand0_truth_phi").set(*event, zcand_truth_jet.Phi(), sys);
        m_Fbranches.at("Zcand0_truth_pt").set(*event, zcand_truth_jet.Pt(), sys);
      } else {
        m0_z_candidate_4vec->setDefault(*event, sys);
        m0_float_copier->setDefault(*event, sys);
        m0_int_copier->setDefault(*event, sys);
      }

      // look up for triggers
      // TODO: __TauTauCalib__ LargeR jet trigger match information is not stored yet
      // see:  https://its.cern.ch/jira/browse/ATR-26050
      // int year = m_year.get(*event, sys);
      // std::vector<std::string> large_r_jet_triggers;
      // getTriggers(year, "HLT_j", large_r_jet_triggers);
      // // Calculate vars
      // for (const xAOD::Jet *lRjet : *largeRjets) {
      //   // check large-r jet vs lare-r jet triggers 
      //   for(const auto& trig : large_r_jet_triggers){
      //     bool match = m_matchingTool->match(*lRjet, trig);
      //     if (match) m_Ibranches.at("largeR_trigMatch").set(*lRjet, match, sys);
      //   }
      // }

      // Count jets
      m_Ibranches.at("nJets").set(*event, largeRjets->size(), sys);
      m_Ibranches.at("nTaus").set(*event, taus->size(), sys);

    }
    return StatusCode::SUCCESS;
  }

  void BaselineVarsZtautauCalibAlg::getTriggers(
    int year,
    const std::basic_string<char>& key_word,
    std::vector<std::string>& triggers
  ) const {

    for (auto&& [m_year, trigger_list] : m_triggerlist)
      if (year == std::stoi(m_year))
        for (auto&& trig : trigger_list)
          if (trig.starts_with(key_word))
            triggers.push_back(trig);
  }
}
