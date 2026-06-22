/*
  Copyright (C) 2002-2023 CERN for the benefit of the ATLAS collaboration
*/

#include "ZbbjCalibSelectorAlg.h"
#include <SystematicsHandles/SysFilterReporter.h>
#include <SystematicsHandles/SysFilterReporterCombiner.h>
#include <AthenaKernel/Units.h>
#include <cmath>
#include <algorithm>

namespace XBBCALIB
{

  ZbbjCalibSelectorAlg::ZbbjCalibSelectorAlg(const std::string &name,
                                ISvcLocator *pSvcLocator)
      : AthHistogramAlgorithm(name, pSvcLocator)
  {

  }

  StatusCode ZbbjCalibSelectorAlg::initialize()
  {
    ATH_CHECK (m_filterParams.initialize(m_systematicsList));
    ATH_CHECK (m_eventHandle.initialize(m_systematicsList));
    ATH_CHECK (m_lrjetHandle.initialize(m_systematicsList));
    ATH_CHECK (m_outHandle.initialize(m_systematicsList));

    // Initialise BJR accessors from property strings
    m_accBjrPt = std::make_unique<SG::AuxElement::ConstAccessor<float>>(m_bjrPtName);
    m_accBjrMass = std::make_unique<SG::AuxElement::ConstAccessor<float>>(m_bjrMassName);

    if (m_remove_zcc_overlap) m_isCC_acc.emplace("z_is_cc");

    // Intialise syst list (must come after all syst-aware inputs and outputs)
    ATH_CHECK (m_systematicsList.initialize());

    return StatusCode::SUCCESS;
  }


  StatusCode ZbbjCalibSelectorAlg::execute(const EventContext& /*ctx*/)
  {

    // Global filter originally false
    CP::SysFilterReporterCombiner filterCombiner (m_filterParams, false);

    // Loop over all systs
    for (const auto& sys : m_systematicsList.systematicsVector())
    {
      CP::SysFilterReporter filter (filterCombiner, sys);

      // Retrieve inputs
      const xAOD::EventInfo *event = nullptr;
      ANA_CHECK (m_eventHandle.retrieve (event, sys));

      const xAOD::JetContainer *lrjets = nullptr;
      ANA_CHECK (m_lrjetHandle.retrieve (lrjets, sys));

      // Create output view container (always written, even if event fails)
      auto ordered = std::make_unique<ConstDataVector<xAOD::JetContainer>>(SG::VIEW_ELEMENTS);

      // Veto Z->cc events from Zqq samples (Zcc/Zqq overlap removal)
      if (m_isCC_acc && (*m_isCC_acc)(*event) == 1) {
        ANA_CHECK (m_outHandle.record (std::move(ordered), sys));
        continue;
      }

      // Jets arrive sorted by calorimeter pT from JetSelectorAlg

      // 1. Require at least 2 jets (always checked — can't access jets that don't exist)
      if (lrjets->size() < m_minJets) {
        ANA_CHECK (m_outHandle.record (std::move(ordered), sys));
        if (m_bypass) filter.setPassed(true);
        continue;
      }

      // 2. Leading jet UFO/calo pT cut (trigger plateau, before BJR reordering)
      //    lrjets->at(0) is the leading calo-pT jet at this point
      if (lrjets->at(0)->pt() < m_leadJetUfoPtCut) {
        ANA_CHECK (m_outHandle.record (std::move(ordered), sys));
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
      const xAOD::Jet* jet1 = jets[1];

      // Get BJR kinematics (now in BJR pT order)
      float bjrPt0 = accPt(*jet0);
      float bjrPt1 = accPt(*jet1);
      float bjrMass0 = (*m_accBjrMass)(*jet0);
      float bjrMass1 = (*m_accBjrMass)(*jet1);

      // 3. BJR mass cut on both jets
      bool pass_mass = (bjrMass0 > m_bjrMassCut) && (bjrMass1 > m_bjrMassCut);

      // 4. Minimum BJR pT cut on both jets
      bool pass_minPt = (bjrPt0 > m_minJetBjrPtCut) &&
                         (bjrPt1 > m_minJetBjrPtCut);

      // 5. Optional leading jet BJR pT cut
      bool pass_leadPt = (m_leadJetPtCut < 0) || (bjrPt0 > m_leadJetPtCut);

      // 6. pT asymmetry using BJR pT: (pt0 - pt1) / (pt0 + pt1) < cut
      float ptSum = bjrPt0 + bjrPt1;
      bool pass_asymmetry = (ptSum > 0) &&
        ((bjrPt0 - bjrPt1) / ptSum < m_asymmetryCut);

      // 7. Rapidity difference: |y0 - y1| < cut
      bool pass_rapidity =
        std::abs(jet0->rapidity() - jet1->rapidity()) < m_rapidityCut;

      bool pass_all = pass_mass && pass_minPt && pass_leadPt &&
                       pass_asymmetry && pass_rapidity;

      if (!m_bypass && !pass_all) {
        ANA_CHECK (m_outHandle.record (std::move(ordered), sys));
        continue;
      }

      // Fill output container in BJR pT order
      for (const xAOD::Jet* jet : jets) {
        ordered->push_back(jet);
      }

      ANA_CHECK (m_outHandle.record (std::move(ordered), sys));
      filter.setPassed(true);
    }

    return StatusCode::SUCCESS;
  }

  StatusCode ZbbjCalibSelectorAlg::finalize()
  {
    ANA_CHECK (m_filterParams.finalize());
    return StatusCode::SUCCESS;
  }

}
