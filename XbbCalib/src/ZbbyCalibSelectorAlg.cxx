/*
  Copyright (C) 2002-2023 CERN for the benefit of the ATLAS collaboration
*/

#include "ZbbyCalibSelectorAlg.h"
#include <SystematicsHandles/SysFilterReporter.h>
#include <SystematicsHandles/SysFilterReporterCombiner.h>
#include <cmath>
#include <algorithm>
#include <iostream>

namespace XBBCALIB
{

  ZbbyCalibSelectorAlg::ZbbyCalibSelectorAlg(const std::string &name,
                                ISvcLocator *pSvcLocator)
      : AthHistogramAlgorithm(name, pSvcLocator)
  {

  }

  StatusCode ZbbyCalibSelectorAlg::initialize()
  {
    // Initialise global event filter
    ATH_CHECK (m_filterParams.initialize(m_systematicsList));
    ATH_CHECK (m_eventHandle.initialize(m_systematicsList));
    ATH_CHECK (m_lrjetHandle.initialize(m_systematicsList));
    ATH_CHECK (m_photonHandle.initialize(m_systematicsList));
    ATH_CHECK (m_outHandle.initialize(m_systematicsList));

    // Initialise BJR accessors from property strings
    m_accBjrPt = std::make_unique<SG::AuxElement::ConstAccessor<float>>(m_bjrPtName);
    m_accBjrMass = std::make_unique<SG::AuxElement::ConstAccessor<float>>(m_bjrMassName);

    if (m_remove_zcc_overlap) m_isCC_acc.emplace("z_is_cc");

    // Intialise syst list (must come after all syst-aware inputs and outputs)
    ATH_CHECK (m_systematicsList.initialize());

    return StatusCode::SUCCESS;
  }


  StatusCode ZbbyCalibSelectorAlg::execute(const EventContext& ctx)
  {

    // Global filter originally false
    CP::SysFilterReporterCombiner filterCombiner (m_filterParams, false);

    // Loop over all systs
    for (const auto& sys : m_systematicsList.systematicsVector())
    {
      CP::SysFilterReporter filter (filterCombiner, sys);
   
        
      // Retrive inputs
      const xAOD::EventInfo *event = nullptr;
      ANA_CHECK (m_eventHandle.retrieve (event, sys, ctx));

      const xAOD::JetContainer *lrjets = nullptr;
      ANA_CHECK (m_lrjetHandle.retrieve (lrjets, sys, ctx));    
        
      const xAOD::PhotonContainer *photons = nullptr;
      ANA_CHECK (m_photonHandle.retrieve(photons, sys, ctx));    
        
      // Create output view container (always written, even if event fails)
      auto ordered = std::make_unique<ConstDataVector<xAOD::JetContainer>>(SG::VIEW_ELEMENTS);
     
      // Veto Z->cc events from Zqq samples (Zcc/Zqq overlap removal)
      if (m_isCC_acc && (*m_isCC_acc)(*event) == 1) {
        ANA_CHECK (m_outHandle.record (std::move(ordered), sys, ctx));
        continue;
      }
        
      // Jets arrive sorted by calorimeter pT from JetSelectorAlg
      // 1. Require the configured minimum number of jets
      if (lrjets->size() < m_minJets) {
        ANA_CHECK (m_outHandle.record (std::move(ordered), sys, ctx));
        if (m_bypass) filter.setPassed(true);
        continue;
      }
        
      // Copy jet pointers and sort by BJR pT (descending)
      std::vector<const xAOD::Jet*> jets(lrjets->begin(), lrjets->end());
      const auto& accPt = *m_accBjrPt;
      std::sort(jets.begin(), jets.end(),
        [&accPt](const xAOD::Jet* a, const xAOD::Jet* b) {
          return accPt(*a) > accPt(*b);
        });

      const xAOD::Jet* jet0 = jets[0];

      // Get BJR kinematics (now in BJR pT order)
      float bjrPt0 = accPt(*jet0);
      float bjrMass0 = (*m_accBjrMass)(*jet0);

      //std::cout<<"BJR Pt0 " << bjrPt0 << " Mass " << bjrMass0 << std::endl;
	
      // --- multiplicity cuts ---
      if (photons->size() != 1) {
          ANA_CHECK(m_outHandle.record(std::move(ordered), sys, ctx));
          if (m_bypass) filter.setPassed(true);
          continue;
      }

      // --- leading objects ---
      const xAOD::Jet* ljet = jet0;
      const xAOD::Photon* photon = photons->at(0);
        
      // --- hasPhoton equivalent ---
      bool hasPhoton = (photon->pt() > m_minPhotonPtCut);    
        
      
      // --- hasLRjet equivalent ---
      bool hasLRjet = (bjrPt0 > m_minJetBjrPtCut) && (bjrMass0 > m_bjrMassCut) && (bjrMass0 < m_bjrMassUpperCut);
        
      // --- delta phi cut ---
      bool passDPhi = std::abs(ljet->p4().DeltaPhi(photon->p4())) > 1.5707963;


      const bool pass_all = hasPhoton && hasLRjet && passDPhi;
        
      if (!m_bypass && !pass_all) { 
          ANA_CHECK(m_outHandle.record(std::move(ordered), sys, ctx));
          continue;
      }
        
      for (const xAOD::Jet* jet : jets) {
          ordered->push_back(jet);
      }
        
      ANA_CHECK(m_outHandle.record(std::move(ordered), sys, ctx));
        
      // Global event filter true if any syst passes and controls
      // if event is passed to output writing or not
      filter.setPassed(true);
    }

    return StatusCode::SUCCESS;
  }

  StatusCode ZbbyCalibSelectorAlg::finalize()
  {
    ANA_CHECK (m_filterParams.finalize());
    return StatusCode::SUCCESS;
  }

}
