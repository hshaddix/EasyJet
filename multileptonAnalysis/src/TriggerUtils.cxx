/*
  Copyright (C) 2002-2024 CERN for the benefit of the ATLAS collaboration
*/

#include "TriggerUtils.h"

namespace MULTILEPTON
{

  void getSingleMuTriggers(int year, const xAOD::EventInfo* eventInfo,
			   const runBoolReadDecoMap& runBoolDecos,
			   std::vector<std::string>& single_mu_paths,
			   std::string& single_mu_SF_path){
    if(year==2015){
      single_mu_paths = {"HLT_mu20_iloose_L1MU15", "HLT_mu50"};
      single_mu_SF_path = "mu20_iloose_L1MU15_OR_mu40";
    }
    else if(2016<=year && year<=2018){
      single_mu_paths = {"HLT_mu26_ivarmedium", "HLT_mu50"};
      single_mu_SF_path = "mu26_ivarmedium_OR_mu50";
    }
    else if(2022<=year && year<=2023 &&
	    !runBoolDecos.at(MULTILEPTON::is22_75bunches)(*eventInfo) &&
	    !runBoolDecos.at(MULTILEPTON::is23_75bunches)(*eventInfo) &&
	    !runBoolDecos.at(MULTILEPTON::is23_400bunches)(*eventInfo)){
      single_mu_paths = {
        "HLT_mu24_ivarmedium_L1MU14FCH", "HLT_mu50_L1MU14FCH",
        "HLT_mu60_0eta105_msonly_L1MU14FCH", "HLT_mu60_L1MU14FCH",
        "HLT_mu80_msonly_3layersEC_L1MU14FCH"
      };
      single_mu_SF_path = "mu24_ivarmedium_L1MU14FCH_OR_mu50_L1MU14FCH";
    }
    else if(year==2024){
      single_mu_paths = {
        "HLT_mu24_ivarmedium_L1MU14FCH", "HLT_mu50_L1MU14FCH"
      };
      single_mu_SF_path = "mu24_ivarmedium_L1MU14FCH_OR_mu50_L1MU14FCH";
    }
  }

  void getSingleEleTriggers(int year, const xAOD::EventInfo* eventInfo,
			    const runBoolReadDecoMap& runBoolDecos,
			    std::vector<std::string>& single_ele_paths,
			    std::string& single_ele_SF_path){
    if(year==2015){
      single_ele_paths = {
        "HLT_e24_lhmedium_L1EM20VH", "HLT_e60_lhmedium",
        "HLT_e120_lhloose"
      };
      single_ele_SF_path = "e24_lhmedium_L1EM20VH_OR_e60_lhmedium_OR_e120_lhloose";
    }
    else if(2016<=year && year<=2018){
      single_ele_paths = {
        "HLT_e26_lhtight_nod0_ivarloose", "HLT_e60_lhmedium_nod0",
        "HLT_e140_lhloose_nod0"
      };
      single_ele_SF_path = "e26_lhtight_nod0_ivarloose_OR_e60_lhmedium_nod0_OR_e140_lhloose_nod0";
    }
    else if(runBoolDecos.at(MULTILEPTON::is22_75bunches)(*eventInfo)){
      single_ele_paths = {
        "HLT_e17_lhvloose_L1EM15VHI", "HLT_e20_lhvloose_L1EM15VH",
        "HLT_e250_etcut_L1EM22VHI"
      };
      single_ele_SF_path = "e26_lhtight_ivarloose_L1EM22VHI_OR_e60_lhmedium_L1EM22VHI_OR_e140_lhloose_L1EM22VHI";
    }
    else if(year==2022){
      single_ele_paths = {
        "HLT_e26_lhtight_ivarloose_L1EM22VHI", "HLT_e60_lhmedium_L1EM22VHI",
        "HLT_e140_lhloose_L1EM22VHI", "HLT_e300_etcut_L1EM22VHI"
      };
      single_ele_SF_path = "e26_lhtight_ivarloose_L1EM22VHI_OR_e60_lhmedium_L1EM22VHI_OR_e140_lhloose_L1EM22VHI";
    }
    else if(runBoolDecos.at(MULTILEPTON::is23_75bunches)(*eventInfo)){
      single_ele_paths = {
        "HLT_e26_lhtight_ivarloose_L1EM22VHI", "HLT_e60_lhmedium_L1EM22VHI",
        "HLT_e140_lhloose_L1EM22VHI", "HLT_e140_lhloose_noringer_L1EM22VHI",
        "HLT_e300_etcut_L1EM22VHI"
      };
      single_ele_SF_path = "e26_lhtight_ivarloose_L1eEM26M_OR_e60_lhmedium_L1eEM26M_OR_e140_lhloose_L1eEM26M_OR_e300_etcut_L1eEM26M";
    }
    else if(year==2023){
      single_ele_paths = {
        "HLT_e26_lhtight_ivarloose_L1eEM26M", "HLT_e60_lhmedium_L1eEM26M",
        "HLT_e140_lhloose_L1eEM26M", "HLT_e140_lhloose_noringer_L1eEM26M",
        "HLT_e300_etcut_L1eEM26M"
      };
      single_ele_SF_path = "e26_lhtight_ivarloose_L1eEM26M_OR_e60_lhmedium_L1eEM26M_OR_e140_lhloose_L1eEM26M_OR_e300_etcut_L1eEM26M";
    }
    else if(year==2024){
      single_ele_paths = {
        "HLT_e26_lhtight_ivarloose_L1eEM26M", "HLT_e60_lhmedium_L1eEM26M",
        "HLT_e140_lhloose_L1eEM26M"
      };
      single_ele_SF_path = "e26_lhtight_ivarloose_L1eEM26M_OR_e60_lhmedium_L1eEM26M_OR_e140_lhloose_L1eEM26M";
    }
  }

  void getMuTauTriggers(int year, const xAOD::EventInfo* eventInfo,
			const runBoolReadDecoMap& runBoolDecos,
			std::vector<std::string>& mu_tau_paths_2016,
			std::vector<std::string>& mu_tau_paths_low,
			std::vector<std::string>& mu_tau_paths_high,
			std::pair<std::string, std::string>& mu_tau_SF_path){
    if(year==2015 || runBoolDecos.at(MULTILEPTON::is16PeriodA)(*eventInfo)){
      mu_tau_paths_2016 = {"HLT_mu14_tau25_medium1_tracktwo"};
      // No muon SF leg yet
      mu_tau_SF_path = std::make_pair("", "tau25_medium1_tracktwo");
    }
    else if(runBoolDecos.at(MULTILEPTON::is16PeriodB_D3)(*eventInfo) ||
	    runBoolDecos.at(MULTILEPTON::is16PeriodD4_end)(*eventInfo)){
      mu_tau_paths_2016 = {"HLT_mu14_ivarloose_tau25_medium1_tracktwo"};
      // No muon SF leg yet
      mu_tau_SF_path = std::make_pair("", "tau25_medium1_tracktwo");
    }
    else if(runBoolDecos.at(MULTILEPTON::is17PeriodB1_B4)(*eventInfo) ||
	    runBoolDecos.at(MULTILEPTON::is17PeriodB5_B7)(*eventInfo) ||
	    runBoolDecos.at(MULTILEPTON::is17PeriodB8_end)(*eventInfo)){
      mu_tau_paths_low = {"HLT_mu14_ivarloose_tau25_medium1_tracktwo_L1MU10_TAU12IM_3J12"};
      mu_tau_paths_high = {"HLT_mu14_ivarloose_tau35_medium1_tracktwo"};
      // No muon SF leg yet
      mu_tau_SF_path = std::make_pair("", "tau25_medium1_tracktwo");
    }
    else if(runBoolDecos.at(MULTILEPTON::is18PeriodB_end)(*eventInfo)){
      mu_tau_paths_low = {"HLT_mu14_ivarloose_tau25_medium1_tracktwoEF_L1MU10_TAU12IM_3J12"};
      mu_tau_paths_high = {"HLT_mu14_ivarloose_tau35_medium1_tracktwoEF"};
      if(runBoolDecos.at(MULTILEPTON::is18PeriodK_end)(*eventInfo)){
        mu_tau_paths_low.emplace_back("HLT_mu14_ivarloose_tau25_mediumRNN_tracktwoMVA_L1MU10_TAU12IM_3J12");
        mu_tau_paths_high.emplace_back("HLT_mu14_ivarloose_tau35_mediumRNN_tracktwoMVA");
      }
      // No muon SF leg yet
      mu_tau_SF_path = std::make_pair("", "tau25_medium1_tracktwoEF");
    }
    else if(year>=2022 && year<=2023){
      mu_tau_paths_low = {"HLT_mu14_ivarloose_tau25_mediumRNN_tracktwoMVA_03dRAB_L1MU8F_TAU12IM_3J12"};
      mu_tau_paths_high = {"HLT_mu14_ivarloose_tau35_mediumRNN_tracktwoMVA_03dRAB_L1MU8F_TAU20IM"};
      mu_tau_SF_path = std::make_pair("", "tau25_medium1_tracktwoEF");
    }
  }

  void getEleTauTriggers(int year, const xAOD::EventInfo* eventInfo,
			 const runBoolReadDecoMap& runBoolDecos,
			 std::vector<std::string>& ele_tau_paths,
			 std::vector<std::string>& ele_tau_paths_4J12,
			 std::pair<std::string, std::string>& ele_tau_SF_path){
    if(year==2015 || runBoolDecos.at(MULTILEPTON::is16PeriodA)(*eventInfo)){
      ele_tau_paths = {"HLT_e17_lhmedium_nod0_tau25_medium1_tracktwo"};
      ele_tau_SF_path = std::make_pair("e17_lhmedium_nod0_L1EM15HI",
				       "tau25_medium1_tracktwo");
    }
    else if(runBoolDecos.at(MULTILEPTON::is16PeriodB_D3)(*eventInfo) ||
	    runBoolDecos.at(MULTILEPTON::is16PeriodD4_end)(*eventInfo)){
      ele_tau_paths = {"HLT_e17_lhmedium_nod0_ivarloose_tau25_medium1_tracktwo"};
      ele_tau_SF_path = std::make_pair("e17_lhmedium_nod0_AND_e17_lhvloose_nod0_L1EM15VHI",
				       "tau25_medium1_tracktwo");
    }
    else if(runBoolDecos.at(MULTILEPTON::is17PeriodB1_B4)(*eventInfo) ||
	    runBoolDecos.at(MULTILEPTON::is17PeriodB5_B7)(*eventInfo) ||
	    runBoolDecos.at(MULTILEPTON::is17PeriodB8_end)(*eventInfo)){
      ele_tau_paths = {"HLT_e17_lhmedium_nod0_ivarloose_tau25_medium1_tracktwo"};
      ele_tau_paths_4J12 = {"HLT_e17_lhmedium_nod0_ivarloose_tau25_medium1_tracktwo_L1EM15VHI_2TAU12IM_4J12"};
      ele_tau_SF_path = std::make_pair("e17_lhmedium_nod0_AND_e17_lhvloose_nod0_L1EM15VHI",
				       "tau25_medium1_tracktwo");
    }
    else if(runBoolDecos.at(MULTILEPTON::is18PeriodB_end)(*eventInfo)){
      ele_tau_paths = {"HLT_e17_lhmedium_nod0_ivarloose_tau25_medium1_tracktwoEF"};
      ele_tau_paths_4J12 = {"HLT_e17_lhmedium_nod0_ivarloose_tau25_medium1_tracktwoEF_L1EM15VHI_2TAU12IM_4J12"};
      if(runBoolDecos.at(MULTILEPTON::is18PeriodK_end)(*eventInfo)){
        ele_tau_paths.emplace_back("HLT_e17_lhmedium_nod0_ivarloose_tau25_mediumRNN_tracktwoMVA");
        ele_tau_paths_4J12.emplace_back("HLT_e17_lhmedium_nod0_ivarloose_tau25_mediumRNN_tracktwoMVA_L1EM15VHI_2TAU12IM_4J12");
      }
      ele_tau_SF_path = std::make_pair("e17_lhmedium_nod0_AND_e17_lhvloose_nod0_L1EM15VHI",
				       "tau25_medium1_tracktwoEF");
    }
    else if(year==2022){
      ele_tau_paths = {"HLT_e24_lhmedium_ivarloose_tau20_mediumRNN_tracktwoMVA_03dRAB_L1EM22VHI"};
      ele_tau_paths_4J12 = {"HLT_e17_lhmedium_ivarloose_tau25_mediumRNN_tracktwoMVA_03dRAB_L1EM15VHI_2TAU12IM_4J12"};
      ele_tau_SF_path = std::make_pair("e17_lhmedium_nod0_AND_e17_lhvloose_nod0_L1EM15VHI",
				       "tau25_mediumRNN_tracktwoMVA");
    }
    else if(year==2023){
      ele_tau_paths = {"HLT_e24_lhmedium_ivarloose_tau20_mediumRNN_tracktwoMVA_03dRAB_L1eEM26M"};
      ele_tau_paths_4J12 = {"HLT_e17_lhmedium_ivarloose_tau25_mediumRNN_tracktwoMVA_03dRAB_L1EM15VHI_2TAU12IM_4J12"};
      ele_tau_SF_path = std::make_pair("e17_lhmedium_nod0_AND_e17_lhvloose_nod0_L1EM15VHI",
				       "tau25_mediumRNN_tracktwoMVA");
    }
    else if(year==2024){
      ele_tau_paths = {
        "HLT_e17_lhmedium_ivarloose_tau25_mediumRNN_tracktwoMVA_03dRAB_L1eEM18M_2eTAU20M_4jJ30",
        "HLT_e17_lhmedium_ivarloose_tau25_mediumRNN_tracktwoMVA_03dRAB_L1eEM18M_2cTAU20M_4jJ30"
        "HLT_e24_lhmedium_ivarloose_tau20_mediumRNN_tracktwoMVA_03dRAB_L1eEM26M",
      };
      ele_tau_SF_path = std::make_pair("e17_lhmedium_nod0_AND_e17_lhvloose_nod0_L1EM15VHI",
				       "tau25_mediumRNN_tracktwoMVA");
    }
  }

  void getSingleTauTriggers(int year, const xAOD::EventInfo* eventInfo,
			    const runBoolReadDecoMap& runBoolDecos,
			    std::vector<std::string>& single_tau_paths,
			    std::string& single_tau_SF_path){
    if(year==2015 || runBoolDecos.at(MULTILEPTON::is16PeriodA)(*eventInfo)){
      single_tau_paths = {"HLT_tau80_medium1_tracktwo_L1TAU60"};
      single_tau_SF_path = "tau80L1TAU60_medium1_tracktwo";
    }
    else if(runBoolDecos.at(MULTILEPTON::is16PeriodB_D3)(*eventInfo)){
      single_tau_paths = {"HLT_tau125_medium1_tracktwo"};
      single_tau_SF_path = "tau125_medium1_tracktwo";
    }
    else if(runBoolDecos.at(MULTILEPTON::is16PeriodD4_end)(*eventInfo) ||
	    runBoolDecos.at(MULTILEPTON::is17PeriodB1_B4)(*eventInfo)){
      single_tau_paths = {"HLT_tau160_medium1_tracktwo"};
      single_tau_SF_path = "tau160_medium1_tracktwo";
    }
    else if(runBoolDecos.at(MULTILEPTON::is17PeriodB5_B7)(*eventInfo) ||
	    runBoolDecos.at(MULTILEPTON::is17PeriodB8_end)(*eventInfo)){
      single_tau_paths = {"HLT_tau160_medium1_tracktwo_L1TAU100"};
      single_tau_SF_path = "tau160L1TAU100_medium1_tracktwo";
    }
    else if(year==2018){
      single_tau_paths = {"HLT_tau160_medium1_tracktwoEF_L1TAU100"};
      single_tau_SF_path = "tau160L1TAU100_medium1_tracktwoEF";
      if(runBoolDecos.at(MULTILEPTON::is18PeriodK_end)(*eventInfo)){
        single_tau_paths.emplace_back("HLT_tau160_mediumRNN_tracktwoMVA_L1TAU100");
      }
    }
    else if(year==2022){
      single_tau_paths = {"HLT_tau160_mediumRNN_tracktwoMVA_L1TAU100"};
      single_tau_SF_path = "tau160_mediumRNN_tracktwoMVA";
    }
    else if(year==2023){
      single_tau_paths = {"HLT_tau160_mediumRNN_tracktwoMVA_L1TAU100"};
      single_tau_SF_path = "tau160_mediumRNN_tracktwoMVA";
      if(runBoolDecos.at(MULTILEPTON::is23_first_2400bunches)(*eventInfo)){
        single_tau_paths = {"HLT_tau160_mediumRNN_tracktwoMVA_L1eTAU140"};
      }
    }
    else if(year==2024){
      single_tau_paths = {"HLT_tau160_mediumRNN_tracktwoMVA_L1eTAU140"};
      single_tau_SF_path = "tau160_mediumRNN_tracktwoMVA";
    }
  }

  void getDiTauTriggers(int year, const xAOD::EventInfo* eventInfo,
			const runBoolReadDecoMap& runBoolDecos,
			std::vector<std::string>& ditau_paths_2016,
			std::vector<std::string>& ditau_paths_L1Topo,
			std::vector<std::string>& ditau_paths_4J12,
			std::vector<std::string>& ditau_paths_L1Topo_delayed,
			std::vector<std::string>& ditau_paths_4J12_delayed,
			std::vector<std::string>& tau35_match_paths,
			std::vector<std::string>& tau25_match_paths,
      std::vector<std::string>& ditau_path,
			std::pair<std::string, std::string>& di_tau_SF_path){
    if(year==2015){
      ditau_paths_2016 = {"HLT_tau35_medium1_tracktwo_tau25_medium1_tracktwo_L1TAU20IM_2TAU12IM"};
      tau35_match_paths = {"HLT_tau35_medium1_tracktwo_L1TAU20"};
      tau25_match_paths = {"HLT_tau25_medium1_tracktwo_L1TAU12"};
      di_tau_SF_path = std::make_pair("tau35_medium1_tracktwo",
				      "tau25_medium1_tracktwo");
    }

    else if(year==2016){
      ditau_paths_2016 = {"HLT_tau35_medium1_tracktwo_tau25_medium1_tracktwo"};
      tau35_match_paths = {"HLT_tau35_medium1_tracktwo"};
      tau25_match_paths = {"HLT_tau25_medium1_tracktwo"};
      di_tau_SF_path = std::make_pair("tau35_medium1_tracktwo",
				      "tau25_medium1_tracktwo");
    }

    else if(year==2017){
      ditau_paths_4J12 = {"HLT_tau35_medium1_tracktwo_tau25_medium1_tracktwo_L1TAU20IM_2TAU12IM_4J12"};
      tau35_match_paths = {"HLT_tau35_medium1_tracktwo"};
      tau25_match_paths = {"HLT_tau25_medium1_tracktwo_L1TAU12"};

      if(runBoolDecos.at(MULTILEPTON::l1topo_disabled)(*eventInfo)){
        ditau_paths_2016 = {"HLT_tau35_medium1_tracktwo_tau25_medium1_tracktwo"};
        tau25_match_paths.emplace_back("HLT_tau25_medium1_tracktwo");
      }

      if(runBoolDecos.at(MULTILEPTON::is17PeriodB1_B4)(*eventInfo)){
        // For Period B1 to B4 in 2017, should use this trigger and 2016 selection
        ditau_paths_2016 = {"HLT_tau35_medium1_tracktwo_tau25_medium1_tracktwo"};
        tau25_match_paths.emplace_back("HLT_tau25_medium1_tracktwo");
      }
      else if(!runBoolDecos.at(MULTILEPTON::l1topo_disabled)(*eventInfo) &&
	      (runBoolDecos.at(MULTILEPTON::is17PeriodB5_B7)(*eventInfo) ||
	       runBoolDecos.at(MULTILEPTON::is17PeriodB8_end)(*eventInfo))){
        ditau_paths_L1Topo = {"HLT_tau35_medium1_tracktwo_tau25_medium1_tracktwo_L1DR_TAU20ITAU12I_J25"};
      }
      di_tau_SF_path = std::make_pair("tau35_medium1_tracktwo",
				      "tau25_medium1_tracktwo");
    }
    
    else if(year==2018){
      ditau_paths_L1Topo = {"HLT_tau35_medium1_tracktwoEF_tau25_medium1_tracktwoEF_L1DR_TAU20ITAU12I_J25"};
      ditau_paths_4J12 = {"HLT_tau35_medium1_tracktwoEF_tau25_medium1_tracktwoEF_L1TAU20IM_2TAU12IM_4J12p0ETA23"};
      // single tau35_medium1 trigger is missing for some runs in 2018
      // COMA list including tau35_medium1 trigger :
      // https://atlas-tagservices.cern.ch/tagservices/RunBrowser/runBrowserReport/runBrowserReport.php?fnt=&pn=&runs=&gfile=&smk=&cn=HLT_tau35_medium1_tracktwoEF&stt=&stm=
      if(runBoolDecos.at(MULTILEPTON::is18_tau35_medium1)(*eventInfo)){
        tau35_match_paths = {"HLT_tau35_medium1_tracktwoEF"};
        tau25_match_paths = {"HLT_tau25_medium1_tracktwoEF"};
      }
      if(runBoolDecos.at(MULTILEPTON::is18PeriodK_end)(*eventInfo)){
        ditau_paths_L1Topo.emplace_back("HLT_tau35_mediumRNN_tracktwoMVA_tau25_mediumRNN_tracktwoMVA_L1DR_TAU20ITAU12I_J25");
        ditau_paths_4J12.emplace_back("HLT_tau35_mediumRNN_tracktwoMVA_tau25_mediumRNN_tracktwoMVA_L1TAU20IM_2TAU12IM_4J12p0ETA23");
        tau35_match_paths.emplace_back("HLT_tau35_mediumRNN_tracktwoMVA");
        tau25_match_paths.emplace_back("HLT_tau25_mediumRNN_tracktwoMVA");
      }

      di_tau_SF_path = std::make_pair("tau35_medium1_tracktwoEF",
				      "tau25_medium1_tracktwoEF");
    }

    else if(year==2022){
      ditau_paths_L1Topo = {"HLT_tau35_mediumRNN_tracktwoMVA_tau25_mediumRNN_tracktwoMVA_03dRAB30_L1DR_TAU20ITAU12I_J25"};
      ditau_paths_4J12 = {"HLT_tau35_mediumRNN_tracktwoMVA_tau25_mediumRNN_tracktwoMVA_03dRAB_L1TAU20IM_2TAU12IM_4J12p0ETA25"};
      // Single leg paths not available for matching for Run 3
      // LTT T&P triggers to be used instead
      tau35_match_paths = {
        "HLT_mu24_ivarmedium_tau35_mediumRNN_tracktwoMVA_probe_03dRAB_L1MU14FCH",
        "HLT_e26_lhtight_ivarloose_tau35_mediumRNN_tracktwoMVA_probe_03dRAB_L1EM22VHI"
      };
      tau25_match_paths = {
        "HLT_mu24_ivarmedium_tau25_mediumRNN_tracktwoMVA_probe_03dRAB_L1MU14FCH",
        "HLT_e26_lhtight_ivarloose_tau25_mediumRNN_tracktwoMVA_probe_03dRAB_L1EM22VHI"
      };
      di_tau_SF_path = std::make_pair("tau35_mediumRNN_tracktwoMVA",
              "tau25_mediumRNN_tracktwoMVA");
    }

    else if(year==2023){
      ditau_paths_L1Topo = {"HLT_tau35_mediumRNN_tracktwoMVA_tau25_mediumRNN_tracktwoMVA_03dRAB30_L1DR_TAU20ITAU12I_J25"};
      ditau_paths_4J12 = {"HLT_tau35_mediumRNN_tracktwoMVA_tau25_mediumRNN_tracktwoMVA_03dRAB_L1TAU20IM_2TAU12IM_4J12p0ETA25"};
      tau35_match_paths = {
        "HLT_mu24_ivarmedium_tau35_mediumRNN_tracktwoMVA_probe_03dRAB_L1MU14FCH",
        "HLT_e26_lhtight_ivarloose_tau35_mediumRNN_tracktwoMVA_probe_03dRAB_L1EM22VHI"
      };
      tau25_match_paths = {
        "HLT_mu24_ivarmedium_tau25_mediumRNN_tracktwoMVA_probe_03dRAB_L1MU14FCH",
        "HLT_e26_lhtight_ivarloose_tau25_mediumRNN_tracktwoMVA_probe_L1TAU12IM_03dRAB_L1eEM26M"
      };

      if (runBoolDecos.at(MULTILEPTON::is23_first_2400bunches)(*eventInfo)){
        ditau_paths_L1Topo_delayed = {"HLT_tau30_mediumRNN_tracktwoMVA_tau20_mediumRNN_tracktwoMVA_03dRAB30_L1DR_TAU20ITAU12I_J25"};
        ditau_paths_4J12_delayed = {"HLT_tau30_mediumRNN_tracktwoMVA_tau20_mediumRNN_tracktwoMVA_03dRAB_L1TAU20IM_2TAU12IM_4J12p0ETA25"};
      }
       di_tau_SF_path = std::make_pair("tau35_mediumRNN_tracktwoMVA",
             "tau25_mediumRNN_tracktwoMVA");
    }
    else if(year==2024){
      if(runBoolDecos.at(MULTILEPTON::is24_timeframeA_C)(*eventInfo) || runBoolDecos.at(MULTILEPTON::is24_timeframeD)(*eventInfo)){
        ditau_path = {"HLT_tau35_mediumRNN_tracktwoMVA_tau25_mediumRNN_tracktwoMVA_03dRAB_L1TAU20IM_2TAU12IM_4J12p0ETA25"};
      }
      else if(runBoolDecos.at(MULTILEPTON::is24_timeframeE_F)(*eventInfo)){
        ditau_path = {"HLT_tau35_mediumRNN_tracktwoMVA_tau25_mediumRNN_tracktwoMVA_03dRAB_L1cTAU30M_2cTAU20M_4jJ30p0ETA25"};
      }
      else {
        ditau_path = {
          "HLT_tau25_mediumRNN_tracktwoMVA_tau20_mediumRNN_tracktwoMVA_03dRAB_j70_j50a_j0_DJMASS900j50_L1jMJJ_500_NFF",
          "HLT_tau35_mediumRNN_tracktwoMVA_tau25_mediumRNN_tracktwoMVA_03dRAB30_L1cTAU30M_2cTAU20M_DR_eTAU30eTAU20_jJ55",
          "HLT_tau40_mediumRNN_tracktwoMVA_tau35_mediumRNN_tracktwoMVA_03dRAB_L1cTAU35M_2cTAU30M_2jJ55_3jJ50",
          "HLT_tau80_mediumRNN_tracktwoMVA_tau35_mediumRNN_tracktwoMVA_03dRAB30_L1eTAU80_2cTAU30M_DR_eTAU30eTAU20",
          "HLT_tau80_mediumRNN_tracktwoMVA_tau60_mediumRNN_tracktwoMVA_03dRAB_L1eTAU80_2eTAU60",
        };
      }
      di_tau_SF_path = std::make_pair("tau35_mediumRNN_tracktwoMVA",
              "tau25_mediumRNN_tracktwoMVA");
    }
  }
}