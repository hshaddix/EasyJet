from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator
from EasyjetHub.algs.event_counter_config import event_counter_cfg
from bbbbAnalysis.config.boosted import boosted_cfg, boosted_branches
from bbbbAnalysis.config.resolved import (
    resolved_branches, resolved_cfg,
    resolved_trigger_SF_cfg, resolved_trigger_bucket_cfg
)
from bbbbAnalysis.config.boost_histograms import histograms_cfg
from AthenaConfiguration.ComponentFactory import CompFactory
from EasyjetHub.output.ttree.selected_objects import (
    get_selected_objects_branches,
)

from EasyjetHub.algs.postprocessing.SelectorAlgConfig import JetSelectorAlgCfg
import AthenaCommon.SystemOfUnits as Units


def dihiggs_cfg(
    flags,
    vbf_rnn_variables=None
):
    if not vbf_rnn_variables:
        vbf_rnn_variables = []

    cfg = ComponentAccumulator()

    couting_jet_container = ""
    couting_rnnjet_container = ""
    couting_bjet_container = ""
    btag_name = ""
    btag_name_sel = ""
    if flags.Analysis.do_small_R_jets:
        smalljetkey = flags.Analysis.container_names.output.reco4PFlowJet
        couting_jet_container = "resolvedAnalysisSmallRJets_%SYS%"
        couting_rnnjet_container = "bbbbAnalysisSmallRRNNJets_%SYS%"
        couting_bjet_container = "bbbbAnalysisSmallRBJets_%SYS%"
        if flags.Analysis.Small_R_jet.btag_wp != "":
            btag_name = f"baselineSelection_{flags.Analysis.Small_R_jet.btag_wp}_%SYS%"
            btag_name_sel = f"ftag_select_{flags.Analysis.Small_R_jet.btag_wp}"
        # Used only for counting
        cfg.merge(
            JetSelectorAlgCfg(
                flags, name="SmallRJetSelectorAlg",
                containerInKey=smalljetkey,
                containerOutKey=couting_jet_container,
                bTagWPDecorName=btag_name,
                selectBjet=False,
                minPt=20. * Units.GeV,
                maxEta=2.5,
                minimumAmount=4,
            )
        )  # -1 means ignores this

        # RNN Jet candidates
        cfg.merge(
            JetSelectorAlgCfg(
                flags, name="SmallRRNNJetSelectorAlg",
                containerInKey=smalljetkey,
                containerOutKey=couting_rnnjet_container,
                bTagWPDecorName=btag_name,
                selectBjet=False,
                minPt=20. * Units.GeV,
                minimumAmount=2,
            )
        )  # -1 means ignores this

        # b-jets for RNN tagger (resolved category)
        cfg.merge(
            JetSelectorAlgCfg(
                flags, name="SmallRBJetSelectorAlg",
                containerInKey=smalljetkey,
                containerOutKey=couting_bjet_container,
                bTagWPDecorName=btag_name,
                selectBjet=True,
                minPt=20. * Units.GeV,
                maxEta=2.5,
                minimumAmount=4,
            )
        )  # -1 means ignores this

    couting_lRjet_container = ""
    if flags.Analysis.do_large_R_UFO_jets:
        lr10jets_sys = flags.Analysis.container_names.output.reco10UFOJet
        couting_lRjet_container = "boostedAnalysisLargeRJets_%SYS%"
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
                maxEta=2.0,
                jetAmount=2,
            )
        )  # -1 means ignores this

    selection_name = flags.Analysis.selection_name
    triggers = flags.Analysis.TriggerChains
    triggers = [trigger.replace(".", "p").replace("-", "_") for trigger in triggers]

    if len(flags.Analysis.CutList) > 0:
        cfg.addEventAlgo(
            CompFactory.HH4B.bbbbSelectorAlg(
                "bbbbSelectorAlg",
                cutList=flags.Analysis.CutList,
                saveCutFlow=flags.Analysis.save_cutflow,
                Triggers=triggers,
                eventDecisionOutputDecoration=f"pass_{selection_name}_%SYS%",
                isMC=flags.Input.isMC,
                bypass=flags.Analysis.bypass,
                smallRJetToCount=couting_jet_container,
                largeRJetToCount=couting_lRjet_container,
                btagSelDecor=btag_name_sel,
            )
        )

    if flags.Analysis.do_resolved_dihiggs:
        bbbb_extra_branches, float_variable_names = resolved_branches(flags)
        cfg.merge(
            resolved_cfg(
                flags,
                smalljetkey=smalljetkey,
                float_variables=float_variable_names,
            )
        )
        cfg.merge(event_counter_cfg("n_resolved"))

    if flags.Analysis.do_boosted_dihiggs:
        bbbb_extra_branches, float_variable_names = boosted_branches(flags)
        cfg.merge(
            boosted_cfg(
                flags,
                largejetkey=lr10jets_sys,
                float_variables=float_variable_names,
            )
        )
        cfg.merge(event_counter_cfg("n_merged"))

    if flags.Analysis.UseVBFRNN:
        # Normalise VerVBFRNN to a list
        ver_vbfrnn = flags.Analysis.VerVBFRNN
        if isinstance(ver_vbfrnn, str):
            ver_vbfrnn = [ver_vbfrnn]

        for ver in ver_vbfrnn:
            # Short tag for StoreGate keys to avoid 32-bit hash collisions
            # e.g. VBFRNNv2p2p2 -> v2p2p2
            short_ver = ver.replace("VBFRNN", "")

            vbftagger = CompFactory.VBFTagger(
                f"VBFTaggerTool_{ver}", modelTag=ver
            )

            # VBF-RNN tagger: boosted
            if flags.Analysis.do_boosted_vbf_tagger:
                cfg.addEventAlgo(
                    CompFactory.VBFTaggerAlgSys(
                        f"VBFTaggerAlg_boosted_{ver}",
                        VBFTagger=vbftagger,
                        containerAllJetsKey=couting_rnnjet_container,
                        containerSigLargeRJetsKey=couting_lRjet_container,
                        OnlyFirstLargeRJet=False,
                        pTCut=20.e3,
                        nMaxJets=2,
                        DecTag=f"_bst_{short_ver}"
                    )
                )

            # VBF-RNN tagger: resolved
            if flags.Analysis.do_resolved_vbf_tagger:
                cfg.addEventAlgo(
                    CompFactory.VBFTaggerAlgSys(
                        f"VBFTaggerAlg_resolved_{ver}",
                        VBFTagger=vbftagger,
                        containerAllJetsKey=couting_rnnjet_container,
                        containerSigJetsKey=couting_bjet_container,
                        pTCut=20.e3,
                        nMaxJets=2,
                        DecTag=f"_res_{short_ver}"
                    )
                )

            cfg.addEventAlgo(
                CompFactory.HH4B.VBFRNNVarsAlg(
                    f"VBFRNNVarsAlg_{ver}",
                    UseVBFRNN=flags.Analysis.UseVBFRNN,
                    doBoosted=flags.Analysis.do_boosted_vbf_tagger,
                    doResolved=flags.Analysis.do_resolved_vbf_tagger,
                    RNNJets_boosted=f"RNNJets_bst_{short_ver}_%SYS%",
                    RNNJets_resolved=f"RNNJets_res_{short_ver}_%SYS%",
                    boostedPrefix=f"bst_{short_ver}_RNNJets",
                    resolvedPrefix=f"res_{short_ver}_RNNJets",
                    floatVariableList=[
                        v for v in vbf_rnn_variables if short_ver in v
                    ],
                )
            )

    if (flags.Analysis.do_resolved_trigger_SF
            or flags.Analysis.do_resolved_trigger_bucket):
        cfg.merge(
            JetSelectorAlgCfg(flags, name="SmallJetPreSelectorAlg",
                              containerInKey=smalljetkey.replace("%SYS%", "NOSYS"),
                              containerOutKey="smallRJetsForTriggerMatching",
                              minPt=20 * Units.GeV,
                              maxEta=3.1))

    if flags.Analysis.do_resolved_trigger_SF:
        cfg.merge(
            resolved_trigger_SF_cfg(flags)
        )

    if flags.Analysis.do_resolved_trigger_bucket:
        cfg.merge(
            resolved_trigger_bucket_cfg(flags)
        )

    if flags.Analysis.histograms:
        cfg.merge(histograms_cfg(flags))

    return cfg


def get_RNNJets_variables(flags):
    float_variable_names = []

    ver_vbfrnn = flags.Analysis.VerVBFRNN
    if isinstance(ver_vbfrnn, str):
        ver_vbfrnn = [ver_vbfrnn]

    for ver in ver_vbfrnn:
        short_ver = ver.replace("VBFRNN", "")
        objects = []
        if flags.Analysis.do_boosted_vbf_tagger:
            objects += [
                f"bst_{short_ver}_RNNJets_Jet1",
                f"bst_{short_ver}_RNNJets_Jet2",
            ]
        if flags.Analysis.do_resolved_vbf_tagger:
            objects += [
                f"res_{short_ver}_RNNJets_Jet1",
                f"res_{short_ver}_RNNJets_Jet2",
            ]

        for obj in objects:
            for var in ["m", "pt", "eta", "phi"]:
                float_variable_names.append(f"{obj}_{var}")

    return float_variable_names


def dihiggs_branches(flags):
    branches = []
    vbf_rnn_variable_names = []

    branches += get_selected_objects_branches(flags, "bbbb")

    vbf_rnn_variable_names += get_RNNJets_variables(flags)

    for var in vbf_rnn_variable_names:
        if flags.Input.isMC and "truthLabel" in var:
            continue
        branches += [
            f"EventInfo.{var}_%SYS%"
            + f" -> bbbb_{var}"
            + flags.Analysis.systematics_suffix_separator + "%SYS%"
        ]

    # VBF tagger
    if flags.Analysis.UseVBFRNN:
        ver_vbfrnn = flags.Analysis.VerVBFRNN
        if isinstance(ver_vbfrnn, str):
            ver_vbfrnn = [ver_vbfrnn]

        vbf_vars = ['RNNScore', 'nRNNJets']
        for ver in ver_vbfrnn:
            short_ver = ver.replace("VBFRNN", "")
            if flags.Analysis.do_boosted_vbf_tagger:
                for var in vbf_vars:
                    branches += [
                        f'EventInfo.{var}_bst_{short_ver}_%SYS%'
                        f' -> bbbb_bst_{short_ver}_{var}'
                        + flags.Analysis.systematics_suffix_separator
                        + '%SYS%'
                    ]
            if flags.Analysis.do_resolved_vbf_tagger:
                for var in vbf_vars:
                    branches += [
                        f'EventInfo.{var}_res_{short_ver}_%SYS%'
                        f' -> bbbb_res_{short_ver}_{var}'
                        + flags.Analysis.systematics_suffix_separator
                        + '%SYS%'
                    ]

    return branches, vbf_rnn_variable_names


def pass_branches(flags):
    branches = []

    branches += get_selected_objects_branches(flags, "bbbb_pass")

    selection_name = flags.Analysis.selection_name
    branches += [
        f"EventInfo.pass_{selection_name}_%SYS%"
        + f" -> bbbb_pass_{selection_name}"
        + flags.Analysis.systematics_suffix_separator + "%SYS%"
    ]

    if (flags.Analysis.save_cutflow):
        cutList = flags.Analysis.CutList
        for cut in cutList:
            branches += [f"EventInfo.{cut}_%SYS%"
                         + f" -> bbbb_pass_{cut}"
                         + flags.Analysis.systematics_suffix_separator + "%SYS%"]

    return branches
