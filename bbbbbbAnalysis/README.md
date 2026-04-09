# bbbbbbAnalysis

An ATLAS Athena-based analysis package for the search for Standard Model tri-Higgs boson production in the **HHH → bbbbbb (6b)** final state.

## Overview

This package provides the full analysis chain for reconstructing and selecting tri-Higgs events where all three Higgs bosons decay to b-quark pairs. It is built on top of the **EasyjetHub** framework and integrates with the ATLAS software infrastructure.

## Directory Structure

```
bbbbbbAnalysis/
├── bin/                    # Executable scripts
├── data/                   # Calibration/data files 
├── datasets/               # Sample lists for grid submission
│   └── HHH6b/
│       ├── data_DAOD_PHYS_Run2.txt
│       └── mc20_DAOD_PHYS_HHH6b_SMHHH.txt
        └── ..              
├── python/
│   ├── config/             # Python configuration modules
│   │   ├── resolved.py     # Resolved analysis configuration
│   │   └── trihiggs.py     # Main tri-Higgs analysis chain
│   └── utils/              # Utility functions
│       └── option_validation.py
├── scripts/                # Grid submission scripts
├── share/                  # YAML run configurations
│   ├── RunConfig-bbbbbb-base.yaml
│   ├── RunConfig-HHH6b-Resolved.yaml
│   └── triggers-resolved.yaml
├── src/                    # C++ algorithm implementations
└── CMakeLists.txt
```

## Algorithms

The package implements the following Athena algorithms (in namespace `HHH6B`):

| Algorithm | Description |
|-----------|-------------|
| `bbbbbbSelectorAlg` | Event selection with configurable cuts (trigger, jet multiplicity, b-tag multiplicity) |
| `JetPairingAlg` | Pairs jets into three Higgs candidates using different strategies |
| `BaselineVarsResolvedAlg` | Computes kinematic variables for the resolved analysis |
| `SmallRJetTriggerSFAlg` | Calculates trigger scale factors for small-R jets |
| `TriggerDecoratorAlg` | Decorates events with trigger bucket information |

### Jet Pairing Strategies

The `JetPairingAlg` supports two pairing strategies:

- **`minDeltaR`**: Pairs jets by minimizing the sum of ΔR within each Higgs candidate
- **`minTargetMass`**: Pairs jets to minimize deviation from target Higgs mass

## Usage

### Running Locally

```bash
bbbbbb-ntupler --run-config bbbbbbAnalysis/RunConfig-HHH6b-Resolved.yaml \
               --input-files <path_to_DAOD_PHYS> \
               --output-ntuple output.root
```

### Grid Submission

Use the provided grid submission script:

```bash
cd scripts/
./RunOnGrid_HHH6b_Run2_NoSyst.sh
```

Or submit manually using `easyjet-gridsubmit`:

```bash
easyjet-gridsubmit --mc-list datasets/HHH6b/mc20_DAOD_PHYS_HHH6b_SMHHH.txt \
    --run-config share/RunConfig-HHH6b-Resolved.yaml \
    --exec bbbbbb-ntupler \
    --nGBperJob 4 \
    --campaign <campaign_name>
```

## Configuration

### Main Configuration Options (`RunConfig-HHH6b-Resolved.yaml`)

| Option | Description | Default |
|--------|-------------|---------|
| `do_resolved_trihiggs` | Enable resolved analysis | `true` |
| `do_boosted_trihiggs` | Enable boosted analysis | `false` |
| `pairingStrategy` | Jet pairing method (`minDeltaR` or `minTargetMass`) | `minDeltaR` |
| `do_trigger_filtering` | Filter events on trigger | `true` |
| `bypass` | Run selector in pass-through mode | `false` |
| `save_cutflow` | Save cutflow histograms | `true` |

### Small-R Jet Configuration

```yaml
Small_R_jet:
  min_pT: 20       # Minimum pT in GeV
  max_eta: 4.5        # Maximum |η|
  btag_wp: "GN2v01_FixedCutBEff_77"  # B-tagging working point
```

### Event Selection Cuts

Available cuts in `CutList`:

- `PASS_TRIGGER`
- `AT_LEAST_FOUR_JETS`
- `AT_LEAST_SIX_JETS`
- `AT_LEAST_FOUR_BJETS`
- `AT_LEAST_SIX_BJETS`
- `AT_LEAST_FOUR_JETS_OR_TWO_LRJETS`
- `AT_LEAST_SIX_JETS_OR_THREE_LRJETS`
- `AT_LEAST_SIX_JETS_FOUR_BJETS`

## Output Variables

The ntuple output includes:

### Higgs Candidate Variables
- `h1_m`, `h1_pt`, `h1_eta`, `h1_phi` (leading Higgs)
- `h2_m`, `h2_pt`, `h2_eta`, `h2_phi` (sub-leading Higgs)
- `h3_m`, `h3_pt`, `h3_eta`, `h3_phi` (third Higgs)
- `hhh_m`, `hhh_pt` (tri-Higgs system)

### Kinematic Variables
- `DeltaR12`, `DeltaR13`, `DeltaR14`, `DeltaR23`, `DeltaR24`, `DeltaR34`, `DeltaR56`, `DeltaR35`
- `DeltaEtaJet1Jet2`, `DeltaEtaJet3Jet4`, `DeltaEtaJet5Jet6`
- `DeltaPhiJet1Jet2`, `DeltaPhiJet3Jet4`, `DeltaPhiJet5Jet6`
- `Jet12Pt`, `Jet34Pt`, `Jet56Pt`, `Jet12Mass`, `Jet34Mass`, `Jet56Mass`
- `M4j` (combined 4-jet mass)
- `MassDiff12vs34`, `MassDiff12vs56`, `MassDiff34vs56`

### Event Shape Variables
- `Thrust`, `Sphericity`, `Planarity`, `Aplanarity`

### Selection Variables
- `pass_4b`, `pass_5b`, `pass_6b` (b-jet multiplicity categories)
- `n_truth_jets`, `n_reco_jets_before_cuts`, `n_reco_jets_after_cuts`

<!-- ## Building

This package is built as part of the Athena framework:

```bash
# Initialize workdir
git atlas init-workdir ssh://git@gitlab.cern.ch:7999/$(git config user.name)/athena.git

# Add package
git atlas addpkg bbbbbbAnalysis

# Build
mkdir build && cd build
cmake ../athena/Projects/WorkDir
make -j
source */setup.sh
``` -->

<!-- ## Dependencies

- AthenaBaseComps
- EasyjetHub
- xAOD containers (EventInfo, Jet, Egamma, Tracking, Truth)
- SystematicsHandles
- TrigCompositeUtils
- HDF5 (optional)
- ROOT

## Signal Samples

The analysis uses Standard Model HHH→bbbbbb Monte Carlo samples, used by the first version of this analysis:

- `mc20_13TeV.521162.MGPy8EG_A14NNPDF30NLO_SM_HHH_bbbbbb_4bFilter` -->

<!-- ## Trigger Strategy -->
<!-- 
The analysis uses b-jet triggers across Run 2 and Run 3 data-taking periods. See `share/triggers-resolved.yaml` for the complete trigger list organized by year. -->

## Code Developers

This package is mostly derived from the bbbbAnalysis package for the HH -> 4b analysis. The package structure and the algorithms have the same pattern. The developers for the bbbbbbAnalysis package: Can Suslu, .. (feel free to add your name). The developers of the bbbbAnalysis can be seen from the README file of that package. 

<!-- ## Authors

ATLAS Collaboration

## License

Copyright (C) 2002-2025 CERN for the benefit of the ATLAS collaboration -->
