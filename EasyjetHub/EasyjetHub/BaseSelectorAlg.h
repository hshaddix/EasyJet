/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#ifndef EASYJET_BASESELECTORALG
#define EASYJET_BASESELECTORALG

#include "AnaAlgorithm/AnaAlgorithm.h"

#include <EasyjetHub/CutManager.h>


namespace Easyjet
{

  /// \brief A base algorithm class for selecting events with CutFlow utilities
  class BaseSelectorAlg : public EL::AnaAlgorithm
  {
    /// \brief The standard constructor
  public:
    using EL::AnaAlgorithm::AnaAlgorithm;

  protected:
    /// \brief Cutflow Variables
    CutManager m_cuts{};
    Gaudi::Property<std::vector<std::string>> m_inputCutList{this, "cutList", {}};
    Gaudi::Property<bool> m_saveCutFlow{this, "saveCutFlow", false};
    Gaudi::Property<bool> m_isMC{this, "isMC", false, "Is this simulation?"};
    long long int m_total_events{0};
    double m_total_mcEventWeight{0.};

    StatusCode initialiseCutflow(const std::vector<std::string>& boolNameList);
    StatusCode applyCutFlow(const std::vector<bool>& cuts, double mc_weight = 0.);
    StatusCode finaliseCutflow();

  };
    
}


#endif
