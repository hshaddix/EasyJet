# Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration

from AnalysisAlgorithmsConfig.ConfigBlock import ConfigBlock


class BJRSystPropagationConfig(ConfigBlock):
    """ConfigBlock for Easyjet::BJRSystPropagationAlg."""

    def __init__(self):
        super(BJRSystPropagationConfig, self).__init__()
        self.setBlockName('BJRSystPropagation')
        self.addOption('containerName', '', type=str,
                       noneAction='error',
                       info="the name of the input large-R jet container.")

    def makeAlgs(self, config):
        alg = config.createAlgorithm(
            'Easyjet::BJRSystPropagationAlg', 'BJRSystPropagationAlg')

        alg.largeRJets = config.readName(self.containerName)
        alg.nominalLargeRJets = config.readName(self.containerName,
                                                nominal=True)
