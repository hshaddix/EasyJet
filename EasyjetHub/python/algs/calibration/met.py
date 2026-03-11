# Copyright (C) 2002-2025 CERN for the benefit of the ATLAS collaboration

from AnalysisAlgorithmsConfig.ConfigSequence import ConfigSequence
from AnalysisAlgorithmsConfig.ConfigFactory import ConfigFactory

from EasyjetHub.steering.utils.name_helper import drop_sys


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
