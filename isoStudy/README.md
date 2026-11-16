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

All four campaigns use the same DSID, generator tag and derivation, so only the simulation
and reconstruction (and with them the pile-up and detector conditions) differ:

```
mc23_13p6TeV.603559.PhPy8EG_PDF4LHC21_HHbbyy_chhh1p0.deriv.DAOD_PHYS.e8564_<s>_<r>_p7266
```

| Campaign | Data year | s-tag | r-tag | ⟨μ⟩ |
|---|---|---|---|---|
| mc23a | 2022 | s4162 | r15540 | 42 |
| mc23d | 2023 | s4159 | r15530 | 51 |
| mc23e | 2024 | s4369 | r16083 | 57 |
| mc23g | 2025 | s4618 | r17610 | 60 |

The ntuples from this study are in `/eos/user/h/hshaddix/isocheck_campaigns/`:
`mc23{a,d,e,g}_kin.root` (kinematics, with H→bb candidate and KF branches) and
`mc23{a,d,e,g}_isocheck.root` (isolation).

## Making the ntuples

Built against AthAnalysis 25.2.113. Copy the run config into your run directory, then for a
quick local test:

```bash
bbyy-ntupler <DAOD_PHYS file> --run-config RunConfig-bbyy-isocheck.yaml --evtMax 1000 --out-file test.root
```

For the full samples, use `submit_grid.sh` (set `RUN_CONFIG` and `CAMPAIGN` at the top):

```bash
source ~/work/easyjet/env.sh; lsetup panda; voms-proxy-init -voms atlas
./submit_grid.sh --dry-run   # check the printed datasets first
./submit_grid.sh
```

It runs one job per input file and submits from an empty directory under `/tmp/$USER`, with
the log in `~/gridsubmit_<campaign>.log`; a log inside the submit directory ends up in the
tarball and breaks the jobs. When the jobs are done, `rucio download` the `_TREE` output
datasets and `hadd` them per campaign.

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
D=root://eosuser.cern.ch//eos/user/h/hshaddix/isocheck_campaigns
S="--samples mc23a=$D/mc23a_kin.root mc23d=$D/mc23d_kin.root mc23e=$D/mc23e_kin.root mc23g=$D/mc23g_kin.root --ref mc23g --g35-g25-only --tag kinG35"

python -u plot_kinematics.py $S                  # all variables, shape and yield
python -u plot_kinematics.py $S --only mbb pTbb  # just some of them
python -u plot_kinematics.py $S --compare-kf     # mbb and pTbb without / with the kinematic fit
python -u plot_kinematics.py $S --cut-scan       # photon1_phi with the cuts added one at a time
```

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
D=/eos/user/h/hshaddix/isocheck_campaigns
for conv in 0 1; do
  if [ $conv -eq 0 ]; then name=unconverted; else name=converted; fi
  for v in pt eta mu; do
    python plot_isoeff.py --var $v --conv $conv --ref mc23g \
      --samples mc23a=$D/mc23a_isocheck.root mc23d=$D/mc23d_isocheck.root \
                mc23e=$D/mc23e_isocheck.root mc23g=$D/mc23g_isocheck.root \
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
