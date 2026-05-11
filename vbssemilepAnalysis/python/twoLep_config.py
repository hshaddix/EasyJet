# Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator
from AthenaConfiguration.ComponentFactory import CompFactory
from AthenaConfiguration.Enums import LHCPeriod

from EasyjetHub.output.ttree.selected_objects import (
    get_selected_objects_branches_variables,
)

from OutputStreamAthenaPool.OutputStreamConfig import OutputStreamCfg
from OutputStreamAthenaPool.OutputStreamConfig import addToESD, addToAOD
from AthenaCommon.Logging import logging
from AthenaCommon.Constants import DEBUG

# Set the global log level to DEBUG
logging.getLogger().setLevel(DEBUG)

def twoLep_cfg(flags, muonkey, electronkey, float_variables=None, int_variables=None):
    if not float_variables:
        float_variables = []
    if not int_variables:
        int_variables = []

    cfg = ComponentAccumulator()
    
    if flags.Analysis.do_resolved:
        # set jets container labels
        if not flags.Analysis.UseVBFRNN:
            # use vbs tagging jets
            VBSJetsLabel = "vbssemilepAnalysisVBSJets_%SYS%"
            # use the small-R jets after selecting tagging jets
            SignalJetsLabel = "vbssemilepAnalysisSignalJets_%SYS%"
        else:
            # use full small-R jets pool to select signal jets 
            VBSJetsLabel = "vbssemilepAnalysisJets_%SYS%"
            SignalJetsLabel = "vbssemilepAnalysisJets_%SYS%"
    
    cfg.addEventAlgo(
        CompFactory.VBSSEMILEP.twoLepSelectorAlg(
            "twoLepSelectorAlg",
            signaljets=SignalJetsLabel,
            UseVBFRNN=flags.Analysis.UseVBFRNN,
            bTagWPDecorName="ftag_select_" + flags.Analysis.Small_R_jet.btag_wp,
            eventDecisionOutputDecoration="vbssemilep_pass_sr_%SYS%",
            cutList=flags.Analysis.CutList,
            saveCutFlow=flags.Analysis.save_cutflow,
            bypass=(flags.Analysis.bypass if hasattr(flags.Analysis, 'bypass')
                    else False),
            do_resolved=True
            #do_resolved=flags.Analysis.do_resolved
        )
    )

    # calculate final vbssemilep vars
    MuonWPLabel = f'{flags.Analysis.Muon.ID}_{flags.Analysis.Muon.Iso}'
    ElectronWPLabel = f'{flags.Analysis.Electron.ID}_{flags.Analysis.Electron.Iso}'
    cfg.addEventAlgo(
        CompFactory.VBSSEMILEP.BaselineVarstwoLepAlg(
            "BaselineVarstwoLepAlg",
            signaljets=SignalJetsLabel,
            UseVBFRNN=flags.Analysis.UseVBFRNN,
            vbsJets=VBSJetsLabel,
            isMC=flags.Input.isMC,
            muons=muonkey, muonWP=MuonWPLabel,
            electrons=electronkey, eleWP=ElectronWPLabel,
            bTagWPDecorName="ftag_select_" + flags.Analysis.Small_R_jet.btag_wp,
            PCBTDecorName="ftag_quantile_"
                          + flags.Analysis.Small_R_jet.btag_extra_wps[0],
            wtag_type=flags.Analysis.Large_R_jet.wtag_type,
            wtag_wp=flags.Analysis.Large_R_jet.wtag_wp,
            floatVariableList=float_variables,
            intVariableList=int_variables,
            do_resolved=True
            #do_resolved=flags.Analysis.do_resolved
        )
    )
    return cfg


def get_BaselineVarstwoLepAlg_variables(flags):
    float_variable_names = []
    int_variable_names = []

    wtag_type = flags.Analysis.Large_R_jet.wtag_type
    wtag_wp = flags.Analysis.Large_R_jet.wtag_wp
    for object in ["LargeJet1"]:
        for var in ["m", "pt", "eta", "phi", "GN3X", "E", "y", "px", "py", "pz"]:
            float_variable_names.append(f"{object}_{var}")
        for var in ["pqcdcx", "pWqq", "pqcdbx", "ptop"]:
            float_variable_names.append(f"{object}_GN3X_{var}")

        float_variable_names += [object + "_" + wtag_type + wtag_wp + "Tagger_Score"]
        int_variable_names += [object + "_Pass_" + wtag_type
                                   + wtag_wp]

    for object in ["Lepton1", "Lepton2"]:
        for var in ["m", "pt", "eta", "phi", "E", "px", "py", "pz", "y", "effSF"]:
            float_variable_names.append(f"{object}_{var}")
        for var in ["ele_ECIDS", "charge", "pdgid", "truthOrigin", "truthType", "isPrompt", "ele_firstEgMotherTruthType", "ele_firstEgMotherPdgId","ele_firstEgMotherTruthOrigin"]:
            int_variable_names.append(f"{object}_{var}")
    
    objects = ["ll", "VJVll"]
    if flags.Analysis.do_resolved:
        objects += ["Sdijet", "VjjVll"]
    if not flags.Analysis.UseVBFRNN:
        objects += ["VBSJ1", "VBSJ2", "VBSdijet"]
    else:
        objects += ["RNNJets_boostedCategory_Jet1",
                    "RNNJets_boostedCategory_Jet2"]
        if flags.Analysis.do_resolved:
            objects += ["RNNJets_resolvedCategory_Jet1",
                        "RNNJets_resolvedCategory_Jet2"]

    for object in objects:
        for var in ["m", "pt", "eta", "phi", "px", "py", "pz", "E", "y"]:
            float_variable_names.append(f"{object}_{var}")

    objects = ["ll", "VJVll"]
    if flags.Analysis.do_resolved:
        objects += ["Sjj", "VjjVll"]
    if not flags.Analysis.UseVBFRNN:
        objects += ["VBSjj"]
    else:
        objects += ["RNNJets_boostedCategory"]
        if flags.Analysis.do_resolved:
            objects += ["RNNJets_resolvedCategory"]
    for object in objects:
        for var in ["dR", "dEta", "dPhi"]:
            float_variable_names.append(f"{var}{object}")

    if flags.Analysis.UseVBFRNN:
        objects = ["RNNJets_boostedCategory"]
        if flags.Analysis.do_resolved:
            objects +=  ["RNNJets_resolvedCategory"]
        for object in objects:
            for var in ["dijet_m", "dijet_pt", "dijet_eta", "dijet_phi", "dijet_px", "dijet_py", "dijet_pz", "dijet_E", "dijet_y"]:
                float_variable_names.append(f"{object}_{var}")
    
    if not flags.Analysis.UseVBFRNN:
        float_variable_names += ["ZeppCent_Vll", "ZeppCent_VJ_merg"]
        if flags.Analysis.do_resolved:
            float_variable_names += ["ZeppCent_Vjj_res"]
    else:
        float_variable_names += ["ZeppCent_Vll_RNN_boosted", "ZeppCent_VJ_merg_RNN"]
        if flags.Analysis.do_resolved:
            float_variable_names += ["ZeppCent_Vll_RNN_resolved", "ZeppCent_Vjj_res_RNN"]
    
    if flags.Analysis.do_resolved:
        for object in ["SignalJet1", "SignalJet2"]:
            for var in ["m", "pt", "eta", "phi", "E", "px", "py", "pz", "y"]:
                float_variable_names.append(f"{object}_{var}")
            for var in ["pcbt", "truthLabel", "isBtagged"]:
                int_variable_names.append(f"{object}_{var}")

    int_variable_names += ["nLargeJets", "nLeptons",
                           "nCentralJets", "nForwardJets", "nElectrons", "nMuons"]

    float_variable_names += ["dR_sjet_lepton_min"]
    int_variable_names += ["nJets", "nBJets"]

    return float_variable_names, int_variable_names


def twoLep_branches(flags):
    branches = []

    # this will be all the variables that are calculated by the
    # BaselineVarstwoLepAlg algorithm
    all_baseline_variable_names = []
    float_variable_names = []
    int_variable_names = []

    # further below there are more high level variables which can be
    # stored using the flag
    # flags.Analysis.store_high_level_variables
    baseline_float_variables, baseline_int_variables \
        = get_BaselineVarstwoLepAlg_variables(flags)
    float_variable_names += baseline_float_variables
    int_variable_names += baseline_int_variables

    all_baseline_variable_names += [*float_variable_names, *int_variable_names]

    for var in all_baseline_variable_names:
        branches += [f"EventInfo.{var}_%SYS% -> {var}_%SYS%"]

    # These are the variables always saved with the objects selected by the analysis
    # This is tunable with the flags amount and variables
    # in the object configs.
    object_level_branches, object_level_float_variables, \
        object_level_int_variables \
        = get_selected_objects_branches_variables(flags, "twoLep")
    #float_variable_names += object_level_float_variables
    #int_variable_names += object_level_int_variables

    branches += object_level_branches
    branches += ["EventInfo.vbssemilep_pass_sr_%SYS% -> pass_SR_%SYS%"]

    cutList = flags.Analysis.CutList
    for cut in cutList:
        branches += [f"EventInfo.{cut}_%SYS% -> {cut}_%SYS%"]

    # truth info
    if flags.Input.isMC and flags.Analysis.AddTruthVBSQuarks:
        branches += ['EventInfo.nVBSQuarks -> nVBSQuarks']
        for var in ['pT', 'eta', 'phi', 'E', 'y', 'px', 'py', 'pz', 'm']:
            branches += [f'EventInfo.VBSQuark1_{var} -> VBSQuark1_{var}']
            branches += [f'EventInfo.VBSQuark2_{var} -> VBSQuark2_{var}']

    # VBF tagger
    if flags.Analysis.UseVBFRNN:
        vars = ['RNNScore', 'nRNNJets']
        regs = ['boostedCategory']
        if flags.Analysis.do_resolved:
            regs += ['resolvedCategory']
        for var in vars:
            for reg in regs:
                branches += [f'EventInfo.{var}_{reg}_%SYS% -> {var}_{reg}_%SYS%']

    for cat in ["SJT", "SLT"]:
        branches += \
            [f"EventInfo.pass_trigger_{cat}_%SYS% -> pass_trigger_{cat}"
                + flags.Analysis.systematics_suffix_separator + "%SYS%"]

    
    if flags.Analysis.save_high_level_variables:
        for cat in ["ele0_passSET", "ele1_passSET", "mu0_passSMT",
                    "mu1_passSMT", "ele0_trigPassed", "ele1_trigPassed",
                    "mu0_trigPassed", "mu1_trigPassed",
                    "ele0_trigMatched", "ele1_trigMatched",
                    "mu0_trigMatched", "mu1_trigMatched"]:
            branches += [f"EventInfo.{cat}_%SYS% -> {cat}_%SYS%"]

    #branches = [branch for branch in branches if "NOSYS" not in branch]
    return branches, float_variable_names, int_variable_names
