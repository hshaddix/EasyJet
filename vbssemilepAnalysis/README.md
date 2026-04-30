Analysis Package for the $VBS VZ(\rightarrow qq'l^{+}l^{-}) $ analyis
=========================

# Folder structure

- `bin/`: Executables
  - `vbssemilep-ntupler`
- `python/`: Main python code to configure the components (objects, selections as well as the variables to save)
  - `*_config`
- `share/`: yaml files containing configurations used by the components
  - `RunConfig-common.yaml` : Common configuration between  3 channels 
  - `RunConfig-oneLep.yaml`: one muon/electron and neutrino specific configurations called by the executables (see below);
  - `RunConfig-twoLep.yaml`: two electrons or muons specific configurations called by the executables (see below);
  - `trigger`: list of the triggers to use per year.
- `src/`: C++ code
  - `oneLepSelectorAlg`: Find if the event pass the baseline VBS one Lepton selection;
  - `twoLepSelectorAlg`: Find if the event pass the baseline VBS two Lepton selection;
  - `BaselineVarsoneLepAlg`: Compute the baseline variables for the one lepton final state.
  - `BaselineVarstwoLepAlg`: Compute the baseline variables for the two lepton final state.

# How to Run (local)
- one Lepton channel
``` 
vbssemilep-ntupler <path to DAOD_PHYS or DAOD_PHYSLITE file> --run-config ../easyjet/vbssemilepAnalysis/share/RunConfig-oneLep.yaml --out-file output.root
```

- two lepton channel
``` 
vbssemilep-ntupler <path to DAOD_PHYS or DAOD_PHYSLITE file> --run-config ../easyjet/vbssemilepAnalysis/share/RunConfig-twoLep.yaml --out-file output.root
```

# How to Run (Grid)
This is taken care of by ```easyjet-gridsubmit``` script. First you need to put the list of containers (DAOD_PHYS or DAOD_PHYSLITE) in .txt file (let's call it mc_list.txt) and then you need to choose the config yml, depending
if one wants to run with or with events skimming, you can use either ```RunConfig-fullLep.yaml``` or ```RunConfig-fullLep-bypass.yaml```. you can run with this command line 
``` 
easyjet-gridsubmit --mc-list mc_list.txt --exec vbssemilep-ntupler --run-config vbssemilepAnalysis
/RunConfig-twoLep.yaml  --campaign VBSSemilep_vXXX  --noTag --mergeOutput --noEmail 
``` 
