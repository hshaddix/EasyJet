from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator
from AthenaConfiguration.ComponentFactory import CompFactory

from EasyjetHub.algs.postprocessing.SelectorAlgConfig import (
    JetSelectorAlgCfg,)
from EasyjetHub.output.ttree.selected_objects import (
    get_selected_objects_branches_variables,
)
from XbbCalib.hists_output_config import hists_output_cfg

from .vars_to_copy import lrj_floats_to_copy, lrj_gn3x_working_points

copiedVariablePrefix0 = "Zcand0_"
copiedVariablePrefix1 = "Zcand1_"

def ZbbjCalib_cfg(flags, largejetkey,
                  float_variables=None, int_variables=None):
    if not float_variables:
        float_variables = []
    if not int_variables:
        int_variables = []

    cfg = ComponentAccumulator()

    selected_large_r = "XbbCalibLRJets_%SYS%"
    bjr_ordered_large_r = "XbbCalibBJROrderedLRJets_%SYS%"

    cfg.merge(
        JetSelectorAlgCfg(
            flags,
            name="LargeJetSelectorAlg",
            containerInKey=largejetkey,
            containerOutKey=selected_large_r,
            minPt=flags.Analysis.Large_R_jet.min_pT,
            maxEta=flags.Analysis.Large_R_jet.maxEta,
            minimumAmount=1,
        )
    )

    cfg.addEventAlgo(
        CompFactory.XBBCALIB.ZbbjCalibSelectorAlg(
            "ZbbjCalibSelectorAlg",
            largeRJets=selected_large_r,
            bjrOrderedJets=bjr_ordered_large_r,
            eventDecisionOutputDecoration="XbbCalib_pass_sr_%SYS%",
            bypass=flags.Analysis.bypass,
        )
    )

    cfg.addEventAlgo(
        CompFactory.XBBCALIB.BaselineVarsZbbjCalibAlg(
            "BaselineVarsZbbjCalibAlg",
            largeRJets=bjr_ordered_large_r,
            floatsToCopy=lrj_floats_to_copy(flags),
            intsToCopy=lrj_gn3x_working_points(flags),
            copiedVariablePrefix0=copiedVariablePrefix0,
            copiedVariablePrefix1 =copiedVariablePrefix1
        )
    )

    out_path = (
        flags.Analysis.h5_output
        if flags.Analysis.output.h5.hists.enable else None
    )
    cfg.merge(hists_output_cfg(
        flags, flags.Analysis.output.h5.hists, out_path,
        tag_wps=lrj_gn3x_working_points(flags)))

    return cfg


def get_baselineVarsZbbjCalibAlg_variables(flags):
    float_variable_names = []
    int_variable_names = ["lrjets_n"]

    for object in [copiedVariablePrefix0 , copiedVariablePrefix1]:
        for var in ["pt","eta","phi","m"]:
            float_variable_names.append(f"{object}{var}")

    
    float_variable_names += [
        f'{p}{x}'
        for x in lrj_floats_to_copy(flags)
        for p in (copiedVariablePrefix0, copiedVariablePrefix1)
    ]

    int_variable_names += [
        f'{p}{x}'
        for x in lrj_gn3x_working_points(flags)
        for p in (copiedVariablePrefix0, copiedVariablePrefix1)
    ]
    return float_variable_names , int_variable_names




def ZbbjCalib_branches(flags):
    branches = []

    # this will be all the variables that are calculated by the
    # BaselineVarsZbbjCalibAlg algorithm
    all_baseline_variable_names = []
    float_variable_names = []
    int_variable_names = []

    # these are the variables that will always be stored by easyjet specific
    # to XbbCalib. However, at this stage we have no varibles that are
    # calculated by this algorithm
    # so we will just make a placeholder list

    # These are the variables always saved with the objects selected
    # by the analysis
    # This is tunable with the flags amount and variables
    # in the object configs.
    baseline_float_variables, baseline_int_variables =(
         get_baselineVarsZbbjCalibAlg_variables(flags)
    )
    float_variable_names += baseline_float_variables
    int_variable_names += baseline_int_variables

    all_baseline_variable_names += [*float_variable_names, *int_variable_names]
    
    for var in all_baseline_variable_names:
        branches += [f"EventInfo.{var}_%SYS% -> ZbbjCalib_{var}"
                     + flags.Analysis.systematics_suffix_separator + "%SYS%"]


    object_level_branches, object_level_float_variables, object_level_int_variables = (
        get_selected_objects_branches_variables(flags, "ZbbCalib")
    )

    float_variable_names += object_level_float_variables
    int_variable_names += object_level_int_variables

    branches += object_level_branches




    return branches, float_variable_names, int_variable_names
