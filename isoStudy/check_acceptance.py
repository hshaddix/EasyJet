#!/usr/bin/env python3
# Where does mc23g lose events compared to mc23a/d/e?
#
# For each campaign this prints, cut by cut, the fraction of generated events
# that survive (cumulative), both unweighted and weighted, and the mean of
# each weight factor after the full selection.
#
# usage:
#   python check_acceptance.py mc23a=mc23a_kin.root mc23d=mc23d_kin.root \
#                              mc23e=mc23e_kin.root mc23g=mc23g_kin.root

import sys
import re
import ROOT

ROOT.gROOT.SetBatch(True)

cuts = ['PASS_TRIGGER', 'TWO_LOOSE_PHOTONS', 'PASS_TRIGGER_MATCHING',
        'TWO_TIGHTID_ISO_PHOTONS', 'PASS_RELPT', 'DIPHOTON_MASS',
        'EXACTLY_ZERO_LEPTONS', 'AT_LEAST_TWO_JETS',
        'LESS_THAN_SIX_CENTRAL_JETS', 'EXACTLY_TWO_B_JETS']


def find_cut_branch(branches, cut):
    # trigger flags have a _SINGLE_OR_DIPHOTON suffix, the rest don't
    pattern = re.compile(rf'^bbyy_{cut}(_SINGLE_OR_DIPHOTON)?_NOSYS$')
    for b in branches:
        if pattern.match(b):
            return b
    return None


def sum_of_weights(f):
    total = 0
    n_initial = 0
    for key in f.GetListOfKeys():
        name = key.GetName()
        if name.startswith('CutBookkeeper') and name.endswith('NOSYS'):
            h = key.ReadObj()
            n_initial += h.GetBinContent(1)   # initial number of events
            total += h.GetBinContent(2)       # initial sum of weights
    return n_initial, total


def weighted_sum(tree, weight, selection):
    h = ROOT.TH1D('h_tmp', '', 1, 0, 2)
    tree.Project('h_tmp', '1', f'({selection}) * ({weight})')
    value = h.GetBinContent(1)
    h.Delete()
    return value


def mean_of(tree, expr, selection):
    h = ROOT.TH1D('h_mean', '', 1000, -1e6, 1e6)
    tree.Project('h_mean', expr, selection)
    value = h.GetMean()
    h.Delete()
    return value


samples = []
for arg in sys.argv[1:]:
    label, path = arg.split('=', 1)
    samples.append((label, path))

if not samples:
    print(__doc__ if __doc__ else 'give label=file arguments')
    sys.exit(1)

results = {}

for label, path in samples:
    f = ROOT.TFile.Open(path)
    tree = f.Get('AnalysisMiniTree')
    branches = [b.GetName() for b in tree.GetListOfBranches()]

    n_initial, sumw_initial = sum_of_weights(f)

    prw = [b for b in branches if b.startswith('PileupWeight') and b.endswith('NOSYS')]
    photon_prw = [b for b in prw if 'photon' in b]
    prw_branch = photon_prw[0] if photon_prw else (prw[0] if prw else None)

    print(f'\n=== {label}  ({path})')
    print(f'  CutBookkeeper: N initial = {n_initial:.0f}, sum of weights = {sumw_initial:.1f}')
    print(f'  entries in tree = {tree.GetEntries()}')
    print(f'  pile-up weight branch: {prw_branch}')
    print(f'  all PRW branches: {prw}')

    # mean weights over ALL events in the tree (before any cut).
    # in bypass mode the tree should hold every event, so <PRW> here should be ~1
    if prw_branch:
        print(f'  <{prw_branch}> before cuts = {mean_of(tree, prw_branch, "1"):.4f}')
    print(f'  <generatorWeight> before cuts = {mean_of(tree, "generatorWeight_NOSYS", "1"):.4f}')

    w_gen = 'generatorWeight_NOSYS'
    w_prw = f'{w_gen} * {prw_branch}' if prw_branch else w_gen
    w_full = f'{w_prw} * bbyy_Photon1_effSF_NOSYS * bbyy_Photon2_effSF_NOSYS'

    print(f'  {"cut":30s} {"unweighted":>11s} {"gen":>9s} {"gen*PRW":>9s} {"gen*PRW*SF":>11s}')

    selection = '1'
    rows = []
    for cut in cuts:
        b = find_cut_branch(branches, cut)
        if b is None:
            print(f'  {cut:30s}   (no branch found, skipped)')
            continue
        selection += f' && {b}'

        n_pass = tree.GetEntries(selection)
        frac_raw = n_pass / n_initial if n_initial else 0
        frac_gen = weighted_sum(tree, w_gen, selection) / sumw_initial
        frac_prw = weighted_sum(tree, w_prw, selection) / sumw_initial

        # photon SFs only make sense once there are two tight photons
        if 'TIGHTID' in selection:
            frac_full = weighted_sum(tree, w_full, selection) / sumw_initial
            full_text = f'{frac_full:11.4f}'
        else:
            frac_full = None
            full_text = f'{"-":>11s}'

        print(f'  {cut:30s} {frac_raw:11.4f} {frac_gen:9.4f} {frac_prw:9.4f} {full_text}')
        rows.append((cut, frac_raw, frac_gen, frac_prw, frac_full))

    print('  after full selection:')
    if prw_branch:
        print(f'    <{prw_branch}>   = {mean_of(tree, prw_branch, selection):.4f}')
    print(f'    <photon1 SF>         = {mean_of(tree, "bbyy_Photon1_effSF_NOSYS", selection):.4f}')
    print(f'    <photon2 SF>         = {mean_of(tree, "bbyy_Photon2_effSF_NOSYS", selection):.4f}')

    results[label] = rows
    f.Close()


# side by side: each campaign's cumulative fraction divided by mc23g's
ref = 'mc23g' if 'mc23g' in results else samples[-1][0]
print(f'\n=== ratio to {ref} of the cumulative fraction (columns: unweighted / gen*PRW*SF)')
header = f'  {"cut":30s}' + ''.join(f'{lab:>18s}' for lab, _ in samples if lab != ref)
print(header)
ref_rows = {r[0]: r for r in results[ref]}
for cut in cuts:
    if cut not in ref_rows:
        continue
    line = f'  {cut:30s}'
    for lab, _ in samples:
        if lab == ref:
            continue
        row = {r[0]: r for r in results[lab]}.get(cut)
        r_ref = ref_rows[cut]
        raw = row[1] / r_ref[1] if r_ref[1] else 0
        if row[4] is not None and r_ref[4]:
            full = f'{row[4] / r_ref[4]:.3f}'
        else:
            full = '-'
        line += f'{raw:>10.3f} / {full:>5s}'
    print(line)

