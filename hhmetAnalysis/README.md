Analysis Package for Analysis Package for the $HH + MET$ dark matter analyis
=========================

# Setup scripts

softlink these to outside the easyjet folder to run them for setting up:
- `first_time_setup.sh`: to run to set up a new athena release after checking out easyjet, sort out the build/run directories, and compile everything.
- `daily_setup.sh`: to run when you are setting up in a fresh terminal but have already setup athena and compiled once. sets up the environment.
- `recompile.sh`: just the steps to recompile, starting from the easyjet directory.

example script to run a local test, put in your run directory:
- `runtest.sh`: runs on the test file detailed in the easyjet readme.

# Folder structure

- `bin/`: Executables
  - `hhmet-ntupler`: produces ntuples;
- `python/`: Main python code to configure the components (objects, selections as well as the variables to save)
    - `dihiggs`: main controller;
    - `boosted`: for the boosted analysis ntuples;
    - `resolved`: for the resolved analysis ntuples;
    - `opt_validation.py`: set of checks to define the correct flags.
- `share/`: yaml files containing configurations used by the components
  - `RunConfig-hhmet-base.yaml`: basic configurations for the main resolved analysis;
  - `RunConfig-hhmet-boosted.yaml`: configurations called by the executable for the potential boosted analysis extension.
  - `triggers*.yaml`: list of triggers to use;
- `src/`: C++ code
  - `hhmetSelectorAlg`: pre-select events to reduce the file size;
  - `BaselineVarsBoostedAlg` / `BaselineVarsResolvedAlg`: compute some variables that aren't produced centrally;
- `datasets/`: input DAOD samples for grid submission
- `scripts/`: grid submission scripts

# How to Run

1.  Get the files to make the test: have a look at the general [README section](https://gitlab.cern.ch/easyjet/easyjet#running-on-files) for updated informations (only <span style="color: #F2385A">PHYS</span>).

2. Run the ntupler on those files:
    ```hhmet-ntupler --run-config hhmetAnalysis/RunConfig-HHMET-base.yaml -o output_PHYS_hhmet_resolved_ntuple.root  ttbar_PHYS_10evt.root```

3. Input PHYS samples can be found in the "datasets" directory. TODO: MAKE THESE FILE LISTS

4. Choose your run-config file.

5. Running on the Grid (* Please test your setup before running bulk grid submission): NOT SET UP YET
- This is hadled by the easyjet-gridsubmit script. To run a test job on the grid please use the following command:
"easyjet-gridsubmit --mc-list input_sample.txt --exec hhmet-ntupler --run-config hhmetAnalysis/RunConfig-HHMET-base.yaml --campaign TestXXX --nGBperJob 5 --noTag"

- It is recommended to run from a central tag for large scale productions. If you have some local updates, you will be invited to commit them and a tag will be created to be pushed on your fork. If you want to avoid this, you can use the --noTag options.

- Grid submission scripts are now available in the "scripts" directory.

# Output

If these run properly, your outputs files should contain a TTree `AnalysisMiniTree` with the following content (X denotes a set of variables associated to the object, usually pT, Eta ...):
* Some information saved for every analyses displayed in the main [README section](https://gitlab.cern.ch/easyjet/easyjet#have-a-look-at-the-output);
* Truth information:
    * jets (implemented as a vector): `truthjet_antikt4_X`, `truthjet_antikt10_X`.
* Reconstructed objects:
    * jet kinematics (implemented as a vector): `recojet_antikt4PFlow_X___NOSYS`, `recojet_antikt10UFO_X___NOSYS`.

# Main developers

The main developments have been performed by (non extensive list, feel free to add your name):
Holly Pacey, using the bbbbAnalysis code as a starting point (thanks!).
