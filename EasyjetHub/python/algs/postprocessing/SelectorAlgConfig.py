from AthenaCommon.Logging import logging
from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator
from AthenaConfiguration.ComponentFactory import CompFactory
from AthenaConfiguration.Enums import LHCPeriod
from EasyjetHub.steering.analysis_configuration import get_trigger_chains_scale_factor
from EasyjetHub.output.ttree.electrons import get_electron_branches
from EasyjetHub.output.ttree.muons import get_muon_branches
from EasyjetHub.output.ttree.photons import get_photon_branches
from EasyjetHub.output.ttree.taus import get_tau_branches
from EasyjetHub.output.ttree.small_R_jets import get_small_R_jet_branches
from EasyjetHub.output.ttree.large_R_jets import get_large_R_jet_branches


log = logging.getLogger(__name__)


def get_sys_decorations(flags, branch_getter, **kwargs):
    """%SYS% decorations the ntuple writes for this object type.

    Selector outputs are views of the input objects, but the systematics
    service only knows which variations affect a decoration on the container
    it was registered for. The selectors re-register these on their output
    (propagateDecorSys), so they also resolve correctly when read through it.
    """
    if not hasattr(flags.Analysis, "ttree_output"):
        return []
    try:
        branches = branch_getter(
            flags,
            flags.Analysis.ttree_output,
            input_container="Selected_%SYS%",
            output_prefix="selected",
            **kwargs,
        )
    except (AttributeError, KeyError, RuntimeError) as err:
        log.warning(
            "Not propagating decoration systematics for %s: %s",
            branch_getter.__name__,
            err,
        )
        return []
    decorations = []
    for branch in branches:
        decoration = branch.split(" -> ")[0].split(".", 1)[1]
        if "%SYS%" in decoration and decoration not in decorations:
            decorations.append(decoration)
    return decorations


def MuonSelectorAlgCfg(flags, name="MuonSelectorAlg", **kwargs):
    cfg = ComponentAccumulator()

    muon_WPs = [f'{flags.Analysis.Muon.ID}_{flags.Analysis.Muon.Iso}']
    consider_extra_wp = any(len(wp) == 4 for wp in flags.Analysis.Muon.extra_wps)

    extra_iso_suffixes = list(flags.Analysis.Muon.extra_iso_suffixes or [])
    muon_WPs += [f'{flags.Analysis.Muon.ID}_{flags.Analysis.Muon.Iso}_{s}'
                 for s in extra_iso_suffixes]

    for wp in flags.Analysis.Muon.extra_wps:
        if len(wp) == 4:
            muon_WPs += [f'{wp[0]}_{wp[1]}_{wp[2]}_{wp[3]}'.replace('.', 'p')]
            muon_WPs += [f'{wp[0]}_{wp[1]}_{s}_{wp[2]}_{wp[3]}'.replace('.', 'p')
                         for s in extra_iso_suffixes]
        elif consider_extra_wp:
            d0 = flags.Analysis.Muon.maxD0Significance
            z0 = flags.Analysis.Muon.maxDeltaZ0SinTheta
            muon_WPs += [f'{wp[0]}_{wp[1]}_{d0}_{z0}'.replace('.', 'p')]
            muon_WPs += [f'{wp[0]}_{wp[1]}_{s}_{d0}_{z0}'.replace('.', 'p') for s in extra_iso_suffixes]
        else:
            muon_WPs += [f'{wp[0]}_{wp[1]}']
            muon_WPs += [f'{wp[0]}_{wp[1]}_{s}' for s in extra_iso_suffixes]
    kwargs.setdefault("muonWPs", muon_WPs)
    kwargs.setdefault("muonAmount", flags.Analysis.Muon.amount)

    if not flags.Analysis.Muon.do_thinning:
        kwargs.setdefault("baselineSelectionName", "")

    kwargs.setdefault(
        "propagateDecorSys", get_sys_decorations(flags, get_muon_branches)
    )

    cfg.addEventAlgo(CompFactory.Easyjet.MuonSelectorAlg(name, **kwargs))
    return cfg


def ElectronSelectorAlgCfg(flags, name="ElectronSelectorAlg", **kwargs):
    cfg = ComponentAccumulator()

    ele_WPs = [f'{flags.Analysis.Electron.ID}_{flags.Analysis.Electron.Iso}']
    ele_WPs += [f'{wp[0]}_{wp[1]}' for wp in flags.Analysis.Electron.extra_wps]
    kwargs.setdefault("eleWPs", ele_WPs)
    kwargs.setdefault("electronAmount", flags.Analysis.Electron.amount)

    if not flags.Analysis.Electron.do_thinning:
        kwargs.setdefault("baselineSelectionName", "")

    kwargs.setdefault(
        "propagateDecorSys", get_sys_decorations(flags, get_electron_branches)
    )

    cfg.addEventAlgo(CompFactory.Easyjet.ElectronSelectorAlg(name, **kwargs))
    return cfg


def TruthElectronSelectorAlgCfg(flags, name="TruthElectronSelectorAlg", **kwargs):
    cfg = ComponentAccumulator()

    kwargs.setdefault("decoration",
                      flags.Analysis.container_names.input.truthelectrons
                      + ".isTruthElectron")
    kwargs.setdefault("decorOutName", "TruthEvents.nElectrons")

    cfg.addEventAlgo(CompFactory.Easyjet.TruthElectronSelectorAlg(name, **kwargs))
    return cfg


def TruthMuonSelectorAlgCfg(flags, name="TruthMuonSelectorAlg", **kwargs):
    cfg = ComponentAccumulator()

    kwargs.setdefault("decoration", 
                      flags.Analysis.container_names.input.truthmuons
                      + ".isTruthMuon")
    kwargs.setdefault("decorOutName", "TruthEvents.nMuons")

    cfg.addEventAlgo(CompFactory.Easyjet.TruthMuonSelectorAlg(name, **kwargs))
    return cfg


def LeptonOrderingAlgCfg(flags, name="LeptonOrderingAlg", **kwargs):
    cfg = ComponentAccumulator()

    kwargs.setdefault("leptonAmount", flags.Analysis.Lepton.amount)

    cfg.addEventAlgo(CompFactory.Easyjet.LeptonOrderingAlg(name, **kwargs))
    return cfg


def TruthLeptonOrderingAlgCfg(flags, name="TruthLeptonOrderingAlg", **kwargs):
    cfg = ComponentAccumulator()

    kwargs.setdefault("leptonAmount", flags.Analysis.Lepton.amount)
    kwargs.setdefault("isTruthElectronDecoration",
                      flags.Analysis.container_names.input.truthelectrons
                      + ".isTruthElectron")
    kwargs.setdefault("isTruthMuonDecoration",
                      flags.Analysis.container_names.input.truthmuons
                      + ".isTruthMuon")

    cfg.addEventAlgo(CompFactory.Easyjet.TruthLeptonOrderingAlg(name, **kwargs))
    return cfg


def TauSelectorAlgCfg(flags, name="TauSelectorAlg", **kwargs):
    cfg = ComponentAccumulator()

    tau_WPs = [flags.Analysis.Tau.ID]
    tau_WPs += flags.Analysis.Tau.extra_wps
    kwargs.setdefault("tauWPs", tau_WPs)
    kwargs.setdefault("keepAntiTaus", 
                      flags.Analysis.OverlapRemoval.doTauAntiTauJet)
    kwargs.setdefault("tauAmount", flags.Analysis.Tau.amount)

    if not flags.Analysis.Tau.do_thinning:
        kwargs.setdefault("baselineSelectionName", "")

    kwargs.setdefault("propagateDecorSys", get_sys_decorations(flags, get_tau_branches))

    cfg.addEventAlgo(CompFactory.Easyjet.TauSelectorAlg(name, **kwargs))
    return cfg


def PhotonSelectorAlgCfg(flags, name="PhotonSelectorAlg", **kwargs):
    cfg = ComponentAccumulator()

    ph_WPs = [f'{flags.Analysis.Photon.ID}_{flags.Analysis.Photon.Iso}']
    ph_WPs += [f'{wp[0]}_{wp[1]}' for wp in flags.Analysis.Photon.extra_wps]
    kwargs.setdefault("photonWPs", ph_WPs)
    kwargs.setdefault("photonAmount", flags.Analysis.Photon.amount)

    if not flags.Analysis.Photon.do_thinning:
        kwargs.setdefault("baselineSelectionName", "")

    kwargs.setdefault(
        "propagateDecorSys", get_sys_decorations(flags, get_photon_branches)
    )

    cfg.addEventAlgo(CompFactory.Easyjet.PhotonSelectorAlg(name, **kwargs))
    return cfg


def JetSelectorAlgCfg(flags, name="JetSelectorAlg", **kwargs):
    cfg = ComponentAccumulator()

    isSmallRJet = "AntiKt4" in kwargs["containerInKey"]
    isLargeRJet = "AntiKt10" in kwargs["containerInKey"]
    smallR_flags = flags.Analysis.Small_R_jet
    largeR_flags = flags.Analysis.Large_R_jet

    if kwargs.get("bTagWPDecorName", ""):
        kwargs.setdefault("bJetAmount", smallR_flags.amount_bjet)
    if (isSmallRJet and smallR_flags.runBJetPtCalib) or \
        (isLargeRJet and largeR_flags.runMuonJetPtCorr):
        kwargs.setdefault("nmuons", "n_muons_%SYS%")

    # b-jet matching decoration
    if isSmallRJet:
        bjet_triggers = []
        sf_config = flags.Analysis.Trigger.scale_factor
        if sf_config.doSF and hasattr(sf_config, 'bjet'):
            triggerChainsPerYear = get_trigger_chains_scale_factor(
                flags, 'bjet')
            for triggerChains in triggerChainsPerYear.values():
                for chain in triggerChains:
                    bjet_triggers.append(chain.replace('-', '_').replace('.', 'p'))
        kwargs.setdefault("bJetTriggerLists", bjet_triggers)

        jet_triggers = []
        sf_config = flags.Analysis.Trigger.scale_factor
        if sf_config.doSF and hasattr(sf_config, 'jet'):
            triggerChainsPerYear = get_trigger_chains_scale_factor(
                flags, 'jet')
            for triggerChains in triggerChainsPerYear.values():
                for chain in triggerChains:
                    jet_triggers.append(chain.replace('-', '_').replace('.', 'p'))
        kwargs.setdefault("jetTriggerLists", jet_triggers)
        kwargs.setdefault("doL1Matching", smallR_flags.doL1Matching)
        kwargs.setdefault("doHLTMatching", smallR_flags.doHLTMatching and (
            flags.GeoModel.Run >= LHCPeriod.Run3 or smallR_flags.doRun2HLTEmulation))

        kwargs.setdefault(
            "propagateDecorSys", get_sys_decorations(flags, get_small_R_jet_branches)
        )

    if isLargeRJet:
        if not largeR_flags.do_thinning:
            kwargs.setdefault("baselineSelectionName", "")
        kwargs.setdefault("hbbWPDecorName", largeR_flags.hbb_tag_wp)
        kwargs.setdefault("httWPDecorName", largeR_flags.htt_tag_wp)
        kwargs.setdefault("hbbJetAmount", largeR_flags.amount_hbb_leadingjet)
        kwargs.setdefault("httJetAmount", largeR_flags.amount_htt_leadingjet)

        lr_jet_type = "UFO" if flags.Analysis.do_large_R_UFO_jets else "Topo"
        kwargs.setdefault(
            "propagateDecorSys",
            get_sys_decorations(
                flags, get_large_R_jet_branches, lr_jet_type=lr_jet_type
            ),
        )

    cfg.addEventAlgo(CompFactory.Easyjet.JetSelectorAlg(name, **kwargs))
    return cfg


def TruthJetSelectorAlgCfg(flags, name="TruthJetSelectorAlg", **kwargs):
    cfg = ComponentAccumulator()

    kwargs.setdefault("bjetAmount", flags.Analysis.Small_R_jet.amount_bjet)
    kwargs.setdefault("cjetAmount", flags.Analysis.Small_R_jet.amount_cjet)

    cfg.addEventAlgo(CompFactory.Easyjet.TruthJetSelectorAlg(name, **kwargs))
    return cfg
