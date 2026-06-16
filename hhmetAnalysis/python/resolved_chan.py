from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator
from AthenaConfiguration.ComponentFactory import CompFactory
import AthenaCommon.SystemOfUnits as Units

from EasyjetHub.algs.postprocessing.SelectorAlgConfig import JetSelectorAlgCfg
from EasyjetHub.output.ttree.selected_objects import (
    get_selected_objects_branches,
)


def resolved_cfg(flags, smalljetkey, float_variables=None):
    if not float_variables:
        float_variables = []

    cfg = ComponentAccumulator()

    # this is a resolved dihiggs analysis chain
    btag_wps = [flags.Analysis.Small_R_jet.btag_wp]
    btag_wps += flags.Analysis.Small_R_jet.btag_extra_wps
    for btag_wp in btag_wps:
        btag_sys = btag_wp + "_%SYS%"
        # get the 4 leading small R jets
        cfg.merge(
            JetSelectorAlgCfg(
                flags, name="resolvedSmallRjetSelectorAlg_" + btag_wp,
                containerInKey=smalljetkey,
                containerOutKey="resolvedAnalysisSmallRjets_" + btag_sys,
                bTagWPDecorName="ftag_select_" + btag_wp,
                selectBjet=True,
                minPt=20 * Units.GeV,
                maxEta=2.5,
                truncateAtAmount=4,  # -1 means keep all
                minimumAmount=4
            )
        )  # -1 means ignores this

        # pair them with some strategy and save them as leading (h1) and
        # subleading (h2) Higgs candidates in the order:
        # h1_leading_pt_jet
        # h1_subleading_pt_jet
        # h2_leading_pt_jet
        # h2_subleading_pt_jet

        # calculate final resolved vars
        cfg.addEventAlgo(
            CompFactory.HHMET.BaselineVarsResolvedAlg(
                "FinalVarsResolvedAlg_" + btag_wp,
                smallRContainerInKey="resolvedAnalysisSmallRjets_" + btag_sys,
                bTagWP=btag_wp
            )
        )

    return cfg


def get_BaselineVarsResolvedAlg_variables(flags):
    float_variable_names = []

    objects = []

    for object in objects:
        for var in ["m", "pt", "eta", "phi"]:
            float_variable_names.append(f"{object}_{var}")

    return float_variable_names


def resolved_branches(flags):
    branches = []
    float_variable_names = []

    baseline_float_variables = get_BaselineVarsResolvedAlg_variables(flags)
    float_variable_names += baseline_float_variables

    # These are the variables always saved with the objects selected by the analysis
    # This is tunable with the flags amount and variables
    # in the object configs.
    branches += get_selected_objects_branches(flags, "hhmet_resolved")

    btag_wps = [flags.Analysis.Small_R_jet.btag_wp]
    btag_wps += flags.Analysis.Small_R_jet.btag_extra_wps

    for btag_wp in btag_wps:
        resolved_vars = ["hh_m", "hh_pt", "hh_deltaEta"]
        for part in ["h1", "h2"]:
            for var in ["m", "pt", "eta", "phi"]:
                resolved_vars += [f"{part}_{var}"]

        for var in resolved_vars:
            deco_suffix = ""

            branches += [
                f"EventInfo.resolved_{var}_{btag_wp}_%SYS%"
                + f" -> hhmet_resolved_{deco_suffix}{btag_wp}_{var}"
                + flags.Analysis.systematics_suffix_separator + "%SYS%"
            ]

    return branches, float_variable_names
