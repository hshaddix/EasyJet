/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#ifndef VBSSEMILEPANALYSIS_TRIGGERUTILS
#define VBSSEMILEPANALYSIS_TRIGGERUTILS

#include <AsgDataHandles/ReadDecorHandle.h>
#include "TriggerDecoratorAlg.h"
#include <xAODEventInfo/EventInfo.h>

namespace VBSSEMILEP
{
    std::vector<std::string> getSingleLargeRJetsTriggers(int year);
    typedef std::unordered_map<VBSSEMILEP::RunBooleans, SG::ReadDecorHandle<xAOD::EventInfo, char> > runBoolReadDecoMap;

    void getSingleEleTriggers(int year, const xAOD::EventInfo* event,
			      const runBoolReadDecoMap& runBoolDecos,
			      std::vector<std::string>& single_ele_paths);

    void getSingleMuTriggers(int year, const xAOD::EventInfo* event,
			     const runBoolReadDecoMap& runBoolDecos,
			     std::vector<std::string>& single_mu_paths);
}

#endif
