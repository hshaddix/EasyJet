HZyAnalysis FW Development Log
=====================================

2026-09-14 (Tagged **HZy_Ntuple_v0.2**)
----------
**Author:** Shaoguang, Eli, Raphael, Qiuping
- Target ptag: `p7017` and Athena AthAnalysis: `25.2.107`
- Fixed `LLGam_M_Cstr` for events with FSR corrections and added the calibrated FSR-object four-momentum.
- Implemented weighted cutflow and JVT treatment.
- Fixed the `JVT` scale factor for data and added the data22 CI job.
- Fixed the ggH generator weight in the weighted cutflow.
- Saved `DFCommonJets_eventClean_LooseBad` variable (**The impacts to be studied**), but jet cleaning with `DFCommonJets_jetClean_LooseBad` added during overlap removal (`PhotonVasHZyAlg`).
- Added associated-Z decay truth-label information and introduced the `save_truth_vars` flag.
- Added additional photon-ID working points available in the `p7017` DAOD and updated the corresponding selections.
- Harmonized HZyAnalysis with the latest EasyJet developments and checked code formatting.
- Added Run-2 and Run-3 MC/data sample lists.
- Added the grid submission helper `SubmitToGrid` via `atlas_install_scripts`.
- Added grid support for Run-2/Run-3 signal, background, and data samples.
- Added dedicated `Run3_purity` submission using `RunConfig-HZy-purity.yaml`.
- Added the `ECMS_Zjets` grid submission option.
- Installed dataset lists via `atlas_install_data(datasets/*)`.
- Updated the grid submission script to resolve configs and dataset lists via `DATAPATH` / `PathResolver`.
- Updated the README documentation for grid submission.
- **NOTE**: [photon isolation](https://gitlab.cern.ch/atlas/athena/-/blob/main/PhysicsAnalysis/ElectronPhotonID/IsolationCorrections/Root/IsolationCorrectionTool.cxx?ref_type=heads#L25) should use `v6` but the [athena algorithm](https://gitlab.cern.ch/atlas/athena/-/blob/main/PhysicsAnalysis/ElectronPhotonID/IsolationCorrections/Root/IsolationCorrectionTool.cxx?ref_type=heads#L25) is still not updated, the current solution is using private modified athena (only photon isolation algorithm changing from `v5` to `v6` witn `REL22`)

Notes
-----
- Keep entries short and user-facing.
- Add new changes at the top of this file.
- Use dates in YYYY-MM-DD format.
