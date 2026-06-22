from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator
from AthenaConfiguration.ComponentFactory import CompFactory
from itertools import chain
from AthenaConfiguration.Enums import LHCPeriod

from EasyjetHub.algs.postprocessing.SelectorAlgConfig import (
    ElectronSelectorAlgCfg, MuonSelectorAlgCfg, LeptonOrderingAlgCfg,
    JetSelectorAlgCfg, PhotonSelectorAlgCfg)

from EasyjetHub.output.ttree.selected_objects import (
    get_selected_obj_branches_variables)


def HZy_TriggerSFCfg(flags):
    from AnalysisAlgorithmsConfig.ConfigAccumulator import ConfigAccumulator
    from EasyjetHub.algs.calibration.triggerSF import triggerSF_sequence
    from EasyjetHub.steering.utils.name_helper import drop_sys

    cfg = ComponentAccumulator()
    configAccumulator = ConfigAccumulator(
        algSeq=CompFactory.AthSequencer("HZyTriggerSFSequence"),
        flags=flags,
    )
    trigSeq = triggerSF_sequence(flags)

    electron_container_sys = flags.Analysis.Trigger.scale_factor.Electron.container
    muon_container_sys = flags.Analysis.Trigger.scale_factor.Muon.container

    electron_container = drop_sys(electron_container_sys)
    muon_container = drop_sys(muon_container_sys)

    # The TriggerSF ConfigBlock is configured in a fresh ConfigAccumulator.
    # Therefore it does not know the HZy Z-candidate containers unless we
    # register them explicitly.
    configAccumulator.setSourceName(electron_container, electron_container_sys)
    configAccumulator.setSourceName(muon_container, muon_container_sys)
    # Override the default EasyJet selected lepton containers with the
    # HZy selected Z-candidate lepton containers.
    trigSeq.setOptionValue('.electrons', electron_container)
    trigSeq.setOptionValue('.muons', muon_container)

    trigSeq.fullConfigure(configAccumulator)
    cfg.merge(configAccumulator.CA)
    return cfg


def HZy_cfg(flags, smalljetkey, photonkey, electronkey, muonkey,
            float_variables=None, int_variables=None):
    keys = ["baseline", "photons", "leptons", "jets", "llcand"]
    if not float_variables:
        float_variables = {key: [] for key in keys}
    if not int_variables:
        int_variables = {key: [] for key in keys}

    cfg = ComponentAccumulator()

    cfg.merge(PhotonSelectorAlgCfg(flags,
                                   containerInKey=photonkey,
                                   containerOutKey="HZyAnalysisPhotons_%SYS%",
                                   minPt=flags.Analysis.Photon.min_pT,
                                   maxEta=flags.Analysis.Photon.max_eta))

    cfg.merge(MuonSelectorAlgCfg(flags,
                                 containerInKey=muonkey,
                                 containerOutKey="HZyAnalysisMuons_%SYS%",
                                 minPt=flags.Analysis.Muon.min_pT,
                                 maxEta=flags.Analysis.Muon.max_eta))

    cfg.merge(ElectronSelectorAlgCfg(flags,
                                     containerInKey=electronkey,
                                     containerOutKey="HZyAnalysisElectrons_%SYS%",
                                     minPt=flags.Analysis.Electron.min_pT,
                                     maxEta=flags.Analysis.Electron.max_eta))

    cfg.merge(LeptonOrderingAlgCfg(flags,
                                   containerInEleKey=electronkey,
                                   containerInMuKey=muonkey))

    cfg.merge(JetSelectorAlgCfg(flags,
                                containerInKey=smalljetkey,
                                containerOutKey="HZyAnalysisJets_%SYS%",
                                minPt=flags.Analysis.Small_R_jet.min_pT,
                                pTsort=True,
                                bTagWPDecorName="",
                                selectBjet=False))

    # selection algorithm added by "addEventAlgo"
    selection_name = flags.Analysis.selection_name

    # HZy: lepton overlap removal algorithm before Z reconstruction
    if flags.Analysis.HZy_OR.do_lepton_overlap_removal:
        cfg.addEventAlgo(
            CompFactory.HZY.HZyLeptonsORAlg(
                "HZyLeptonsORAlg",
                dEta_ElEl=flags.Analysis.HZy_OR.lep_thr.dEta_ElEl,
                dPhi_ElEl=flags.Analysis.HZy_OR.lep_thr.dPhi_ElEl,
                dR_El_StacoMu=flags.Analysis.HZy_OR.lep_thr.dR_El_StacoMu,
                dR_StacoMu_StacoMu=flags.Analysis.HZy_OR.lep_thr.dR_StacoMu_StacoMu,
                dR_CaloMu_StacoMu=flags.Analysis.HZy_OR.lep_thr.dR_CaloMu_StacoMu,
                dR_CaloMu_Electron=flags.Analysis.HZy_OR.lep_thr.dR_CaloMu_Electron,
                doSystematics=flags.Analysis.do_CP_systematics,
            )
        )

    # Egamma Calibration Tool for FSRCorrectionTool and ZMassConstraintTool
    is_run3 = flags.GeoModel.Run is LHCPeriod.Run3
    egamma_es_model = "es2024_Run3_v0" if is_run3 else "es2023_R22_Run2_v1"
    egCalibTool = CompFactory.CP.EgammaCalibrationAndSmearingTool(
        "CP::EgammaCalibrationAndSmearingTool",
        useFastSim=0,
        decorrelationModel="1NP_v1",
        ESModel=egamma_es_model,
    )
    cfg.addPublicTool(egCalibTool)

    # Muon Calibration Tool for ZMassConstraintTool
    muonCalibTool = CompFactory.CP.MuonCalibTool(
        "MuonCalibTool",
        IsRun3Geo=is_run3,
        release="Recs2025_12_11_Run2Run3",
        calibMode=3 if flags.Input.isMC else 0,
    )
    cfg.addPublicTool(muonCalibTool)

    # FSR Tool needing the Egamma Calibration Tool
    # to recalibrate the electrons as photons
    fsrTool = CompFactory.FSR.FsrPhotonTool(
        "FsrPhotonTool",
        egCalibToolName=egCalibTool.name,
    )
    cfg.addPublicTool(fsrTool)

    cfg.addEventAlgo(
        CompFactory.HZY.LeptonVarsHZyAlg(
            "LeptonVarsHZyAlg",
            isMC=flags.Input.isMC,
            FsrPhotonTool=fsrTool,
            EgammaCalibTool=egCalibTool,
            MuonCalibTool=muonCalibTool,
            useConstrainedZMass=flags.Analysis.useConstrainedZMass,
            mllWindowInfsr=flags.Analysis.fsr_selections.mll_window_infsr,
            mllFsrThreshold=flags.Analysis.fsr_selections.mll_fsr_threshold,
            floatVariableList=float_variables['leptons'],
            intVariableList=int_variables['leptons'],
            doSystematics=flags.Analysis.do_CP_systematics,
        )
    )

    # Trigger matching / trigger SF using the selected Z-candidate leptons.
    # This must run after LeptonVarsHZyAlg creates:
    #   HZyElectronsOR_ZCand_%SYS% and HZyMuonsOR_ZCand_%SYS%
    # and before HZySelectorAlg reads:
    #   globalTriggerMatch_%SYS%
    cfg.merge(HZy_TriggerSFCfg(flags))

    cfg.addEventAlgo(
        CompFactory.HZY.PhotonVarsHZyAlg(
            "PhotonVarsHZyAlg",
            isMC=flags.Input.isMC,
            dR_Ph_El=flags.Analysis.HZy_OR.phjet_thr.dR_Ph_El,
            dR_Ph_CaloMu=flags.Analysis.HZy_OR.phjet_thr.dR_Ph_CaloMu,
            dR_Ph_StacoMu=flags.Analysis.HZy_OR.phjet_thr.dR_Ph_StacoMu,
            dR_Jet_Ph=flags.Analysis.HZy_OR.phjet_thr.dR_Jet_Ph,
            dR_Jet_El=flags.Analysis.HZy_OR.phjet_thr.dR_Jet_El,
            dR_Jet_CaloMu=flags.Analysis.HZy_OR.phjet_thr.dR_Jet_CaloMu,
            dR_Jet_StacoMu=flags.Analysis.HZy_OR.phjet_thr.dR_Jet_StacoMu,
            floatVariableList=float_variables['photons'],
            intVariableList=int_variables['photons'],
            doSystematics=flags.Analysis.do_CP_systematics,
        )
    )

    cfg.addEventAlgo(
        CompFactory.HZY.BaselineVarsHZyAlg(
            "BaselineVarsHZyAlg",
            floatVariableList=float_variables['baseline'],
            floatVariableList_leptons=float_variables['leptons'],
            doSystematics=flags.Analysis.do_CP_systematics,
        )
    )

    cfg.addEventAlgo(
        CompFactory.HZY.HZySelectorAlg(
            "HZySelectorAlg",
            eventDecisionOutputDecoration=f"HZy_pass_{selection_name}_%SYS%",
            cutList=flags.Analysis.CutList,
            saveCutFlow=flags.Analysis.save_cutflow,
            isMC=flags.Input.isMC,
            triggerLists=flags.Analysis.TriggerChains,
            max_mll_veto_hmumu=flags.Analysis.mass_relpt_cuts.max_mll_veto_hmumu,
            mll_cstr_window=flags.Analysis.mass_relpt_cuts.mll_cstr_window,
            mllg_cstr_window=flags.Analysis.mass_relpt_cuts.mllg_cstr_window,
            photon_relpt_min=flags.Analysis.mass_relpt_cuts.photon_relpt_min,
            floatVariableList_leptons=float_variables['leptons'],
            intVariableList_leptons=int_variables['leptons'],
            intVariableList_photons=int_variables['photons'],
            floatVariableList_baseline=float_variables['baseline'],
            bypass=flags.Analysis.bypass
        )
    )

    cfg.addEventAlgo(
        CompFactory.HZY.JetVarsHZyAlg(
            "JetVarsHZyAlg",
            floatVariableList=float_variables['jets'],
            intVariableList=int_variables['jets'],
            doSystematics=flags.Analysis.do_CP_systematics,
        )
    )

    return cfg


def get_PhotonVarsHZyAlg_variables(flags):
    float_variable_names = [
        "Photon_effSF", "Ph_Pt_Raw", "Ph_Eta_Raw", "Ph_Phi_Raw",
    ]
    int_variable_names = [
        "nPhotons_LIdNIso", "nPhotons_OR_LIdNIso", "nPhotons_OR_TIdNIso", 
        "nPhotons_OR_TIdLIso", "Ph_ConvType", "Ph_isEMTight"]

    return float_variable_names, int_variable_names


def get_LeptonVarsHZyAlg_variables(flags):
    float_variable_names = [
        "LepLead_effSF", "LepSub_effSF", "Muon_Fsr_dR", "LepLepZ_M_Fsr",
    ]
    for suffix in ["Raw", "Cstr"]:
        for obj in ["LepLepZ", "LepLead", "LepSub"]:
            for var in ["M", "Pt", "Eta", "Phi"]:
                float_variable_names.append(f"{obj}_{var}_{suffix}")

    int_variable_names = [
        "Lep_Channel", "LepLepZ_Size", "Muon_hasFsr", "Muon_Index_Fsr",
        "nElectrons_LIdNIso", "nMuons_MIdNIso", 
        "nElectrons_LIdLIso", "nMuons_MIdLIso"]

    return float_variable_names, int_variable_names


def get_JetVarsHZyAlg_variables(flags):
    float_variable_names = ["Mjj"]
    int_variable_names = ["nJets"]

    return float_variable_names, int_variable_names


def get_BaselineVarsHZyAlg_variables(flags):
    float_variable_names = [
        "BeamSpotWeight",
        "MC_Weight",
        "LLGam_M_Fsr",
    ]

    for obj in ["LLGam"]:
        for suffix in ["Raw", "Cstr"]:
            for var in ["M", "Pt", "Eta", "Phi"]:
                float_variable_names.append(f"{obj}_{var}_{suffix}")

    float_variable_names += [
        "LLGam_RelPhpT_Cstr",
    ]
    int_variable_names = []

    return float_variable_names, int_variable_names


def HZy_branches(flags):
    # a list of strings which maps the variable name in the c++ code
    # to the outputname:
    # "EventInfo.<var> -> <outputvarname>"
    branches = []

    # this will be all the variables that are calculated by the
    # BaselineVarsAlg algorithm
    all_baseline_variable_names = []
    keys = ["baseline", "photons", "leptons", "jets"]
    float_variable_names = {key: [] for key in keys}
    int_variable_names = {key: [] for key in keys}

    # these are the variables that will always be stored by easyjet specific to analysis
    # further below there are more high level variables
    # which can be stored using the flag
    # flags.Analysis.store_high_level_variables
    baseline_float_variables, baseline_int_variables \
        = get_BaselineVarsHZyAlg_variables(flags)
    photon_float_variables, photon_int_variables \
        = get_PhotonVarsHZyAlg_variables(flags)
    lepton_float_variables, lepton_int_variables \
        = get_LeptonVarsHZyAlg_variables(flags)
    jet_float_variables, jet_int_variables \
        = get_JetVarsHZyAlg_variables(flags)

    float_variable_names['baseline'] += baseline_float_variables
    int_variable_names['baseline'] += baseline_int_variables
    float_variable_names['photons'] += photon_float_variables
    int_variable_names['photons'] += photon_int_variables
    float_variable_names['leptons'] += lepton_float_variables
    int_variable_names['leptons'] += lepton_int_variables
    float_variable_names['jets'] += jet_float_variables
    int_variable_names['jets'] += jet_int_variables

    all_baseline_variable_names += [*chain.from_iterable(float_variable_names.values()),
                                    *chain.from_iterable(int_variable_names.values())]

    if not flags.Analysis.do_CP_systematics:
        sys_suffix = "NOSYS"
    else:
        sys_suffix = "%SYS%"

    for var in all_baseline_variable_names:
        branches += [f"EventInfo.{var}_{sys_suffix} -> HZy_{var}"
                     + flags.Analysis.systematics_suffix_separator + sys_suffix]

    # These are the variables always saved with the objects selected by the analysis
    # This is tunable with the flags amount and variables in the object configs.
    object_level_branches = {key: [] for key in keys}
    object_level_float_variables = {key: [] for key in keys}
    object_level_int_variables = {key: [] for key in keys}

    (object_level_branches['photons'],
     object_level_float_variables['photons'],
     object_level_int_variables['photons']) = \
        get_selected_obj_branches_variables(flags, "HZy", "Photon")

    float_variable_names['photons'] += object_level_float_variables['photons']
    int_variable_names['photons'] += object_level_int_variables["photons"]

    (object_level_branches['leptons'],
     object_level_float_variables['leptons'],
     object_level_int_variables['leptons']) = \
        get_selected_obj_branches_variables(flags, "HZy", "Lepton")

    float_variable_names['leptons'] += object_level_float_variables['leptons']
    int_variable_names['leptons'] += object_level_int_variables['leptons']

    (object_level_branches['jets'],
     object_level_float_variables['jets'],
     object_level_int_variables['jets']) = \
        get_selected_obj_branches_variables(flags, "HZy", "Jet")

    float_variable_names['jets'] += object_level_float_variables['jets']
    int_variable_names['jets'] += object_level_int_variables['jets']

    branches += object_level_branches["leptons"]
    branches += object_level_branches["photons"]
    branches += object_level_branches["jets"]

    s_name = flags.Analysis.selection_name
    branches += [f"EventInfo.Pass_{s_name}_{sys_suffix} -> HZy_pass_{s_name}"
                 + flags.Analysis.systematics_suffix_separator + sys_suffix]

    # ---------------------------------------------------------
    # Save per-event cut flags from HZySelectorAlg (EventInfo decors)
    # ---------------------------------------------------------
    if (flags.Analysis.save_cutflow):
        cutList = flags.Analysis.CutList
        for cut in cutList:
            branches += [f"EventInfo.{cut}_{sys_suffix} -> CUT_{cut}"
                         + flags.Analysis.systematics_suffix_separator + sys_suffix]

    return branches, float_variable_names, int_variable_names
