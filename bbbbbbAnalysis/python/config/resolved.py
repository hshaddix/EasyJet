from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator
from AthenaConfiguration.ComponentFactory import CompFactory
import AthenaCommon.SystemOfUnits as Units

from EasyjetHub.algs.postprocessing.SelectorAlgConfig import JetSelectorAlgCfg
from EasyjetHub.output.ttree.selected_objects import (
    get_selected_objects_branches,
)


def resolved_trigger_matching_enabled(flags) -> bool:
    """L1 and/or HLT small-R trigger matching;
       required for bucket / SF algs and ntuple aux."""
    return (
        flags.Analysis.Small_R_jet.doL1Matching
        or flags.Analysis.Small_R_jet.doHLTMatching
    )


def resolved_trigger_SF_cfg(flags, smalljetkey):
    cfg = ComponentAccumulator()

    # This is a jet trigger scale factor block
    if (flags.Input.isMC
            and (flags.Analysis.Small_R_jet.doHLTMatching
                 or flags.Analysis.Small_R_jet.doL1Matching)):
        # temporary solution for boosted trigger
        resolved_chain = [
            t for t in flags.Analysis.TriggerChains
            if "_a10" not in t
        ]

        from TrigDecisionTool.TrigDecisionToolConfig import TrigDecisionToolCfg
        trigDecisionTool = cfg.getPrimaryAndMerge(TrigDecisionToolCfg(flags))

        if flags.Analysis.Small_R_jet.doL1Matching:
            cfg.addEventAlgo(
                CompFactory.HHH6B.SmallRJetTriggerSFAlg(
                    "SmallRJetL1SFAlg",
                    containerInKey="smallRJetsForTriggerMatching",
                    containerOutKey="trigL1MatchedSmallRJets",
                    TrigDecisionTool=trigDecisionTool,
                    triggers=resolved_chain,
                    years=flags.Analysis.Years,
                    matchingLevel="L1",
                    doL1SF=flags.Analysis.Small_R_jet.doL1Matching,
                    doHLTSF=flags.Analysis.Small_R_jet.doHLTMatching
                )
            )
        if flags.Analysis.Small_R_jet.doHLTMatching:
            cfg.addEventAlgo(
                CompFactory.HHH6B.SmallRJetTriggerSFAlg(
                    "SmallRJetHLTSFAlg",
                    containerInKey="smallRJetsForTriggerMatching",
                    containerOutKey="trigHLTMatchedSmallRJets",
                    TrigDecisionTool=trigDecisionTool,
                    triggers=resolved_chain,
                    years=flags.Analysis.Years,
                    matchingLevel="HLT",
                    doL1SF=flags.Analysis.Small_R_jet.doL1Matching,
                    doHLTSF=flags.Analysis.Small_R_jet.doHLTMatching
                )
            )

    return cfg


def resolved_trigger_bucket_cfg(flags):
    cfg = ComponentAccumulator()

    # calculate trigger buckets
    jets_name = "smallRJetsForTriggerMatching"

    cfg.addEventAlgo(
        CompFactory.HHH6B.TriggerDecoratorAlg(
            "HHH6bTriggerDecoratorAlg",
            jets=jets_name,
            triggerLists=flags.Analysis.TriggerChains,
            doL1SF=flags.Analysis.Small_R_jet.doL1Matching,
            doHLTSF=flags.Analysis.Small_R_jet.doHLTMatching,
            isMC=flags.Input.isMC
        )
    )

    return cfg


def resolved_cfg(flags, smalljetkey, float_variables=None):
    if not float_variables:
        float_variables = []

    cfg = ComponentAccumulator()

    btag_wps = [flags.Analysis.Small_R_jet.btag_wp]
    btag_wps += flags.Analysis.Small_R_jet.btag_extra_wps
    for btag_wp in btag_wps:
        btag_sys = btag_wp + "_%SYS%"
        cfg.merge(JetSelectorAlgCfg(
            flags, name="SmallJetSelectorAlg_" + btag_wp,
            containerInKey=smalljetkey,
            containerOutKey="resolvedAnalysisJets_" + btag_sys,
            bTagWPDecorName="ftag_select_" + btag_wp,
            selectBjet=False,
            minPt=20 * Units.GeV,
            maxEta=4.5,  # might need to change to 2.5
            truncateAtAmount=-1,
            minimumAmount=-1))

        cfg.addEventAlgo(
            CompFactory.HHH6B.JetPairingAlg(
                "JetPairingAlg_" + btag_wp,
                containerInKey=(
                    "resolvedAnalysisJets_" + btag_sys),
                containerOutKey=(
                    "pairedResolvedAnalysisJets_" + btag_sys),
                pairingStrategy=flags.Analysis.pairingStrategy,
                analysisRegion="resolved",
            )
        )

        cfg.addEventAlgo(
            CompFactory.HHH6B.BaselineVarsResolvedAlg(
                "FinalVarsResolvedAlg_" + btag_wp,
                smallRContainerInKey=(
                    "pairedResolvedAnalysisJets_" + btag_sys),
                bTagWP=btag_wp,
                floatVariableList=float_variables,
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


def _resolved_kinematic_vars():
    """Variable list shared by both pairing strategies."""
    return [
        "DeltaR56",
        "DeltaR35",
        "DeltaEtaJet1Jet2",
        "DeltaEtaJet3Jet4",
        "DeltaEtaJet5Jet6",
        "DeltaPhiJet1Jet2",
        "DeltaPhiJet3Jet4",
        "DeltaPhiJet5Jet6",
        "Jet12Pt",
        "Jet34Pt",
        "Jet56Pt",
        "Jet12Mass",
        "Jet34Mass",
        "Jet56Mass",
        "M4j",
        "MassDiff12vs34",
        "MassDiff12vs56",
        "MassDiff34vs56",
        "Thrust",
        "Sphericity",
        "TransverseSphericity",
        "Planarity",
        "Aplanarity",
        "jet_pairing_indices",
        "mH_radius",
    ]


def resolved_branches(flags):
    branches = []
    float_variable_names = []

    baseline_float_variables = (
        get_BaselineVarsResolvedAlg_variables(flags))
    float_variable_names += baseline_float_variables

    branches += get_selected_objects_branches(
        flags, "bbbbbb_resolved")

    btag_wps = [flags.Analysis.Small_R_jet.btag_wp]
    btag_wps += flags.Analysis.Small_R_jet.btag_extra_wps
    for btag_wp in btag_wps:
        resolved_vars = ["hhh_m", "hhh_pt"]
        for part in ["h1", "h2", "h3"]:
            for var in ["m", "pt", "eta", "phi"]:
                resolved_vars += [part + "_" + var]

        strategy = flags.Analysis.pairingStrategy
        if strategy == "minDeltaR":
            resolved_vars += [
                "DeltaR12", "DeltaR13", "DeltaR14",
                "DeltaR23", "DeltaR24", "DeltaR34",
            ] + _resolved_kinematic_vars()

        elif strategy == "minTargetMass":
            resolved_vars += _resolved_kinematic_vars()

        for var in resolved_vars:
            deco_suffix = ""
            if strategy == "minDeltaR":
                deco_suffix = "mindrPaired_"
            elif strategy == "minTargetMass":
                deco_suffix = "mintargetmassPaired_"

            branches += [
                f"EventInfo.resolved_{var}_{btag_wp}_%SYS%"
                + " -> bbbbbb_resolved_"
                + f"{deco_suffix}{btag_wp}_{var}"
                + flags.Analysis.systematics_suffix_separator
                + "%SYS%"
            ]

    return branches, float_variable_names


def resolved_trigger_SF_branches(flags):
    branches = []

    if flags.Input.isMC and flags.Analysis.do_resolved_trigger_SF and (
       flags.Analysis.triggerSFBranchSaveStrategy == "detail"
       or flags.Analysis.triggerSFBranchSaveStrategy == "both"):
        resolved_chain = [
            t for t in flags.Analysis.TriggerChains
            if "_a10" not in t
        ]
        for trig in resolved_chain:
            trig = trig.replace("-", "_").replace(".", "p")
            matchLevels = []
            if flags.Analysis.Small_R_jet.doL1Matching:
                matchLevels.append("L1")
            if flags.Analysis.Small_R_jet.doHLTMatching:
                matchLevels.append("HLT")
            for matchLevel in matchLevels:
                sfx = flags.Analysis.systematics_suffix_separator
                branches += [
                    f'EventInfo.trigSF_{trig}_{matchLevel}SF'
                    f'->trigSF_{trig}_{matchLevel}SF',
                    f'EventInfo.trigSF_{trig}_{matchLevel}SF'
                    f'_stats__1up'
                    f'->trigSF_{trig}_{matchLevel}SF'
                    + sfx + 'stats__1up',
                    f'EventInfo.trigSF_{trig}_{matchLevel}SF'
                    f'_syst__1up'
                    f'->trigSF_{trig}_{matchLevel}SF'
                    + sfx + 'syst__1up',
                ]
                if flags.Analysis.Small_R_jet.saveTriggerInfo:
                    jet_coll = (
                        f'trig{matchLevel}MatchedSmallRJets')
                    branches += [
                        f'{jet_coll}.NoBJetCalibMomentum_pt'
                        f'->{jet_coll}_NoBJetCalibMomentum_pt',
                        f'{jet_coll}.{trig}_{matchLevel}'
                        f'threshold_NOSYS'
                        f'->{jet_coll}_{trig}_{matchLevel}'
                        f'threshold',
                        f'{jet_coll}.{trig}_{matchLevel}'
                        f'SF_NOSYS'
                        f'->{jet_coll}_{trig}_{matchLevel}SF',
                        f'{jet_coll}.{trig}_{matchLevel}'
                        f'SF_stats__1up'
                        f'->{jet_coll}_{trig}_{matchLevel}SF'
                        + sfx + 'stats__1up',
                        f'{jet_coll}.{trig}_{matchLevel}'
                        f'SF_syst__1up'
                        f'->{jet_coll}_{trig}_{matchLevel}SF'
                        + sfx + 'syst__1up',
                    ]

    return branches


def resolved_trigger_bucket_branches(flags):
    if not resolved_trigger_matching_enabled(flags):
        return []
    # AsgxAODNTupleMakerAlg needs explicit types for EventInfo dynamic aux (cp bbbbAnalysis/resolved.py).
    branches = ['EventInfo.bucket -> trigger_bucket type=int']

    for trig in ["1b", "2b1j", "2bHT", "2b2j", "j75", "j80"]:
        branches += [
            f'EventInfo.pass_trigger_{trig}'
            f' -> trigger_bucket_{trig} type=char',
        ]

    if flags.Input.isMC and flags.Analysis.do_resolved_trigger_SF and (
       flags.Analysis.triggerSFBranchSaveStrategy == "simple"
       or flags.Analysis.triggerSFBranchSaveStrategy == "both"):
        matchLevels = []
        if flags.Analysis.Small_R_jet.doL1Matching:
            matchLevels.append("L1")
        if flags.Analysis.Small_R_jet.doHLTMatching:
            matchLevels.append("HLT")
        for matchLevel in matchLevels:
            ml = matchLevel.lower()
            branches += [
                f'EventInfo.trigger_smallrjet_sf_{ml}_nosys'
                f'-> trigger_SmallRJet_{matchLevel}SF_NOSYS type=float',
                f'EventInfo.trigger_smallrjet_sf_{ml}_stats__1up'
                f'-> trigger_SmallRJet_{matchLevel}SF_'
                f'{matchLevel}Stats__1up type=float',
                f'EventInfo.trigger_smallrjet_sf_{ml}_syst__1up'
                f'-> trigger_SmallRJet_{matchLevel}SF_'
                f'{matchLevel}Syst__1up type=float',
            ]

    return branches
