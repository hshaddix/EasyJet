from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator
from AthenaConfiguration.ComponentFactory import CompFactory

from bbyyAnalysis.bbyy_base_config import bbyy_branches
from bbyyAnalysis.bbyy_base_config import bbyy_cfg

from itertools import chain


def bbyy_HH_H_cfg(flags, smalljetkey, photonkey, muonkey, electronkey,
                  largeRjetkey, float_variables=None, int_variables=None):
    # Call base bbyy config
    cfg = ComponentAccumulator()
    cfg.merge(
        bbyy_cfg(flags, smalljetkey, photonkey, muonkey, electronkey,
                 largeRjetkey, float_variables, int_variables)
    )

    if flags.Input.isMC:
        cfg.addEventAlgo(
            CompFactory.Easyjet.JetNeutrinoDecoratorAlg(
                "JetNeutrinoDecoratorAlg",
                jetsIn="AntiKt4TruthDressedWZJets",
                truthParticlesIn="TruthNeutrinos"
            )
        )

    cfg.addEventAlgo(
        CompFactory.HHBBYY.TopRecoAlg(
            "TopRecoAlg",
            bTagWPDecorName="ftag_select_" + flags.Analysis.Small_R_jet.btag_wp,
            PCBTDecorName="ftag_quantile_" + flags.Analysis.Small_R_jet.btag_extra_wps[0],  # noqa
            floatVariableList=float_variables["top"],
            intVariableList=int_variables["top"],
            BDT_path=flags.Analysis.TopReconstruction.BDT_path,
            doSystematics=flags.Analysis.do_CP_systematics,
            jets="HyyAnalysisJets_%SYS%"
        )
    )

    if flags.Analysis.do_Hyy_multiclass:
        floatVariableList_HHyyMVAInput = (
            float_variables["top"]
            + float_variables["jets"]
            + float_variables["photonjets"]
            + float_variables['photons']
            + float_variables['leptons']
        )

        intVariableList_HHyyMVAInput = (
            int_variables["top"]
            + int_variables["jets"]
            + int_variables["photonjets"]
            + int_variables['photons']
            + int_variables['leptons']
        )

        (MulticlassBDT_model_name,
         MulticlassBDT_variables,
         MulticlassBDT_bins,
         BinaryBDT_boundaries,
         BinaryBDT_weights,
         BinaryBDT_variables) = get_HyyHHbbyyMVAAlgConfig(flags)

        cfg.addEventAlgo(
            CompFactory.HHBBYY.HyyHHbbyyMVAAlg(
                "HyyHHbbyyMVAAlg",
                doSystematics=flags.Analysis.do_CP_systematics,
                floatVariableList_BDTInput=floatVariableList_HHyyMVAInput,
                intVariableList_BDTInput=intVariableList_HHyyMVAInput,
                floatVariableList=float_variables['HyyMVA'],
                intVariableList=int_variables['HyyMVA'],
                doHyy_multiclass=flags.Analysis.do_Hyy_multiclass,
                HyyMultiBDT_folder=flags.Analysis.HyyMultiBDT_folder,
                MulticlassBDT_model_name=MulticlassBDT_model_name,
                MulticlassBDT_variables=MulticlassBDT_variables,
                MulticlassBDT_bins=MulticlassBDT_bins,
                BinaryBDT_boundaries=BinaryBDT_boundaries,
                BinaryBDT_weights=BinaryBDT_weights,
                BinaryBDT_variables=BinaryBDT_variables
            )
        )

    return cfg


def get_TopRecoAlg_variables(flags):
    float_variable_names = []
    int_variable_names = []
    # Top reconstruction variables
    float_variable_names += ["score_recotop1", "recotop1_pT", "recotop1_eta",
                             "recotop1_phi", "recotop1_m", "score_recotop2",
                             "recotop2_pT", "recotop2_eta", "recotop2_phi",
                             "recotop2_m", "hybrtop_pT", "hybrtop_eta",
                             "hybrtop_phi", "hybrtop_m", "dR_Wb_t2"]

    int_variable_names += ["is_top1_had", "is_top1_lep", "has_top2", "is_hybridtop2",]

    return float_variable_names, int_variable_names


def get_BaselineVarsHyyHHbbyyMVAAlg(flags):
    float_variable_names = []
    int_variable_names = []

    float_variable_names += ["HyyMultiBDT_binary_score"]
    int_variable_names += ["HyyMultiBDT_multiclass_index", "HyyMultiBDT_binary_index"]
    if (flags.Analysis.do_HHbbyy_Hyy_Analysis):
        float_variable_names += list(flags.Analysis.HyyMultiBDT_config.BinaryBDT.keys())

    return float_variable_names, int_variable_names


def get_HyyHHbbyyMVAAlgConfig(flags):
    def _aslist_str(_list):
        return [str(n) for n in _list]

    HyyMultiBDT_config = flags.Analysis.HyyMultiBDT_config

    MulticlassBDT_model_name = HyyMultiBDT_config.MulticlassBDT.model_name
    MulticlassBDT_variables = HyyMultiBDT_config.MulticlassBDT.variables
    MulticlassBDT_bins = {
        HyyMultiBDT_config.BinaryBDT[bin_name]['index']: bin_name
        for bin_name in HyyMultiBDT_config.BinaryBDT.keys()
    }
    BinaryBDT_weights = {
        bin_name: str(HyyMultiBDT_config.BinaryBDT[bin_name]['weight'])
        for bin_name in MulticlassBDT_bins.values()
    }
    BinaryBDT_boundaries = {
        bin_name: _aslist_str(HyyMultiBDT_config.BinaryBDT[bin_name]['boundaries'])
        for bin_name in MulticlassBDT_bins.values()
    }
    BinaryBDT_variables = {
        bin_name: _aslist_str(HyyMultiBDT_config.BinaryBDT[bin_name]['variables'])
        for bin_name in MulticlassBDT_bins.values()
    }
    return (MulticlassBDT_model_name, MulticlassBDT_variables, MulticlassBDT_bins,
            BinaryBDT_boundaries, BinaryBDT_weights, BinaryBDT_variables)


def bbyy_HH_H_branches(flags):
    # Get base bbyy branches and variables
    branches, float_variable_names, int_variable_names = bbyy_branches(flags)

    HH_H_variable_names = []
    keys = ["top",
            "HyyMVA"]

    HH_H_float_variable_names = {key: [] for key in keys}
    HH_H_int_variable_names = {key: [] for key in keys}

    # these are the variables that will always be stored by easyjet specific to HHbbyy
    top_float_variables, top_int_variables \
        = get_TopRecoAlg_variables(flags)
    hyyMVA_float_variables, hyyMVA_int_variables \
        = get_BaselineVarsHyyHHbbyyMVAAlg(flags)

    if flags.Analysis.save_detailed_Input_variables:
        HH_H_float_variable_names['top'] += top_float_variables
        HH_H_int_variable_names['top'] += top_int_variables
        if flags.Analysis.do_Hyy_multiclass:
            HH_H_float_variable_names['HyyMVA'] += hyyMVA_float_variables
            HH_H_int_variable_names['HyyMVA'] += hyyMVA_int_variables

    HH_H_variable_names += [*chain.from_iterable(HH_H_float_variable_names.values()),
                            *chain.from_iterable(HH_H_int_variable_names.values())]

    if not flags.Analysis.do_CP_systematics:
        sys_suffix = "NOSYS"
    else:
        sys_suffix = "%SYS%"

    for var in HH_H_variable_names:
        branches += [f"EventInfo.{var}_{sys_suffix} -> bbyy_{var}"
                     + flags.Analysis.systematics_suffix_separator + sys_suffix]

    return branches, \
        float_variable_names | HH_H_float_variable_names, \
        int_variable_names | HH_H_int_variable_names
