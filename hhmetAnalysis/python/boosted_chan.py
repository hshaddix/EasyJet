from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator
from AthenaConfiguration.ComponentFactory import CompFactory
from EasyjetHub.algs.postprocessing.SelectorAlgConfig import JetSelectorAlgCfg
from EasyjetHub.output.ttree.selected_objects import (
    get_selected_objects_branches,
)
from hhmetAnalysis.helpers import get_basic_fourmom_variables, get_fixed_btag_wps


def boosted_cfg(flags, largejetkey, smalljetkey, float_variables=None):
    if not float_variables:
        float_variables = []

    cfg = ComponentAccumulator()

    btag_wps_fixed = get_fixed_btag_wps(flags)
    for btag_wp in btag_wps_fixed:

        cfg.addEventAlgo(
            CompFactory.HHMET.BaselineVarsBoostedAlg(
                "BaselineVarsBoostedAlg_" + btag_wp,
                smallRContainerInKey=smalljetkey,
                largeRContainerInKey=largejetkey,
                isMC=flags.Input.isMC,
                bTagWP=btag_wp
            )
        )

    return cfg

def boosted_branches(flags):
    branches = []
    float_variable_names = []

    baseline_float_variables = get_basic_fourmom_variables()
    float_variable_names += baseline_float_variables

    # These are the variables always saved with the objects selected by the analysis
    # This is tunable with the flags amount and variables
    # in the object configs.
    branches += get_selected_objects_branches(flags, "hhmet_boosted")

    btag_wps_fixed = get_fixed_btag_wps(flags)
    for btag_wp in btag_wps_fixed:

        hh_vars = ["hh_m", "hh_pt", "hh_deltaEta", "hh_deltaPhi"]

        variables = ["m", "pt", "eta", "phi", "E"]

        for var in variables:
            hh_vars += [f"h2_{var}"]

        for var in hh_vars:
            if not flags.Input.isMC and "truthLabel" in var:
                continue
            branches += [
                f"EventInfo.boosted_{var}_{btag_wp}_%SYS%"
                + f" -> hhmet_boosted_{btag_wp}_{var}_"
                + flags.Analysis.systematics_suffix_separator + "%SYS%"
            ]

    boosted_h_vars = []
    largeR_vars = [
        "m", "pt", "eta", "phi", "E",
        "Tau32_wta", "truthLabel"
    ]

    for var in largeR_vars:
        boosted_h_vars += [f"h1_{var}"]

    for var in boosted_h_vars:
        if not flags.Input.isMC and "truthLabel" in var:
            continue
        branches += [
            f"EventInfo.boosted_{var}_%SYS%"
            + f" -> hhmet_boosted_{var}"
            + flags.Analysis.systematics_suffix_separator + "%SYS%"
        ]

    return branches, float_variable_names
