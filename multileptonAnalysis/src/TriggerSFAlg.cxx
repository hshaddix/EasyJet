/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "TriggerSFAlg.h"

#include "AthContainers/AuxElement.h"
#include <AthContainers/ConstDataVector.h>

#include "MultileptonSelectorAlg.h"
#include "TriggerUtils.h"

#include "TLorentzVector.h"

namespace MULTILEPTON
{
  StatusCode TriggerSFAlg::initialize()
  {

    // Read syst-aware input handles
    ATH_CHECK (m_jetHandle.initialize(m_systematicsList));
    ATH_CHECK (m_tauHandle.initialize(m_systematicsList));
    ATH_CHECK (m_hhmlTauHandle.initialize(m_systematicsList));
    ATH_CHECK (m_electronHandle.initialize(m_systematicsList));
    ATH_CHECK (m_hhmlEleHandle.initialize(m_systematicsList));
    ATH_CHECK (m_muonHandle.initialize(m_systematicsList));
    ATH_CHECK (m_hhmlMuonHandle.initialize(m_systematicsList));
    ATH_CHECK (m_eventHandle.initialize(m_systematicsList));

    for(const auto& trig : m_eleTrigSF){
      m_eleTriggerSF.emplace
      (trig, CP::SysReadDecorHandle<float>("el_trigEffSF_"+trig+"_%SYS%", this));
      ATH_CHECK (m_eleTriggerSF.at(trig).initialize(m_systematicsList, m_electronHandle));
    }

    for(const auto& trig : m_muonTrigSF){
      m_muonTriggerSF.emplace
      (trig, CP::SysReadDecorHandle<float>("muon_trigEffSF_"+trig+"_%SYS%", this));
      ATH_CHECK (m_muonTriggerSF.at(trig).initialize(m_systematicsList, m_muonHandle));
    }

    for(const auto& trig : m_tauTrigSF){
      m_tauTriggerSF.emplace
      (trig, CP::SysReadDecorHandle<float>("tau_trigEffSF_"+trig+"_%SYS%", this));
      ATH_CHECK (m_tauTriggerSF.at(trig).initialize(m_systematicsList, m_tauHandle));
    }

    ATH_CHECK(m_year.initialize(m_systematicsList, m_eventHandle));

    for(const auto& [runBool, name] : m_runBooleans) {
      SG::ReadDecorHandleKey<xAOD::EventInfo> deco;
      deco = "EventInfo." + name;
      m_runBooleans_key.emplace(runBool, deco);
      ATH_CHECK(m_runBooleans_key.at(runBool).initialize());
    }

    m_boolnames.emplace(MULTILEPTON::pass_trigger_LTT, "pass_trigger_LTT");

    for (auto& [key, value] : m_boolnames) {
      CP::SysReadDecorHandle<char> whandle{value+"_%SYS%", this};
      m_categoryBranches.emplace(key, whandle);
      ATH_CHECK(m_categoryBranches.at(key).initialize(m_systematicsList, m_eventHandle));
    };

    ATH_CHECK(m_eventTriggerSF.initialize(m_systematicsList, m_eventHandle));
    ATH_CHECK(m_triggerSF_ele.initialize(m_systematicsList, m_eventHandle));
    ATH_CHECK(m_triggerSF_mu.initialize(m_systematicsList, m_eventHandle));
    ATH_CHECK(m_triggerSF_tau0.initialize(m_systematicsList, m_eventHandle));
    ATH_CHECK(m_triggerSF_tau1.initialize(m_systematicsList, m_eventHandle));

    // Intialise syst list (must come after all syst-aware inputs and outputs)
    ATH_CHECK (m_systematicsList.initialize());
    return StatusCode::SUCCESS;
  }

  StatusCode TriggerSFAlg::execute(const EventContext& ctx)
  {

    runBoolReadDecoMap runBoolDecos;
    for (const auto& [runBool, key] : m_runBooleans_key){
      runBoolDecos.emplace(runBool, key);
    }

    // Loop over all systs
    for (const auto& sys : m_systematicsList.systematicsVector())
    {

      // Retrieve inputs
      const xAOD::EventInfo *event = nullptr;
      ANA_CHECK (m_eventHandle.retrieve (event, sys, ctx));

      const xAOD::JetContainer *jets = nullptr;
      ANA_CHECK (m_jetHandle.retrieve (jets, sys, ctx));

      const xAOD::MuonContainer *muons = nullptr;
      ANA_CHECK (m_hhmlMuonHandle.retrieve (muons, sys, ctx));

      const xAOD::ElectronContainer *electrons = nullptr;
      ANA_CHECK (m_hhmlEleHandle.retrieve (electrons, sys, ctx));

      const xAOD::TauJetContainer *taus = nullptr;
      ANA_CHECK (m_hhmlTauHandle.retrieve (taus, sys, ctx));

      m_eventTriggerSF.set(*event, -999., sys);
      m_triggerSF_ele.set(*event, -999., sys);
      m_triggerSF_mu.set(*event, -999., sys);
      m_triggerSF_tau0.set(*event, -999., sys);
      m_triggerSF_tau1.set(*event, -999., sys);

      // hhmlAnalysis* containers are already selected and pT-sorted
      const xAOD::Electron* ele0 = electrons->empty() ? nullptr : electrons->at(0);
      const xAOD::Muon*     mu0  = muons->empty()     ? nullptr : muons->at(0);
      const xAOD::TauJet*   tau0 = taus->size() > 0   ? taus->at(0) : nullptr;
      const xAOD::TauJet*   tau1 = taus->size() > 1   ? taus->at(1) : nullptr;

      computeTriggerSF(event, sys, runBoolDecos, ele0, mu0, tau0, tau1);

    }

    return StatusCode::SUCCESS;
  }

  void TriggerSFAlg::computeTriggerSF
  (const xAOD::EventInfo *event, const CP::SystematicSet& sys,
   const runBoolReadDecoMap& runBoolDecos,
   const xAOD::Electron* ele0, const xAOD::Muon* mu0,
   const xAOD::TauJet* tau0, const xAOD::TauJet* tau1){
    float triggerSF = 1.;
    float sf_ele = 1., sf_mu = 1., sf_tau0 = 1., sf_tau1 = 1.;
    int year = m_year.get(*event, sys);

    // Multiply in one leg SF; skip (SF=1) if the leg or the object is missing,
    // e.g. legs without an SF in trigger.yaml (Run 3 e+tau electron leg)
    auto applyLegSF = [&](const auto& sfMap, const std::string& leg, const auto* obj, float& legSF) {
      if (leg.empty() || !obj) return;
      auto it = sfMap.find(leg);
      if (it == sfMap.end()) {
        ATH_MSG_DEBUG("No trigger SF decoration for leg " << leg << ", using SF=1");
        return;
      }
      legSF = it->second.get(*obj, sys);
      ATH_MSG_DEBUG("Trigger SF leg " << leg << " = " << legSF);
      triggerSF *= legSF;
    };

    if(m_categoryBranches.at(MULTILEPTON::pass_trigger_SLT).get(*event, sys)){
      if(ele0){
        std::string single_ele_SF_path;
        getSingleEleTriggers(year, event, runBoolDecos, single_ele_SF_path);
        applyLegSF(m_eleTriggerSF, single_ele_SF_path, ele0, sf_ele);
      }
      else if(mu0){
        std::string single_mu_SF_path;
        getSingleMuTriggers(year, event, runBoolDecos, single_mu_SF_path);
        applyLegSF(m_muonTriggerSF, single_mu_SF_path, mu0, sf_mu);
      }
    }

    else if(m_categoryBranches.at(MULTILEPTON::pass_trigger_LTT).get(*event, sys)){
      if(ele0){
        std::pair<std::string, std::string> ele_tau_SF_path;
        getEleTauTriggers(year, event, runBoolDecos, ele_tau_SF_path);
        applyLegSF(m_eleTriggerSF, ele_tau_SF_path.first, ele0, sf_ele);
        applyLegSF(m_tauTriggerSF, ele_tau_SF_path.second, tau0, sf_tau0);
      }
      else if(mu0){
        std::pair<std::string, std::string> mu_tau_SF_path;
        getMuTauTriggers(year, event, runBoolDecos, mu_tau_SF_path);
        applyLegSF(m_muonTriggerSF, mu_tau_SF_path.first, mu0, sf_mu);
        applyLegSF(m_tauTriggerSF, mu_tau_SF_path.second, tau0, sf_tau0);
      }
    }

    else if(m_categoryBranches.at(MULTILEPTON::pass_trigger_STT).get(*event, sys)){
      std::string single_tau_SF_path;
      getSingleTauTriggers(year, event, runBoolDecos, single_tau_SF_path);
      applyLegSF(m_tauTriggerSF, single_tau_SF_path, tau0, sf_tau0);
    }

    else if(m_categoryBranches.at(MULTILEPTON::pass_trigger_DTT).get(*event, sys)){
      std::pair<std::string, std::string> di_tau_SF_path;
      getDiTauTriggers(year, event, runBoolDecos, di_tau_SF_path);
      applyLegSF(m_tauTriggerSF, di_tau_SF_path.first, tau0, sf_tau0);
      applyLegSF(m_tauTriggerSF, di_tau_SF_path.second, tau1, sf_tau1);
    }

    m_eventTriggerSF.set(*event, triggerSF, sys);
    m_triggerSF_ele.set(*event, sf_ele, sys);
    m_triggerSF_mu.set(*event, sf_mu, sys);
    m_triggerSF_tau0.set(*event, sf_tau0, sys);
    m_triggerSF_tau1.set(*event, sf_tau1, sys);
  }
}
