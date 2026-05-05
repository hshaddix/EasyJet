from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator
from EasyjetHub.algs.event_counter_config import event_counter_cfg
from bbbbbbAnalysis.config.resolved import (
    resolved_branches, resolved_cfg,
    resolved_trigger_SF_cfg, resolved_trigger_bucket_cfg,
    resolved_trigger_matching_enabled,
)
from bbbbbbAnalysis.config.boosted import (
    boosted_branches, boosted_cfg
)

from AthenaConfiguration.ComponentFactory import CompFactory
from EasyjetHub.output.ttree.selected_objects import (
    get_selected_objects_branches,
)

from EasyjetHub.algs.postprocessing.SelectorAlgConfig import JetSelectorAlgCfg
import AthenaCommon.SystemOfUnits as Units


def trihiggs_cfg(
    flags,
):
    cfg = ComponentAccumulator()

    couting_jet_container = ""
    couting_bjet_container = ""
    btag_name = ""
    if flags.Analysis.do_small_R_jets:
        smalljetkey = (
            flags.Analysis.container_names.output.reco4PFlowJet)
        couting_jet_container = (
            "resolvedAnalysisSmallRJets_%SYS%")
        couting_rnnjet_container = (
            "bbbbbbAnalysisSmallRRNNJets_%SYS%")
        couting_bjet_container = (
            "bbbbbbAnalysisSmallRBJets_%SYS%")
        if flags.Analysis.Small_R_jet.btag_wp != "":
            btag_name = (
                f"ftag_select_{flags.Analysis.Small_R_jet.btag_wp}")
        cfg.merge(
            JetSelectorAlgCfg(
                flags, name="SmallRJetSelectorAlg",
                containerInKey=smalljetkey,
                containerOutKey=couting_jet_container,
                bTagWPDecorName=btag_name,
                selectBjet=False,
                minPt=20. * Units.GeV,
                maxEta=2.5,
                minimumAmount=-1,  # ignore for now
            )
        )

        cfg.merge(
            JetSelectorAlgCfg(
                flags, name="SmallRRNNJetSelectorAlg",
                containerInKey=smalljetkey,
                containerOutKey=couting_rnnjet_container,
                bTagWPDecorName=btag_name,
                selectBjet=False,
                minPt=20. * Units.GeV,
                minimumAmount=-1,
            )
        )

        cfg.merge(
            JetSelectorAlgCfg(
                flags, name="SmallRBJetSelectorAlg",
                containerInKey=smalljetkey,
                containerOutKey=couting_bjet_container,
                bTagWPDecorName=btag_name,
                selectBjet=True,
                minPt=20. * Units.GeV,
                maxEta=2.5,
                minimumAmount=-1,
            )
        )

    couting_lRjet_container = ""
    if flags.Analysis.do_large_R_UFO_jets:
        lr10jets_sys = (
            flags.Analysis.container_names.output.reco10UFOJet)
        couting_lRjet_container = (
            "boostedAnalysisLargeRJets_%SYS%")
        cfg.merge(
            JetSelectorAlgCfg(
                flags, name="LargeRJetSelectorAlg",
                containerInKey=lr10jets_sys,
                containerOutKey=couting_lRjet_container,
                bTagWPDecorName="",
                selectBjet=False,
                minPt=200. * Units.GeV,
                maxPt=3000. * Units.GeV,
                maxMass=600. * Units.GeV,
                maxEta=2.5,
                jetAmount=-1,  # ignore for now
            )
        )

    selection_name = flags.Analysis.selection_name
    triggers = flags.Analysis.TriggerChains
    triggers = [
        trigger.replace(".", "p").replace("-", "_")
        for trigger in triggers
    ]

    if len(flags.Analysis.CutList) > 0:
        truth_jet_container = ""
        if flags.Input.isMC and flags.Analysis.do_small_R_jets:
            truth_jet_container = (
                flags.Analysis.container_names.input.truth4Jet)

        cfg.addEventAlgo(
            CompFactory.HHH6B.bbbbbbSelectorAlg(
                "bbbbbbSelectorAlg",
                cutList=flags.Analysis.CutList,
                saveCutFlow=flags.Analysis.save_cutflow,
                Triggers=triggers,
                eventDecisionOutputDecoration=(
                    f"pass_{selection_name}_%SYS%"),
                isMC=flags.Input.isMC,
                bypass=flags.Analysis.bypass,
                smallRJetToCount=couting_jet_container,
                smallRJetBeforeSelection=(
                    smalljetkey
                    if flags.Analysis.do_small_R_jets
                    else ""),
                truthJets=truth_jet_container,
                largeRJetToCount=couting_lRjet_container,
                btagSelDecor=btag_name,
            )
        )

    if flags.Analysis.do_resolved_trihiggs:
        bbbbbb_extra_branches, float_variable_names = (
            resolved_branches(flags))
        cfg.merge(
            resolved_cfg(
                flags,
                smalljetkey=smalljetkey,
                float_variables=float_variable_names,
            )
        )
        cfg.merge(event_counter_cfg("n_resolved"))

    if flags.Analysis.do_boosted_trihiggs:
        bbbbbb_extra_branches, float_variable_names = (
            boosted_branches(flags))
        cfg.merge(
            boosted_cfg(
                flags,
                largejetkey=lr10jets_sys,
                smalljetkey=smalljetkey,
                float_variables=float_variable_names,
            )
        )
        cfg.merge(event_counter_cfg("n_merged"))

    if (flags.Analysis.do_resolved_trigger_SF
            or flags.Analysis.do_resolved_trigger_bucket) and (
                resolved_trigger_matching_enabled(flags)):
        cfg.merge(
            JetSelectorAlgCfg(
                flags, name="SmallJetPreSelectorAlg",
                containerInKey=smalljetkey.replace(
                    "%SYS%", "NOSYS"),
                containerOutKey=(
                    "smallRJetsForTriggerMatching"),
                minPt=20 * Units.GeV,
                maxEta=2.5))

    if flags.Analysis.do_resolved_trigger_SF:
        cfg.merge(
            resolved_trigger_SF_cfg(
                flags,
                smalljetkey=smalljetkey,
            )
        )

    if (flags.Analysis.do_resolved_trigger_bucket
            and resolved_trigger_matching_enabled(flags)):
        cfg.merge(
            resolved_trigger_bucket_cfg(flags)
        )

    return cfg


def pass_branches(flags):
    branches = []

    branches += get_selected_objects_branches(
        flags, "bbbbbb_pass")

    selection_name = flags.Analysis.selection_name
    sfx = flags.Analysis.systematics_suffix_separator
    branches += [
        f"EventInfo.pass_{selection_name}_%SYS%"
        + f" -> bbbbbb_pass_{selection_name}"
        + sfx + "%SYS%"
    ]

    if flags.Analysis.save_cutflow:
        cutList = flags.Analysis.CutList
        for cut in cutList:
            branches += [
                f"EventInfo.{cut}_%SYS%"
                + f" -> bbbbbb_pass_{cut}"
                + sfx + "%SYS%"]

    branches += [
        "EventInfo.pass_4b_%SYS%"
        " -> bbbbbb_pass_4b" + sfx + "%SYS%",
        "EventInfo.pass_5b_%SYS%"
        " -> bbbbbb_pass_5b" + sfx + "%SYS%",
        "EventInfo.pass_6b_%SYS%"
        " -> bbbbbb_pass_6b" + sfx + "%SYS%",
    ]

    jet_count_vars = [
        "n_truth_jets",
        "n_truth_jets_after_cuts",
        "n_reco_jets_before_cuts",
        "n_reco_jets_after_cuts"
    ]
    for var in jet_count_vars:
        branches += [
            f"EventInfo.{var}_%SYS%"
            + f" -> bbbbbb_{var}"
            + sfx + "%SYS%"
        ]

    return branches
