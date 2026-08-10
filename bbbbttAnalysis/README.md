Analysis Package for the $HHH\rightarrow b\bar{b}b\bar{b} \tau\tau$ analysis
=========================

# Folder structure

- `bin/`: Executables
  - `bbbbtt-ntupler`
- `python/`: Main python code to configure the components (objects, selections as well as the variables to save)
  - `bbbbtt_config`
- `share/`: yaml files containing configurations used by the components
  - `RunConfig-bbbbtt-base`: where all the common flags are set, configurations called by the executables
  - `RunConfig-bbbbtt[-nomOnly]`: config file to run jobs without systematics.
  - `RunConfig-bbbbtt-syst`: config file to run jobs with systematics.
  - `RunConfig-bbbbtt-syst-hadhad`: config file to run HadHad-only jobs with systematics.
  - `RunConfig-bbbbtt-syst-lephad`: config file to run LepHad-only jobs with systematics.
  - `RunConfig-bbbbtt-syst-lowTauPt`: config file setting configuration with low pT taus (pT < 20 GeV).
- `src/`: C++ code
  - `HHHbbbbttSelectorAlg`: Find if the event pass the baseline bbbbtt selections;
  - `MMCDecoratorAlg`: Compute the tau tau MMC mass;
  - `MMCSelectorAlg`: Filter events based on MMC;
  - `BaselineVarsbbbbttAlg`: Compute the baseline variables for the analysis.
  - `ttbarRewVarsAlg`: Implements ttbar truth variables needed for the ttbar NNLO reweighting downstream.
  - `KappaReweightingAlg`: Saves variables needed tor the HHH signal reweighting to different tri-linear and quartic Higgs self-coupling values.
  - `TriggerDecoratorAlg`: Gets trigger matching information;
  - `TriggerSFAlg`: Gets trigger SFs;
  - `TriggerUtils`: Defines trigger pass.

# How to Run

- Follow the general [EasyJet README](https://gitlab.cern.ch/easyjet/easyjet#installation) to install all the necessary packages
  - At the `git sparse-checkout set` step, include the `bbbbtt` and `bbtt` packages:
  ```
  git sparse-checkout set EasyjetHub EasyjetTests bbbbttAnalysis bbttAnalysis
  ```

-  Run the ntupler on an available PHYS file: 
```
bbbbtt-ntupler [input DAOD file] -c ../easyjet/bbbbttAnalysis/share/RunConfig-bbbbtt-nomOnly.yaml --out-file test.root -e 1000
  ```
Examples of DAOD files for signal HHH and data can be found in this [link](https://cernbox.cern.ch/s/12GpYGVOGhr52JO).

-  For sending jobs to the grid, follow the following example for nominal-only:
```
for f in ../easyjet/bbbbttAnalysis/datasets/PHYS/prod/mc20_13TeV.HHH.prod.txt ; do easyjet-gridsubmit --exec bbbbtt-ntupler --run-config ../easyjet/bbbbttAnalysis/share/RunConfig-bbbbtt-nomOnly.yaml --channels LepHad HadHad --noTag --campaign EJ_0_52_1 --nGBperJob 3  --mergeOutput --mc-list $f; done
```
In case of submitting with systematics, some extra care is needed when selecting the job parameters. Here is one example for signal submission:
```
for f in ../easyjet/bbbbttAnalysis/datasets/PHYS/prod/prod_mc23.txt ; do easyjet-gridsubmit --exec bbbbtt-ntupler --run-config ../easyjet/bbbbttAnalysis/share/RunConfig-bbbbtt-syst.yaml --channels HadHad LepHad --noTag --campaign EJ_0_52_0_v3i_syst --nGBperJob 3 --memory 4000 --maxNFilesPerJob 1 --mergeOutput --mc-list $f; done
```
For non-signal submission, the `maxNFilesPerJob` parameter can be removed (and let the grid optimize the job submission).


# Output

If these run properly, your outputs files should contain a TTree `AnalysisMiniTree` with the following content (XXX denotes a set of variables associated to the object, usually pT, Eta ...):
* Some information saved for every analyses displayed in the main [README section](https://gitlab.cern.ch/easyjet/easyjet#have-a-look-at-the-output);
* Truth information:
    * jets (implemented as a vector): `truthjet_antikt4_X`.
* Reconstructed objects:
    * electron / muon / tau kinematics (implemented as a vector): `el_XXX` / `mu_XXX` / `tau_XXX`;
    * jet kinematics (implemented as a vector): `recojet_antikt4PFlow_XXX`;
    * $E_T^{miss}$ : `met_XXX`.
* Standard set of `bbbbtt` variables, including:
    * cuts to pass: `bbbbtt_pass_XXX`.
    * Missing Mass Calculator (MMC) outputs: `bbbbtt_mmc_XXX`;

# Main developers

This code is an adaptation from the bbtt package. The main developments in this HHH package have been performed by (non extensive list, feel free to add your name):
Marcus Rodrigues, Jordy Degens, Gabriel Oliveira Correa
