# Copyright (C) 2002-2025 CERN for the benefit of the ATLAS collaboration

from AnalysisAlgorithmsConfig.ConfigSequence import ConfigSequence
from AnalysisAlgorithmsConfig.ConfigFactory import ConfigFactory

from EasyjetHub.algs.calibration.photons import photon_sequence
from EasyjetHub.steering.utils.name_helper import drop_sys

metnet_network_files = {
    2: "METUtilities/R22_PreRecs/metnet_run2_22Jan2026_patched.onnx",
    3: "METUtilities/R22_PreRecs/metnet_run3_22Jan2026_patched.onnx",
}


def get_preMET_collections(flags):
    """Collections (with WP selection) to be fed into the MET calculation"""
    container_names = flags.Analysis.container_names

    preMET_collections = {}
    objflags = {x: f'do_{x}' for x in ['electrons', 'photons', 'muons', 'taus']}
    METselections = dict(
        electrons=f'{flags.Analysis.Electron.ID}_{flags.Analysis.Electron.Iso}',
        photons=f'{flags.Analysis.Photon.ID}_{flags.Analysis.Photon.Iso}',
        muons=f'{flags.Analysis.Muon.ID}_{flags.Analysis.Muon.Iso}',
        taus=(
            flags.Analysis.Tau.extra_wps[0]
            if flags.Analysis.OverlapRemoval.doTauAntiTauJet
            else flags.Analysis.Tau.ID
        ),
    )
    # Construct the names of the view containers with working point selection
    # We need to use the '.' style so that the algs operate on the full
    # container, and avoid incomplete decorations
    for objtype, objflag in objflags.items():
        if flags.Analysis[objflag]:
            collname = drop_sys(container_names.output[objtype])
            selection = METselections[objtype]
            preMET_collections[objtype] = f'{collname}.{selection}'

    return preMET_collections


def met_sequence(flags, configAcc):

    assert flags.Analysis.do_small_R_jets, (
        "Small-R jets are strictly necessary for MET"
    )
    jet_flags = flags.Analysis.Small_R_jet

    configSeq = ConfigSequence()
    config = ConfigFactory()
    makeConfig = config.makeConfig
    met_flags = flags.Analysis.Met

    container_names = flags.Analysis.container_names

    preMET_collections = get_preMET_collections(flags)

    configSeq += makeConfig('MissingET')
    configSeq.setOptionValue('.containerName', drop_sys(container_names.output.met))
    configSeq.setOptionValue(
        ".useLRT",
        flags.Analysis.Muon.MergeLRT or flags.Analysis.Electron.MergeLRT)
    # Pass all the calibrated jets
    configSeq.setOptionValue(
        ".useJVT",
        flags.Analysis.Small_R_jet.jet_type != "reco4EMTopoJet"
    )
    configSeq.setOptionValue(
        '.jets',
        drop_sys(container_names.output[flags.Analysis.Small_R_jet.jet_type])
    )

    configSeq.setOptionValue(".metWP", met_flags.metWP)
    configSeq.setOptionValue(".saveSignificance", met_flags.saveSignificance)

    configSeq.setOptionValue(
        ".addExtraSignificanceVars",
        met_flags.addExtraSignificanceVars
    )

    # If building METSignificance should provide
    #   obj. calibrations to retrive their resolutions.
    if met_flags.saveSignificance:
        if jet_flags.calibToolConfigFile and jet_flags.calibToolCalibArea:
            configSeq.setOptionValue(
                ".JetCalibConfig",
                jet_flags.calibToolConfigFile
            )
            configSeq.setOptionValue(
                ".JetCalibArea",
                jet_flags.calibToolCalibArea
            )
        if jet_flags.calibToolCalibSeq:
            configSeq.setOptionValue(
                ".JetCalibSequence",
                jet_flags.calibToolCalibSeq
            )

    # Should turn this off if muons are not used in the analysis.
    if 'muons' not in preMET_collections.keys():
        configSeq.setOptionValue('.setMuonJetEMScale', False)

    # Add whatever collections are active in the job
    for objtype, coll in preMET_collections.items():
        configSeq.setOptionValue(f'.{objtype}', coll)

    return configSeq


def metnet_sequence(flags, configAcc):
    """Schedule the experimental METNet MET estimate alongside the
    standard MET, for direct comparison of the two.

    This drives the NN-based MET evaluation through the standard CP
    ``MissingET`` config block (``makeConfig('MissingET', useNN=True)``),
    provided by MetAnalysisAlgorithms in the analysis release. The network
    output is written as the ``metnet`` container's Final term (met/phi aux
    decorations), read out unchanged by ``get_metnet_branches`` into the
    ``metnet_met`` / ``metnet_phi`` branches.
    """

    assert flags.Analysis.do_small_R_jets, (
        "Small-R jets are strictly necessary for MET"
    )
    assert flags.Analysis.Small_R_jet.jet_type == "reco4PFlowJet", (
        "METNet only supports R=0.4 PFlow jets"
    )

    met_flags = flags.Analysis.Met
    container_names = flags.Analysis.container_names

    network_file = met_flags.METNetFile
    if not network_file:
        if flags.Analysis.Run not in metnet_network_files:
            raise ValueError(
                f"No METNet model available for Run {flags.Analysis.Run}, "
                "please provide one via Met.METNetFile"
            )
        network_file = metnet_network_files[flags.Analysis.Run]

    configSeq = ConfigSequence()
    config = ConfigFactory()
    makeConfig = config.makeConfig

    # Use the same input objects as the standard MET. The NN evaluation
    # requires a configured container for every hard-object type, so if the
    # analysis does not otherwise use photons, calibrate them here purely as
    # METNet input (Photon.ID / Photon.Iso select the working point).
    preMET_collections = get_preMET_collections(flags)
    if 'photons' not in preMET_collections:
        assert flags.Analysis.Photon.ID and flags.Analysis.Photon.Iso, (
            "METNet requires a photon term: enable do_photons or set "
            "Photon.ID and Photon.Iso for the MET-input photon selection"
        )
        configSeq += photon_sequence(flags, configAcc)
        photon_wp = f"{flags.Analysis.Photon.ID}_{flags.Analysis.Photon.Iso}"
        preMET_collections['photons'] = (
            f"{drop_sys(container_names.output.photons)}.{photon_wp}"
        )
    missing = {'electrons', 'muons', 'taus'} - preMET_collections.keys()
    assert not missing, (
        f"METNet requires every hard-object type, missing: {sorted(missing)}"
    )

    configSeq += makeConfig('MissingET')
    configSeq.setOptionValue(
        '.containerName', drop_sys(container_names.output.metnet))
    configSeq.setOptionValue('.useNN', True)
    configSeq.setOptionValue('.networkFile', network_file)
    configSeq.setOptionValue(
        '.jets',
        drop_sys(container_names.output[flags.Analysis.Small_R_jet.jet_type])
    )
    # METNet is R=0.4 PFlow only, so JVT applies
    configSeq.setOptionValue('.useJVT', True)

    for objtype, coll in preMET_collections.items():
        configSeq.setOptionValue(f'.{objtype}', coll)

    return configSeq
