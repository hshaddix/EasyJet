import AthenaCommon.SystemOfUnits as Units
from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator
from AthenaConfiguration.ComponentFactory import CompFactory
from EasyjetHub.algs.event_counter_config import event_counter_cfg
from EasyjetHub.algs.postprocessing.SelectorAlgConfig import (
    ElectronSelectorAlgCfg,
    JetSelectorAlgCfg,
    MuonSelectorAlgCfg,
    PhotonSelectorAlgCfg,
)
from EasyjetHub.output.ttree.selected_objects import (
    get_selected_objects_branches,
)
from hhmetAnalysis.boosted_chan import boosted_branches, boosted_cfg
from hhmetAnalysis.resolved_chan import resolved_branches, resolved_cfg


def dihiggs_cfg(
    flags,
):
    cfg = ComponentAccumulator()

    srjet_container = ""
    bjet_container = ""
    lrjet_container = ""
    btag_name = ""

    if flags.Analysis.do_small_R_jets:

        srjet_key = flags.Analysis.container_names.output.reco4PFlowJet
        srjet_container = "hhmetAnalysisSmallRjets_%SYS%"
        bjet_container = "hhmetAnalysisSmallRBjets_%SYS%"

        if flags.Analysis.Small_R_jet.btag_wp != "":
            btag_name = f"ftag_select_{flags.Analysis.Small_R_jet.btag_wp}"

        # small-r jets
        cfg.merge(
            JetSelectorAlgCfg(
                flags, name="smallRjetSelectorAlg",
                containerInKey=srjet_key,
                containerOutKey=srjet_container,
                bTagWPDecorName=btag_name,
                selectBjet=False,
                minPt=float(flags.Analysis.Small_R_jet.min_pT),
                maxEta=float(flags.Analysis.Small_R_jet.max_eta)
            )
        )  # -1 means ignores this

        # b-jets
        cfg.merge(
            JetSelectorAlgCfg(
                flags, name="smallRBjetSelectorAlg",
                containerInKey=srjet_key,
                containerOutKey=bjet_container,
                bTagWPDecorName=btag_name,
                selectBjet=True,
                minPt=float(flags.Analysis.Small_R_jet.min_pT),
                maxEta=float(flags.Analysis.Small_R_jet.max_eta)
            )
        )  # -1 means ignores this

    if flags.Analysis.do_large_R_UFO_jets:

        lrjet_key = flags.Analysis.container_names.output.reco10UFOJet
        lrjet_container = "hhmetAnalysisLargeRjets_%SYS%"
        cfg.merge(
            JetSelectorAlgCfg(
                flags, name="largeRjetSelectorAlg",
                containerInKey=lrjet_key,
                containerOutKey=lrjet_container,
                bTagWPDecorName="",
                selectBjet=False,
                minPt=float(flags.Analysis.Large_R_jet.min_pT),
                maxEta=float(flags.Analysis.Large_R_jet.max_eta)
            )
        )  # -1 means ignores this

    muon_key = flags.Analysis.container_names.output.muons
    muon_container = "hhmetAnalysisMuons_%SYS%"
    cfg.merge(
        MuonSelectorAlgCfg(
            flags, name="MuonSelectorAlg",
            containerInKey=muon_key,
            containerOutKey=muon_container,
            minPt=float(flags.Analysis.Muon.min_pT),
            maxEta=float(flags.Analysis.Muon.max_eta)
        )
    )  # -1 means ignores this

    electron_key = flags.Analysis.container_names.output.electrons
    electron_container = "hhmetAnalysisElectrons_%SYS%"
    cfg.merge(
        ElectronSelectorAlgCfg(
            flags, name="ElectronSelectorAlg",
            containerInKey=electron_key,
            containerOutKey=electron_container,
            minPt=float(flags.Analysis.Electron.min_pT),
            maxEta=float(flags.Analysis.Electron.max_eta)
        )
    )  # -1 means ignores this

    photon_key = flags.Analysis.container_names.output.photons
    photon_container = "hhmetAnalysisPhotons_%SYS%"
    cfg.merge(
        PhotonSelectorAlgCfg(
            flags, name="PhotonSelectorAlg",
            containerInKey=photon_key,
            containerOutKey=photon_container,
            minPt=float(flags.Analysis.Photon.min_pT),
            maxEta=float(flags.Analysis.Photon.max_eta)
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
                smallRjetToCount=srjet_container,
                largeRjetToCount=lrjet_container,
                electronToCount=electron_container,
                muonToCount=muon_container,
                photonToCount=photon_container,
                btagSelDecor=btag_name,
                met="AnalysisMET_%SYS%"
            )
        )

    if flags.Analysis.do_resolved_dihiggs:
        _, float_variable_names = resolved_branches(flags)
        cfg.merge(
            resolved_cfg(
                flags,
                smalljetkey=srjet_container,
                float_variables=float_variable_names,
            )
        )
        cfg.merge(event_counter_cfg("n_resolved"))

    if flags.Analysis.do_boosted_dihiggs:
        _, float_variable_names = boosted_branches(flags)
        cfg.merge(
            boosted_cfg(
                flags,
                smalljetkey=srjet_container,
                largejetkey=lrjet_container,
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
        + f"{flags.Analysis.systematics_suffix_separator}%SYS%"
    ]

    if (flags.Analysis.save_cutflow):
        cutList = flags.Analysis.CutList
        for cut in cutList:
            branches += [
                f"EventInfo.{cut}_%SYS%"
                + f" -> hhmet_pass_{cut}"
                + f"{flags.Analysis.systematics_suffix_separator}%SYS%"
            ]

    return branches
