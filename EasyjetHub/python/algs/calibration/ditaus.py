from AnalysisAlgorithmsConfig.ConfigSequence import ConfigSequence
from AnalysisAlgorithmsConfig.ConfigFactory import ConfigFactory

from EasyjetHub.steering.utils.name_helper import drop_sys


def ditau_sequence(flags, configAcc):

    configSeq = ConfigSequence()
    config = ConfigFactory()
    makeConfig = config.makeConfig

    output_name = drop_sys(flags.Analysis.container_names.output.ditaus)

    configSeq += makeConfig('DiTauJets',)
    configSeq.setOptionValue('.containerName', output_name)
    configSeq.setOptionValue('.decorateTruth', True)

    # Kinematic selection
    configSeq += makeConfig('DiTauJets.PtEtaSelection')
    configSeq.setOptionValue('.containerName', output_name)
    configSeq.setOptionValue('.selectionName', 'selectPtEta')
    configSeq.setOptionValue('.selectionDecoration', 'selectPtEta')
    configSeq.setOptionValue('.minPt', 20e3)
    configSeq.setOptionValue('.maxEta', 2.5)

    # Add systematic object links
    configSeq += makeConfig('SystObjectLink')
    configSeq.setOptionValue('.containerName', output_name)

    configSeq += makeConfig('Thinning')
    configSeq.setOptionValue('.containerName', output_name)
    configSeq.setOptionValue('.selectionName', 'selectPtEta')

    return configSeq
