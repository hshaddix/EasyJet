/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "MbbKinFitDecoratorAlg.h"

#include <xAODJet/JetAuxContainer.h>
#include "TLorentzVector.h"

namespace MULTILEPTON
{
  MbbKinFitDecoratorAlg ::MbbKinFitDecoratorAlg(const std::string &name,
      ISvcLocator *pSvcLocator)
    : EL::AnaAlgorithm(name, pSvcLocator)
  {
  }
  StatusCode MbbKinFitDecoratorAlg ::initialize()
  {

    ATH_CHECK(m_eventHandle.initialize(m_systematicsList));
    ATH_CHECK(m_electronHandle.initialize(m_systematicsList));
    ATH_CHECK(m_muonHandle.initialize(m_systematicsList));

    ATH_CHECK(m_jetHandle.initialize(m_systematicsList));

    ATH_CHECK (m_jetOutHandle_KF1.initialize(m_systematicsList));
    ATH_CHECK (m_jetOutHandle_KF2.initialize(m_systematicsList));
    ATH_CHECK (m_KF_MBB.initialize(m_systematicsList, m_eventHandle));
    ATH_CHECK (m_LH_KF1.initialize(m_systematicsList, m_eventHandle));
    ATH_CHECK (m_LH_KF2.initialize(m_systematicsList, m_eventHandle));
    ATH_CHECK (m_Conv_KF1.initialize(m_systematicsList, m_eventHandle));
    ATH_CHECK (m_Conv_KF2.initialize(m_systematicsList, m_eventHandle));

    if (!m_isBtag.empty()) {
      ATH_CHECK (m_isBtag.initialize(m_systematicsList, m_eventHandle));
    }

    ANA_CHECK(m_systematicsList.initialize());

    ATH_CHECK(m_KFTool.retrieve());

    return StatusCode::SUCCESS;
  }

  StatusCode MbbKinFitDecoratorAlg::execute(const EventContext& ctx)
  {

    for (const auto &sys : m_systematicsList.systematicsVector())
    {
      if (!m_doSystematics && sys.name() != "")
        continue;

      const xAOD::EventInfo *event = nullptr;
      ANA_CHECK(m_eventHandle.retrieve(event, sys, ctx));
      const xAOD::ElectronContainer *electrons = nullptr;
      ANA_CHECK(m_electronHandle.retrieve(electrons, sys, ctx));
      const xAOD::MuonContainer *muons = nullptr;
      ANA_CHECK(m_muonHandle.retrieve(muons, sys, ctx));
      const xAOD::JetContainer *jets = nullptr;
      ANA_CHECK(m_jetHandle.retrieve(jets, sys, ctx));

      // setup KF tool inputs
      const size_t nJets = jets->size();
      double KF1_Mbb = -99.;
      if (nJets >= 2)
      {
        KF1_Mbb = (jets->at(0)->p4() + jets->at(1)->p4()).M();
      }

      double LH_KF1=999;
      double LH_KF2=999;

      int Conv_KF1=-1;
      int Conv_KF2=-1;

      // ========================================================
      // ITERATION 1: Unconstrained Fit
      // ========================================================
      auto workJetCont_Iter1 = std::make_unique<xAOD::JetContainer>();
      auto workJetAuxCont_Iter1 = std::make_unique<xAOD::JetAuxContainer>();
      workJetCont_Iter1->setStore(workJetAuxCont_Iter1.get());
      workJetCont_Iter1->reserve(nJets);

      for (const auto* jet : *jets) {
        xAOD::Jet* thisJet = new xAOD::Jet();
        workJetCont_Iter1->push_back(thisJet);
        *thisJet = *jet;
      }

      // Run KF tool for Iteration 1
      ATH_CHECK(m_KFTool->applyKF_bb4l(*electrons, *muons, *workJetCont_Iter1, LH_KF1, Conv_KF1, 1));

      //CALCULATE KF1 Mbb (before copying to CDV)
      xAOD::Jet* bjet1 = nullptr;
      xAOD::Jet* bjet2 = nullptr;

      for (xAOD::Jet* jet : *workJetCont_Iter1) {
        if (jet->pt() < m_Jet_Min_Pt) continue;
        if (m_isBtag.get(*jet, sys) && std::abs(jet->eta()) < 2.5) {
          if (!bjet1) bjet1 = jet;
          else if (!bjet2) { bjet2 = jet; break; }
        }
      }
      if (bjet1 && bjet2) {
        KF1_Mbb = (bjet1->p4() + bjet2->p4()).M();
      }

      // Record working containers to event store so jets survive the scope
      const xAOD::JetContainer* rawWorkJets_Iter1 = workJetCont_Iter1.get();
      std::string workKey_Iter1 = "MbbKFWorkJets_Iter1_" + sys.name();
      ATH_CHECK(evtStore()->record(std::move(workJetCont_Iter1), workKey_Iter1));
      ATH_CHECK(evtStore()->record(std::move(workJetAuxCont_Iter1), workKey_Iter1 + "Aux."));

      // Create VIEW_ELEMENTS CDV (avoids deep-copying jets a second time)
      auto constDataWorkJetCont_Iter1 = std::make_unique<ConstDataVector<xAOD::JetContainer>>(SG::VIEW_ELEMENTS);
      constDataWorkJetCont_Iter1->reserve(rawWorkJets_Iter1->size());
      for (const xAOD::Jet* jet : *rawWorkJets_Iter1) {
        constDataWorkJetCont_Iter1->push_back(jet);
      }

      // Set the Mbb decorator
      m_KF_MBB.set(*event, KF1_Mbb, sys);
      m_LH_KF1.set(*event, LH_KF1, sys);
      m_Conv_KF1.set(*event, Conv_KF1, sys);

      // Write Iteration 1 jets to event store
      ATH_CHECK(m_jetOutHandle_KF1.record(std::move(constDataWorkJetCont_Iter1), sys, ctx));

      // ========================================================
      // ITERATION 2: Mass-Constrained Fit
      // ========================================================
      auto workJetCont_Iter2 = std::make_unique<xAOD::JetContainer>();
      auto workJetAuxCont_Iter2 = std::make_unique<xAOD::JetAuxContainer>();
      workJetCont_Iter2->setStore(workJetAuxCont_Iter2.get());
      workJetCont_Iter2->reserve(nJets);

      // We must copy from the ORIGINAL *jets again to get a fresh start!
      for (const auto* jet : *jets) {
        xAOD::Jet* thisJet = new xAOD::Jet();
        workJetCont_Iter2->push_back(thisJet);
        *thisJet = *jet;
      }

      ATH_CHECK(m_KFTool->applyKF_bb4l(*electrons, *muons, *workJetCont_Iter2, LH_KF2, Conv_KF2, 2));

      // Record working containers to event store so jets survive the scope
      const xAOD::JetContainer* rawWorkJets_Iter2 = workJetCont_Iter2.get();
      std::string workKey_Iter2 = "MbbKFWorkJets_Iter2_" + sys.name();
      ATH_CHECK(evtStore()->record(std::move(workJetCont_Iter2), workKey_Iter2));
      ATH_CHECK(evtStore()->record(std::move(workJetAuxCont_Iter2), workKey_Iter2 + "Aux."));

      // Create VIEW_ELEMENTS CDV (avoids deep-copying jets a second time)
      auto constDataWorkJetCont_Iter2 = std::make_unique<ConstDataVector<xAOD::JetContainer>>(SG::VIEW_ELEMENTS);
      constDataWorkJetCont_Iter2->reserve(rawWorkJets_Iter2->size());
      for (const xAOD::Jet* jet : *rawWorkJets_Iter2) {
        constDataWorkJetCont_Iter2->push_back(jet);
      }

      m_LH_KF2.set(*event, LH_KF2, sys);
      m_Conv_KF2.set(*event, Conv_KF2, sys);

      // Write Iteration 2 jets to event store
      ATH_CHECK(m_jetOutHandle_KF2.record(std::move(constDataWorkJetCont_Iter2), sys, ctx));
    }
    return StatusCode::SUCCESS;
  }
}
