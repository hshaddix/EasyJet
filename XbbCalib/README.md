# Analysis Package for the Xbb Calibration
=========================

The analysis package for the Xbb Calibration is used to produce the nutples for the signal calibration and top mis-tag calibration of the Xbb tagger. 

# Folder structure

- `bin/`: Executables
  - `xbbcalib-ntupler`
- `datasets/`
    - "/ {process} / {ptag} / {data_period}.txt"
- `python/`: Main python code to configure the components (objects, selections as well as the variables to save)
  - `ttCalib_config`
  - `ZbbjCalib_config`
  - `ZbbyCalib_config`
  - `ZllCalib_config`
- `scripts`
  - `XbbCalib-submitGrid.py`: Scripts for submitting grid jobs.
  - `ttCalibPostProcess`: script for computing sumofweights
- `share/`: yaml files containing configurations used by the components
  - `RunConfig-{ANA}.yaml`: configurations called by the executables (see below);	
  - `RunConfig-{ANA}-syst_Set<1/2>.yaml`: configurations called by the executables for systematics production;	
  - `trigger_{ANA}.yaml`: list of the triggers to use per year.
- `src/`: C++ code
  - `{ANA}CalibSelectorAlg`: Find if the event pass the baseline selection for the calibration;
  - `BaselineVars<ANA>CalibAlg`: Compute the baseline variables for the calibration.


# Submitting a grid prodcution

To run the Xbb calibration ntupler a sparse-checkout of `EasyjetHub`, `EasyjetTests`, `XbbCalib`is needed. After builiding easyjet, a test job can be run. First download a file locally, the example uses the Zbb+j signal sample.

```
lsetup rucio
rucio get --nrandom 1 mc20_13TeV.700855.Sh_2214_Zbb_ptZ_200_ECMS.deriv.DAOD_PHYS.e8536_s3681_r13167_p7018
```
 
Run test 

```
xbbcalib-ntupler run/mc20_13TeV.700855.Sh_2214_Zbb_ptZ_200_ECMS.deriv.DAOD_PHYS.e8536_s3681_r13167_p7018/DAOD_PHYS.<sample downloaded>.pool.root.1 --run-config ../easyjet/XbbCalib/share/RunConfig_ZbbjCalib.yaml --out-file output.root -e 100
```

Then to submit to the grid first setup rucio and panda as:

```
lsetup rucio
lsetup panda
```

and then use the following script

```
XbbCalib-submitGrid.py [-h] --tag TAG --ptag PTAG [--data {run2,run3} [{run2,run3} ...]] [--mc {mc20,mc23} [{mc20,mc23} ...]] [-c CONFIG]
                              [-s SAMPLES [SAMPLES ...]] --analysis {Zbby,Zbbj,Zll,tt} [--dry-run]

options:
  -h, --help            show this help message and exit
  --tag TAG             A tag to identify the submission - will be included in the output names (default: None)
  --ptag PTAG           Production tag to use, e.g. p7018, p6490 (default: None)
  --data {run2,run3} [{run2,run3} ...]
                        Space seperated list of data periods to process (default: None)
  --mc {mc20,mc23} [{mc20,mc23} ...]
                        Space seperated list of mc campaigns to process (default: None)
  -c CONFIG, --config CONFIG
                        Use this config instead of the default one (default: None)
  -s SAMPLES [SAMPLES ...], --samples SAMPLES [SAMPLES ...]
                        Space-delimited list of samples, "MG_Zqqgamma SinglePhoton" If none are given, all samples for a given analysis are run (default:
                        None)
  --analysis {Zbby,Zbbj,Zll,tt}
                        Which analysis to run - defines the default config and samples (default: None)
  --dry-run             If set, will only print the commands that would be run, without executing (default: False)
```


For example to launch a systematics production for Zbb+j, the systematics are split into two set, `*syst_Set1.yaml` and `*syst_Set2.yaml`. The following command is to be run:

```
XbbCalib-submitGrid.py --tag <ANA>_<DATE>_SYST_SET1  --analysis Zbbj --ptag p701X --mc mc20 mc23 --data run2 run3 --nGBPerJob 2 --memory 4000 -c RunConfig-ZbbjCalib-syst_Set1.yaml
XbbCalib-submitGrid.py --tag <ANA>_<DATE>_SYST_SET2  --analysis Zbbj --ptag p701X --mc mc20 mc23 --nGBPerJob 2 --memory 4000 -c RunConfig-ZbbjCalib-syst_Set2.yaml
```
**WARNING:** Before submitting a grid production please conntact the Xbb conveners. The ntuples are centrally produced and generally stored at `/eos/atlas/atlascerngroupdisk/perf-flavtag/calib/xbb`.

**WARNING:** Make sure you define something meaningul for the tag and always tag the verison of the code you use to make a grid production. 

**NOTE:** The parton shower variation should submitted with the defualm (nominal only) config thorugh the option of submiting specific samples with `XbbCalib-submitGrid.py`. For example for the Zbbj calibration: 

```
XbbCalib-submitGrid.py --analysis Zbb --tag <ANA>_<DATE>_SYST -s "Zbb_ptZ_200_ECMS_Lund" --ptag p701X --mc mc20 mc23
```

The parton shower variation sample is different for each analysis. The following alternative samples are currently used:
- Zbb+jets: `Zbb_ptZ_200_ECMS_Lund`
- Zbb+gamma: `Zqqgamma_MGPy8EvtGen`
- Zll+jets: `Zll_Lund` (only avaible for Run 2)
- Zll+gamma: ??? (currently no alternative sample avaible)
- ttbar: `ttbar_Single_Lep_PhH7EG`

Nominal production can be run as:

```
XbbCalib-submitGrid.py --tag <ANA>_<DATE>_SYST  --analysis Zbbj --ptag p7018 --mc mc20 mc23 --data run2 run3
```

# ttbar calibration

Once your grid jobs have ran succesfully and you retrived the outputs, you will need to hadd the outputs before reweighting. Theres is a problem with hadding files which contain empty trees; For this, there is a custom mergeRootFiles.C macro. <https://gitlab.cern.ch/dallen/gn2x_calibration/-/blob/master/mergeRootFiles.C?ref_type=heads>


Using the ttCalibPostProcess.py script will add a 'weight' branch to your analyis tree.

```
../easyjet/XbbCalib/scripts/ttCalibPostProcess.py --xSectionsConfig ../easyjet/XbbCalib/share/XSectionsData.yaml --mergeMyFiles --mergeToOutput --inFile calibration-vars.root --ouFile reweighted.root
```

Contact: atlas-cp-flavtag-jetetmiss-boostedxbbtagging-conveners@cern.ch 