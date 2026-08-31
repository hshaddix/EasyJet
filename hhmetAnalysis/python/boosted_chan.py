from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator
from AthenaConfiguration.ComponentFactory import CompFactory
import AthenaCommon.SystemOfUnits as Units

from EasyjetHub.algs.postprocessing.SelectorAlgConfig import JetSelectorAlgCfg
from EasyjetHub.output.ttree.selected_objects import (
    get_selected_objects_branches,
)


def boosted_cfg(flags, largejetkey, smalljetkey, float_variables=None):
    if not float_variables:
        float_variables = []

    cfg = ComponentAccumulator()

    cfg.merge(
        JetSelectorAlgCfg(
            flags,
            name="boostedLargeRjetSelectorAlg",
            containerInKey=largejetkey,
            containerOutKey="boostedAnalysisLargeRjets_%SYS%",
            bTagWPDecorName="",
            selectBjet=False,
            minPt=float(flags.Analysis.Large_R_jet.min_pT),
            maxEta=float(flags.Analysis.Large_R_jet.max_eta)
        )
    )  # -1 means ignores this

    btag_wps = [flags.Analysis.Small_R_jet.btag_wp]
    btag_wps += flags.Analysis.Small_R_jet.btag_extra_wps
    for btag_wp in btag_wps:
        btag_sys = btag_wp + "_%SYS%"

        # get the 4 leading small R jets
        cfg.merge(
            JetSelectorAlgCfg(
                flags,
                name="boostedSmallRjetSelectorAlg_" + btag_wp,
                containerInKey=smalljetkey,
                containerOutKey="boostedAnalysisSmallRjets_" + btag_sys,
                bTagWPDecorName="ftag_select_" + btag_wp,
                selectBjet=True,
                minPt=float(flags.Analysis.Small_R_jet.min_pT),
                maxEta=float(flags.Analysis.Small_R_jet.max_eta),
                truncateAtAmount=2,  # -1 means keep all
                minimumAmount=2
            )
        )  # -1 means ignores this

        cfg.addEventAlgo(
            CompFactory.HHMET.BaselineVarsBoostedAlg(
                "BaselineVarsBoostedAlg_" + btag_wp,
                smallRContainerInKey="boostedAnalysisSmallRjets_" + btag_sys,
                largeRContainerInKey="boostedAnalysisLargeRjets_%SYS%",
                isMC=flags.Input.isMC,
                bTagWP=btag_wp
            )
        )

    return cfg


def get_BaselineVarsBoostedAlg_variables(flags):
    float_variable_names = []
#    int_variable_names = []
#
#    # Define the variables that will be saved in the output TTree
#    #boosted_vars = ["hh_m", "hh_pt", "hh_delta_eta", "hh_delta_phi"]
#
#    variables = [
#        "m", "pt", "eta", "phi", "E",
#        "Tau32_wta", "GN2Xv01_disc",
#        "GN2Xv01_phbb", "GN2Xv01_pqcd",
#        "GN2Xv01_phcc", "GN2Xv01_ptop"
#    ]
#    for var in variables:
#        for hc in ["h1", "h2"]:
#            boosted_vars.append(f"{hc}_{var}")
#    boosted_vars += ["h1_truthLabel", "h2_truthLabel"]
#    boosted_vars = [f"boosted_{var}" for var in boosted_vars]
#    float_variable_names += boosted_vars

    objects = []
    for object in objects:
        for var in ["m", "pt", "eta", "phi"]:
            float_variable_names.append(f"{object}_{var}")

    return float_variable_names


def boosted_branches(flags):
    branches = []
    float_variable_names = []

    baseline_float_variables = get_BaselineVarsBoostedAlg_variables(flags)
    float_variable_names += baseline_float_variables

    # These are the variables always saved with the objects selected by the analysis
    # This is tunable with the flags amount and variables
    # in the object configs.
    branches += get_selected_objects_branches(flags, "hhmet_boosted")

    btag_wps = [flags.Analysis.Small_R_jet.btag_wp]
    btag_wps += flags.Analysis.Small_R_jet.btag_extra_wps

    for btag_wp in btag_wps:

        hh_vars = ["hh_m", "hh_pt", "hh_deltaEta", "hh_deltaPhi"]

        variables = ["m", "pt", "eta", "phi", "E"]

        for var in variables:
            hh_vars += [f"h2_{var}"]

        for var in hh_vars:
            if not flags.Input.isMC and "truthLabel" in var:
                continue
            deco_suffix = ""
            branches += [
                f"EventInfo.boosted_{var}_{btag_wp}_%SYS%"
                + f" -> hhmet_boosted_{deco_suffix}{btag_wp}_{var}"
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
