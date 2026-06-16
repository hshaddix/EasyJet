Analysis Package for Full Run3 $H\rightarrow Z \gamma$ analyis
================================

# Folder structure

- `bin/`: Executables
  - `HZy-ntupler`
- `python/`: Main python code to configure the components (objects, selections as well as the variables to save)
  - `HZy_config`
- `share/`: yaml files containing configurations used by the components
  - `RunConfig-HZy-base`: base configuration file, where all the common flags are set, configurations called by the executables, no `CutList` defined here, all configs that define a `CutList` should include this (see below);
  - `RunConfig-HZy`: analysis configuration file for selected events, defining a `CutList` and including `RunConfig-HZy-base`, where all the common flags are set;
  - `RunConfig-HZy-bypass`: bypass configuration file for the run-through/debug mode, including `RunConfig-HZy`, but with the `bypass` flag set to true, flags for the cuts are saved, which allows to run the ntupler without applying any cuts;
  - `trigger`: list of the triggers to use per year.
  - `lumicalc`: list of the luminosity to use per year.
  - `prw_lepton`: list of the pileup reweighting files to use per year.
  - `HZy-base-output.yaml`: list of variables to save in the output ntuple, included in `RunConfig-HZy-base`.
- `src/`: C++ code
  The main algorithms called in [HZy_config.py](python/HZy_config.py) are excuted in the following order:
  - `HZyLeptonORAlg`: Perform the lepton overlap removal before Z selection;
  - `LeptonVarsHZyAlg`: Compute the lepton-related variables; Find the Z(ll)y candidate ([FSRCorrectionTool](https://gitlab.cern.ch/atlas/athena/-/tree/main/PhysicsAnalysis/AnalysisCommon/FsrUtils) and [ZMassConstrainTool](https://gitlab.cern.ch/atlas/athena/-/tree/main/PhysicsAnalysis/HiggsPhys/Run2/HZZ/Tools/ZMassConstraint)) in the event, and compute the associated variables (masses, pT, etc...); 
  - `PhotonVarsHZyAlg`: Compute the photon-related variables; Perform the photon-jet overlap removal after the Z(ll)y selection;
  - `HZySelectorAlg`: Apply the event selections following the configuration in the `RunConfig-HZy.yaml` file, and save the cutflow;
  - `JetVarsHZyAlg`: Compute the small-R jet variables.
  - `BaselineVarsHZyAlg`: Compute the baseline variables.

*Note:* The HZy-specific algorithms run on `HZyAnalysis*` containers, for example `HZyAnalysisElectrons`, which are produced by [`EasyjetHub`](easyjet/EasyjetHub/src) from the corresponding `Analysis*` containers.
For DAOD_PHYSLITE inputs, the `Analysis*` containers are already available and can be processed directly by `EasyjetHub`. For DAOD_PHYS inputs, the original DAOD containers, such as `Electrons`/`ElectronsAuxDyn`, are first processed by the CP algorithms in [`PhysicsAnalysis/Algorithms`](https://gitlab.cern.ch/atlas/athena/-/blob/main/PhysicsAnalysis/Algorithms) to produce the `Analysis*` containers. These are then used by `EasyjetHub` to create the `HZyAnalysis*` containers.

# How to Run

1. Get the files to make the test: have a look at the general [README section](https://gitlab.cern.ch/easyjet/easyjet#running-on-files) for updated informations.

2. Run the ntupler on those files:
- run the analysis on <span style="color: #F2385A">PHYS</span>: ```HZy-ntupler ttbar_PHYS_10evt.root --run-config HZyAnalysis/RunConfig-HZy.yaml --out-file output_PHYS_ttbar.root```
- run the analysis on <span style="color: #4BD9D9;">PHYSLITE</span>: ```HZy-ntupler ttbar_PHYSLITE_10evt.root --run-config HZyAnalysis/RunConfig-HZy.yaml --out-file output_PHYSLITE_ttbar.root```

# Output

If these run properly, your outputs files should contain a TTree `AnalysisMiniTree` with the following content (X denotes a set of variables associated to the object, usually pT, Eta ...):
* Some information saved for every analyses displayed in the main [README section](https://gitlab.cern.ch/easyjet/easyjet#have-a-look-at-the-output);
* Reconstructed objects:
    * photon kinematics (implemented as a vector): `ph_X_NOSYS`;
    * lepton kinematics (implemented as a vector): `lep_X_NOSYS`;
    * jet kinematics (implemented as a vector): `recojet_antikt4PFlow_X_NOSYS`;
* Standard variables with a prefix `HZy_*` associated to the `HZ(ll)y` candidate:
    * Cut to pass with a prefix `CUT_*`: `CUT_PASS_TRIGGERS_NOSYS`, `CUT_PASS_LEPTONID_NOSYS`, ..., `CUT_PASS_MASS_TWO_LEPTONS_PHOTON_NOSYS`;
    * Leading and subleading lepton kinematics: `HZy_LepLead_X_NOSYS`, `HZy_LepSub_X_NOSYS`, etc...;
    * Photon kinematics: `HZy_Ph_X_NOSYS`; only for the selected signal photon.
    * Jet kinematics: `HZy_Jet_X_NOSYS`, `HZy_mjj_NOSYS`, etc...; mainly for VBF category.
    * Z(ll)y candidate kinematics: `HZy_LepLepZ_X_NOSYS`, etc...;
    * Global variables: `HZy_nPhotons_NOSYS`, `HZy_nElectrons_NOSYS`, `HZy_nMuons_NOSYS`, `HZy_nJets_NOSYS` etc..
    * Variables ended with a following suffix:
      - `Raw` for the variables computed with the raw kinematics of the objects, without any correction applied;
      - `Fsr` for the variables computed with the FSR-corrected kinematics of the objects, where the FSR photon is added to the closest muon in the event, if any FSR photon is found;
      - `Cstr` for the variables computed with the Z mass constrained kinematics of the objects, where the lepton momenta are rescaled to constrain the dilepton mass to the Z mass.

* Weight variables on trigger SF, lepton and photon efficiency SF, pileup weights, MC weight, beam spot weight, and jet vertex tagger weight: 
    * `globalTriggerEffSF_NOSYS`, `HZy_LepLead_effSF_NOSYS`, `HZy_LepSub_effSF_NOSYS`, `HZy_Ph_effSF_NOSYS`
    * `PileupWeight_NOSYS`, `HZy_MC_Weight_NOSYS`, `HZy_BeamSpotWeight_NOSYS`, `jvt_effSF_NOSYS`

# Post processing

The ntupler output must be post-processed to apply event-weight normalization and any additional analysis-level steps, such as temporarily adding R&D variables. The post-processing framework is available in [HZyPostProcess](https://gitlab.cern.ch/atlas-physics/higp/photons/run-3-h-zy/hzypostprocess), which is based on [FastFrame](https://gitlab.cern.ch/atlas-physics/higp/photons/run-3-h-zy/fastframes).

# Main developers

The main developments have been performed by (non extensive list, feel free to add your name):
Qiuping Shen, Fabio Lucio Alves, Shaoguang Wu.
