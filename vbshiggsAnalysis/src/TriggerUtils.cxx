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
        "HLT_e140_lhloose_nod0", "HLT_e300_etcut"
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
    else if(runBoolDecos.at(VBSHIGGS::is23_75bunches)(*event)){
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
    else if(year==2024){
      single_ele_paths = {
        "HLT_e26_lhtight_ivarloose_L1eEM26M", "HLT_e26_lhtight_ivarloose_L1eEM26T", 
        "HLT_e26_lhtight_ivarmedium_L1eEM26M", "HLT_e26_lhtight_ivartight_L1eEM26M", 
        "HLT_e60_lhmedium_L1eEM26M", "HLT_e140_lhloose_L1eEM26M", 
        "HLT_e140_lhloose_noringer_L1eEM26M", "HLT_e300_etcut_L1eEM26M"
      };
    }
  }

  void getSingleMuTriggers(int year, const xAOD::EventInfo* event,
			   const runBoolReadDecoMap& runBoolDecos,
			   std::vector<std::string>& single_mu_paths){
    if(year==2015){
      single_mu_paths = {"HLT_mu20_iloose_L1MU15", "HLT_mu40", "HLT_mu60_0eta105_msonly"};
    }
    else if(year==2016){
      single_mu_paths = {"HLT_mu26_ivarmedium", "HLT_mu50"};
    }
    else if(2017<=year && year<=2018){
      single_mu_paths = {"HLT_mu26_ivarmedium", "HLT_mu50", "HLT_mu60_0eta105_msonly"};
    }
    else if(2022<=year && year<=2024 &&
	    !runBoolDecos.at(VBSHIGGS::is22_75bunches)(*event) &&
	    !runBoolDecos.at(VBSHIGGS::is23_75bunches)(*event) &&
	    !runBoolDecos.at(VBSHIGGS::is23_400bunches)(*event) &&
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

  void getDiEleTriggers(int year, const xAOD::EventInfo* event,
			  const runBoolReadDecoMap& runBoolDecos,
			  std::vector<std::string>& di_ele_paths){
    if(year==2015){
      di_ele_paths = {"HLT_2e12_lhloose_L12EM10VH"};
    }
    else if(year==2016){
      di_ele_paths = {"HLT_2e17_lhvloose_nod0"};
    }
    else if(runBoolDecos.at(VBSHIGGS::is17_periodB5_B8)(*event)){
      di_ele_paths = {
        "HLT_2e24_lhvloose_nod0"
      };
    }
    else if(2017<=year && year<=2018){
      di_ele_paths = {
        "HLT_2e17_lhvloose_nod0_L12EM15VHI", "HLT_2e24_lhvloose_nod0"
      };
    }
    else if((year==2022 && !runBoolDecos.at(VBSHIGGS::is22_75bunches)(*event)) || 
            (year==2023 && runBoolDecos.at(VBSHIGGS::is23_75bunches)(*event))){
      di_ele_paths = {
        "HLT_2e17_lhvloose_L12EM15VHI", "HLT_2e24_lhvloose_L12EM20VH"
      };
    }
    else if(year==2023 || year==2024){
      di_ele_paths = {
        "HLT_2e17_lhvloose_L12eEM18M", "HLT_2e24_lhvloose_L12eEM24L"
      };
    }
  }

  void getDiMuTriggers(int year, const xAOD::EventInfo* event,
			  const runBoolReadDecoMap& runBoolDecos,
        std::vector<std::string>& di_mu_paths){
    if(year==2015){
      di_mu_paths = {"HLT_2mu10","HLT_mu18_mu8noL1"};
    }
    else if(2016<=year && year<=2018){
      di_mu_paths = {"HLT_2mu14","HLT_mu22_mu8noL1"};
    }
    else if(2022<=year && year<=2024 &&
	    !runBoolDecos.at(VBSHIGGS::is22_75bunches)(*event) &&
	    !runBoolDecos.at(VBSHIGGS::is23_75bunches)(*event) &&
	    !runBoolDecos.at(VBSHIGGS::is23_400bunches)(*event) &&
      !runBoolDecos.at(VBSHIGGS::is24_75bunches)(*event) &&
	    !runBoolDecos.at(VBSHIGGS::is24_400bunches_periodD)(*event) &&
	    !runBoolDecos.at(VBSHIGGS::is24_400bunches_periodL)(*event)){
      di_mu_paths = {
        "HLT_2mu10_l2mt_L1MU10BOM", "HLT_2mu14_L12MU8F", 
        "HLT_mu22_mu8noL1_L1MU14FCH", "HLT_mu20_ivarmedium_mu8noL1_L1MU14FCH"};
    }
  }

  void getAsymLep2Triggers(int year, const xAOD::EventInfo* event,
			  const runBoolReadDecoMap& runBoolDecos, 
        std::vector<std::string>& asym_lepton_paths){
    if(year==2015){
      asym_lepton_paths = {"HLT_e17_lhloose_mu14"};
    }
    else if(2016<=year && year<=2018){
      asym_lepton_paths = {"HLT_e17_lhloose_nod0_mu14"};
    }
    else if(2022==year && !runBoolDecos.at(VBSHIGGS::is22_75bunches)(*event)){
      asym_lepton_paths = {"HLT_e17_lhloose_mu14_L1EM15VH_MU8F"};
    }
    else if(2023<=year && year<=2024 &&
      !runBoolDecos.at(VBSHIGGS::is23_75bunches)(*event) &&
	    !runBoolDecos.at(VBSHIGGS::is23_400bunches)(*event) &&
      !runBoolDecos.at(VBSHIGGS::is24_75bunches)(*event) &&
	    !runBoolDecos.at(VBSHIGGS::is24_400bunches_periodD)(*event) &&
	    !runBoolDecos.at(VBSHIGGS::is24_400bunches_periodL)(*event)){
      asym_lepton_paths = {"HLT_e17_lhloose_mu14_L1eEM18L_MU8F"};
    }
  }

  void getAsymLep1emTriggers(int year, const xAOD::EventInfo* event,
			  const runBoolReadDecoMap& runBoolDecos,
        std::vector<std::string>& asym_lepton_paths){
    if(year==2016){
      asym_lepton_paths = {"HLT_e24_lhmedium_nod0_L1EM20VHI_mu8noL1"};
    }
    else if(2017<=year && year<=2018){
      asym_lepton_paths = {"HLT_e26_lhmedium_nod0_mu8noL1"};
    }
    else if(2022==year && !runBoolDecos.at(VBSHIGGS::is22_75bunches)(*event)){
      asym_lepton_paths = {"HLT_e26_lhmedium_mu8noL1_L1EM22VHI"};
    }
    else if(2023<=year && year>=2024 && 
      !runBoolDecos.at(VBSHIGGS::is23_75bunches)(*event) &&
	    !runBoolDecos.at(VBSHIGGS::is23_400bunches)(*event) &&
      !runBoolDecos.at(VBSHIGGS::is24_75bunches)(*event) &&
	    !runBoolDecos.at(VBSHIGGS::is24_400bunches_periodD)(*event) &&
	    !runBoolDecos.at(VBSHIGGS::is24_400bunches_periodL)(*event)){
      asym_lepton_paths = {"HLT_e26_lhmedium_mu8noL1_L1eEM26M"};
    }
  }

  void getAsymLep1meTriggers(int year, const xAOD::EventInfo* event,
			  const runBoolReadDecoMap& runBoolDecos,
        std::vector<std::string>& asym_lepton_paths){
    if(year==2015){
      asym_lepton_paths = {"HLT_e7_lhmedium_mu24"};
    }
    else if(2016<=year && year<=2018){
      asym_lepton_paths = {"HLT_e7_lhmedium_nod0_mu24"};
    }
    else if(2022==year && !runBoolDecos.at(VBSHIGGS::is22_75bunches)(*event)){
      asym_lepton_paths = {"HLT_e7_lhmedium_mu24_L1MU14FCH"};
    }
    else if(2023<=year && year<=2024 &&
      !runBoolDecos.at(VBSHIGGS::is23_75bunches)(*event) &&
	    !runBoolDecos.at(VBSHIGGS::is23_400bunches)(*event) &&
      !runBoolDecos.at(VBSHIGGS::is24_75bunches)(*event) &&
	    !runBoolDecos.at(VBSHIGGS::is24_400bunches_periodD)(*event) &&
	    !runBoolDecos.at(VBSHIGGS::is24_400bunches_periodL)(*event)){
      asym_lepton_paths = {"HLT_e7_lhmedium_L1eEM5_mu24_L1MU14FCH"};
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

    else if (2022 <= year && year <= 2024 &&
             !runBoolDecos.at(VBSHIGGS::is22_75bunches)(*event) &&
             !runBoolDecos.at(VBSHIGGS::is23_75bunches)(*event) &&
             !runBoolDecos.at(VBSHIGGS::is23_400bunches)(*event)&&
             !runBoolDecos.at(VBSHIGGS::is24_75bunches)(*event) &&
             !runBoolDecos.at(VBSHIGGS::is24_400bunches_periodD)(*event) &&
             !runBoolDecos.at(VBSHIGGS::is24_400bunches_periodL)(*event)){
      single_large_jet_paths={"HLT_j460_a10sd_cssk_pf_jes_ftf_preselj225_L1J100"};
    }

}

void getSingleMassiveLargeJetTriggers(int year, const xAOD::EventInfo* event,
         const runBoolReadDecoMap& runBoolDecos,
         std::vector<std::string>& single_massive_large_jet_paths){
    (void)event;
    (void)runBoolDecos;
    if (year == 2017) {
      single_massive_large_jet_paths = {"HLT_j390_a10t_lcw_jes_30smcINF_L1J100", "HLT_j420_a10t_lcw_jes_40smcINF_L1J100"};
    } else if (year == 2018) {
      single_massive_large_jet_paths = {"HLT_j420_a10t_lcw_jes_35smcINF_L1SC111", "HLT_j420_a10t_lcw_jes_35smcINF_L1J100"};
    } else if (year == 2022 || year == 2023 || year == 2024) {
      single_massive_large_jet_paths = {"HLT_j420_35smcINF_a10sd_cssk_pf_jes_ftf_preselj225_L1J100"};
    } 
}

void getDoubleMassiveLargeJetTriggers(int year, const xAOD::EventInfo* event,
         const runBoolReadDecoMap& runBoolDecos,
         std::vector<std::string>& double_massive_large_jet_paths){
    (void)event;
    (void)runBoolDecos;
    if (year == 2017) {
      double_massive_large_jet_paths = {"HLT_2j330_a10t_lcw_jes_30smcINF_L1J100", "HLT_2j330_a10t_lcw_jes_40smcINF_L1J100"};
    } else if (year == 2018) {
      double_massive_large_jet_paths = {"HLT_2j330_a10t_lcw_jes_35smcINF_L1J100", "HLT_2j330_a10t_lcw_jes_35smcINF_L1SC111", "HLT_j360_a10t_lcw_jes_60smcINF_j360_a10t_lcw_jes_L1SC111", "HLT_j370_a10t_lcw_jes_35smcINF_j370_a10t_lcw_jes_L1SC111"};
    } else if (year == 2022 || year == 2023 || year == 2024) {
      double_massive_large_jet_paths = {"HLT_2j330_35smcINF_a10sd_cssk_pf_jes_ftf_presel2j225_L1J100"};
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