from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator
from AthenaConfiguration.ComponentFactory import CompFactory

from EasyjetHub.algs.postprocessing.SelectorAlgConfig import (
    MuonSelectorAlgCfg, ElectronSelectorAlgCfg, LeptonOrderingAlgCfg,
    JetSelectorAlgCfg)
from EasyjetHub.steering.analysis_configuration import (
    get_trigger_legs_scale_factor_list)

from vbssemilepAnalysis.twoLep_config import twoLep_cfg, twoLep_branches
from vbssemilepAnalysis.oneLep_config import oneLep_cfg, oneLep_branches

import AthenaCommon.SystemOfUnits as Units


def vbssemilep_cfg(flags, smalljetkey, largejetkey, muonkey, electronkey):

    cfg = ComponentAccumulator()

    cfg.merge(MuonSelectorAlgCfg(flags,
                                 containerInKey=muonkey,
                                 containerOutKey="vbssemilepAnalysisMuons_%SYS%",
                                 minPt=0 * Units.GeV))
                                 #minPt=9 * Units.GeV))

    cfg.merge(ElectronSelectorAlgCfg(flags,
                                     containerInKey=electronkey,
                                     containerOutKey="vbssemilepAnalysisElectrons_%SYS%",
                                     minPt=0 * Units.GeV))
                                     #minPt=9 * Units.GeV))

    cfg.merge(LeptonOrderingAlgCfg(flags,
                                   containerInEleKey=electronkey,
                                   containerInMuKey=muonkey))

    cfg.merge(JetSelectorAlgCfg(flags, name="SmallJetSelectorAlg",
                                containerInKey=smalljetkey,
                                containerOutKey="vbssemilepAnalysisJets_%SYS%",
                                bTagWPDecorName="",
                                selectBjet=False,
                                minPt=0 * Units.GeV,
                                #minPt=20 * Units.GeV,
                                minimumAmount=2))  # -1 means ignores this

    cfg.merge(JetSelectorAlgCfg(flags,
                                containerInKey=largejetkey,
                                containerOutKey="vbssemilepAnalysisLargeJets_%SYS%",
                                minPt=0 * Units.GeV,
                                #minPt=190 * Units.GeV,
                                maxEta=2.7,
                                minMass=0 * Units.GeV,
                                #minMass=40 * Units.GeV,
                                minimumAmount=flags.Analysis.Large_R_jet.amount))

    if not flags.Analysis.UseVBFRNN:
        cfg.addEventAlgo(
            CompFactory.VBSSEMILEP.VBSJetsSelectorAlg(
                "VBSJetsSelectorAlg",
                bTagWPDecorName="ftag_select_" + flags.Analysis.Small_R_jet.btag_wp,
            )
        )

    if "twoLep" in flags.Analysis.channels:
        if flags.Analysis.do_resolved:
            # set jets container labels
            if not flags.Analysis.UseVBFRNN:
                SignalJetsLabel = "vbssemilepAnalysisSignalJets_%SYS%"
                VBSJetsLabel = "vbssemilepAnalysisVBSJets_%SYS%"
            else:
                vbftagger = CompFactory.VBFTagger("VBFTaggerTool", modelTag="VBFRNNv1p1p0")
                # use full small-R jets pool to select signal jets
                SignalJetsLabel = "vbssemilepAnalysisJets_%SYS%"
                cfg.addEventAlgo(
                    CompFactory.VBFTaggerAlgSys(
                        "VBFTaggerAlg_resolved",
                        VBFTagger=vbftagger,
                        containerAllJetsKey="vbssemilepAnalysisJets_%SYS%",
                        containerSigJetsKey=SignalJetsLabel,
                        pTCut=20.e3,
                        nMaxJets=2,
                        DecTag="_resolvedCategory"
                    )
                )
    
    if flags.Analysis.UseVBFRNN:
        vbftagger = CompFactory.VBFTagger("VBFTaggerTool", modelTag="VBFRNNv1p1p0")
        # VBF-RNN tagger: boosted
        cfg.addEventAlgo(
            CompFactory.VBFTaggerAlgSys(
                "VBFTaggerAlg_boosted",
                VBFTagger=vbftagger,
                containerAllJetsKey="vbssemilepAnalysisJets_%SYS%",
                containerSigLargeRJetsKey="vbssemilepAnalysisLargeJets_%SYS%",
                OnlyFirstLargeRJet=True,
                pTCut=20.e3,
                nMaxJets=2,
                DecTag="_boostedCategory"
            )
        )
                

    from EasyjetHub.algs.postprocessing.trigger_matching import TriggerMatchingToolCfg

    trigger_branches = [
        f"trigPassed_{c.replace('-', '_').replace('.', 'p')}"
        for c in flags.Analysis.TriggerChains
    ]

    # do trigger pass and matching for each lepton for single lepton triggers
    cfg.addEventAlgo(
        CompFactory.VBSSEMILEP.TriggerDecoratorAlg(
            "VBSSEMILEPTriggerDecoratorAlg",
            muons="vbssemilepAnalysisMuons_%SYS%",
            electrons="vbssemilepAnalysisElectrons_%SYS%",
            trigMatchingTool=cfg.popToolsAndMerge(TriggerMatchingToolCfg(flags)),
            jets=flags.Analysis.container_names.output.reco4PFlowJet,
            triggerLists=trigger_branches,
            eleTriggerSF=get_trigger_legs_scale_factor_list(flags, 'Electron'),
            saveHighLevelVariables=flags.Analysis.save_high_level_variables,
        )
    )

    # truth info
    if flags.Input.isMC and flags.Analysis.AddTruthVBSQuarks:
        cfg.addEventAlgo(
            CompFactory.VBSSEMILEP.TruthVBSQuarksInfoAlg(
                "TruthVBSQuarksInfoAlg",
                TruthParticleInKey="HardScatterParticles",
            )
        )

    if "twoLep" in flags.Analysis.channels:
        extra_vbssemilep_branches, float_variable_names, \
            int_variable_names = twoLep_branches(flags)
        cfg.merge(
            twoLep_cfg(
                flags,
                muonkey, electronkey,
                float_variables=float_variable_names,
                int_variables=int_variable_names)
        )

    if "oneLep" in flags.Analysis.channels:
        extra_vbssemilep_branches, float_variable_names, \
            int_variable_names = oneLep_branches(flags)
        cfg.merge(
            oneLep_cfg(
                flags,
                muonkey, electronkey,
                float_variables=float_variable_names,
                int_variables=int_variable_names)
        )

    return cfg
