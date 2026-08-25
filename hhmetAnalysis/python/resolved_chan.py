from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator
from AthenaConfiguration.ComponentFactory import CompFactory
import AthenaCommon.SystemOfUnits as Units

from EasyjetHub.algs.postprocessing.SelectorAlgConfig import JetSelectorAlgCfg
from EasyjetHub.output.ttree.selected_objects import (
    get_selected_objects_branches,
)


def _get_resolved_option(flags, name, default):
    """Read optional Analysis.Resolved config without requiring it to exist."""
    try:
        return getattr(flags.Analysis.Resolved, name)
    except AttributeError:
        return default


def _pairing_algorithms(flags):
    algorithms = _get_resolved_option(
        flags,
        "pairing_algorithms",
        ["mass_125", "min_deltaR_max"],
    )
    if isinstance(algorithms, str):
        algorithms = [algorithms]
    return list(algorithms)


def _pairing_output_variables(flags):
    """Determine the pairing vars requested."""

    output_profile = _get_resolved_option(flags, "pairing_output_profile", "compact")

    common_float_vars = ["hh_m", "hh_pt", "hh_eta", "hh_phi"]
    common_int_vars = ["reco_success"]

    if output_profile == "compact":
        # Save higgs candidates 4-vectors, and some truth reco info
        alg_float_vars = [
            "h1_m", "h1_pt", "h1_eta", "h1_phi",
            "h2_m", "h2_pt", "h2_eta", "h2_phi",
            "hh_deltaR", "metric",
        ]
        alg_int_vars = ["j1_index", "j2_index", "j3_index", "j4_index"]
        if flags.Input.isMC:
            alg_int_vars += ["truth_correct"]

        return common_float_vars, common_int_vars, alg_float_vars, alg_int_vars

    if output_profile == "full":
        # Save full higgs and di-higgs info, plus extensive truth reco info
        alg_float_vars = [
            "hh_deltaEta", "metric", "hh_deltaR"
        ]
        common_int_vars += [
            "n_truth_pairing_bjets",
            "n_truth_higgs_pairing_bjets",
            "truth_status"
        ]

        for part in ["h1", "h2"]:
            for var in ["m", "pt", "eta", "phi", "deltaR"]:
                alg_float_vars += [f"{part}_{var}"]

        alg_int_vars = [
            "j1_index", "j2_index", "j3_index", "j4_index",
            "j1_is_b", "j2_is_b", "j3_is_b", "j4_is_b",
            "truth_correct", "truth_parent_j1", "truth_parent_j2",
            "truth_parent_j3", "truth_parent_j4",
        ]

        return common_float_vars, common_int_vars, alg_float_vars, alg_int_vars

    raise ValueError(
        "Unknown resolved pairing output profile "
        f"'{output_profile}'. Expected 'compact' or 'full'."
    )


def resolved_cfg(flags, smalljetkey, float_variables=None):

    cfg = ComponentAccumulator()

    pairing_algorithms = _pairing_algorithms(flags)
    pairing_truth_b_label = _get_resolved_option(
        flags,
        "pairing_truth_b_label",
        "HadronConeExclTruthLabelID",
    )
    pairing_truth_higgs_parent_mask = _get_resolved_option(
        flags,
        "pairing_truth_higgs_parent_mask",
        "parentHiggsParentsMask",
    )
    pairing_higgs_mass = _get_resolved_option(flags,
                                              "pairing_higgs_mass", 125.0 * Units.GeV)

    if not float_variables:
        float_variables = []

    # loop over the btagging WPs
    btag_wps = [flags.Analysis.Small_R_jet.btag_wp]
    btag_wps += flags.Analysis.Small_R_jet.btag_extra_wps
    for btag_wp in btag_wps:
        btag_sys = btag_wp + "_%SYS%"
        btag_name = f"ftag_select_{btag_wp}"

        # get the 4 leading small R jets
        cfg.merge(
            JetSelectorAlgCfg(
                flags, name="resolvedSmallRjetSelectorAlg_" + btag_wp,
                containerInKey=smalljetkey,
                containerOutKey="resolvedAnalysisSmallRjets_" + btag_sys,
                bTagWPDecorName=btag_name,
                selectBjet=True,
                minPt=20 * Units.GeV,
                maxEta=2.5,
                truncateAtAmount=4,  # -1 means keep all
                minimumAmount=3
            )
        )  # -1 means ignores this

        # calculate final resolved vars
        cfg.addEventAlgo(
            CompFactory.HHMET.BaselineVarsResolvedAlg(
                "ResolvedAlg_" + btag_wp,
                smallRContainerInKey="resolvedAnalysisSmallRjets_" + btag_sys,
                recoJetContainerInKey=flags.Analysis.container_names.output.reco4PFlowJet,
                bTagWP=btag_wp,
                bTagSelDecor=btag_name,
                pairingAlgorithms=pairing_algorithms,
                higgsMass=pairing_higgs_mass,
                isMC=flags.Input.isMC,
                truthBLabelDecorName=pairing_truth_b_label,
                truthHiggsParentsMaskDecorName=pairing_truth_higgs_parent_mask
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

    common_float_vars, common_int_vars, \
        alg_float_vars, alg_int_vars = _pairing_output_variables(flags)

    btag_wps = [flags.Analysis.Small_R_jet.btag_wp]
    btag_wps += flags.Analysis.Small_R_jet.btag_extra_wps

    for btag_wp in btag_wps:

        for var in common_float_vars + common_int_vars:
            branches += [
                f"EventInfo.resolved_{var}_{btag_wp}_%SYS%"
                + f" -> hhmet_resolved_{btag_wp}_{var}"
                + flags.Analysis.systematics_suffix_separator + "%SYS%"
            ]

        for algorithm in _pairing_algorithms(flags):
            for var in alg_float_vars + alg_int_vars:
                branches += [
                    f"EventInfo.resolved_{algorithm}_{var}_{btag_wp}_%SYS%"
                    + f" -> hhmet_resolved_{algorithm}_{btag_wp}_{var}"
                    + flags.Analysis.systematics_suffix_separator + "%SYS%"
                ]

    return branches, float_variable_names
