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

    void getDiEleTriggers(int year, const xAOD::EventInfo* event,
			     const runBoolReadDecoMap& runBoolDecos,
			     std::vector<std::string>& di_ele_paths);

    void getDiMuTriggers(int year, const xAOD::EventInfo* event,
			     const runBoolReadDecoMap& runBoolDecos,
                    std::vector<std::string>& di_mu_paths);
     
    void getAsymLep2Triggers(int year, const xAOD::EventInfo* event,
			     const runBoolReadDecoMap& runBoolDecos, 
                    std::vector<std::string>& asym_lepton_paths);

    void getAsymLep1emTriggers(int year, const xAOD::EventInfo* event,
			     const runBoolReadDecoMap& runBoolDecos,
                    std::vector<std::string>& asym_lepton_paths);

    void getAsymLep1meTriggers(int year, const xAOD::EventInfo* event,
			     const runBoolReadDecoMap& runBoolDecos,
                    std::vector<std::string>& asym_lepton_paths);

    void getSingleLargeJetTriggers(int year, const xAOD::EventInfo* event,
         const runBoolReadDecoMap& runBoolDecos,
         std::vector<std::string>& single_large_jet_paths);

    void getSingleMassiveLargeJetTriggers(int year, const xAOD::EventInfo* event,
         const runBoolReadDecoMap& runBoolDecos,
         std::vector<std::string>& single_massive_large_jet_paths);

    void getDoubleMassiveLargeJetTriggers(int year, const xAOD::EventInfo* event,
         const runBoolReadDecoMap& runBoolDecos,
         std::vector<std::string>& double_massive_large_jet_paths);

    void getSingleSmallJetTriggers(int year, const xAOD::EventInfo* event,
         const runBoolReadDecoMap& runBoolDecos,
         std::vector<std::string>& single_small_jet_paths);
}

#endif
