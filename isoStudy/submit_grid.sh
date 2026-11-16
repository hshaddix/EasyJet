#!/bin/bash
# Submit bbyy-ntupler grid jobs for the photon isolation study.
# usage: source ~/work/easyjet/env.sh; lsetup panda; voms-proxy-init -voms atlas
#        ./submit_grid.sh [--dry-run]

RUN_CONFIG=~/work/easyjet/run/RunConfig-bbyy-isocheck.yaml
CAMPAIGN=isoKin
FILES_PER_JOB=1      # one job per input file
# N_FILES=3          # uncomment to only run over the first N files (quick test)

# datasets to run over, one per line (DAOD_PHYS, all p7266)
mc_list=(
    mc23_13p6TeV:mc23_13p6TeV.603559.PhPy8EG_PDF4LHC21_HHbbyy_chhh1p0.deriv.DAOD_PHYS.e8564_s4159_r15530_p7266  # mc23d
    mc23_13p6TeV:mc23_13p6TeV.603559.PhPy8EG_PDF4LHC21_HHbbyy_chhh1p0.deriv.DAOD_PHYS.e8564_s4162_r15540_p7266  # mc23a
    mc23_13p6TeV:mc23_13p6TeV.603559.PhPy8EG_PDF4LHC21_HHbbyy_chhh1p0.deriv.DAOD_PHYS.e8564_s4369_r16083_p7266  # mc23e
    mc23_13p6TeV:mc23_13p6TeV.603559.PhPy8EG_PDF4LHC21_HHbbyy_chhh1p0.deriv.DAOD_PHYS.e8564_s4618_r17610_p7266  # mc23g
)

# prun archives the submit directory, so submit from a clean one
# and keep the log outside of it
SUBMIT_DIR=/tmp/$USER/grid_${CAMPAIGN}
LOG=~/gridsubmit_${CAMPAIGN}.log
rm -rf "$SUBMIT_DIR"
mkdir -p "$SUBMIT_DIR"
cd "$SUBMIT_DIR"
cp "$RUN_CONFIG" .
printf '%s\n' "${mc_list[@]}" > mc_list.txt

opts=(--mc-list mc_list.txt
      --run-config "$(basename "$RUN_CONFIG")"
      --exec bbyy-ntupler
      --campaign "$CAMPAIGN"
      --nFilesPerJob "$FILES_PER_JOB"
      --noTag)
[[ -n "$N_FILES" ]] && opts+=(--nFiles "$N_FILES")

echo "submitting from $SUBMIT_DIR, log in $LOG"
cat mc_list.txt
easyjet-gridsubmit "${opts[@]}" "$@" > "$LOG" 2>&1
status=$?
grep -iE "jediTaskID|error|prun --inDS" "$LOG"
exit $status
