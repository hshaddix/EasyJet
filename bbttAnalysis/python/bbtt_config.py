# Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration

from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator
from AthenaConfiguration.ComponentFactory import CompFactory
import AthenaCommon.SystemOfUnits as Units
from AthenaConfiguration.Enums import LHCPeriod

from EasyjetHub.algs.postprocessing.SelectorAlgConfig import (
    MuonSelectorAlgCfg, ElectronSelectorAlgCfg, LeptonOrderingAlgCfg,
    TauSelectorAlgCfg, JetSelectorAlgCfg)
from EasyjetHub.output.ttree.selected_objects import (
    get_selected_objects_branches_variables,
)
from EasyjetHub.steering.analysis_configuration import (
    get_trigger_legs_scale_factor_list, get_trigger_chains_scale_factor)


def bbtt_ObjectSelectorAlgCfg(flags, smalljetkey, muonkey, electronkey, taukey):
    cfg = ComponentAccumulator()

    # muons:
    cfg.merge(MuonSelectorAlgCfg(flags,
                                 containerInKey=muonkey,
                                 containerOutKey="bbttAnalysisMuons_%SYS%"))

    # electrons:
    cfg.merge(ElectronSelectorAlgCfg(flags,
                                     containerInKey=electronkey,
                                     containerOutKey="bbttAnalysisElectrons_%SYS%"))
    # leptons:
    cfg.merge(LeptonOrderingAlgCfg(flags,
                                   containerInEleKey=electronkey,
                                   containerInMuKey=muonkey))

    cfg.merge(TauSelectorAlgCfg(flags,
                                # Baseline always needed for anti-taus
                                containerInKey=taukey,
                                containerOutKey="bbttAnalysisTaus_%SYS%"))

    cfg.merge(JetSelectorAlgCfg(
        flags,
        containerInKey=smalljetkey,
        containerRawInKey=smalljetkey.replace('_thin_%SYS%', '_%SYS%'),
        containerOutKey="bbttAnalysisJets_%SYS%",
        minPt=20 * Units.GeV,
        bTagWPDecorName=(
            "baselineSelection_"
            + flags.Analysis.Small_R_jet.btag_wp
            + "_%SYS%"
        ),
        selectBjet=False,
        minimumAmount=2))

    if flags.Analysis.do_large_R_UFO_jets:
        cfg.merge(
            JetSelectorAlgCfg(
                flags,
                name="LargeRJetSelectorAlg",
                containerInKey=flags.Analysis.container_names.output.reco10UFOJet,
                containerOutKey="bbttAnalysisLargeRJets_%SYS%",
                bTagWPDecorName="",
                selectBjet=False,
                minPt=200.0 * Units.GeV,
                maxPt=3000.0 * Units.GeV,
                maxMass=600.0 * Units.GeV,
                maxEta=2.0,
                jetAmount=flags.Analysis.Large_R_jet.amount_leadingjet,
            )
        )

    return cfg


def bbtt_BJetTriggerDecoratorAlgCfg(flags):
    cfg = ComponentAccumulator()

    # b-jet matching decoration
    bjet_triggers = []
    sf_config = flags.Analysis.Trigger.scale_factor
    if sf_config.doSF and hasattr(sf_config, 'bjet'):
        triggerChainsPerYear = get_trigger_chains_scale_factor(
            flags, 'bjet')
        for triggerChains in triggerChainsPerYear.values():
            for chain in triggerChains:
                bjet_triggers.append(chain)

    cfg.addEventAlgo(
        CompFactory.HHBBTT.BJetTriggerDecoratorAlg(
            "BJetTriggerDecoratorAlg",
            triggerLists=bjet_triggers,
        )
    )

    return cfg


def bbtt_SelectorAlgCfg(flags):
    cfg = ComponentAccumulator()

    # anti-iso lepton control region is not compatible with the other
    # regions at the moment:
    use_noniso_leptons = "AntiIsoLepHad" in flags.Analysis.channels
    if (use_noniso_leptons and len(flags.Analysis.channels) > 1):
        raise ValueError("Cannot run 'antiiso-lephad' with any other channels")

    muon_WPs = [f'{wp[0]}_{wp[1]}' for wp in flags.Analysis.Muon.extra_wps]
    ele_WPs = [f'{wp[0]}_{wp[1]}' for wp in flags.Analysis.Electron.extra_wps]
    if flags.Analysis.do_large_R_UFO_jets:
        fms = flags.Analysis.Large_R_jet.flat_mass_selections
        GN3XPV01_hbb_bjr_WP = f"xbb_select_GN3XPV01_hbb_bjr_{fms.GN3XPV01_hbb_bjr.wps[-1]}"
        GN3XPV01_htt_WP = f"xbb_select_GN3XPV01_htt_{fms.GN3XPV01_htt.wps[-1]}"
    else:
        GN3XPV01_hbb_bjr_WP = GN3XPV01_htt_WP = ""
    cfg.addEventAlgo(
        CompFactory.HHBBTT.HHbbttSelectorAlg(
            "HHbbttSelectorAlg",
            bTagWPDecorName="ftag_select_" + flags.Analysis.Small_R_jet.btag_wp,
            tauWP=flags.Analysis.Tau.extra_wps[0],
            muonWPs=muon_WPs,
            eleWPs=ele_WPs,
            useNonIsoLeptons=use_noniso_leptons,
            eventDecisionOutputDecoration=(
                "bbtt_pass_presel_noMMC_%SYS%" if flags.Analysis.enable_MMC_cut
                else "bbtt_pass_presel_%SYS%"),
            channel=flags.Analysis.channels,
            isMC=flags.Input.isMC,
            doAntiIDRegions=flags.Analysis.do_antiID_regions,
            do1BRegions=flags.Analysis.do_1B_regions,
            doLTT=flags.Analysis.do_LTT,
            useTriggerSelections=flags.Analysis.do_trigger_offline_filtering,
            bypass=flags.Analysis.bypass,
            saveCutFlow=flags.Analysis.save_cutflow,
            cutList=(
                flags.Analysis.CutList if hasattr(flags.Analysis, "CutList") else []),
            RootStreamName=('CBK' if flags.Analysis.splitCBK else
                            flags.Analysis.ttree_output.stream_name),
            doLargeRJets=flags.Analysis.do_large_R_UFO_jets,
            ditaus=flags.Analysis.container_names.output.ditaus,
            GN3XPV01_hbb_bjr_WP=GN3XPV01_hbb_bjr_WP,
            GN3XPV01_htt_WP=GN3XPV01_htt_WP
        )
    )

    return cfg


def bbtt_MMC_Cfg(flags):
    cfg = ComponentAccumulator()

    from EasyjetHub.algs.mmc_tool_config import MissingMassToolCfg
    mmcTool = cfg.popToolsAndMerge(
        MissingMassToolCfg(
            flags, CalibSet="2024",
            ParamFilePath="MMC_params_v051224_angle_noLikelihoodFit.root"))

    cfg.addEventAlgo(
        CompFactory.HHBBTT.MMCDecoratorAlg(
            "MMCDecoratorAlg",
            channel=flags.Analysis.channels,
            bTagWPDecorName="ftag_select_" + flags.Analysis.Small_R_jet.btag_wp,
            mmcTool=mmcTool
        )
    )

    if flags.Analysis.enable_MMC_cut:
        cfg.addEventAlgo(
            CompFactory.HHBBTT.MMCSelectorAlg(
                "MMCSelectorAlg",
                channel=flags.Analysis.channels,
                MMC_min=60 * Units.GeV,
                eventDecisionOutputDecoration="bbtt_pass_presel_%SYS%",
                bypass=flags.Analysis.bypass,
            )
        )

    return cfg


def bbtt_BaselineVarsAlgCfg(flags, muonkey, electronkey, taukey,
                            float_variables=None, int_variables=None):
    cfg = ComponentAccumulator()

    if flags.Analysis.do_boosted_dihiggs:
        fms = flags.Analysis.Large_R_jet.flat_mass_selections
        GN3XPV01_hbb_bjr_WP = f"xbb_select_GN3XPV01_hbb_bjr_{fms.GN3XPV01_hbb_bjr.wps[-1]}"
        GN3XPV01_htt_WP = f"xbb_select_GN3XPV01_htt_{fms.GN3XPV01_htt.wps[-1]}"
        cfg.addEventAlgo(
            CompFactory.HHBBTT.BaselineVarsBoostedbbttAlg(
                "BaselineVarsBoostedbbttAlg",
                isMC=flags.Input.isMC,
                floatVariableList=float_variables,
                GN3XPV01_hbb_bjr_WP=GN3XPV01_hbb_bjr_WP,
                GN3XPV01_htt_WP=GN3XPV01_htt_WP
            )
        )

    else:
        use_noniso_leptons = "AntiIsoLepHad" in flags.Analysis.channels
        muon_WPs = [f'{wp[0]}_{wp[1]}' for wp in flags.Analysis.Muon.extra_wps]
        ele_WPs = [f'{wp[0]}_{wp[1]}' for wp in flags.Analysis.Electron.extra_wps]
        btag_pcbt_wps \
            = [wp for wp in flags.Analysis.Small_R_jet.btag_extra_wps if "Continuous" in wp] # noqa

        cfg.addEventAlgo(
            CompFactory.HHBBTT.BaselineVarsbbttAlg(
                "FinalVarsbbttAlg",
                isMC=flags.Input.isMC,
                useNonIsoLeptons=use_noniso_leptons,
                electrons=electronkey, eleWPs=ele_WPs,
                muons=muonkey, muonWPs=muon_WPs,
                taus=taukey, tauWP=flags.Analysis.Tau.extra_wps[0],
                doMMC=flags.Analysis.do_mmc,
                bTagWPDecorName="ftag_select_" + flags.Analysis.Small_R_jet.btag_wp,
                PCBTDecorList=["ftag_quantile_" + pcbt_wp for pcbt_wp in btag_pcbt_wps], # noqa
                floatVariableList=float_variables,
                intVariableList=int_variables
            )
        )

    return cfg


def bbtt_TriggerSFAlgCfg(flags, muonkey, electronkey, taukey):
    cfg = ComponentAccumulator()

    cfg.addEventAlgo(
        CompFactory.HHBBTT.TriggerSFAlg(
            "TriggerSFAlg",
            eleTriggerSF=get_trigger_legs_scale_factor_list(flags, 'Electron'),
            muonTriggerSF=get_trigger_legs_scale_factor_list(flags, 'Muon'),
            tauTriggerSF=get_trigger_legs_scale_factor_list(flags, 'Tau'),
            electrons=electronkey,
            muons=muonkey,
            taus=taukey,
            doLTT=flags.Analysis.do_LTT
        )
    )

    return cfg


def bbtt_cfg(flags, smalljetkey, muonkey, electronkey, taukey,
             float_variables=None, int_variables=None):
    if not float_variables:
        float_variables = []
    if not int_variables:
        int_variables = []

    if flags.Analysis.Small_R_jet.save_all_jets:
        smalljetkey = smalljetkey.replace('_%SYS%', '_thin_%SYS%')

    cfg = ComponentAccumulator()

    cfg.merge(bbtt_ObjectSelectorAlgCfg(
        flags, smalljetkey, muonkey, electronkey, taukey))

    cfg.merge(bbtt_BJetTriggerDecoratorAlgCfg(flags))

    cfg.merge(bbtt_SelectorAlgCfg(flags))

    # MMC decoration
    if flags.Analysis.do_mmc:
        cfg.merge(bbtt_MMC_Cfg(flags))

    # calculate final bbtt vars
    if flags.Analysis.store_high_level_variables:
        cfg.merge(bbtt_BaselineVarsAlgCfg(flags, muonkey, electronkey, taukey,
                                          float_variables, int_variables))

    # calculate event trigger SF
    # TODO: __BOOSTED__ for the moment we don't have SF for boosted Jets
    if flags.Input.isMC and not flags.Analysis.do_boosted_dihiggs:
        cfg.merge(bbtt_TriggerSFAlgCfg(flags, muonkey, electronkey, taukey))

    return cfg


def get_BaselineVarsbbttAlg_variables(flags):
    float_variable_names = []
    int_variable_names = []

    combined_particles = [
        "H_bb",
        "H_vis_tautau",
        "HH_vis",
    ]

    if flags.Analysis.do_mmc:
        combined_particles += ["HH"]

    for particle in combined_particles:
        for var in ["pt", "eta", "phi", "m"]:
            float_variable_names.append(f"{particle}_{var}")
    return float_variable_names, int_variable_names


def get_BaselineVarsbbttAlg_highlevelvariables(flags):
    high_level_float_variables = []
    high_level_int_variables = []

    high_level_float_variables += [
        "HH_vis_delta_phi",
    ]

    if flags.Analysis.do_mmc:
        high_level_float_variables += [
            "HH_delta_phi",
        ]

    return high_level_float_variables, high_level_int_variables


def bbtt_branches(flags):
    # a list of strings which maps the variable name in the c++ code
    # to the outputname:
    # "EventInfo.<var> -> <outputvarname> type=..."
    branches = []
    float_var_names = []
    int_var_names = []

    # this will be all the variables that are calculated by the
    # BaselineVarsbbttAlg algorithm
    float_baseline_var_names = []
    int_baseline_var_names = []

    # these are the variables that will always be stored by easyjet specific to HHbbtt
    # further below there are more high level variables which can be
    # stored using the flag
    if flags.Analysis.store_high_level_variables:
        baseline_float_variables, baseline_int_variables \
            = get_BaselineVarsbbttAlg_variables(flags)
        float_baseline_var_names += baseline_float_variables
        float_var_names += baseline_float_variables
        int_baseline_var_names += baseline_int_variables
        int_var_names += baseline_int_variables

    if flags.Input.isMC:
        float_var_names.append("eventTriggerSF")

    if flags.Analysis.do_mmc:
        # do not append mmc variables to float_baseline_var_names
        # or int_baseline_var_names as they are stored by the
        # mmc algorithm not BaselineVarsbbttAlg
        int_var_names += ["mmc_status"]
        for part in ["", "nu1_", "nu2_"]:
            for var in ["pt", "eta", "phi", "m"]:
                float_var_names += [f"mmc_{part}{var}"]

    if flags.Analysis.store_high_level_variables:
        high_level_float_variables, high_level_int_variables \
            = get_BaselineVarsbbttAlg_highlevelvariables(flags)
        float_baseline_var_names += high_level_float_variables
        float_var_names += high_level_float_variables
        int_baseline_var_names += high_level_int_variables
        int_var_names += high_level_int_variables

    for var in float_var_names:
        branches += [f"EventInfo.{var}_%SYS% -> bbtt_{var}"
                     + flags.Analysis.systematics_suffix_separator + "%SYS%"
                     + " type=float"]
    for var in int_var_names:
        branches += [f"EventInfo.{var}_%SYS% -> bbtt_{var}"
                     + flags.Analysis.systematics_suffix_separator + "%SYS%"
                     + " type=int"]

    # These are the variables always saved with the objects selected by the analysis
    # This is tunable with the flags amount and variables
    # in the object configs.
    if flags.Analysis.store_high_level_variables:
        obj_branches, obj_float_variables, obj_int_variables = \
            get_selected_objects_branches_variables(flags, "bbtt")
        float_baseline_var_names += obj_float_variables
        int_baseline_var_names += obj_int_variables

        branches += obj_branches
        branches += ["EventInfo.bbtt_pass_presel_%SYS% -> bbtt_pass_presel"
                     + flags.Analysis.systematics_suffix_separator + "%SYS%"
                     + " type=char"]

    # special jet branches only for bbtautau
    smalljetkey = flags.Analysis.container_names.output.reco4PFlowJet
    small_R_prefix = (
        "recojet_antikt4"
        + flags.Analysis.Small_R_jet.jet_type.replace("reco4", "").replace("Jet", "")  # noqa: E501
    )
    syst_suff = flags.Analysis.systematics_suffix_separator + "%SYS%"
    branches += [
        (f"{smalljetkey}.trigMatch_L1_online{var}"
         + f"->{small_R_prefix}_trigMatch_L1_online{var}" + syst_suff
         + " type=float")
        for var in ["pt", "eta"]
    ]
    if flags.GeoModel.Run is LHCPeriod.Run3:
        branches += [
            f"{smalljetkey}.trigMatch_HLT_onlinept"
            + f"->{small_R_prefix}_trigMatch_HLT_onlinept" + syst_suff
            + " type=float"
        ]

    categories = ["SR", "SLT", "STT", "DTT",
                  "DTT_2016", "DTT_4J12", "DTT_L1Topo",
                  "DTT_4J12_delayed", "DTT_L1Topo_delayed", "DBT"]
    if flags.Analysis.do_LTT:
        categories += ["LTT"]

    # trigger variables do not need to be added to variable_names
    # as it is written out in HHbbttSelectorAlg
    if flags.Analysis.store_high_level_variables:
        for var in ["_trigger_", "_baseline_"]:
            for cat in categories:
                if (var == "_baseline_"
                        and cat in ["DTT_4J12_delayed", "DTT_L1Topo_delayed"]):
                    continue
                branches += [f"EventInfo.pass{var}{cat}_%SYS% -> "
                             f"bbtt_pass{var}{cat}"
                             + flags.Analysis.systematics_suffix_separator + "%SYS%"]

    branches += ["EventInfo.pass_trigger_uniqueRNNTauTrig18_%SYS% -> "
                 "bbtt_pass_trigger_uniqueRNNTauTrig18"
                 + flags.Analysis.systematics_suffix_separator + "%SYS%"]

    cat_b = ["1B", "2B"] if flags.Analysis.do_1B_regions else ["2B"]

    for cat in categories + ["LepHad", "HadHad"]:
        for nb in cat_b:
            branches += [f"EventInfo.pass_{cat}_{nb}_%SYS% ->"
                         f"bbtt_pass_{cat}_{nb}"
                         + flags.Analysis.systematics_suffix_separator + "%SYS%"]

    if flags.Analysis.store_high_level_variables:
        for cat in ["baseline_LepHad", "baseline_HadHad"]:
            branches += [f"EventInfo.pass_{cat}_%SYS% -> bbtt_pass_{cat}"
                         + flags.Analysis.systematics_suffix_separator + "%SYS%"]

    for cat in ["LepHad", "HadHad",
                "ZCR", "TopEMuCR", "AntiIsoLepHad"]:
        branches += [f"EventInfo.pass_{cat}_%SYS% -> bbtt_pass_{cat}"
                     + flags.Analysis.systematics_suffix_separator + "%SYS%"]

    if "Boosted" in flags.Analysis.channels:
        for pass_selection in ['Boosted', 'Boosted_ditau', 'Boosted_taujet',
                               'trigger_SINGLE_LARGE_R_JETS', 'trigger_DTT',
                               'trigger_DI_LARGE_R_JETS',
                               'trigger_STT', 'trigger_DBT',
                               'trigger_MET']:
            branches += [
                f"EventInfo.pass_{pass_selection}_%SYS% -> bbtt_pass_{pass_selection}"
                + flags.Analysis.systematics_suffix_separator + "%SYS%"
            ]

    return branches, float_baseline_var_names, int_baseline_var_names
