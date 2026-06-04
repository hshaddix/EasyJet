from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator
from AthenaConfiguration.ComponentFactory import CompFactory
from itertools import chain

from EasyjetHub.algs.postprocessing.trigger_matching import TriggerMatchingToolCfg
from EasyjetHub.algs.postprocessing.SelectorAlgConfig import (
    ElectronSelectorAlgCfg, MuonSelectorAlgCfg, LeptonOrderingAlgCfg,
    JetSelectorAlgCfg, PhotonSelectorAlgCfg)

from EasyjetHub.output.ttree.selected_objects import (
    get_selected_obj_branches_variables)


def HZy_cfg(flags, smalljetkey, photonkey, electronkey, muonkey,
            float_variables=None, int_variables=None):
    keys = ["baseline", "photons", "leptons", "jets", "llcand"]
    if not float_variables:
        float_variables = {key: [] for key in keys}
    if not int_variables:
        int_variables = {key: [] for key in keys}

    cfg = ComponentAccumulator()

    cfg.merge(PhotonSelectorAlgCfg(flags,
                                   containerInKey=photonkey,
                                   containerOutKey="HZyAnalysisPhotons_%SYS%",
                                   minPt=flags.Analysis.Photon.min_pT,
                                   maxEta=flags.Analysis.Photon.max_eta))

    cfg.merge(MuonSelectorAlgCfg(flags,
                                 containerInKey=muonkey,
                                 containerOutKey="HZyAnalysisMuons_%SYS%",
                                 minPt=flags.Analysis.Muon.min_pT,
                                 maxEta=flags.Analysis.Muon.max_eta))

    cfg.merge(ElectronSelectorAlgCfg(flags,
                                     containerInKey=electronkey,
                                     containerOutKey="HZyAnalysisElectrons_%SYS%",
                                     minPt=flags.Analysis.Electron.min_pT,
                                     maxEta=flags.Analysis.Electron.max_eta))

    cfg.merge(LeptonOrderingAlgCfg(flags,
                                   containerInEleKey=electronkey,
                                   containerInMuKey=muonkey))

    cfg.merge(JetSelectorAlgCfg(flags,
                                containerInKey=smalljetkey,
                                containerOutKey="HZyAnalysisJets_%SYS%",
                                minPt=flags.Analysis.Small_R_jet.min_pT,
                                pTsort=True,
                                bTagWPDecorName="",
                                selectBjet=False))

    # selection algorithm added by "addEventAlgo"
    selection_name = flags.Analysis.selection_name

    cfg.addEventAlgo(
        CompFactory.HZY.HZySelectorAlg(
            "HZySelectorAlg",
            eventDecisionOutputDecoration=f"HZy_pass_{selection_name}_%SYS%",
            cutList=flags.Analysis.CutList,
            saveCutFlow=flags.Analysis.save_cutflow,
            isMC=flags.Input.isMC,
            triggerLists=flags.Analysis.TriggerChains,
            trigMatchingTool=cfg.popToolsAndMerge(TriggerMatchingToolCfg(flags)),
            bypass=flags.Analysis.bypass
        )
    )

    cfg.addEventAlgo(
        CompFactory.HZY.PhotonVarsHZyAlg(
            "PhotonVarsHZyAlg",
            isMC=flags.Input.isMC,
            floatVariableList=float_variables['photons'],
            intVariableList=int_variables['photons'],
            doSystematics=flags.Analysis.do_CP_systematics,
        )
    )

    cfg.addEventAlgo(
        CompFactory.HZY.LeptonVarsHZyAlg(
            "LeptonVarsHZyAlg",
            intVariableList=int_variables['leptons'],
            doSystematics=flags.Analysis.do_CP_systematics,
        )
    )

    cfg.addEventAlgo(
        CompFactory.HZY.JetVarsHZyAlg(
            "JetVarsHZyAlg",
            floatVariableList=float_variables['jets'],
            intVariableList=int_variables['jets'],
            doSystematics=flags.Analysis.do_CP_systematics,
        )
    )

    cfg.addEventAlgo(
        CompFactory.HZY.BaselineVarsHZyAlg(
            "BaselineVarsHZyAlg",
            floatVariableList=float_variables['baseline'],
            doSystematics=flags.Analysis.do_CP_systematics,
        )
    )

    return cfg


def get_PhotonVarsHZyAlg_variables(flags):
    float_variable_names = []
    int_variable_names = []

    float_variable_names += [
        "Photon_effSF", "Ph_Pt_Raw", "Ph_Eta_Raw", "Ph_Phi_Raw",
    ]
    int_variable_names += ["nPhotons", "Ph_ConvType", "Ph_isEMTight"]
    return float_variable_names, int_variable_names


def get_LeptonVarsHZyAlg_variables(flags):
    float_variable_names = []
    int_variable_names = []

    int_variable_names += ["nLeptons", "nElectrons", "nMuons"]
    return float_variable_names, int_variable_names


def get_JetVarsHZyAlg_variables(flags):
    float_variable_names = []
    int_variable_names = []

    float_variable_names += ["Mjj"]
    int_variable_names += ["nJets"]
    return float_variable_names, int_variable_names


def get_BaselineVarsHZyAlg_variables(flags):
    float_variable_names = []
    int_variable_names = []

    float_variable_names += ["BeamSpotWeight", "MC_Weight"]

    return float_variable_names, int_variable_names


def HZy_branches(flags):
    # a list of strings which maps the variable name in the c++ code
    # to the outputname:
    # "EventInfo.<var> -> <outputvarname>"
    branches = []

    # this will be all the variables that are calculated by the
    # BaselineVarsAlg algorithm
    all_baseline_variable_names = []
    keys = ["baseline", "photons", "leptons", "jets"]
    float_variable_names = {key: [] for key in keys}
    int_variable_names = {key: [] for key in keys}

    # these are the variables that will always be stored by easyjet specific to analysis
    # further below there are more high level variables
    # which can be stored using the flag
    # flags.Analysis.store_high_level_variables
    baseline_float_variables, baseline_int_variables \
        = get_BaselineVarsHZyAlg_variables(flags)
    photon_float_variables, photon_int_variables \
        = get_PhotonVarsHZyAlg_variables(flags)
    lepton_float_variables, lepton_int_variables \
        = get_LeptonVarsHZyAlg_variables(flags)
    jet_float_variables, jet_int_variables \
        = get_JetVarsHZyAlg_variables(flags)

    float_variable_names['baseline'] += baseline_float_variables
    int_variable_names['baseline'] += baseline_int_variables
    float_variable_names['photons'] += photon_float_variables
    int_variable_names['photons'] += photon_int_variables
    float_variable_names['leptons'] += lepton_float_variables
    int_variable_names['leptons'] += lepton_int_variables
    float_variable_names['jets'] += jet_float_variables
    int_variable_names['jets'] += jet_int_variables

    all_baseline_variable_names += [*chain.from_iterable(float_variable_names.values()),
                                    *chain.from_iterable(int_variable_names.values())]

    if not flags.Analysis.do_CP_systematics:
        sys_suffix = "NOSYS"
    else:
        sys_suffix = "%SYS%"

    for var in all_baseline_variable_names:
        branches += [f"EventInfo.{var}_{sys_suffix} -> HZy_{var}"
                     + flags.Analysis.systematics_suffix_separator + sys_suffix]

    # These are the variables always saved with the objects selected by the analysis
    # This is tunable with the flags amount and variables in the object configs.
    object_level_branches = {key: [] for key in keys}
    object_level_float_variables = {key: [] for key in keys}
    object_level_int_variables = {key: [] for key in keys}

    (object_level_branches['photons'],
     object_level_float_variables['photons'],
     object_level_int_variables['photons']) = \
        get_selected_obj_branches_variables(flags, "HZy", "Photon")

    float_variable_names['photons'] += object_level_float_variables['photons']
    int_variable_names['photons'] += object_level_int_variables["photons"]

    (object_level_branches['leptons'],
     object_level_float_variables['leptons'],
     object_level_int_variables['leptons']) = \
        get_selected_obj_branches_variables(flags, "HZy", "Lepton")

    float_variable_names['leptons'] += object_level_float_variables['leptons']
    int_variable_names['leptons'] += object_level_int_variables['leptons']

    (object_level_branches['jets'],
     object_level_float_variables['jets'],
     object_level_int_variables['jets']) = \
        get_selected_obj_branches_variables(flags, "HZy", "Jet")

    float_variable_names['jets'] += object_level_float_variables['jets']
    int_variable_names['jets'] += object_level_int_variables['jets']

    branches += object_level_branches["leptons"]
    branches += object_level_branches["photons"]
    branches += object_level_branches["jets"]

    s_name = flags.Analysis.selection_name
    branches += [f"EventInfo.HZy_pass_{s_name}_{sys_suffix} -> HZy_pass_{s_name}"
                 + flags.Analysis.systematics_suffix_separator + sys_suffix]

    # ---------------------------------------------------------
    # Save per-event cut flags from HZySelectorAlg (EventInfo decors)
    # ---------------------------------------------------------
    if (flags.Analysis.save_cutflow):
        cutList = flags.Analysis.CutList
        for cut in cutList:
            branches += [f"EventInfo.{cut}_{sys_suffix} -> CUT_{cut}"
                         + flags.Analysis.systematics_suffix_separator + sys_suffix]

    return branches, float_variable_names, int_variable_names
