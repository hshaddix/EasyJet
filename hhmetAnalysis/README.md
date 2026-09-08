Analysis Package for the $HH + MET$ dark matter analyis
=========================

# Setup scripts

softlink these to outside the easyjet folder to run them for setting up:
- `first_time_setup.sh`: to run to set up a new athena release after checking out easyjet, sort out the build/run directories, and compile everything.
- `daily_setup.sh`: to run when you are setting up in a fresh terminal but have already setup athena and compiled once. sets up the environment.
- `recompile.sh`: just the steps to recompile, starting from the easyjet directory.

example script to run a local test, put in your run directory:
- `runtest.sh`: runs on the test file detailed in the easyjet readme.

# Folder structure

- `bin/`: Executables:
  - `hhmet-ntupler`: produces ntuples.
- `python/`: Main python code to configure the components (objects, selections as well as the variables to save):
  - `dihiggs.py`: main controller;
  - `boosted_chan.py`: for the boosted analysis ntuples;
  - `resolved_chan.py`: for the resolved analysis ntuples;
  - `opt_validation.py`: set of checks to define the correct flags;
  - `helpers.py`: some help functions common to both boosted and resolved channels.
- `share/`: yaml files containing configurations used by the components:
  - `RunConfig-hhmet-base.yaml`: basic configurations for both analyses;
  - `RunConfig-hhmet-resolved.yaml`: configurations for the resolved analysis;
  - `RunConfig-hhmet-boosted.yaml`: configurations for the potential boosted analysis extension;
  - `RunConfig-hhmet-all.yaml`: configurations to run resolved & boosted analyses together.
  - `triggers_MET.yaml`: list of triggers to use;
  - `grl_*.yaml`: setup for the GRLs.
  - `prw_files.yaml`: setup for PRW.
  - `lumicalc_files.yaml`: setup for lumi calculation
- `src/`: C++ code:
  - `hhmetSelectorAlg`: pre-select events to reduce the file size;
  - `BaselineVarsResolvedAlg`: compute some variables that aren't produced centrally, specific to the resolved analysis;
  - `BaselineVarsBoostedAlg`: compute some variables that aren't produced centrally, specific to the boosted analysis;
  - `BaselineVarsCommonAlg`: compute some variables that aren't produced centrally and are common to both the boosted/resolved analyses.
- `datasets/`: input DAOD samples for grid submission
- `scripts/`: grid submission scripts

# How to Run

1.  Get the files to make the test: have a look at the general [README section](https://gitlab.cern.ch/easyjet/easyjet#running-on-files) for updated informations (only <span style="color: #F2385A">PHYS</span>).

2. Run the ntupler on those files:
    ```hhmet-ntupler --run-config hhmetAnalysis/RunConfig-HHMET-all.yaml -o output_PHYS_hhmet_ntuple.root  ttbar_PHYS_10evt.root```

3. Input PHYS samples can be found in the "datasets" directory.

4. Choose your run-config file.

5. Running on the Grid (* Please test your setup before running bulk grid submission):
- This is hadled by the easyjet-gridsubmit script. To run a test job on the grid please use the following command:
"easyjet-gridsubmit --mc-list input_sample.txt --exec hhmet-ntupler --run-config hhmetAnalysis/RunConfig-HHMET-base.yaml --campaign TestXXX --nGBperJob 5 --noTag"

- It is recommended to run from a central tag for large scale productions. If you have some local updates, you will be invited to commit them and a tag will be created to be pushed on your fork. If you want to avoid this, you can use the --noTag options.

- Grid submission scripts are now available in the "scripts" directory.
  - `RunOnGrid_HHMET_Run*_NoSyst.sh` to run the latest config for resolved+boosted with the newest p726* tags. 


# Output

If these run properly, your outputs files should contain a TTree `AnalysisMiniTree` with the following content (X denotes a set of variables associated to the object, usually pT, Eta ...):
* Some information saved for every analyses displayed in the main [README section](https://gitlab.cern.ch/easyjet/easyjet#have-a-look-at-the-output);
* Truth information:
    * jets (implemented as a vector): `truthjet_antikt4_X`, `truthjet_antikt10_X`.
* Reconstructed objects:
    * jet kinematics (implemented as a vector): `recojet_antikt4PFlow_X___NOSYS`, `recojet_antikt10UFO_X___NOSYS`.
    * paired higgs bosons: `hhmet_resolved_<algorithm>_<btagWP>_X_<SYS>`

# Main developers

The main developments have been performed by (non extensive list, feel free to add your name):
- Holly Pacey & Ruben Hönscheid
We used the bbbbAnalysis code as a starting point: thanks!
