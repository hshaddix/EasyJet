from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator
from EasyjetHub.algs.event_counter_config import event_counter_cfg
from hhmetAnalysis.boosted_chan import boosted_cfg, boosted_branches
from hhmetAnalysis.resolved_chan import (
    resolved_branches, resolved_cfg
)
from AthenaConfiguration.ComponentFactory import CompFactory
from EasyjetHub.output.ttree.selected_objects import (
    get_selected_objects_branches,
)

from EasyjetHub.algs.postprocessing.SelectorAlgConfig import (
    JetSelectorAlgCfg, MuonSelectorAlgCfg, ElectronSelectorAlgCfg,
)
import AthenaCommon.SystemOfUnits as Units


def dihiggs_cfg(
    flags,
):
    cfg = ComponentAccumulator()

    couting_jet_container = ""
    couting_bjet_container = ""
    btag_name = ""
    if flags.Analysis.do_small_R_jets:
        smalljetkey = flags.Analysis.container_names.output.reco4PFlowJet
        couting_jet_container = "resolvedAnalysisSmallRjets_%SYS%"
        couting_bjet_container = "hhmetAnalysisSmallRBjets_%SYS%"
        if flags.Analysis.Small_R_jet.btag_wp != "":
            btag_name = f"ftag_select_{flags.Analysis.Small_R_jet.btag_wp}"
        # Used only for counting
        cfg.merge(
            JetSelectorAlgCfg(
                flags, name="smallRjetSelectorAlg",
                containerInKey=smalljetkey,
                containerOutKey=couting_jet_container,
                bTagWPDecorName=btag_name,
                selectBjet=False,
                minPt=20. * Units.GeV,
                maxEta=2.5,
                minimumAmount=2,
            )
        )  # -1 means ignores this

        # b-jets for RNN tagger (resolved category)
        cfg.merge(
            JetSelectorAlgCfg(
                flags, name="smallRBjetSelectorAlg",
                containerInKey=smalljetkey,
                containerOutKey=couting_bjet_container,
                bTagWPDecorName=btag_name,
                selectBjet=True,
                minPt=20. * Units.GeV,
                maxEta=2.5,
                minimumAmount=2,
            )
        )  # -1 means ignores this

    couting_lRjet_container = ""
    if flags.Analysis.do_large_R_UFO_jets:
        lr10jets_sys = flags.Analysis.container_names.output.reco10UFOJet
        couting_lRjet_container = "boostedAnalysisLargeRjets_%SYS%"
        cfg.merge(
            JetSelectorAlgCfg(
                flags, name="largeRjetSelectorAlg",
                containerInKey=lr10jets_sys,
                containerOutKey=couting_lRjet_container,
                bTagWPDecorName="",
                selectBjet=False,
                minPt=200. * Units.GeV,
                maxPt=3000. * Units.GeV,
                maxMass=600. * Units.GeV,
                maxEta=2.0,
                jetAmount=1,
            )
        )  # -1 means ignores this

    couting_muon_container = "hhmetAnalysisMuons_%SYS%"
    muon_sys = flags.Analysis.container_names.output.muons
    cfg.merge(
        MuonSelectorAlgCfg(
            flags, name="MuonSelectorAlg",
            containerInKey=muon_sys,
            containerOutKey=couting_muon_container,
            minPt=8. * Units.GeV,
            maxEta=2.7,
            muonAmount=0,
        )
    )  # -1 means ignores this

    couting_electron_container = "hhmetAnalysisElectrons_%SYS%"
    electron_sys = flags.Analysis.container_names.output.electrons
    cfg.merge(
        ElectronSelectorAlgCfg(
            flags, name="ElectronSelectorAlg",
            containerInKey=electron_sys,
            containerOutKey=couting_electron_container,
            minPt=8. * Units.GeV,
            maxEta=2.47,
            electronAmount=0,
        )
    )  # -1 means ignores this

    selection_name = flags.Analysis.selection_name
    triggers = flags.Analysis.TriggerChains
    triggers = [trigger.replace(".", "p").replace("-", "_") for trigger in triggers]

    if len(flags.Analysis.CutList) > 0:
        cfg.addEventAlgo(
            CompFactory.HHMET.hhmetSelectorAlg(
                "hhmetSelectorAlg",
                cutList=flags.Analysis.CutList,
                saveCutFlow=flags.Analysis.save_cutflow,
                Triggers=triggers,
                eventDecisionOutputDecoration=f"pass_{selection_name}_%SYS%",
                isMC=flags.Input.isMC,
                bypass=flags.Analysis.bypass,
                smallRjetToCount=couting_jet_container,
                largeRjetToCount=couting_lRjet_container,
                electronToCount=couting_electron_container,
                muonToCount=couting_muon_container,
                btagSelDecor=btag_name
            )
        )

    if flags.Analysis.do_resolved_dihiggs:
        hhmet_extra_branches, float_variable_names = resolved_branches(flags)
        cfg.merge(
            resolved_cfg(
                flags,
                smalljetkey=smalljetkey,
                float_variables=float_variable_names,
            )
        )
        cfg.merge(event_counter_cfg("n_resolved"))

    if flags.Analysis.do_boosted_dihiggs:
        hhmet_extra_branches, float_variable_names = boosted_branches(flags)
        cfg.merge(
            boosted_cfg(
                flags,
                smalljetkey=smalljetkey,
                largejetkey=lr10jets_sys,
                float_variables=float_variable_names,
            )
        )
        cfg.merge(event_counter_cfg("n_merged"))

    return cfg


def pass_branches(flags):
    branches = []

    branches += get_selected_objects_branches(flags, "hhmet_pass")

    selection_name = flags.Analysis.selection_name
    branches += [
        f"EventInfo.pass_{selection_name}_%SYS%"
        + f" -> hhmet_pass_{selection_name}"
        + flags.Analysis.systematics_suffix_separator + "%SYS%"
    ]

    if (flags.Analysis.save_cutflow):
        cutList = flags.Analysis.CutList
        for cut in cutList:
            branches += [f"EventInfo.{cut}_%SYS%"
                         + f" -> hhmet_pass_{cut}"
                         + flags.Analysis.systematics_suffix_separator + "%SYS%"]

    return branches
