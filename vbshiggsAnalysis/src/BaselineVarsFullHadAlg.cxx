/*
  Copyright (C) 2002-2024 CERN for the benefit of the ATLAS collaboration
*/

#include "BaselineVarsFullHadAlg.h"
#include "AthContainers/AuxElement.h"
#include "TLorentzVector.h"
#include <cmath>

namespace VBSHIGGS {

BaselineVarsFullHadAlg::BaselineVarsFullHadAlg(
  const std::string& name, ISvcLocator* pSvcLocator)
  : AthHistogramAlgorithm(name, pSvcLocator) {}

StatusCode BaselineVarsFullHadAlg::initialize() {

  ATH_MSG_INFO("*********************************");
  ATH_MSG_INFO("       FullHadBaselineVarsAlg    ");
  ATH_MSG_INFO("*********************************");

  ATH_CHECK(m_eventHandle.initialize(m_systematicsList));
  ATH_CHECK(m_metHandle.initialize(m_systematicsList));
  ATH_CHECK(m_vbsLRJetHandle.initialize(m_systematicsList));

  if (m_doResolved) {
    ATH_CHECK(m_signaljetHandle.initialize(m_systematicsList));
    ATH_CHECK(m_HCandHandle.initialize(m_systematicsList));
    if (!m_isBtag.empty()) {
      ATH_CHECK(m_isBtag.initialize(m_systematicsList, m_signaljetHandle));
    }
  }

  for (const auto& var : m_floatVariables) {
    CP::SysWriteDecorHandle<float> h(var + "_%SYS%", this);
    m_Fbranches.emplace(var, h);
    ATH_CHECK(m_Fbranches.at(var).initialize(m_systematicsList, m_eventHandle));
  }

  for (const auto& var : m_intVariables) {
    CP::SysWriteDecorHandle<int> h(var + "_%SYS%", this);
    m_Ibranches.emplace(var, h);
    ATH_CHECK(m_Ibranches.at(var).initialize(m_systematicsList, m_eventHandle));
  }

  ATH_CHECK(m_systematicsList.initialize());
  return StatusCode::SUCCESS;
}

StatusCode BaselineVarsFullHadAlg::execute() {

  for (const auto& sys : m_systematicsList.systematicsVector()) {

    const xAOD::EventInfo* event = nullptr;
    ANA_CHECK(m_eventHandle.retrieve(event, sys));

    const xAOD::JetContainer* largeJets = nullptr;
    ANA_CHECK(m_vbsLRJetHandle.retrieve(largeJets, sys));

    const xAOD::MissingETContainer* metCont = nullptr;
    ANA_CHECK(m_metHandle.retrieve(metCont, sys));
    const xAOD::MissingET* met = (*metCont)["Final"];
    if (!met) return StatusCode::FAILURE;

    for (const auto& v : m_floatVariables)
      m_Fbranches.at(v).set(*event, -99.f, sys);
    for (const auto& v : m_intVariables)
      m_Ibranches.at(v).set(*event, -99, sys);

    int nCentralJets = 0;
    int nForwardJets = 0;
    int nLargeJets   = largeJets->size();

    if (m_doResolved) {
      const xAOD::JetContainer* signalJets = nullptr;
      const xAOD::JetContainer* HJets = nullptr;
      ANA_CHECK(m_signaljetHandle.retrieve(signalJets, sys));
      ANA_CHECK(m_HCandHandle.retrieve(HJets, sys));

      int nJets = signalJets->size();
      int nBJets = 0;

      for (const xAOD::Jet* jet : *signalJets) {
        if (std::abs(jet->eta()) < 2.5) nCentralJets++;
        else nForwardJets++;

        if (!m_isBtag.empty() &&
            std::abs(jet->eta()) < 2.5 &&
            m_isBtag.get(*jet, sys)) {
          nBJets++;
        }
      }

      m_Ibranches.at("nJets").set(*event, nJets, sys);
      m_Ibranches.at("nBJets").set(*event, nBJets, sys);

      if (HJets->size() >= 2) {
        TLorentzVector h1 = HJets->at(0)->p4();
        TLorentzVector h2 = HJets->at(1)->p4();
        TLorentzVector hh = h1 + h2;

        m_Fbranches.at("Hdijet_m").set(*event, hh.M(), sys);
        m_Fbranches.at("Hdijet_pt").set(*event, hh.Pt(), sys);
        m_Fbranches.at("dRHjj").set(*event, h1.DeltaR(h2), sys);
      }
    }

    m_Ibranches.at("nCentralJets").set(*event, nCentralJets, sys);
    m_Ibranches.at("nForwardJets").set(*event, nForwardJets, sys);
    m_Ibranches.at("nLargeJets").set(*event, nLargeJets, sys);
  }

  return StatusCode::SUCCESS;
}

}
