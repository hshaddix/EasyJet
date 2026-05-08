from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator
from AthenaConfiguration.ComponentFactory import CompFactory
import AthenaCommon.SystemOfUnits as Units

from EasyjetHub.algs.postprocessing.SelectorAlgConfig import JetSelectorAlgCfg
from EasyjetHub.output.ttree.selected_objects import (
    get_selected_objects_branches,
)

def semiboosted_cfg(flags, largejetkey, smalljetkey, btag_WP, float_variables=None):
    if not float_variables:
        float_variables = []

    cfg = ComponentAccumulator()

    # this is a semi-boosted trihiggs analysis chain
    btag_wps = [flags.Analysis.Small_R_jet.btag_wp]
    btag_wps += flags.Analysis.Small_R_jet.btag_extra_wps
    for btag_wp in btag_wps:
        btag_sys = btag_wp + "_%SYS%"
        # get the small-R jets
        cfg.merge(JetSelectorAlgCfg(flags, name="SmallJetSelectorAlg_" + btag_wp,
                                    containerInKey=smalljetkey,
                                    containerOutKey="resolvedAnalysisJets_" + btag_sys,
                                    bTagWPDecorName="ftag_select_" + btag_wp,
                                    selectBjet=False,  # Changed to False to get all jets, not just b-jets
                                    minPt=20 * Units.GeV,
                                    maxEta=2.5,                                         
                                    truncateAtAmount=-1,  # -1 means keep all            KEEP AT -1 FOR NOW
                                    minimumAmount=-1))  # -1 means ignores this          KEEP AT -1 FOR NOW

        cfg.addEventAlgo(
            CompFactory.HHH6B.BaselineVarsSemiBoostedAlg(
                "BaselineVarsSemiBoostedAlg",
                isMC=flags.Input.isMC,
                jets=largejetkey,
                smallRjets=smalljetkey,
            )
        )

    return cfg


def get_BaselineVarsSemiBoostedAlg_variables(flags):
    float_variable_names = []

    # Define the variables that will be saved in the output TTree
    boosted_vars = ["hhh_m", "hhh_pt", "h1h2_delta_eta", "h1h3_delta_eta", "h2h3_delta_eta", "h1h2_delta_phi", "h1h3_delta_phi", "h2h3_delta_phi", "h1Jet5_deltaR", "h1Jet6_deltaR", "h2Jet5_deltaR", "h2Jet6_deltaR", "h1h2_MassDiff", "h1h3_MassDiff", "h2h3_MassDiff", "Thrust", "Sphericity", "TransverseSphericity", "Aplanarity", "Planarity"]

    variables = ["m", "pt", "eta", "phi", "E"] 
    for var in variables:
        for hc in ["h1", "h2"]:
            boosted_vars.append(f"{hc}_{var}")
    if flags.Input.isMC:
        boosted_vars += ["h1_truthLabel", "h2_truthLabel"]
    
    boosted_vars = [f"semiboosted_{var}" for var in boosted_vars]

    resolved_vars = ["DeltaR56", "DeltaEtaJet5Jet6", "DeltaPhiJet5Jet6", "Jet56Pt", "Jet56Mass", "Jet5_parentHiggsParentsMask", "Jet6_parentHiggsParentsMask"]
    resolved_vars = [f"semiboosted_h3_{var}" for var in resolved_vars]

    float_variable_names += boosted_vars
    float_variable_names += resolved_vars

    return float_variable_names


def semiboosted_branches(flags):
    branches = []
    float_variable_names = []

    baseline_float_variables = get_BaselineVarsSemiBoostedAlg_variables(flags)
    float_variable_names += baseline_float_variables

    # These are the variables always saved with the objects selected by the analysis
    # This is tunable with the flags amount and variables
    # in the object configs.
    branches += get_selected_objects_branches(flags, "bbbbbb_semiboosted")

    for var in float_variable_names:
        branches += [
            f"EventInfo.{var}_%SYS%"
            + f" -> bbbbbb_{var}"
            + flags.Analysis.systematics_suffix_separator + "%SYS%"
        ]

    return branches, float_variable_names