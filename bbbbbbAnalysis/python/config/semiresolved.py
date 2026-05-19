from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator
from AthenaConfiguration.ComponentFactory import CompFactory
import AthenaCommon.SystemOfUnits as Units

from EasyjetHub.algs.postprocessing.SelectorAlgConfig import JetSelectorAlgCfg
from EasyjetHub.output.ttree.selected_objects import (
    get_selected_objects_branches,
)


def semiresolved_cfg(flags, largejetkey, smalljetkey, btag_WP, float_variables=None):
    if not float_variables:
        float_variables = []

    cfg = ComponentAccumulator()

    # this is a resolved trihiggs analysis chain
    btag_wps = [flags.Analysis.Small_R_jet.btag_wp]
    btag_wps += flags.Analysis.Small_R_jet.btag_extra_wps
    for btag_wp in btag_wps:
        btag_sys = btag_wp + "_%SYS%"
        # get the 4 leading small R jets
        cfg.merge(JetSelectorAlgCfg(flags, name="SmallJetSelectorAlg_" + btag_wp,
                                    containerInKey=smalljetkey,
                                    containerOutKey="resolvedAnalysisJets_" + btag_sys,
                                    bTagWPDecorName="ftag_select_" + btag_wp,
                                    selectBjet=False,  # Changed to False to get all jets, not just b-jets
                                    minPt=20 * Units.GeV,
                                    maxEta=2.5,                                         #WHY 2.5? MIGHT NEED TO CHANGE 
                                    truncateAtAmount=-1,  # -1 means keep all            KEEP AT -1 FOR NOW
                                    minimumAmount=-1))  # -1 means ignores this          KEEP AT -1 FOR NOW

        # pair them with some strategy and save them as leading (h1) and
        # subleading (h2) Higgs candidates in the order:
        # h1_leading_pt_jet
        # h1_subleading_pt_jet
        # h2_leading_pt_jet
        # h2_subleading_pt_jet
        cfg.addEventAlgo(
            CompFactory.HHH6B.JetPairingAlg(
                "JetPairingAlg_" + btag_wp,
                containerInKey=(
                    "resolvedAnalysisJets_" + btag_sys),
                containerOutKey=(
                    "pairedResolvedAnalysisJets_" + btag_sys),
                pairingStrategy=flags.Analysis.pairingStrategy,
                analysisRegion="semiresolved",
            )
        )

        cfg.addEventAlgo(
            CompFactory.HHH6B.BaselineVarsSemiResolvedAlg(
                "BaselineVarsSemiResolvedAlg",
                isMC=flags.Input.isMC,
                jets=largejetkey,
                smallRjets=smalljetkey,
                #btagSelDecor=btag_WP,
            )
        )

    return cfg


def get_BaselineVarsSemiResolvedAlg_variables(flags):
    float_variable_names = []
#    int_variable_names = []


    # Define the variables that will be saved in the output TTree
    boosted_vars = ["hhh_m", "hhh_pt", "h1h2_delta_eta", "h1h3_delta_eta", "h2h3_delta_eta", "h1h2_delta_phi", "h1h3_delta_phi", "h2h3_delta_phi", "h1Jet3_deltaR", "h1Jet4_deltaR", "h1Jet5_deltaR", "h1Jet6_deltaR", "h1h2_MassDiff", "h2h3_MassDiff", "h1h3_MassDiff", "Thrust", "Sphericity", "TransverseSphericity", "Aplanarity", "Planarity"]

    variables = ["m", "pt", "eta", "phi", "E"] 
    for var in variables:
        for hc in ["h1"]:
            boosted_vars.append(f"{hc}_{var}")
    
    boosted_vars += ["h1_truthLabel"]
    boosted_vars = [f"semiresolved_{var}" for var in boosted_vars]
    
    resolved_vars34 = ["DeltaR34", "DeltaEtaJet3Jet4", "DeltaPhiJet3Jet4", "Jet34Pt", "Jet34Mass", "Jet3_parentHiggsParentsMask", "Jet4_parentHiggsParentsMask"]
    resolved_vars34 = [f"semiresolved_h2_{var}" for var in resolved_vars34]
    resolved_vars56 = ["DeltaR56", "DeltaEtaJet5Jet6", "DeltaPhiJet5Jet6", "Jet56Pt", "Jet56Mass", "Jet5_parentHiggsParentsMask", "Jet6_parentHiggsParentsMask"]
    resolved_vars56 = [f"semiresolved_h3_{var}" for var in resolved_vars56]

    float_variable_names += boosted_vars
    float_variable_names += resolved_vars34
    float_variable_names += resolved_vars56

    return float_variable_names


def semiresolved_branches(flags):
    branches = []
    float_variable_names = []

    baseline_float_variables = get_BaselineVarsSemiResolvedAlg_variables(flags)
    float_variable_names += baseline_float_variables

    # These are the variables always saved with the objects selected by the analysis
    # This is tunable with the flags amount and variables
    # in the object configs.
    branches += get_selected_objects_branches(flags, "bbbbbb_semiresolved")


    for var in float_variable_names:    
        branches += [
            f"EventInfo.{var}_%SYS%"
            + f" -> bbbbbb_{var}"
            + flags.Analysis.systematics_suffix_separator + "%SYS%"
        ]

    return branches, float_variable_names