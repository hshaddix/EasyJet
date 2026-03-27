/*
  Copyright (C) 2002-2024 CERN for the benefit of the ATLAS collaboration
*/

/// @author 

#include "BaselineVarsZbbjCalibAlg.h"

namespace XBBCALIB
{
  BaselineVarsZbbjCalibAlg::BaselineVarsZbbjCalibAlg(const std::string &name,
                                           ISvcLocator *pSvcLocator)
      : AthHistogramAlgorithm(name, pSvcLocator)
  { }

  //BaselineVarsZbbjCalibAlg::~BaselineVarsZbbjCalibAlg() = default;
  BaselineVarsZbbjCalibAlg::~BaselineVarsZbbjCalibAlg()
{
    
}

  StatusCode BaselineVarsZbbjCalibAlg::initialize()
  {
    ATH_CHECK (m_lrjetHandle.initialize(m_systematicsList));
    ATH_CHECK (m_eventHandle.initialize(m_systematicsList));

    ATH_CHECK(m_lrjets_n.initialize(m_systematicsList, m_eventHandle));
   
    m0_z_candidate_4vec = std::make_unique<FourVectorOutBlock>(
      this, m_copied_variable_prefix0, m_systematicsList, m_eventHandle);

    m1_z_candidate_4vec = std::make_unique<FourVectorOutBlock>(
      this, m_copied_variable_prefix1, m_systematicsList, m_eventHandle);  

     // // now the float copying 

    m0_float_copier = std::make_unique<SystVariableCopier<float>> (
      this,
      m_floats_to_copy,
      m_systematicsList,
      m_lrjetHandle,
      m_eventHandle,
      m_copied_variable_prefix0);

    
    m1_float_copier = std::make_unique<SystVariableCopier<float>> (
      this,
      m_floats_to_copy,
      m_systematicsList,
      m_lrjetHandle,
      m_eventHandle,
      m_copied_variable_prefix1);

     // int copying

    m0_int_copier = std::make_unique<SystVariableCopier<int>> (
      this,
      m_ints_to_copy,
      m_systematicsList,
      m_lrjetHandle,
      m_eventHandle,
      m_copied_variable_prefix0);

    m1_int_copier = std::make_unique<SystVariableCopier<int>> (
      this,
      m_ints_to_copy,
      m_systematicsList,
      m_lrjetHandle,
      m_eventHandle,
      m_copied_variable_prefix1);
    // Intialise syst list (must come after all syst-aware inputs and outputs)
    ATH_CHECK (m_systematicsList.initialize());

    return StatusCode::SUCCESS;
  }

  StatusCode BaselineVarsZbbjCalibAlg::execute()
  {
    //Loop over all systs
    for (const auto& sys : m_systematicsList.systematicsVector())
    {
      const xAOD::EventInfo *event = nullptr;
      ANA_CHECK (m_eventHandle.retrieve (event, sys));

      const xAOD::JetContainer *lrjets = nullptr;
      ANA_CHECK (m_lrjetHandle.retrieve (lrjets, sys));

      m_lrjets_n.set(*event, static_cast<int>(lrjets->size()), sys);
      
      if (lrjets->size() >= 1) {
        const xAOD::Jet* largeJet0 = lrjets->at(0);
        m0_z_candidate_4vec->set(*event, *lrjets->at(0), sys);
        m0_float_copier->set(*event, *largeJet0, sys);
        m0_int_copier->set(*event, *largeJet0, sys);
      }

      if (lrjets->size() >= 2) {
        const xAOD::Jet* largeJet1 = lrjets->at(1);
        m1_z_candidate_4vec->set(*event, *lrjets->at(1), sys);
        m1_float_copier->set(*event, *largeJet1, sys);
        m1_int_copier->set(*event, *largeJet1, sys);
      }
    
    }
    return StatusCode::SUCCESS;
  }

}

