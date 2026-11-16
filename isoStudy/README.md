# mc23g checks: kinematics and photon isolation (HH→bbγγ)

Scripts and config for comparing the mc23g campaign with mc23a, mc23d and mc23e using the
HH→bbγγ signal sample and the bbyy ntupler in EasyJet:

1. kinematic distributions after the bbyy pre-selection, and
2. the photon isolation efficiency (FixedCutLoose) for Tight photons.

| File | What it does |
|---|---|
| `RunConfig-bbyy-isocheck.yaml` | run config for `bbyy-ntupler` (bypass mode, ID/iso SFs, H→bb candidates, kinematic fit) |
| `plot_kinematics.py` | kinematic plots, KF comparison and cut scan |
| `plot_isoeff.py` | isolation efficiency vs photon pT, η or ⟨μ⟩ |
| `check_acceptance.py` | prints the cumulative cutflow fraction per campaign, to find where campaigns differ |
| `submit_grid.sh` | submits the ntupler on the grid for the four datasets |

## Samples

HH→bbγγ signal (κλ = 1, DSID 603559), DAOD_PHYS with p7266 in all four campaigns. Only the
simulation and reconstruction tags differ, so the differences come from the campaign
conditions (pile-up, detector), not from the generation or derivation:

| Campaign | Data year | ⟨μ⟩ | Files | Dataset |
|---|---|---|---|---|
| mc23a | 2022 | 42 | 12 | `mc23_13p6TeV.603559.PhPy8EG_PDF4LHC21_HHbbyy_chhh1p0.deriv.DAOD_PHYS.e8564_s4162_r15540_p7266` |
| mc23d | 2023 | 51 | 13 | `mc23_13p6TeV.603559.PhPy8EG_PDF4LHC21_HHbbyy_chhh1p0.deriv.DAOD_PHYS.e8564_s4159_r15530_p7266` |
| mc23e | 2024 | 57 | 27 | `mc23_13p6TeV.603559.PhPy8EG_PDF4LHC21_HHbbyy_chhh1p0.deriv.DAOD_PHYS.e8564_s4369_r16083_p7266` |
| mc23g | 2025 | 60 | 33 | `mc23_13p6TeV.603559.PhPy8EG_PDF4LHC21_HHbbyy_chhh1p0.deriv.DAOD_PHYS.e8564_s4618_r17610_p7266` |

The rucio scope is `mc23_13p6TeV`. Note that r15540 is mc23a and r15530 is mc23d.

## Making the ntuples

Set up EasyJet (built against AthAnalysis 25.2.113) and copy `RunConfig-bbyy-isocheck.yaml`
into your run directory. One ntuple per campaign is enough: the same files are used for both
the kinematic and the isolation plots.

### Quick local test on one file

```bash
lsetup rucio; voms-proxy-init -voms atlas
DS=mc23_13p6TeV:mc23_13p6TeV.603559.PhPy8EG_PDF4LHC21_HHbbyy_chhh1p0.deriv.DAOD_PHYS.e8564_s4618_r17610_p7266
rucio list-files $DS                      # the files in the dataset
rucio download --nrandom 1 $DS            # one file, into a folder named after the dataset
bbyy-ntupler <downloaded file> --run-config RunConfig-bbyy-isocheck.yaml \
  --evtMax 1000 --out-file test_mc23g.root
```

Drop `--evtMax` to run over the whole file.

### On the grid

`submit_grid.sh` submits `bbyy-ntupler` for the four datasets above, one job per input file.
Set `RUN_CONFIG` (path to the run config) and `CAMPAIGN` (a name that goes into the output
dataset names) at the top, then:

```bash
source ~/work/easyjet/env.sh        # your EasyJet setup
lsetup panda; voms-proxy-init -voms atlas
./submit_grid.sh --dry-run          # prints what would be submitted, submits nothing
./submit_grid.sh                    # submit
```

- **Part of each dataset** (for a test): uncomment `N_FILES=3` in the script. Only
  N files of each dataset are run over (`--nFiles`).
- **Full datasets**: leave `N_FILES` commented out. This is 85 jobs in total.

The script submits from an empty directory under `/tmp/$USER` and writes the log to
`~/gridsubmit_<CAMPAIGN>.log`. A log inside the submit directory ends up in the job tarball
and makes the jobs fail with "command not found".

Follow the jobs with `pbook` (`show()`, `retry(<taskID>)`, `kill(<taskID>)`) or on
BigPanDA. The outputs are datasets named `user.<you>.<CAMPAIGN>.<date>.603559.<tags>_TREE`.
When they are done, download and merge one file per campaign:

```bash
rucio download user.<you>.<CAMPAIGN>.*_TREE
hadd mc23a_kin.root user.<you>.<CAMPAIGN>.*r15540*_TREE/*.root
hadd mc23d_kin.root user.<you>.<CAMPAIGN>.*r15530*_TREE/*.root
hadd mc23e_kin.root user.<you>.<CAMPAIGN>.*r16083*_TREE/*.root
hadd mc23g_kin.root user.<you>.<CAMPAIGN>.*r17610*_TREE/*.root
```

The file names are only a convention; the commands below assume them.

Things to know:

- The config uses bypass mode: every event is kept and each cut is stored as a flag
  (`bbyy_<CUT>_NOSYS`). The selection is applied in the plotting, so it can be changed
  without rerunning. `bbyy_pass_allcuts` is always true in bypass mode, so don't use it.
- `PASS_TRIGGER_MATCHING` must be in the cut list for MC, otherwise `PASS_RELPT` is never
  true and nothing passes.
- mc23g needs `Campaign.MC23g` in `EasyjetHub/python/steering/sample_metadata.py`.
  It has to map to `(2025,)` for now. With `(2025, 2026)` the job stops with
  "Invalid list of years, cannot combine runs", because 2026 has no GRL, lumicalc, PRW or
  trigger entries yet.

## Kinematic plots

```bash
D=/path/to/merged/ntuples           # the folder with mc23{a,d,e,g}_kin.root
S="--samples mc23a=$D/mc23a_kin.root mc23d=$D/mc23d_kin.root mc23e=$D/mc23e_kin.root mc23g=$D/mc23g_kin.root --ref mc23g --g35-g25-only --tag kinG35"

python -u plot_kinematics.py $S                  # all variables, shape and yield
python -u plot_kinematics.py $S --only mbb pTbb  # just some of them
python -u plot_kinematics.py $S --compare-kf     # mbb and pTbb without / with the kinematic fit
python -u plot_kinematics.py $S --cut-scan       # photon1_phi with the cuts added one at a time
```

For files on EOS, reading through XRootD is much faster than the `/eos` mount, e.g.
`D=root://eosuser.cern.ch//eos/user/<initial>/<username>/<folder>`.

Outputs are `<tag>_shape_<variable>.png`, `<tag>_yield_<variable>.png`, one PDF per mode,
`<tag>_kfcompare_*.png`, `<tag>_cutscan_*` and `<tag>_prw_weights.png`. The list of
variables, their binning and axis titles is the `plots` list near the top of the script.

Selection: the bbyy pre-selection with `--g35-g25-only`, i.e. the g35_g25 diphoton trigger
(fired and matched) and pT/mγγ > 0.35 / 0.25 as the only relpT cut. Without the flag the
g140 and (2024 only) g45_g20 routes are included too, which makes mc23e look ~7% higher
than the other campaigns.

Event weight: generator weight × pile-up weight × photon SFs (ID × iso) × jet SFs
(JVT, fJVT, GN2 b-tagging). In the cut scan the jet SFs are only applied once a jet cut is
included.

Normalisation:

- **shape**: each histogram scaled to unit area.
- **yield**: divided by the total generated sum of weights of the sample (CutBookkeeper), so
  each bin is the fraction of all generated events that pass the selection and fall in
  that bin. Summed over bins this gives the selection efficiency. The cross-section and
  luminosity are not applied, since they would only rescale each campaign by a constant.

Options:

| Option | |
|---|---|
| `--mode shape` / `yield` | make only one of the two (default both) |
| `--only NAME ...` | only these variables |
| `--g35-g25-only` | g35_g25 trigger and relpT only (use this) |
| `--at-least-two-bjets` | ≥ 2 b-jets instead of exactly 2 |
| `--no-prw`, `--no-sf`, `--no-jet-sf` | leave that weight out |
| `--compare-kf` | KF comparison only |
| `--cut-scan [NAME]` | cut scan of one variable (default `photon1_phi`) |
| `--no-skim` | read the files for every plot instead of making an in-memory copy first (slower) |
| `--tag` | prefix for the output files |

The b-tag SF is the event-level `ftag_effSF_GN2v01_Continuous_NOSYS`: EasyJet doesn't write
SFs for the fixed-cut working point used in the selection (`GN2v01_FixedCutBEff_90`). Worth
checking whether that is the right SF to use here.

## Isolation efficiency

Efficiency = (Tight + FixedCutLoose photons) / (Tight photons), unweighted, for the leading
and subleading photon, with Clopper–Pearson 68% CL errors. Events need the g35_g25 trigger
(fired and matched) and two loose photons. Bins with fewer than 20 photons in a sample are
not shown.

```bash
for conv in 0 1; do
  if [ $conv -eq 0 ]; then name=unconverted; else name=converted; fi
  for v in pt eta mu; do
    python plot_isoeff.py --var $v --conv $conv --ref mc23g \
      --samples mc23a=$D/mc23a_kin.root mc23d=$D/mc23d_kin.root \
                mc23e=$D/mc23e_kin.root mc23g=$D/mc23g_kin.root \
      --out isoEff_G35_${name}_${v}
  done
done
```

The pT plots also require |η| < 0.6 (`--eta-max`). `--photons leading` uses only the
leading photon.

## Acceptance check

```bash
python check_acceptance.py mc23a=$D/mc23a_kin.root mc23d=$D/mc23d_kin.root \
                           mc23e=$D/mc23e_kin.root mc23g=$D/mc23g_kin.root
```

Prints, for each campaign, the fraction of generated events left after each cut
(unweighted and with each weight added), and at the end each campaign divided by mc23g.
This uses the standard bbyy cuts (all trigger routes).

## Results so far

- With all events and no cuts, all campaigns agree (ratio 1.00), so the normalisation and
  pile-up weights are fine.
- With all trigger routes, mc23e is ~7.5% above mc23g after the full selection; the cut
  scan shows this comes from the trigger and relpT steps (the 2024-only g45_g20 route).
  The g35_g25-only selection removes that route.
- mc23a and mc23d are ~9% and ~5% above mc23g after the full selection. The difference
  builds up at the Tight ID + isolation cut and at the b-tag cut and follows ⟨μ⟩.
- Isolation efficiency: mc23a is up to ~4% higher at low pT (lower pile-up); at the same
  ⟨μ⟩ the campaigns agree within ~1%, and mc23g agrees with mc23e within ~1%.

## Next steps

- Check the b-tagging SF choice (see above), and whether the standard is exactly 2 or
  ≥ 2 b-jets.
- Add 2026 to the mc23g campaign once the 2026 GRL, lumicalc and PRW files exist.
