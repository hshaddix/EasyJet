from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator
from AthenaConfiguration.ComponentFactory import CompFactory

from EasyjetHub.output.ttree.selected_objects import (
    get_selected_objects_branches,
)


def boosted_cfg(flags, smalljetkey, largejetkey, float_variables=None):
    if not float_variables:
        float_variables = []

    cfg = ComponentAccumulator()

    cfg.addEventAlgo(
        CompFactory.HHH6B.BaselineVarsBoostedAlg(
            "BaselineVarsBoostedAlg",
            isMC=flags.Input.isMC,
            floatVariableList=float_variables,
        )
    )

    return cfg


def get_BaselineVarsBoostedAlg_variables(flags):
    float_variable_names = []

    # Define the variables that will be saved in the output TTree
    boosted_vars = ["hhh_m", "hhh_pt", "h1h2_delta_eta", "h1h3_delta_eta",
                    "h2h3_delta_eta", "h1h2_delta_phi", "h1h3_delta_phi",
                    "h2h3_delta_phi", "h1h2_MassDiff", "h1h3_MassDiff",
                    "h2h3_MassDiff", "Thrust", "Sphericity",
                    "TransverseSphericity", "Planarity", "Aplanarity",]

    variables = ["m", "pt", "eta", "phi", "E", "Tau32_wta"]
    for var in variables:
        for hc in ["h1", "h2", "h3"]:
            boosted_vars.append(f"{hc}_{var}")
    if flags.Input.isMC:
        boosted_vars += ["h1_truthLabel", "h2_truthLabel", "h3_truthLabel"]
    
    boosted_vars = [f"boosted_{var}" for var in boosted_vars]
    float_variable_names += boosted_vars

    return float_variable_names


def boosted_branches(flags):
    branches = []
    float_variable_names = []

    baseline_float_variables = get_BaselineVarsBoostedAlg_variables(flags)
    float_variable_names += baseline_float_variables

    # These are the variables always saved with the objects selected by the analysis
    # This is tunable with the flags amount and variables
    # in the object configs.
    branches += get_selected_objects_branches(flags, "bbbbbb_boosted")

    sys = "%SYS%" if flags.Analysis.do_CP_systematics else "NOSYS"
    for var in float_variable_names:
        branches += [
            f"EventInfo.{var}_{sys}"
            + f" -> bbbbbb_{var}"
            + flags.Analysis.systematics_suffix_separator + sys
        ]

    return branches, float_variable_names
