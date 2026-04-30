/*
  Copyright (C) 2002-2024 CERN for the benefit of the ATLAS collaboration
*/

#include "TriggerUtils.h"

namespace VBSSEMILEP
{
  void getSingleEleTriggers(int year, const xAOD::EventInfo* event,
			    const runBoolReadDecoMap& runBoolDecos,
			    std::vector<std::string>& single_ele_paths){
    if(year==2015){
      single_ele_paths = {
        "HLT_e24_lhmedium_L1EM20VH", "HLT_e60_lhmedium",
        "HLT_e120_lhloose"
      };
    }
    else if(2016<=year && year<=2018){
      single_ele_paths = {
        "HLT_e26_lhtight_nod0_ivarloose", "HLT_e60_lhmedium_nod0",
        "HLT_e140_lhloose_nod0"
      };
    }
    else if(runBoolDecos.at(VBSSEMILEP::is22_75bunches)(*event)){
      single_ele_paths = {
        "HLT_e17_lhvloose_L1EM15VHI", "HLT_e20_lhvloose_L1EM15VH",
        "HLT_e250_etcut_L1EM22VHI"
      };
    }
    else if(year==2022){
      single_ele_paths = {
        "HLT_e26_lhtight_ivarloose_L1EM22VHI", "HLT_e60_lhmedium_L1EM22VHI",
        "HLT_e140_lhloose_L1EM22VHI", "HLT_e300_etcut_L1EM22VHI"
      };
    }
    else if(runBoolDecos.at(VBSSEMILEP::is23_75bunches)(*event)){
      single_ele_paths = {
        "HLT_e26_lhtight_ivarloose_L1EM22VHI", "HLT_e60_lhmedium_L1EM22VHI",
        "HLT_e140_lhloose_L1EM22VHI", "HLT_e140_lhloose_noringer_L1EM22VHI",
        "HLT_e300_etcut_L1EM22VHI"
      };
    }
    else if(year==2023){
      single_ele_paths = {
        "HLT_e26_lhtight_ivarloose_L1eEM26M", "HLT_e60_lhmedium_L1eEM26M",
        "HLT_e140_lhloose_L1eEM26M", "HLT_e140_lhloose_noringer_L1eEM26M",
        "HLT_e300_etcut_L1eEM26M"
      };
    }
  }

  void getSingleMuTriggers(int year, const xAOD::EventInfo* event,
			   const runBoolReadDecoMap& runBoolDecos,
			   std::vector<std::string>& single_mu_paths){
    if(year==2015){
      single_mu_paths = {"HLT_mu20_iloose_L1MU15", "HLT_mu40"};
    }
    else if(2016<=year && year<=2018){
      single_mu_paths = {"HLT_mu26_ivarmedium", "HLT_mu50"};
    }
    else if(2022<=year && year<=2023 &&
	    !runBoolDecos.at(VBSSEMILEP::is22_75bunches)(*event) &&
	    !runBoolDecos.at(VBSSEMILEP::is23_75bunches)(*event) &&
	    !runBoolDecos.at(VBSSEMILEP::is23_400bunches)(*event)){
      single_mu_paths = {
        "HLT_mu24_ivarmedium_L1MU14FCH", "HLT_mu50_L1MU14FCH",
        "HLT_mu60_0eta105_msonly_L1MU14FCH", "HLT_mu60_L1MU14FCH",
        "HLT_mu80_msonly_3layersEC_L1MU14FCH"
      };
    }
  }
  
  std::vector<std::string> getSingleLargeRJetsTriggers(int year){

    std::vector<std::string> list;

    if(year == 2015){
      list = {"HLT_j360_a10_lcw_sub_L1J100"};
    }
    else if(2016==year){
      list = {"HLT_j420_a10_lcw_L1J100"};
    }
    else if(2017<=year && year<=2018){
      list = {"HLT_j460_a10t_lcw_jes_L1J100"};
    }

    return list;
    
  }

  
}
