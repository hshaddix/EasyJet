# Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration

from AnalysisAlgorithmsConfig.ConfigBlock import ConfigBlock


class BJRKinematicShiftConfig(ConfigBlock):
    """ConfigBlock for Easyjet::BJRKinematicShiftSystAlg."""

    def __init__(self):
        super(BJRKinematicShiftConfig, self).__init__()
        self.setBlockName('BJRKinematicShift')
        self.addOption('containerName', '', type=str,
                       noneAction='error',
                       info="the name of the input large-R jet container.")

    def makeAlgs(self, config):
        alg = config.createAlgorithm(
            'Easyjet::BJRKinematicShiftSystAlg',
            'BJRKinematicShiftSystAlg')

        alg.jets = config.readName(self.containerName)
        alg.jetsOut = config.copyName(self.containerName)
