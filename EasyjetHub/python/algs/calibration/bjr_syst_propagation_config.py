# Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration

from AnalysisAlgorithmsConfig.ConfigBlock import ConfigBlock


class BJRSystPropagationConfig(ConfigBlock):
    """ConfigBlock for the BJR systematic-propagation algorithm.

    The bJR10v01 mass/pT are produced by a GNN regression that runs
    nominal-only in the derivation, so they do not move under CP
    systematics. This block schedules ``Easyjet::BJRSystPropagationAlg``,
    which analytically propagates the JES/JMS/JER shift of the calibrated
    UFO four-vector onto the BJR mass/pT per systematic:

        bjr_mass_sys = bjr_mass_nom * (ufo_mass_sys / ufo_mass_nom)
        bjr_pt_sys   = bjr_pt_nom   * (ufo_pt_sys   / ufo_pt_nom)

    It must run after large-R calibration but before any selection /
    pT-reordering, so the per-systematic and nominal jet containers are
    still index-aligned shallow copies.
    """

    def __init__(self):
        super(BJRSystPropagationConfig, self).__init__()
        self.setBlockName('BJRSystPropagation')
        self.addOption('containerName', '', type=str,
                       noneAction='error',
                       info="the name of the input large-R jet container.")

    def makeAlgs(self, config):
        alg = config.createAlgorithm(
            'Easyjet::BJRSystPropagationAlg', 'BJRSystPropagationAlg')

        # Per-systematic calibrated UFO jets (tracked container name).
        alg.largeRJets = config.readName(self.containerName)
        # Nominal (NOSYS) calibrated UFO jets, used as the denominator
        # reference for the multiplicative propagation.
        alg.nominalLargeRJets = config.readName(self.containerName,
                                                nominal=True)

        # NOTE: no output variables are declared here. The alg OVERWRITES the
        # existing bare "bJR10v01_mass" / "bJR10v01_pt" aux in place on each
        # per-systematic jet shallow copy. Any downstream SystVariableCopier
        # that reads these bare names (e.g. the Zbbj calibration's snapshot into
        # Zcand0/1_bJR10v01_mass) then picks up the propagated variation per
        # systematic with no further output wiring. (A TTree path that reads the
        # bare aux with a NOSYS container forced by syst_only_for=["pt","m"]
        # would still need separate work to make its branches systematic-aware;
        # that is out of scope here.)
