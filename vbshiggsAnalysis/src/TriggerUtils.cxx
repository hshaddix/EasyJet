/*
  Copyright (C) 2002-2024 CERN for the benefit of the ATLAS collaboration
*/

#include "TriggerUtils.h"

namespace VBSHIGGS
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
    else if(runBoolDecos.at(VBSHIGGS::is22_75bunches)(*event)){
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
    else if(runBoolDecos.at(VBSHIGGS::is23_75bunches)(*event) || runBoolDecos.at(VBSHIGGS::is24_75bunches)(*event)){
      single_ele_paths = {
        "HLT_e26_lhtight_ivarloose_L1EM22VHI", "HLT_e60_lhmedium_L1EM22VHI",
        "HLT_e140_lhloose_L1EM22VHI", "HLT_e140_lhloose_noringer_L1EM22VHI",
        "HLT_e300_etcut_L1EM22VHI"
      };
    }
    else if(year==2023 || year==2024){
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
    else if(2022<=year && year<=2024 &&
	    !runBoolDecos.at(VBSHIGGS::is22_75bunches)(*event) &&
	    !runBoolDecos.at(VBSHIGGS::is23_75bunches)(*event) &&
	    !runBoolDecos.at(VBSHIGGS::is23_400bunches)(*event)&&
      !runBoolDecos.at(VBSHIGGS::is24_75bunches)(*event) &&
	    !runBoolDecos.at(VBSHIGGS::is24_400bunches_periodD)(*event) &&
	    !runBoolDecos.at(VBSHIGGS::is24_400bunches_periodL)(*event)){
      single_mu_paths = {
        "HLT_mu24_ivarmedium_L1MU14FCH", "HLT_mu50_L1MU14FCH",
        "HLT_mu60_0eta105_msonly_L1MU14FCH", "HLT_mu60_L1MU14FCH",
        "HLT_mu80_msonly_3layersEC_L1MU14FCH"
      };
    }
  }

  void getSingleLargeJetTriggers(int year, const xAOD::EventInfo* event,
         const runBoolReadDecoMap& runBoolDecos,
         std::vector<std::string>& single_large_jet_paths){
    if (year==2016){
      single_large_jet_paths={"HLT_j420_a10_lcw_L1J100"};
    }
    else if (year==2017){
      single_large_jet_paths={"HLT_j420_a10t_lcw_jes_L1J100", "HLT_j460_a10t_lcw_jes_L1J100"};
    }
    else if (year==2018){
      single_large_jet_paths={"HLT_j460_a10t_lcw_jes_L1J100"};
    }

    else if (2022 <= year && year <= 2023 &&
             !runBoolDecos.at(VBSHIGGS::is22_75bunches)(*event) &&
             !runBoolDecos.at(VBSHIGGS::is23_75bunches)(*event) &&
             !runBoolDecos.at(VBSHIGGS::is23_400bunches)(*event)&&
             !runBoolDecos.at(VBSHIGGS::is24_75bunches)(*event) &&
             !runBoolDecos.at(VBSHIGGS::is24_400bunches_periodD)(*event) &&
             !runBoolDecos.at(VBSHIGGS::is24_400bunches_periodL)(*event)){
      single_large_jet_paths={"HLT_j460_a10sd_cssk_pf_jes_ftf_preselj225_L1J100"};
    }

}

void getSingleSmallJetTriggers(int year, const xAOD::EventInfo* event,
         const runBoolReadDecoMap& runBoolDecos,
         std::vector<std::string>& single_small_jet_paths){
    if (year==2016){
      single_small_jet_paths={"HLT_j100_2j55_bmv2c2060_split", "HLT_2j55_bmv2c2060_split_ht300_L14J15"};
    }
    else if (year==2017){
      single_small_jet_paths={"HLT_j225_gsc300_bmv2c1070_split", "HLT_j150_gsc175_bmv2c1060_split_j45_gsc60_bmv2c1060_split"};
    }
    else if (year==2018){
      single_small_jet_paths={"HLT_j225_gsc300_bmv2c1070_split", "HLT_j175_gsc225_bmv2c1040_split"};
    }

    else if (year==2022 &&
             !runBoolDecos.at(VBSHIGGS::is22_75bunches)(*event) &&
             !runBoolDecos.at(VBSHIGGS::is23_75bunches)(*event) &&
             !runBoolDecos.at(VBSHIGGS::is23_400bunches)(*event)&&
             !runBoolDecos.at(VBSHIGGS::is24_75bunches)(*event) &&
             !runBoolDecos.at(VBSHIGGS::is24_400bunches_periodD)(*event) &&
             !runBoolDecos.at(VBSHIGGS::is24_400bunches_periodL)(*event)){
      single_small_jet_paths={"HLT_j80c_020jvt_j55c_020jvt_j28c_020jvt_j20c_020jvt_SHARED_2j20c_020jvt_bdl1d77_pf_ftf_presel2c20XX2c20b85_L1J45p0ETA21_3J15p0ETA25", "HLT_j150_2j55_0eta290_020jvt_bdl1d70_pf_ftf_preselj80XX2j45b90_L1J85_3J30"};
             }
    else if (year==2023 &&
            !runBoolDecos.at(VBSHIGGS::is22_75bunches)(*event) &&
             !runBoolDecos.at(VBSHIGGS::is23_75bunches)(*event) &&
             !runBoolDecos.at(VBSHIGGS::is23_400bunches)(*event)&&
             !runBoolDecos.at(VBSHIGGS::is24_75bunches)(*event) &&
             !runBoolDecos.at(VBSHIGGS::is24_400bunches_periodD)(*event) &&
             !runBoolDecos.at(VBSHIGGS::is24_400bunches_periodL)(*event)){
              single_small_jet_paths={"HLT_j75c_020jvt_j50c_020jvt_j25c_020jvt_j20c_020jvt_SHARED_2j20c_020jvt_bgn177_pf_ftf_presel2c20XX2c20b85_L1J45p0ETA21_3J15p0ETA25", "HLT_j150_2j55_0eta290_020jvt_bgn170_pf_ftf_preselj80XX2j45b90_L1J85_3J30"};
             }
    }
}