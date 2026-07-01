from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator
from AthenaConfiguration.ComponentFactory import CompFactory
from AthenaConfiguration.Enums import LHCPeriod
from EasyjetHub.steering.sample_metadata import is_at_least

from BoostedJetTaggers.BoostedJetTaggerConfig import qgTagAlgCfg


def jet_decor_cfg(flags, **kwargs):
    cfg = ComponentAccumulator()

    jetcoll = flags.Analysis.container_names.input[
        flags.Analysis.Small_R_jet.jet_type
    ]

    kwargs.setdefault("isMC", flags.Input.isMC)

    # to handle the special case of Run 2 HLT

    do_hlt_matching = flags.Analysis.Small_R_jet.doHLTMatching
    if flags.GeoModel.Run is LHCPeriod.Run2:
        do_hlt_matching = do_hlt_matching and flags.Analysis.Small_R_jet.doRun2HLTEmulation

    if do_hlt_matching or flags.Analysis.Small_R_jet.doL1Matching:
        kwargs.setdefault("doL1Matching", flags.Analysis.Small_R_jet.doL1Matching)
        kwargs.setdefault("doHLTMatching", do_hlt_matching)
        kwargs.setdefault("triggerList", flags.Analysis.TriggerChainsDeco)

        from TrigDecisionTool.TrigDecisionToolConfig import TrigDecisionToolCfg
        kwargs.setdefault("TrigDecisionTool", cfg.getPrimaryAndMerge(
            TrigDecisionToolCfg(flags)))

        if do_hlt_matching and (flags.GeoModel.Run is LHCPeriod.Run2):
            kwargs.setdefault("useEmulationTool", True)

            from TrigBtagEmulationTool.TrigBtagEmulationToolConfig import (
                TrigBtagEmulationToolCfg)
            kwargs.setdefault("trigBtagEmulationTool", cfg.popToolsAndMerge(
                TrigBtagEmulationToolCfg(
                    flags,
                    toBeEmulatedTriggers=list(flags.Analysis.TriggerChainsDeco))))

    cfg.addEventAlgo(
        CompFactory.Easyjet.JetDecoratorAlg(
            f"JetDecor_{jetcoll}",
            jetsIn=jetcoll,
            **kwargs
        )
    )

    return cfg


def large_R_jet_decor_cfg(flags, **kwargs):

    cfg = ComponentAccumulator()

    is_valid_for_gn3x = is_at_least(flags, "p7017")
    jet_output_flags = flags.Analysis.ttree_output.collection_options.large_R_jets

    # compute discriminants
    if is_valid_for_gn3x and jet_output_flags.tautag_details:

        jetcoll = flags.Analysis.container_names.input['reco10UFOJet']
        cfg.addEventAlgo(
            CompFactory.Easyjet.GNXLargeJetDecoratorAlg(
                f"JetDecor_{jetcoll}", jetsIn=jetcoll, **kwargs
            )
        )

    return cfg


def qg_tagger_decor_cfg(flags):
    """Configure the q/g tagger (and optional SFs) for every requested WP.

    Reuses the upstream ``BoostedJetTaggerConfig.qgTagAlgCfg`` with its default
    calibration paths; only the WP, the SF switch and the run period are set
    (as in ``BoostedJetTaggers/test_qgTagger.py``). Each WP gets its own set of
    decorations (``QGTransformer_PassScore<wp>WP``, ...) so all WPs coexist on
    the same jet container.
    """
    cfg = ComponentAccumulator()

    jet_flags = flags.Analysis.Small_R_jet
    qg_wps = jet_flags.qg_tagger_wps
    if not qg_wps:
        return cfg

    apply_sfs = jet_flags.qg_tagger_apply_sfs and flags.Input.isMC
    run = "run2" if flags.GeoModel.Run is LHCPeriod.Run2 else "run3"

    for wp in qg_wps:
        cfg.merge(qgTagAlgCfg(flags, WP=str(wp), addSFs=apply_sfs, run=run))

    return cfg
