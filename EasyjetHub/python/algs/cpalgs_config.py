from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator
from AnalysisAlgorithmsConfig.ConfigAccumulator import ConfigAccumulator
from AnalysisAlgorithmsConfig.ConfigSequence import ConfigSequence

from AthenaConfiguration.ComponentFactory import CompFactory
from EasyjetHub.algs.calibration.jets import (
    jet_sequence, lr_jet_sequence, rc_jet_sequence)
from EasyjetHub.output.ttree.wtag_decor_config import wtag_decor_cfg
from EasyjetHub.output.ttree.lundnet_decor_config import lundnet_decor_cfg
from EasyjetHub.output.ttree.tau_decor_config import tau_decor_cfg
from EasyjetHub.output.ttree.jet_decor_config import (
    jet_decor_cfg, large_R_jet_decor_cfg
)
from EasyjetHub.output.ttree.electron_decor_config import electron_decor_config
from EasyjetHub.output.ttree.muon_decor_config import muon_track_decor_config

from EasyjetHub.algs.calibration.muons import muon_sequence
from EasyjetHub.algs.calibration.electrons import electron_sequence
from EasyjetHub.algs.calibration.photons import photon_sequence
from EasyjetHub.algs.calibration.taus import tau_sequence
from EasyjetHub.algs.calibration.ditaus import ditau_sequence
from EasyjetHub.algs.calibration.met import met_sequence
from EasyjetHub.algs.calibration.tracks import track_sequence
from EasyjetHub.algs.calibration.selection_decoration import (
    selection_decoration_sequence)
from EasyjetHub.algs.postprocessing.overlap_removal import overlap_sequence
from EasyjetHub.algs.postprocessing.thinning import thinning_sequence
from EasyjetHub.algs.calibration.triggerSF import triggerSF_sequence
from EasyjetHub.steering.utils.log_helper import log

from EasyjetHub.algs.event_counter_config import event_counter_cfg
from EasyjetHub.algs.event_info_global_alg_config import event_info_global_alg_cfg


# Map object types to sequence configurators
analysis_seqs = {
    "muons": muon_sequence,
    "electrons": electron_sequence,
    "photons": photon_sequence,
    "taus": tau_sequence,
    "ditaus": ditau_sequence,
    "small_R_jets": jet_sequence,
    "tracks": track_sequence
}


def cpalgs_cfg(flags):

    log.debug(f"Containers available in dataset: {flags.Input.Collections}")

    cfg = ComponentAccumulator()
    if not flags.Analysis.suppress_metadata_json:
        cfg.merge(event_counter_cfg("n_input"))

    # Extra decoration algorithms

    # Comput global EventInfo decoration, in particular data-taking year
    # Needed for MC20a = 2015+2016
    cfg.merge(event_info_global_alg_cfg(flags))

    if flags.Analysis.do_small_R_jets:
        cfg.merge(jet_decor_cfg(flags))

    if flags.Analysis.do_large_R_UFO_jets:
        cfg.merge(large_R_jet_decor_cfg(flags))

    if flags.Analysis.do_taus:
        cfg.merge(tau_decor_cfg(flags))

    if flags.Analysis.do_electrons:
        cfg.merge(electron_decor_config(flags))

    if flags.Analysis.do_muons:
        if flags.Analysis.Muon.do_track_decoration:
            cfg.merge(muon_track_decor_config(flags))

    if flags.Analysis.Large_R_jet.wtag_type and flags.Analysis.Large_R_jet.wtag_wp:
        cfg.merge(wtag_decor_cfg(flags))

    if flags.Analysis.Large_R_jet.lundnet_type:
        cfg.merge(lundnet_decor_cfg(flags))

    # Aggregate the configured CP algs in one ConfigSequence,
    # which will handle the container names, copying etc
    configSeq = ConfigSequence()

    # Define the sequence holding all the calibration
    # Activate the full configuration, which stitches together
    # the ConfigBlocks with interstitial container names etc
    calibSeq = CompFactory.AthSequencer('CPAlgSequence')
    configAccumulator = ConfigAccumulator(
        algSeq=calibSeq,
        flags=flags,
    )

    if not flags.Analysis.disable_calib:

        for objtype in [
            "electrons",
            "photons",
            "muons",
            "taus",
            "ditaus",
            "small_R_jets",
            "tracks"
        ]:
            if flags.Analysis[f"do_{objtype}"]:
                log.info(f"Adding {objtype} seq")

                # Append the configured CP calibration sequence for
                # the given object type
                # Pass the configAccumulator because we may need to
                # provide some container name info
                configSeq += analysis_seqs[objtype](flags, configAccumulator)

        if flags.Analysis.do_large_R_Topo_jets:
            log.info("Adding large-R jet seq")
            configSeq += lr_jet_sequence(
                flags,
                lr_jet_type="Topo",
                configAcc=configAccumulator,
            )

        if flags.Analysis.do_large_R_UFO_jets:
            log.info("Adding UFO large-R jet seq")
            configSeq += lr_jet_sequence(
                flags,
                lr_jet_type="UFO",
                configAcc=configAccumulator,
            )

        if flags.Analysis.do_large_R_RC_jets:
            log.info("Adding large-R RC jet seq")
            configSeq += rc_jet_sequence(
                flags,
                configAcc=configAccumulator,
            )

        if flags.Analysis.do_met:
            log.info("Adding MET seq")
            configSeq += met_sequence(flags, configAccumulator)

    ########################################################################
    # Begin postprocessing
    ########################################################################

    if flags.Analysis.do_overlap_removal:
        log.info("Adding Overlap Removal sequence")

        configSeq += overlap_sequence(flags)

    if not flags.Analysis.disable_calib and \
            ((flags.Input.isMC and flags.Analysis.Trigger.scale_factor.doSF)
                or flags.Analysis.Trigger.scale_factor.do_trigger_match):
        configSeq += triggerSF_sequence(flags)

    configSeq += thinning_sequence(flags)
    configSeq += selection_decoration_sequence(flags)

    configSeq.fullConfigure(configAccumulator)

    cfg.merge(configAccumulator.CA)

    return cfg
