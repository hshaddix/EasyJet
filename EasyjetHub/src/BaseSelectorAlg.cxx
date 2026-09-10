/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "EasyjetHub/BaseSelectorAlg.h"

namespace Easyjet
{

  StatusCode BaseSelectorAlg::initialiseCutflow
  (const std::vector<std::string>& boolNameList,
   const std::string& prefix) {
    m_cuts.CheckInputCutList(m_inputCutList, boolNameList);

    // Initialize a vector of CutEntry structs based on the input Cut List
    for (const auto &cut : m_inputCutList) m_cuts.add(cut);

    const unsigned int nbins = m_cuts.size() + 1; //  need an extra bin for the total num of events.
    ANA_CHECK (book (TEfficiency(("Absolute"+prefix+"Efficiency").c_str(),
				 "Absolute Efficiency;Cuts;#epsilon",
				 nbins, 0.5, nbins + 0.5)));
    ANA_CHECK (book (TEfficiency(("Relative"+prefix+"Efficiency").c_str(),
				 "Relative Efficiency;Cuts;#epsilon",
				 nbins, 0.5, nbins + 0.5)));
    ANA_CHECK (book (TEfficiency(("Standard"+prefix+"CutFlow").c_str(),
				 "StandardCutFlow;Cuts;#epsilon",
				 nbins, 0.5, nbins + 0.5)));
    if(m_isMC) {
      ANA_CHECK (book (TEfficiency(("WeightedAbsolute"+prefix+"Efficiency").c_str(),
				   "Weighted Absolute Efficiency;Cuts;#epsilon",
				   nbins, 0.5, nbins + 0.5)));
      ANA_CHECK (book (TEfficiency(("WeightedRelative"+prefix+"Efficiency").c_str(),
				   "Weighted Relative Efficiency;Cuts;#epsilon",
				   nbins, 0.5, nbins + 0.5)));
      ANA_CHECK (book (TEfficiency(("WeightedStandard"+prefix+"CutFlow").c_str(),
				   "Weighted StandardCutFlow;Cuts;#epsilon",
				   nbins, 0.5, nbins + 0.5)));
    }
    ANA_CHECK (book (TH1F((prefix+"EventsPassed_BinLabeling").c_str(),
			  "Events passed by each cut / Bin labeling",
			  nbins, 0.5, nbins + 0.5)));

    return StatusCode::SUCCESS;
    }

  StatusCode BaseSelectorAlg::applyCutFlow(const std::vector<bool>& bool_cuts, double mc_weight) {
    if(bool_cuts.size()!=m_inputCutList.size()){
      ATH_MSG_ERROR("Misconfigured boolean cuts in applyCutFlow");
      return StatusCode::FAILURE;
    }
    
    //****************
    // Cutflow
    //****************
    // Compute total_events
    m_total_events += 1;
    m_total_mcEventWeight += mc_weight;

    // Count which cuts the event passed
    for (unsigned int i=0; i<m_inputCutList.size(); i++){
      std::string cut = m_inputCutList[i];
      m_cuts(cut).passed = bool_cuts[i];
      if (m_cuts(cut).passed) {
	m_cuts(cut).counter += 1;
	m_cuts(cut).w_counter += mc_weight;
      }
    }

    // Check how many consecutive cuts are passed by the event.
    for (auto& cut : m_cuts) {
      if(!cut.passed) break;
      cut.relativeCounter += 1;
      cut.w_relativeCounter += mc_weight;
    }

    return StatusCode::SUCCESS;
  }

  StatusCode BaseSelectorAlg::finaliseCutflow() {
    m_cuts.CheckCutResults(); // Print CheckCutResults

    if(m_saveCutFlow) {
      m_cuts.DoAbsoluteEfficiency(m_total_events, efficiency("AbsoluteEfficiency"));
      m_cuts.DoRelativeEfficiency(m_total_events, efficiency("RelativeEfficiency"));
      m_cuts.DoStandardCutFlow(m_total_events, efficiency("StandardCutFlow"));
      if(m_isMC) {
        m_cuts.DoWeightedAbsoluteEfficiency(m_total_mcEventWeight, efficiency("WeightedAbsoluteEfficiency"));
        m_cuts.DoWeightedRelativeEfficiency(m_total_mcEventWeight, efficiency("WeightedRelativeEfficiency"));
        m_cuts.DoWeightedStandardCutFlow(m_total_mcEventWeight, efficiency("WeightedStandardCutFlow"));
      }
      m_cuts.DoCutflowLabeling(m_total_events, hist("EventsPassed_BinLabeling"));
    }

    return StatusCode::SUCCESS;
  }

}
