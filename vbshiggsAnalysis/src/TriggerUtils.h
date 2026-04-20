/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#ifndef VBSHIGGSANALYSIS_TRIGGERUTILS
#define VBSHIGGSANALYSIS_TRIGGERUTILS

#include <AsgDataHandles/ReadDecorHandle.h>
#include "TriggerDecoratorAlg.h"
#include <xAODEventInfo/EventInfo.h>

namespace VBSHIGGS
{
    typedef std::unordered_map<VBSHIGGS::RunBooleans, SG::ReadDecorHandle<xAOD::EventInfo, char> > runBoolReadDecoMap;

    void getSingleEleTriggers(int year, const xAOD::EventInfo* event,
			      const runBoolReadDecoMap& runBoolDecos,
			      std::vector<std::string>& single_ele_paths);

    void getSingleMuTriggers(int year, const xAOD::EventInfo* event,
			     const runBoolReadDecoMap& runBoolDecos,
			     std::vector<std::string>& single_mu_paths);
    void getSingleLargeJetTriggers(int year, const xAOD::EventInfo* event,
         const runBoolReadDecoMap& runBoolDecos,
         std::vector<std::string>& single_large_jet_paths);
    void getSingleSmallJetTriggers(int year, const xAOD::EventInfo* event,
         const runBoolReadDecoMap& runBoolDecos,
         std::vector<std::string>& single_small_jet_paths);
}

#endif
