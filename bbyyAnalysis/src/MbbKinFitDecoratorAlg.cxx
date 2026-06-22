/*
  Copyright (C) 2002-2024 CERN for the benefit of the ATLAS collaboration
*/

#include "MbbKinFitDecoratorAlg.h"

#include <xAODJet/JetAuxContainer.h>
#include "TLorentzVector.h"

namespace HHBBYY{
      MbbKinFitDecoratorAlg ::MbbKinFitDecoratorAlg(const std::string &name,
                                  ISvcLocator *pSvcLocator)
    : EL::AnaAlgorithm(name, pSvcLocator)
  {
    
  }
    StatusCode MbbKinFitDecoratorAlg ::initialize()
  {
    // Read syst-aware input handles
    ATH_CHECK (m_jetHandle.initialize(m_systematicsList));
    ATH_CHECK (m_photonHandle.initialize(m_systematicsList));
    ATH_CHECK (m_eventHandle.initialize(m_systematicsList));
    ATH_CHECK (m_metHandle.initialize(m_systematicsList));
    // Intialise syst-aware output decorators
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

    // Intialise syst list (must come after all syst-aware inputs and outputs)
    ANA_CHECK (m_systematicsList.initialize());    

    ATH_CHECK (m_KFTool.retrieve());

    return StatusCode::SUCCESS;
  }

  StatusCode MbbKinFitDecoratorAlg::execute(const EventContext& /*ctx*/) {
    
    // Loop over all systs
    for (const auto& sys : m_systematicsList.systematicsVector())
    {
      // In case of special Higgs sample, run only on NOSYS
      if (!m_doSystematics && sys.name()!="") continue;

      // Retrieve inputs
      const xAOD::EventInfo *event = nullptr;
      ANA_CHECK (m_eventHandle.retrieve (event, sys));
        
      const xAOD::PhotonContainer *photons = nullptr;
      ANA_CHECK (m_photonHandle.retrieve (photons, sys));

      const xAOD::JetContainer *jets = nullptr;
      ANA_CHECK (m_jetHandle.retrieve (jets, sys));

      const xAOD::MissingETContainer *MET = nullptr;
      ANA_CHECK (m_metHandle.retrieve (MET, sys));

      //setup KF tool inputs
      double KF1_Mbb = -99.;
      if (jets->size()>=2) {
        TLorentzVector j1 = jets->at(0)->p4();
        TLorentzVector j2 = jets->at(1)->p4();
        TLorentzVector jj = j1 + j2;
        KF1_Mbb = jj.M();
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

      for (const auto jet : *jets) {
        xAOD::Jet* thisJet = new xAOD::Jet();
        workJetCont_Iter1->push_back(thisJet);
        *thisJet = *jet; 

        // Ensuring the correct handling of systematics of n_muons decoration
        static const SG::AuxElement::Accessor<int> acc_muons_nosys("n_muons_NOSYS");
        SG::AuxElement::Accessor<int> acc_muons_sys("n_muons_" + sys.name());

        if (!acc_muons_nosys.isAvailable(*thisJet)) {
          if (sys.name() != "" && acc_muons_sys.isAvailable(*thisJet)) {
            // Copy the systematic muon count into the nominal decoration
            acc_muons_nosys(*thisJet) = acc_muons_sys(*thisJet);
          } else {
            // Failsafe to prevent framework crashes if upstream drops the variable entirely
            acc_muons_nosys(*thisJet) = 0; 
          }
        }
      }

      // Run KF tool for Iteration 1
      ATH_CHECK(m_KFTool->applyKF(*photons, *workJetCont_Iter1, MET, LH_KF1, Conv_KF1, 1));

      auto constDataWorkJetCont_Iter1 = std::make_unique<ConstDataVector<xAOD::JetContainer>>();
      constDataWorkJetCont_Iter1->reserve(workJetCont_Iter1->size());
      for (xAOD::Jet* jet : *workJetCont_Iter1) {
        constDataWorkJetCont_Iter1->push_back(std::make_unique<xAOD::Jet>(*jet).release());
      }

      //CALCULATE KF1 Mbb
      xAOD::Jet* bjet1 = nullptr;
      xAOD::Jet* bjet2 = nullptr;

      for(xAOD::Jet* jet : *workJetCont_Iter1){
        if(jet->pt() < m_Jet_Min_Pt) continue;
        if(m_isBtag.get(*jet, sys) && std::abs(jet->eta())<2.5){
          if(!bjet1) bjet1 = jet;
          else if(!bjet2) bjet2 = jet;
        }
      }
      if(bjet1 && bjet2){
        KF1_Mbb=(bjet1->p4()+bjet2->p4()).M();
      }

      // Set the Mbb decorator
      m_KF_MBB.set(*event, KF1_Mbb, sys);
      m_LH_KF1.set(*event, LH_KF1, sys);
      m_Conv_KF1.set(*event, Conv_KF1, sys);

      // Write Iteration 1 jets to event store
      ATH_CHECK(m_jetOutHandle_KF1.record(std::move(constDataWorkJetCont_Iter1), sys));

      // ========================================================
      // ITERATION 2: Mass-Constrained Fit
      // ========================================================
      auto workJetCont_Iter2 = std::make_unique<xAOD::JetContainer>();
      auto workJetAuxCont_Iter2 = std::make_unique<xAOD::JetAuxContainer>();
      workJetCont_Iter2->setStore(workJetAuxCont_Iter2.get());

      // We must copy from the ORIGINAL *jets again to get a fresh start!
      for (const auto jet : *jets) {
        xAOD::Jet* thisJet = new xAOD::Jet();
        workJetCont_Iter2->push_back(thisJet);
        *thisJet = *jet; 

        // Ensuring the correct handling of systematics of n_muons decoration
        static const SG::AuxElement::Accessor<int> acc_muons_nosys("n_muons_NOSYS");
        SG::AuxElement::Accessor<int> acc_muons_sys("n_muons_" + sys.name());

        if (!acc_muons_nosys.isAvailable(*thisJet)) {
          if (sys.name() != "" && acc_muons_sys.isAvailable(*thisJet)) {
            // Copy the systematic muon count into the nominal decoration
            acc_muons_nosys(*thisJet) = acc_muons_sys(*thisJet);
          } else {
            // Failsafe to prevent framework crashes if upstream drops the variable entirely
            acc_muons_nosys(*thisJet) = 0; 
          }
        }
      }

      ATH_CHECK(m_KFTool->applyKF(*photons, *workJetCont_Iter2, MET, LH_KF2, Conv_KF2, 2));

      auto constDataWorkJetCont_Iter2 = std::make_unique<ConstDataVector<xAOD::JetContainer>>();
      constDataWorkJetCont_Iter2->reserve(workJetCont_Iter2->size());
      for (xAOD::Jet* jet : *workJetCont_Iter2) {
        constDataWorkJetCont_Iter2->push_back(std::make_unique<xAOD::Jet>(*jet).release());
      }

      m_LH_KF2.set(*event, LH_KF2, sys);
      m_Conv_KF2.set(*event, Conv_KF2, sys);

      // Write Iteration 2 jets to event store
      ATH_CHECK(m_jetOutHandle_KF2.record(std::move(constDataWorkJetCont_Iter2), sys));
    }
    return StatusCode::SUCCESS;
  }
}
