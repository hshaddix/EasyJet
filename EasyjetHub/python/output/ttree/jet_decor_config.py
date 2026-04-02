from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator
from AthenaConfiguration.ComponentFactory import CompFactory
from AthenaConfiguration.Enums import LHCPeriod
from EasyjetHub.steering.sample_metadata import is_at_least


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
