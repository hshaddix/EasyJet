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


def HZy_JvtEventSFCfg(flags):
    """Compute event-level JVT SF on HZyJetsOR_NoJvt (after HZy OR, before JVT cut).

    This container includes both JVT-passing and JVT-failing jets,
    and excludes only jets removed by HZy custom OR. Using 'no_jvt'
    as preselection ensures only jets within the efficiency tool's
    validity range contribute to the product.
    """
    cfg = ComponentAccumulator()

    if not flags.Input.isMC:
        return cfg

    cfg.addEventAlgo(
        CompFactory.CP.AsgEventScaleFactorAlg(
            "HZyJvtEventScaleFactorAlg",
            particles="HZyJetsOR_NoJvt_%SYS%",
            preselection="no_jvt",
            scaleFactorInputDecoration="jvt_effSF_%SYS%",
            scaleFactorOutputDecoration="hzy_jvt_effSF_%SYS%",
        )
    )
    return cfg


def HZy_cfg(flags, smalljetkey, photonkey, electronkey, muonkey,
            float_variables=None, int_variables=None):
    keys = ["baseline", "photons", "leptons", "jets", "llcand", "truth"]
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

    doJvtSel = getattr(flags.Analysis.Small_R_jet, 'doJvtSelection', True)
    cfg.merge(JetSelectorAlgCfg(flags,
                                containerInKey=smalljetkey,
                                containerOutKey="HZyAnalysisJets_%SYS%",
                                minPt=flags.Analysis.Small_R_jet.min_pT,
                                pTsort=True,
                                baselineSelectionName=(
                                    "" if not doJvtSel
                                    else "baselineSelection_outputSelect_thin_%SYS%"
                                ),
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
            dR_Ph_El=flags.Analysis.HZy_OR.phjet_thr.dR_Ph_El,
            dR_Ph_CaloMu=flags.Analysis.HZy_OR.phjet_thr.dR_Ph_CaloMu,
            dR_Ph_StacoMu=flags.Analysis.HZy_OR.phjet_thr.dR_Ph_StacoMu,
            floatVariableList=float_variables['leptons'],
            intVariableList=int_variables['leptons'],
            doSystematics=flags.Analysis.do_CP_systematics,
        )
    )

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

    # Trigger matching / trigger SF
    # using the selected Z-candidate leptons + possible third leptons.
    # This must run after PhotonVarsHZyAlg creates:
    # HZyElOR_Photons_Extra_%SYS% and HZyMuOR_Photons_Extra_%SYS%
    # and before HZySelectorAlg reads:
    # globalTriggerMatch_%SYS%
    cfg.merge(HZy_TriggerSFCfg(flags))

    # Compute event-level JVT SF on HZyJetsOR_NoJvt (after HZy OR, before JVT cut).
    # This container includes both JVT-passing and JVT-failing jets,
    # so the SF correctly accounts for both efficiency and inefficiency contributions.
    # Using HZy-specific OR instead of standard CP OR ensures analysis consistency.
    cfg.merge(HZy_JvtEventSFCfg(flags))

    cfg.addEventAlgo(
        CompFactory.HZY.BaselineVarsHZyAlg(
            "BaselineVarsHZyAlg",
            isMC=flags.Input.isMC,
            floatVariableList=float_variables['baseline'],
            intVariableList=int_variables['baseline'],
            floatVariableList_leptons=float_variables['leptons'],
            intVariableList_leptons=int_variables['leptons'],
            floatVariableList_photons=float_variables['photons'],
            intVariableList_photons=int_variables['photons'],
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

    # Add per-jet vector variable support
    jet_floatvec_variables = get_JetVarsHZyAlg_floatvec_variables(flags)
    cfg.addEventAlgo(
        CompFactory.HZY.JetVarsHZyAlg(
            "JetVarsHZyAlg",
            isMC=flags.Input.isMC,
            floatVariableList=float_variables['jets'],
            intVariableList=int_variables['jets'],
            floatVecVariableList=jet_floatvec_variables,
            doSystematics=flags.Analysis.do_CP_systematics,
            bTagWPDecorName="ftag_select_" + flags.Analysis.Small_R_jet.btag_wp,
        )
    )

    # Truth label for the decay of the associated Z
    if flags.Analysis.save_truth_vars:
        cfg.addEventAlgo(
            CompFactory.HZY.TruthVarsHZyAlg(
                "TruthVarsHZyAlg",
                isMC=flags.Input.isMC,
                TruthBosonsInKey=(
                    flags.Analysis.container_names.input.truthSMParticles
                ),
                intVariableList=int_variables['truth'],
                doSystematics=flags.Analysis.do_CP_systematics,
            )
        )

    return cfg


def get_PhotonVarsHZyAlg_variables(flags):
    float_variable_names = [
        "Photon_effSF", "Ph_Pt_Raw", "Ph_Eta_Raw", "Ph_Phi_Raw",
        "ExtraLep_Pt", "ExtraLep_Eta", "ExtraLep_Phi",
        # Isolation variables
        "Ph_topoetcone20", "Ph_topoetcone30", "Ph_topoetcone40",
        "Ph_ptcone20",
        # Shower shapes, for possible R&D with non-PHYS-DAOD
        "Ph_Reta", "Ph_Rhad", "Ph_Rhad1", "Ph_Rphi",
        "Ph_f1", "Ph_f3",
        "Ph_weta1", "Ph_weta2", "Ph_wtots1",
        "Ph_Eratio", "Ph_fracs1", "Ph_DeltaE",
    ]
    int_variable_names = [
        "nPhotons_LIdNIso", "nPhotonsOR_LIdNIso", "nPhotonsOR_TIdNIso",
        "nPhotonsOR_TIdLIso", "Ph_ConvType", "Ph_isEMTight",
        "nElectronsIdIso_ORPh", "nMuonsIdIso_ORPh",
        "ExtraLep_Label"]

    return float_variable_names, int_variable_names


def get_LeptonVarsHZyAlg_variables(flags):
    float_variable_names = [
        "LepLead_effSF", "LepSub_effSF", "Muon_Fsr_dR", "LepLepZ_M_Fsr",
        # Calibrated FSR photon kinematics, published so that every consumer
        # rebuilds the same four-vector the constraint fit was given. The raw
        # xAOD object in HZyFsrObject is NOT interchangeable with these
        "Muon_Fsr_Pt", "Muon_Fsr_Eta", "Muon_Fsr_Phi",
    ]
    # Z-mass constraint applied with the FSR photon left out of the fit.
    # With LepLepZ_M_Raw (neither), LepLepZ_M_Fsr (FSR only) and
    # LepLepZ_M_Cstr (both) these give all four FSR x constraint
    # combinations, for isolating what each step does to m(ll)
    for suffix in ["Raw", "Cstr", "CstrNoFsr"]:
        if suffix == "CstrNoFsr":
            objs = ["LepLepZ"]
        else:
            objs = ["LepLepZ", "LepLead", "LepSub"]
        for obj in objs:
            for var in ["M", "Pt", "Eta", "Phi"]:
                float_variable_names.append(f"{obj}_{var}_{suffix}")

    int_variable_names = [
        "Lep_Channel", "LepLepZ_Size", "Muon_hasFsr", "Muon_Index_Fsr",
        "nElectrons_LIdNIso", "nMuons_MIdNIso",
        "nElectrons_LIdLIso", "nMuons_MIdLIso",
        "has4LepSwap"]

    return float_variable_names, int_variable_names


def get_JetVarsHZyAlg_variables(flags):
    float_variable_names = [
        "Mjj",
        "leadBjet_Pt", "leadBjet_Eta", "leadBjet_Phi", "leadBjet_M",
        "subBjet_Pt", "subBjet_Eta", "subBjet_Phi", "subBjet_M",
    ]
    int_variable_names = ["nJets", "nbJets"]

    return float_variable_names, int_variable_names


def get_TruthVarsHZyAlg_variables(flags):
    """Truth label for the associated Z decay, written by TruthVarsHZyAlg.

    AssocZ_DecayLabel is 0 when the event has no associated Z (ggF, VBF and
    data), 1 for Z->ee/mumu, 2 for Z->nunu, 3 for Z->qq and 4 for Z->tautau.
    """
    float_variable_names = []
    int_variable_names = (["AssocZ_DecayLabel"]
                          if flags.Analysis.save_truth_vars else [])

    return float_variable_names, int_variable_names


def get_BaselineVarsHZyAlg_variables(flags):
    float_variable_names = [
        "BeamSpotWeight", "MC_Weight", "LLGam_M_Fsr",
        # m(llg) built from the constrained Z of the FSR-free fit; the
        # LLGam_M_{Raw,Fsr,CstrNoFsr,Cstr} set is the FSR x constraint 2x2
        "LLGam_M_CstrNoFsr",
    ]

    for obj in ["LLGam"]:
        for suffix in ["Raw", "Cstr"]:
            for var in ["M", "Pt", "Eta", "Phi"]:
                float_variable_names.append(f"{obj}_{var}_{suffix}")

    float_variable_names += [
        # more VBF variables for R&D
        "VBF_Yrel", "VBF_Yrel_ph", "VBF_ph_eta_center",
        "VBF_Agap", "VBF_Agap_rapidity", "VBF_pt_balance",
        "LLGam_RelPhpT_Cstr", "VBF_Score_Run3",
        "ggF_Score_Run3", "RelpT_Score_Run3",
    ]

    # MVA input feature names for ONNX inference
    onnx_input_features = [
        "VBF_DRmin_y_j", "VBF_Dphi_Zy_jj_Fullrange",
        "VBF_Dy_j_j", "VBF_N_j",
        "VBF_Zepp", "VBF_eta_j1",
        "VBF_eta_j2", "VBF_m_jj",
        "VBF_mass_j1", "VBF_mass_j2",
        "VBF_pT_j1", "VBF_pT_j2",
        "VBF_pT_jj", "VBF_pTt_Zy",
        "VBF_phi_j1", "Zy_Dphi_j1",
        "ll_eta", "ll_pt",
        "ll_pt_rel", "ph_eta",
        "ph_pt", "ph_pt_rel",
        "llg_angles_costheta_ginH", "llg_angles_costheta_linZ",
        "llg_deta_Zy", "llg_dphi_Zy", "llg_angles_phi_linZ",
        "llg_eta", "llg_pt",
        "llg_pt_rel", "llg_pTt",
    ]
    float_variable_names += onnx_input_features
    int_variable_names = ["category_Run3"]

    return float_variable_names, int_variable_names


def get_JetVarsHZyAlg_floatvec_variables(flags):
    return [
        "jets_pt", "jets_eta", "jets_phi", "jets_mass", "jets_e",
        "jets_PartonTruthLabelID", "jets_QGScore", "jets_NNJvt", "jets_jvtEffSF",
        "jets_NumTrkPt500", "jets_NumTrkPt1000", "jets_NTracks",
    ]


def HZy_branches(flags):
    # a list of strings which maps the variable name in the c++ code
    # to the outputname:
    # "EventInfo.<var> -> <outputvarname>"
    branches = []

    # this will be all the variables that are calculated by the
    # BaselineVarsAlg algorithm
    all_baseline_variable_names = []
    keys = ["baseline", "photons", "leptons", "jets", "truth"]
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
    truth_float_variables, truth_int_variables \
        = get_TruthVarsHZyAlg_variables(flags)

    float_variable_names['baseline'] += baseline_float_variables
    int_variable_names['baseline'] += baseline_int_variables
    float_variable_names['photons'] += photon_float_variables
    int_variable_names['photons'] += photon_int_variables
    float_variable_names['leptons'] += lepton_float_variables
    int_variable_names['leptons'] += lepton_int_variables
    float_variable_names['jets'] += jet_float_variables
    int_variable_names['jets'] += jet_int_variables
    float_variable_names['truth'] += truth_float_variables
    int_variable_names['truth'] += truth_int_variables

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

    # Per-jet vector variables
    jet_vec_vars = get_JetVarsHZyAlg_floatvec_variables(flags)
    for var in jet_vec_vars:
        branches += [f"EventInfo.{var}_{sys_suffix} -> HZy_{var}"
                     + flags.Analysis.systematics_suffix_separator + sys_suffix]

    s_name = flags.Analysis.selection_name
    # MC only: HZy_JvtEventSFCfg is a no-op on data, so the decoration does
    # not exist there and requesting it unconditionally makes the ntuple
    # maker fail in initialize().
    if flags.Input.isMC:
        branches += [f"EventInfo.hzy_jvt_effSF_{sys_suffix} -> HZy_hzy_jvt_effSF"
                     + flags.Analysis.systematics_suffix_separator + sys_suffix]
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
