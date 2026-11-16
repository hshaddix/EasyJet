# Kinematic comparison of mc23 campaigns for the HH->bbyy signal, ratio to a reference campaign.
# shape: unit area. yield: divided by the generated sum of weights. See README.md.

import argparse
import re

import ROOT

ROOT.gROOT.SetBatch(True)
ROOT.gStyle.SetOptStat(0)

# bbyy pre-selection (ntuples are made in bypass mode)
cuts = ['PASS_TRIGGERSINGLE_OR_DIPHOTON', 'TWO_LOOSE_PHOTONS', 'PASS_TRIGGER_MATCHINGSINGLE_OR_DIPHOTON',
        'TWO_TIGHTID_ISO_PHOTONS', 'PASS_RELPT', 'DIPHOTON_MASS', 'EXACTLY_ZERO_LEPTONS',
        'AT_LEAST_TWO_JETS', 'LESS_THAN_SIX_CENTRAL_JETS', 'EXACTLY_TWO_B_JETS']
presel_cuts = [(c, 'bbyy_' + c + '_NOSYS') for c in cuts]

# --g35-g25-only: g35_g25 trigger + matching, relpT 0.35/0.25
g35_g25_replace = {
    'PASS_TRIGGERSINGLE_OR_DIPHOTON': ('G35_G25_TRIGGER', 'pass_trigger_diphoton_g35_g25_NOSYS'),
    'PASS_TRIGGER_MATCHINGSINGLE_OR_DIPHOTON': ('G35_G25_MATCHING', 'pass_matching_trigger_diphoton_g35_g25_NOSYS'),
    'PASS_RELPT': ('G35_G25_RELPT', 'bbyy_Photon1_ptOvermyy_NOSYS > 0.35 && bbyy_Photon2_ptOvermyy_NOSYS > 0.25'),
}
g35_g25_cuts = [g35_g25_replace.get(c, (c, e)) for c, e in presel_cuts]

presel = ' && '.join(e for c, e in presel_cuts)
presel_text = 'All bbyy pre-selections'

# plots drawn without a ratio panel
no_ratio = set()

# y-axis headroom (default 1.45)
headroom = {'mu': 1.8}

colors = [ROOT.kRed, ROOT.kBlue, ROOT.kGreen + 2, ROOT.kMagenta + 1]

# name, expression, binning (or 'auto'), x axis title
plots = [
    ('photon1_pt',    'bbyy_Photon1_pt_NOSYS/1000.',  (30, 20, 320),   'leading photon p_{T} [GeV]'),
    ('photon1_eta',   'bbyy_Photon1_eta_NOSYS',       (24, -2.4, 2.4), 'leading photon #eta'),
    ('photon1_phi',   'bbyy_Photon1_phi_NOSYS',       (16, -3.2, 3.2), 'leading photon #phi'),
    ('photon1_E',     'bbyy_Photon1_E_NOSYS/1000.',   (30, 0, 600),    'leading photon E [GeV]'),
    ('photon1_relpt', 'bbyy_Photon1_ptOvermyy_NOSYS', (30, 0, 2),      'leading photon p_{T}/m_{#gamma#gamma}'),

    ('photon2_pt',    'bbyy_Photon2_pt_NOSYS/1000.',  (30, 20, 320),   'subleading photon p_{T} [GeV]'),
    ('photon2_eta',   'bbyy_Photon2_eta_NOSYS',       (24, -2.4, 2.4), 'subleading photon #eta'),
    ('photon2_phi',   'bbyy_Photon2_phi_NOSYS',       (16, -3.2, 3.2), 'subleading photon #phi'),
    ('photon2_E',     'bbyy_Photon2_E_NOSYS/1000.',   (30, 0, 600),    'subleading photon E [GeV]'),
    ('photon2_relpt', 'bbyy_Photon2_ptOvermyy_NOSYS', (30, 0, 2),      'subleading photon p_{T}/m_{#gamma#gamma}'),

    ('pTyy', 'bbyy_pTyy_NOSYS/1000.', (30, 0, 400),   'p_{T}^{#gamma#gamma} [GeV]'),
    ('myy',  'bbyy_myy_NOSYS/1000.',  (20, 115, 135), 'm_{#gamma#gamma} [GeV]'),

    ('bjet1_pt',    'bbyy_HbbCandidate_Jet1_pt_NOSYS/1000.', (30, 0, 300),    'leading H#rightarrowbb jet p_{T} [GeV]'),
    ('bjet1_eta',   'bbyy_HbbCandidate_Jet1_eta_NOSYS',      (25, -2.5, 2.5), 'leading H#rightarrowbb jet #eta'),
    ('bjet1_phi',   'bbyy_HbbCandidate_Jet1_phi_NOSYS',      (16, -3.2, 3.2), 'leading H#rightarrowbb jet #phi'),
    ('bjet1_E',     'bbyy_HbbCandidate_Jet1_E_NOSYS/1000.',  (30, 0, 600),    'leading H#rightarrowbb jet E [GeV]'),
    ('bjet1_relpt', 'bbyy_HbbCandidate_Jet1_pt_NOSYS/bbyy_mbb_NOSYS', (30, 0, 2), 'leading H#rightarrowbb jet p_{T}/m_{bb}'),

    ('bjet2_pt',    'bbyy_HbbCandidate_Jet2_pt_NOSYS/1000.', (30, 0, 300),    'subleading H#rightarrowbb jet p_{T} [GeV]'),
    ('bjet2_eta',   'bbyy_HbbCandidate_Jet2_eta_NOSYS',      (25, -2.5, 2.5), 'subleading H#rightarrowbb jet #eta'),
    ('bjet2_phi',   'bbyy_HbbCandidate_Jet2_phi_NOSYS',      (16, -3.2, 3.2), 'subleading H#rightarrowbb jet #phi'),
    ('bjet2_E',     'bbyy_HbbCandidate_Jet2_E_NOSYS/1000.',  (30, 0, 600),    'subleading H#rightarrowbb jet E [GeV]'),
    ('bjet2_relpt', 'bbyy_HbbCandidate_Jet2_pt_NOSYS/bbyy_mbb_NOSYS', (30, 0, 2), 'subleading H#rightarrowbb jet p_{T}/m_{bb}'),

    ('mbb',        'bbyy_mbb_NOSYS/1000.',        (30, 0, 300),    'm_{bb} [GeV]'),
    ('KF_mbb',     'bbyy_KF_mbb_NOSYS/1000.',     (30, 0, 300),    'm_{bb} (KF) [GeV]'),
    ('pTbb',       'bbyy_pTbb_NOSYS/1000.',       (30, 0, 400),    'p_{T}^{bb} [GeV]'),
    ('KF_pTbb',    'bbyy_KF_pTbb_NOSYS/1000.',    (30, 0, 400),    'p_{T}^{bb} (KF) [GeV]'),
    ('mbbyy',      'bbyy_mbbyy_NOSYS/1000.',      (32, 200, 1000), 'm_{bb#gamma#gamma} [GeV]'),
    ('mbbyy_star', 'bbyy_mbbyy_star_NOSYS/1000.', (32, 200, 1000), 'm*_{bb#gamma#gamma} [GeV]'),

    ('KF_HT',        'bbyy_KF_HT_NOSYS/1000.',  'auto',          'H_{T} (KF) [GeV]'),
    ('KF_topness',   'bbyy_KF_topness_NOSYS',   'auto',          'topness (KF)'),
    ('nCentralJets', 'bbyy_nCentralJets_NOSYS', (8, -0.5, 7.5),  'number of central jets'),
    ('nBJets',       'bbyy_nBJets_NOSYS',       (5, -0.5, 4.5),  'number of b-jets (GN2v01 90%)'),
    ('mu',           'averageInteractionsPerCrossing', (40, 0, 80), '#LT#mu#GT'),

    ('photon1_IDSF', 'ph_ph_id_effSF_Tight_FixedCutLoose_NOSYS[0]', (40, 0.9, 1.1), 'leading photon ID SF'),
    ('photon2_IDSF', 'ph_ph_id_effSF_Tight_FixedCutLoose_NOSYS[1]', (40, 0.9, 1.1), 'subleading photon ID SF'),
]


def get_prw_branch(branches):
    # use the photon PRW if available
    prw = sorted(b for b in branches if b.startswith('PileupWeight') and b.endswith('NOSYS'))
    photon_prw = [b for b in prw if 'photon' in b]
    if photon_prw:
        return photon_prw[0]
    if prw:
        return prw[0]
    return None


# event-level jet SFs: JVT, fJVT, GN2 b-tagging
jet_sf_branches = ['jvt_effSF_NOSYS', 'fjvt_effSF_NOSYS', 'ftag_effSF_GN2v01_Continuous_NOSYS']

# jet SFs are only applied once one of these cuts is in the selection
jet_cuts = ['AT_LEAST_TWO_JETS', 'LESS_THAN_SIX_CENTRAL_JETS', 'EXACTLY_TWO_B_JETS', 'AT_LEAST_TWO_B_JETS']


def get_weight(branches, use_sf, use_prw, use_gen=True, use_jet=False):
    w = []
    text = []
    if use_gen and 'generatorWeight_NOSYS' in branches:
        w.append('generatorWeight_NOSYS')
        text.append('MC weight')

    prw = get_prw_branch(branches)
    if use_prw and prw:
        w.append(prw)
        text.append('pile-up weight')

    if use_sf:
        w.append('bbyy_Photon1_effSF_NOSYS')
        w.append('bbyy_Photon2_effSF_NOSYS')
        text.append('photon ID #times iso SF')

    if use_jet:
        found = [b for b in jet_sf_branches if b in branches]
        missing = [b for b in jet_sf_branches if b not in branches]
        if missing:
            print(f'  warning: jet SF branches not in the file, left out: {missing}')
        if found:
            w += found
            text.append('jet SFs')

    if not w:
        return '1', 'unweighted'
    return ' * '.join(w), ' #times '.join(text)


def get_cutbookkeeper(f):
    n_events = 0
    total = 0
    for key in f.GetListOfKeys():
        name = key.GetName()
        if name.startswith('CutBookkeeper') and name.endswith('NOSYS'):
            h = key.ReadObj()
            # bin 1 is "Initial events", bin 2 is "Initial sum of weights"
            print('  ', name, h.GetBinContent(1), 'events,', h.GetBinContent(2), 'sum of weights')
            n_events += h.GetBinContent(1)
            total += h.GetBinContent(2)
    return n_events, total


def auto_range(tree, expr, nbins=30):
    # 1-99% range of the reference sample
    tree.SetEstimate(tree.GetEntries() + 1)
    n = tree.Draw(expr, presel, 'goff')
    if n <= 0:
        return (nbins, 0, 1)
    values = sorted(tree.GetV1()[i] for i in range(n))
    lo = values[int(0.01 * n)]
    hi = values[int(0.99 * n) - 1]
    if hi <= lo:
        hi = lo + 1
    return (nbins, lo, hi)


def fold_overflow(h, name, label):
    # move under/overflow into the edge bins
    n = h.GetNbinsX()
    total = h.Integral(0, n + 1)
    outside = h.GetBinContent(0) + h.GetBinContent(n + 1)
    if total > 0 and outside / total > 0.01:
        print(f'  warning: {name} ({label}) has {100 * outside / total:.1f}% of events outside the axis range')

    h.SetBinContent(1, h.GetBinContent(1) + h.GetBinContent(0))
    h.SetBinError(1, (h.GetBinError(1)**2 + h.GetBinError(0)**2)**0.5)
    h.SetBinContent(n, h.GetBinContent(n) + h.GetBinContent(n + 1))
    h.SetBinError(n, (h.GetBinError(n)**2 + h.GetBinError(n + 1)**2)**0.5)
    h.SetBinContent(0, 0)
    h.SetBinContent(n + 1, 0)


def per_bin_text(binning, xtitle):
    if xtitle.startswith('number of'):
        return ''
    width = (binning[2] - binning[1]) / binning[0]
    if '[GeV]' in xtitle:
        return f' / {width:.3g} GeV'
    return f' / {width:.3g}'


def make_prw_plot(c, trees, branches, labels, color, tag):
    # PRW weight distribution per campaign, all events
    c.Clear()
    c.SetLeftMargin(0.15)
    c.SetRightMargin(0.05)
    c.SetLogy(True)

    hists = []
    leg = ROOT.TLegend(0.50, 0.65, 0.92, 0.88)
    leg.SetBorderSize(0)
    print('mean pile-up weight before any selection (generator-weighted):')
    for label in labels:
        prw = get_prw_branch(branches[label])
        if prw is None:
            print(f'  {label}: no PileupWeight branch')
            continue

        h = ROOT.TH1D(f'h_prw_{label}', '', 60, 0, 3)
        h.Sumw2()
        # include overflow in the mean
        ROOT.TH1.StatOverflows(True)
        trees[label].Draw(f'{prw} >> h_prw_{label}', 'generatorWeight_NOSYS', 'goff')
        ROOT.TH1.StatOverflows(False)
        h.SetDirectory(0)
        mean = h.GetMean()
        fold_overflow(h, 'prw', label)
        if h.Integral() > 0:
            h.Scale(1 / h.Integral())
        h.SetLineColor(color[label])
        h.SetLineWidth(2)
        print(f'  {label}: {prw}, mean = {mean:.4f}')
        leg.AddEntry(h, f'{label}, mean = {mean:.3f}', 'l')
        hists.append(h)

    if not hists:
        c.SetLogy(False)
        return

    hists[0].SetMaximum(5 * max(h.GetMaximum() for h in hists))
    hists[0].SetMinimum(1e-5)
    hists[0].GetXaxis().SetTitle('pile-up weight')
    hists[0].GetYaxis().SetTitle('Fraction of events / 0.05')
    hists[0].GetYaxis().SetTitleOffset(1.4)
    hists[0].Draw('hist')
    for h in hists[1:]:
        h.Draw('hist same')
    leg.Draw()

    tex = ROOT.TLatex()
    tex.SetNDC()
    tex.SetTextSize(0.032)
    tex.DrawLatex(0.19, 0.86, 'HH#rightarrowbb#gamma#gamma, DAOD_PHYS p7266')
    tex.DrawLatex(0.19, 0.81, 'All events, no selection')

    c.SaveAs(f'{tag}_prw_weights.png')
    c.SetLogy(False)


def ratio_range(ratios):
    values = []
    for r in ratios:
        for i in range(1, r.GetNbinsX() + 1):
            if r.GetBinContent(i) > 0:
                values.append(r.GetBinContent(i))
    if not values or (min(values) > 0.8 and max(values) < 1.2):
        return 0.8, 1.2
    lo = min(values)
    hi = max(values)
    pad = 0.15 * (hi - lo)
    return max(0, min(lo - pad, 0.8)), max(hi + pad, 1.2)


def make_plot(c, hists, ref, others, binning, xtitle, ytitle, weight_text, show_ratio=True,
              show_selection=True, ymax_factor=1.45):
    c.Clear()
    c.cd()
    if show_ratio:
        top = ROOT.TPad('top', '', 0, 0.35, 1, 1)
        top.SetBottomMargin(0.02)
    else:
        top = ROOT.TPad('top', '', 0, 0, 1, 1)
        top.SetBottomMargin(0.12)
    bottom = ROOT.TPad('bottom', '', 0, 0, 1, 0.35)
    top.SetLeftMargin(0.15)
    top.SetRightMargin(0.05)
    bottom.SetLeftMargin(0.15)
    bottom.SetRightMargin(0.05)
    bottom.SetTopMargin(0.03)
    bottom.SetBottomMargin(0.30)
    top.Draw()
    if show_ratio:
        bottom.Draw()

    # distributions
    top.cd()
    ymax = max(h.GetMaximum() for h in hists.values())
    hists[ref].SetMaximum(ymax_factor * ymax)
    hists[ref].SetMinimum(0)
    hists[ref].GetYaxis().SetTitle(ytitle)
    hists[ref].GetYaxis().SetTitleOffset(1.4)
    if show_ratio:
        hists[ref].GetXaxis().SetLabelSize(0)
    else:
        hists[ref].GetXaxis().SetTitle(xtitle)
    hists[ref].Draw('hist')
    for label in others:
        hists[label].Draw('hist same')
    hists[ref].Draw('hist same')  # reference on top

    # keep text size the same when there is no ratio pad
    scale = 1.0 if show_ratio else 0.65
    leg = ROOT.TLegend(0.68, 0.88 - 0.26 * scale, 0.92, 0.88)
    leg.SetBorderSize(0)
    leg.AddEntry(hists[ref], ref + ' (ref)', 'l')
    for label in others:
        leg.AddEntry(hists[label], label, 'l')
    leg.Draw()

    tex = ROOT.TLatex()
    tex.SetNDC()
    tex.SetTextSize(0.032 * scale)
    tex.DrawLatex(0.19, 0.86, 'HH#rightarrowbb#gamma#gamma (#kappa_{#lambda}=1), DAOD_PHYS p7266')
    y = 0.86 - 0.05 * scale
    if show_selection:
        tex.DrawLatex(0.19, y, presel_text)
        y -= 0.05 * scale
    tex.SetTextSize(0.024 * scale)
    tex.DrawLatex(0.19, y + 0.005 * scale, 'Weights: ' + weight_text)

    if not show_ratio:
        return [top, bottom, leg, tex]

    # ratio to the reference
    bottom.cd()
    ratios = []
    for label in others:
        r = hists[label].Clone(hists[label].GetName() + '_ratio')
        r.Divide(hists[ref])
        r.SetMarkerStyle(20)
        r.SetMarkerSize(0.8)
        ratios.append(r)

    rmin, rmax = ratio_range(ratios)
    frame = bottom.DrawFrame(binning[1], rmin, binning[2], rmax, f';{xtitle};mc23X / {ref}')
    frame.GetXaxis().SetTitleSize(0.08)
    frame.GetXaxis().SetLabelSize(0.07)
    frame.GetXaxis().SetTitleOffset(1.1)
    frame.GetYaxis().SetTitleSize(0.08)
    frame.GetYaxis().SetLabelSize(0.07)
    frame.GetYaxis().SetTitleOffset(0.75)
    frame.GetYaxis().SetNdivisions(504)

    for r in ratios:
        r.Draw('e same')

    line = ROOT.TLine(binning[1], 1, binning[2], 1)
    line.SetLineStyle(2)
    line.Draw()

    # keep references until the canvas is saved
    return [top, bottom, leg, tex, line, frame] + ratios


def fill_hists(name, expr, binning, xtitle, mode, labels, trees, weights, total, color, suffix='',
               selection=None):
    if selection is None:
        selection = presel
    hists = {}
    for label in labels:
        weight = weights[label]

        hname = f'h_{name}_{label}_{mode}{suffix}'
        h = ROOT.TH1D(hname, '', *binning)
        h.Sumw2()
        trees[label].Draw(f'({expr}) >> {hname}', f'({selection}) * ({weight})', 'goff')
        h.SetDirectory(0)
        fold_overflow(h, name, label)
        h.SetLineColor(color[label])
        h.SetMarkerColor(color[label])
        h.SetLineWidth(2)

        hists[label] = h

    if mode == 'unscaled':
        return hists, ''
    return scale_hists(hists, mode, binning, xtitle, total)


def scale_hists(hists, mode, binning, xtitle, total, suffix=''):
    scaled = {}
    for label, h0 in hists.items():
        h = h0.Clone(h0.GetName() + suffix) if suffix else h0
        h.SetDirectory(0)
        if mode == 'shape':
            if h.Integral() > 0:
                h.Scale(1 / h.Integral())
        elif mode == 'yield':
            h.Scale(1 / total[label])
        scaled[label] = h

    if mode == 'shape':
        ytitle = 'Normalised events' + per_bin_text(binning, xtitle)
    else:
        ytitle = 'Fraction of generated events' + per_bin_text(binning, xtitle)
    return scaled, ytitle


def skim_trees(trees, branches, labels):
    # copy the selected events and needed branches into memory once
    exprs = [presel] + [p[1] for p in plots]
    for label in labels:
        exprs.append(get_weight(branches[label], True, True, True, True)[0])
    tokens = set()
    for e in exprs:
        tokens.update(re.findall(r'[A-Za-z_]\w*', e))

    skims = {}
    for label in labels:
        tree = trees[label]
        keep = [b for b in branches[label] if b in tokens]
        tree.SetBranchStatus('*', 0)
        for b in keep:
            tree.SetBranchStatus(b, 1)
        ROOT.gROOT.cd()  # make the copy in memory, not in the input file
        skim = tree.CopyTree(presel)
        tree.SetBranchStatus('*', 1)
        print(f'{label}: {skim.GetEntries()} events and {len(keep)} branches kept in memory', flush=True)
        skims[label] = skim
    return skims


def cut_scan(plot, args, labels, others, trees, branches, total, color):
    # plot one variable with 0..N cuts applied
    global presel_text
    name, expr, binning, xtitle = plot
    if binning == 'auto':
        binning = auto_range(trees[args.ref], expr)

    mode = 'shape' if args.mode == 'shape' else 'yield'

    # skip default values (-99) before the object cuts
    filled = f'({expr}) > -90'

    # weights with and without jet SFs; photon SFs set to 1 where not defined
    scan_weights = {}
    scan_text = {}
    for use_jet in [False, True]:
        scan_weights[use_jet] = {}
        for label in labels:
            w, scan_text[use_jet] = get_weight(branches[label], not args.no_sf, not args.no_prw, True,
                                               use_jet and not args.no_jet_sf)
            for sf in ['bbyy_Photon1_effSF_NOSYS', 'bbyy_Photon2_effSF_NOSYS']:
                w = w.replace(sf, f'(({sf} > 0) * {sf} + ({sf} <= 0))')
            scan_weights[use_jet][label] = w

    c = ROOT.TCanvas('c_scan', '', 800, 800)
    pdf = f'{args.tag}_cutscan_{name}.pdf'
    c.Print(pdf + '[')

    rows = []
    keep = []
    for n in range(len(presel_cuts), -1, -1):
        applied = presel_cuts[:n]
        selection = ' && '.join([e for _, e in applied] + [filled])
        if n == len(presel_cuts):
            presel_text = 'All pre-selections'
        elif n == 0:
            presel_text = 'No pre-selection'
        else:
            presel_text = f'Cuts up to {applied[-1][0]}'

        has_jet_cut = any(c in jet_cuts for c, _ in applied)
        weights = scan_weights[has_jet_cut]
        weight_text = scan_text[has_jet_cut]

        unscaled, _ = fill_hists(name, expr, binning, xtitle, 'unscaled', labels, trees, weights, total, color,
                                 f'_scan{n}', selection)
        sums = {label: unscaled[label].Integral() / total[label] for label in labels}
        hists, ytitle = scale_hists(unscaled, mode, binning, xtitle, total)
        keep += make_plot(c, hists, args.ref, others, binning, xtitle, ytitle, weight_text,
                          show_ratio=name not in no_ratio, show_selection=False)
        keep += list(hists.values())
        c.Print(pdf)
        c.SaveAs(f'{args.tag}_cutscan_{name}_{n:02d}.png')

        last = applied[-1][0] if applied else '(none)'
        rows.append((n, last, sums))

    c.Print(pdf + ']')

    print(f'\ncut scan for {name}: fraction of generated events after the cuts up to and including')
    header = f'  {"n":>2s}  {"last cut applied":40s}' + ''.join(f'{l:>10s}' for l in labels)
    header += ''.join(f'{l + "/" + args.ref:>14s}' for l in others)
    print(header)
    for n, last, sums in rows:
        line = f'  {n:2d}  {last:40s}' + ''.join(f'{sums[l]:10.4f}' for l in labels)
        line += ''.join(f'{sums[l] / sums[args.ref]:14.4f}' for l in others)
        print(line)


# (without KF, with KF)
kf_pairs = [('mbb', 'KF_mbb'), ('pTbb', 'KF_pTbb')]


def compare_kf(args, labels, others, trees, branches, weights, weight_text, total, color):
    mode = 'shape' if args.mode == 'shape' else 'yield'
    by_name = {p[0]: p for p in plots}

    for plain, kf in kf_pairs:
        missing = [l for l in labels if 'bbyy_' + kf + '_NOSYS' not in branches[l]]
        if missing:
            print(f'skip {kf}: branch not found in {missing} (was do_KinematicFit on?)')
            continue

        c = ROOT.TCanvas(f'c_kf_{plain}', '', 1600, 800)
        c.Divide(2, 1)
        keep = []
        totals = {}
        for i, name in enumerate([plain, kf]):
            _, expr, binning, xtitle = by_name[name]
            hists, ytitle = fill_hists(name, expr, binning, xtitle, mode, labels, trees, weights, total, color,
                                       '_kfcompare')
            totals[name] = {label: hists[label].Integral() for label in labels}

            pad = c.cd(i + 1)
            keep += make_plot(pad, hists, args.ref, others, binning, xtitle, ytitle, weight_text)
            keep += list(hists.values())

            for label in labels:
                h = hists[label]
                print(f'  {name:8s} {label}: mean = {h.GetMean():7.2f}, RMS = {h.GetRMS():6.2f}')

        c.SaveAs(f'{args.tag}_kfcompare_{plain}.png')
        c.SaveAs(f'{args.tag}_kfcompare_{plain}.pdf')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--samples', nargs='+', required=True, help='label=file, e.g. mc23g=mc23g_kin.root')
    parser.add_argument('--ref', default='mc23g')
    parser.add_argument('--mode', choices=['shape', 'yield', 'both'], default='both')
    parser.add_argument('--no-sf', action='store_true', help='leave out the photon SFs')
    parser.add_argument('--no-prw', action='store_true', help='leave out the pile-up weight')
    parser.add_argument('--no-jet-sf', action='store_true', help='leave out the jet SFs (JVT, fJVT, b-tagging)')
    parser.add_argument('--only', nargs='*', help='only make these plots')
    parser.add_argument('--cut-scan', nargs='?', const='photon1_phi', metavar='NAME',
                        help='cut scan of one variable (default photon1_phi)')
    parser.add_argument('--compare-kf', action='store_true',
                        help='mbb and pTbb without and with the kinematic fit')
    parser.add_argument('--at-least-two-bjets', action='store_true',
                        help='require >= 2 b-jets instead of exactly 2')
    parser.add_argument('--g35-g25-only', action='store_true',
                        help='g35_g25 trigger and relpT cuts only')
    parser.add_argument('--no-skim', action='store_true',
                        help='no in-memory copy of the selected events (slower)')
    parser.add_argument('--tag', default='kin')
    args = parser.parse_args()

    global presel, presel_cuts, presel_text
    if args.g35_g25_only:
        presel_cuts = g35_g25_cuts
        presel_text = 'bbyy pre-selection, g35_g25 trigger only'

    if args.at_least_two_bjets:
        presel_cuts = [('AT_LEAST_TWO_B_JETS', 'bbyy_nBJets_NOSYS >= 2') if c == 'EXACTLY_TWO_B_JETS' else (c, e)
                       for c, e in presel_cuts]
        presel_text += ', #geq 2 b-jets'

    presel = ' && '.join(e for c, e in presel_cuts)
    if args.g35_g25_only or args.at_least_two_bjets:
        print('pre-selection:', presel)

    labels = []
    files = {}
    trees = {}
    branches = {}
    for s in args.samples:
        label, path = s.split('=', 1)
        labels.append(label)
        files[label] = ROOT.TFile.Open(path)
        trees[label] = files[label].Get('AnalysisMiniTree')
        trees[label].SetCacheSize(200 * 1024 * 1024)
        trees[label].AddBranchToCache('*', True)
        branches[label] = set(b.GetName() for b in trees[label].GetListOfBranches())

    if args.ref not in labels:
        raise SystemExit(f'reference {args.ref} is not one of {labels}')
    others = [l for l in labels if l != args.ref]

    if args.mode == 'both':
        modes = ['shape', 'yield']
    else:
        modes = [args.mode]

    weights = {}
    weight_text = ''
    n_gen = {}
    sumw = {}
    for label in labels:
        weights[label], weight_text = get_weight(branches[label], not args.no_sf, not args.no_prw, True,
                                                 not args.no_jet_sf)
        print(f'{label}: CutBookkeeper')
        n_gen[label], sumw[label] = get_cutbookkeeper(files[label])
        n_sel = trees[label].GetEntries(presel)
        print(f'{label}: {n_sel} of {n_gen[label]:.0f} generated events pass the pre-selection '
              f'({100 * n_sel / n_gen[label]:.2f}%), weight = {weights[label]}')

    color = {args.ref: ROOT.kBlack}
    for i, label in enumerate(others):
        color[label] = colors[i]

    c = ROOT.TCanvas('c', '', 800, 800)
    make_prw_plot(c, trees, branches, labels, color, args.tag)

    if args.cut_scan:
        chosen = [p for p in plots if p[0] == args.cut_scan]
        if not chosen:
            raise SystemExit(f'no plot called {args.cut_scan}, choose from: {[p[0] for p in plots]}')
        total = sumw
        cut_scan(chosen[0], args, labels, others, trees, branches, total, color)
        return

    if not args.no_skim:
        trees = skim_trees(trees, branches, labels)

    if args.compare_kf:
        total = sumw
        compare_kf(args, labels, others, trees, branches, weights, weight_text, total, color)
        return

    for mode in modes:
        c.Print(f'{args.tag}_{mode}_plots.pdf[')

    keep = []
    for name, expr, binning, xtitle in plots:
        if args.only and name not in args.only:
            continue

        needed = [t for t in re.findall(r'[A-Za-z_]\w*', expr) if t.endswith('NOSYS') or t.startswith('average')]
        missing = [l for l in labels if any(b not in branches[l] for b in needed)]
        if missing:
            print(f'skip {name}: branches not found in {missing}')
            continue

        if binning == 'auto':
            binning = auto_range(trees[args.ref], expr)

        total = sumw
        weighted = None
        for mode in modes:
            if weighted is None:
                weighted, _ = fill_hists(name, expr, binning, xtitle, 'unscaled', labels, trees, weights,
                                         total, color)
            hists, ytitle = scale_hists(weighted, mode, binning, xtitle, total, f'_{mode}')
            keep += make_plot(c, hists, args.ref, others, binning, xtitle, ytitle, weight_text,
                              show_ratio=name not in no_ratio, ymax_factor=headroom.get(name, 1.45))
            keep += list(hists.values())
            c.Print(f'{args.tag}_{mode}_plots.pdf')
            c.SaveAs(f'{args.tag}_{mode}_{name}.png')

    for mode in modes:
        c.Print(f'{args.tag}_{mode}_plots.pdf]')


if __name__ == '__main__':
    main()
