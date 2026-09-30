/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "BaselineVarsZtautauCalibAlg.h"

#include <algorithm>

namespace XBBCALIB
{
  StatusCode BaselineVarsZtautauCalibAlg::initialize()
  {
    ATH_CHECK (m_lRjetHandle.initialize(m_systematicsList));
    ATH_CHECK (m_zcandJetHandle.initialize(m_systematicsList));
    ATH_CHECK (m_zcandMetMassDecor.initialize(m_systematicsList, m_zcandJetHandle));
    ATH_CHECK (m_tauHandle.initialize(m_systematicsList));
    ATH_CHECK (m_allTauHandle.initialize(m_systematicsList));
    ATH_CHECK (m_tau0IDDecor.initialize(m_systematicsList, m_allTauHandle));
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
      if (var.starts_with("tau_trigMatch_")) m_tauTrigMatchVars.push_back(var);
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

      const int year = m_year.get(*event, sys);

      // tau trigger matching: one flag per chain plus the STT group flag,
      // decorated on every tau of the full calibrated container
      const xAOD::TauJetContainer *allTaus = nullptr;
      ANA_CHECK (m_allTauHandle.retrieve (allTaus, sys, ctx));

      for (const xAOD::TauJet *tau : *allTaus)
        for (const std::string &var : m_tauTrigMatchVars)
          m_Ibranches.at(var).set(*tau, 0, sys);

      // returns the OR of the group's chains; fills `matched` with the taus
      // matched to at least one fired chain of the group
      auto matchTauTriggers =
        [&](const std::vector<std::string>& triggers, const std::string& groupVar,
            std::vector<const xAOD::TauJet*>& matched) {
          bool groupPassed = false;
          for (const auto& trig : triggers) {
            // only match chains that fired (decorated upstream by the trigger
            // selection alg for every chain in the trigger list)
            if (!chainFired(*event, trig)) continue;
            groupPassed = true;

            const std::string var = "tau_trigMatch_" + sanitizeChainName(trig);
            for (const xAOD::TauJet *tau : *allTaus) {
              if (m_matchingTool->match(*tau, trig, 0.2)) {
                if (m_Ibranches.count(var)) m_Ibranches.at(var).set(*tau, 1, sys);
                m_Ibranches.at("tau_trigMatch_" + groupVar).set(*tau, 1, sys);
                if (std::find(matched.begin(), matched.end(), tau) == matched.end())
                  matched.push_back(tau);
              }
            }
          }
          m_Ibranches.at("trigPassed_" + groupVar).set(*event, groupPassed, sys);
          return groupPassed;
        };

      std::vector<const xAOD::TauJet*> sttMatchedTaus;
      const bool passSTTTrigger =
        matchTauTriggers(triggersForYear(m_singleTauTriggers, year), "STT", sttMatchedTaus);

      // large-R jet trigger (LJT) decision: OR of the year's chains
      bool passLJTTrigger = false;
      for (const auto& trig : triggersForYear(m_largeRJetTriggers, year))
        if (chainFired(*event, trig)) { passLJTTrigger = true; break; }

      // ---------------------------------------------------------------------
      // Analysis selections of the two trigger paths (mirrors the offline
      // zllntuples zttj_tautrig analysis + the ZCalibrationFitter selections):
      //   common: baseline selection of ZtautauCalibSelectorAlg (Z candidate
      //           found, i.e. 2 large-R jets, no electrons, |dphi| >= 1.5,
      //           MET > minMet, t21 <= 0.2, 0 <= alpha <= 1)
      //   LJT: any large-R jet trigger of the year fired
      //   STT: any single-tau trigger fired, the highest-pT STT-matched tau
      //        (tau0) lies inside the Z-candidate jet, has pT > minSTTTauPt
      //        and passes the tau ID (RNNLoose, tau0IDSelectionDecor);
      //        additionally split into 1-prong / 3-prong tau0 (stt1p / stt3p)
      // ---------------------------------------------------------------------
      const xAOD::Jet* zcandJet = (zcandJets->size() == 2) ? zcandJets->at(0) : nullptr;

      // tau0: highest-pT STT-matched tau
      const xAOD::TauJet* tau0 = nullptr;
      for (const xAOD::TauJet* tau : sttMatchedTaus)
        if (!tau0 || tau->pt() > tau0->pt()) tau0 = tau;

      m_Ibranches.at("nTaus_STT_matched").set(*event, sttMatchedTaus.size(), sys);
      const int tau0_nProng = tau0 ? static_cast<int>(tau0->nTracks()) : -1;
      // DeltaR(tau0, Z candidate); only meaningful when both exist
      const float tau0_dR_zcand =
        (tau0 && zcandJet) ? zcandJet->p4().DeltaR(tau0->p4()) : -99.;
      if (tau0) {
        m_Fbranches.at("tau0_pt").set(*event, tau0->pt(), sys);
        m_Fbranches.at("tau0_eta").set(*event, tau0->eta(), sys);
        m_Fbranches.at("tau0_phi").set(*event, tau0->phi(), sys);
        m_Ibranches.at("tau0_nProng").set(*event, tau0_nProng, sys);
        m_Ibranches.at("tau0_charge").set(*event, static_cast<int>(tau0->charge()), sys);
        m_Ibranches.at("tau0_index").set(*event, static_cast<int>(tau0->index()), sys);
        if (zcandJet) m_Fbranches.at("tau0_dR_Zcand").set(*event, tau0_dR_zcand, sys);
      }

      const bool passLJT = zcandJet && passLJTTrigger;
      const bool passSTT = zcandJet && passSTTTrigger && tau0
        && tau0_dR_zcand < m_maxSTTTauZcandDeltaR
        && tau0->pt() > m_minSTTTauPt
        && m_tau0IDDecor.get(*tau0, sys);

      m_Ibranches.at("pass_LJT").set(*event, passLJT, sys);
      m_Ibranches.at("pass_STT").set(*event, passSTT, sys);

      // per-path copies of the histogram variables, filled only when the path
      // is selected (the -99 default lands in the histogram underflow)
      auto setPathVars = [&](const std::string& path, bool pass) {
        if (!pass) return;
        m_Fbranches.at("Zcand0_" + path + "_met_mass").set(
          *event, m_zcandMetMassDecor.get(*zcandJet, sys), sys);
        m_Fbranches.at("Zcand0_" + path + "_pt").set(*event, zcandJet->pt(), sys);
        m_Fbranches.at("Zcand0_" + path + "_truth_pt").set(
          *event, m_zcandTruthJetDecor.get(*zcandJet, sys).Pt(), sys);
      };
      setPathVars("ljt", passLJT);
      setPathVars("stt", passSTT);
      // STT path split by the prongness of tau0
      setPathVars("stt1p", passSTT && tau0_nProng == 1);
      setPathVars("stt3p", passSTT && tau0_nProng == 3);

    }
    return StatusCode::SUCCESS;
  }

  std::string BaselineVarsZtautauCalibAlg::sanitizeChainName(std::string chain)
  {
    std::replace(chain.begin(), chain.end(), '-', '_');
    std::replace(chain.begin(), chain.end(), '.', 'p');
    return chain;
  }

  std::vector<std::string> BaselineVarsZtautauCalibAlg::triggersForYear(
    const std::map<std::string, std::vector<std::string>>& trigMap, int year)
  {
    for (const auto& [trig_year, trigger_list] : trigMap)
      if (year == std::stoi(trig_year))
        return trigger_list;
    return {};
  }

  bool BaselineVarsZtautauCalibAlg::chainFired(
    const xAOD::EventInfo& event, const std::string& chain)
  {
    const SG::AuxElement::ConstAccessor<bool> passed("trigPassed_" + sanitizeChainName(chain));
    return passed.isAvailable(event) && passed(event);
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
