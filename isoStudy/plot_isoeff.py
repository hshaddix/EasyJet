import argparse
from array import array

import ROOT

ROOT.gROOT.SetBatch(True)
ROOT.gStyle.SetOptStat(0)

MIN_PHOTONS = 20  # drop bins with fewer photons than this in a sample
COLORS = [ROOT.kRed, ROOT.kBlue, ROOT.kGreen + 2, ROOT.kMagenta + 1, ROOT.kOrange + 7]
MARKERS = [20, 21, 22, 23, 33]

ETA_BINS = [-2.37, -2.01, -1.81, -1.52, -1.37, -1.15, -0.9, -0.6, -0.3, 0.,
            0.3, 0.6, 0.9, 1.15, 1.37, 1.52, 1.81, 2.01, 2.37]

# expression (with {i} for the photon), bins, x title, apply the |eta| cut
VARS = {
    'pt':  ('bbyy_Photon{i}_pt_NOSYS*1e-3',
            [25, 30, 35, 40, 50, 60, 70, 80, 100, 125, 150, 200, 250, 300, 500],
            'photon p_{T} [GeV]', True),
    'eta': ('bbyy_Photon{i}_eta_NOSYS', ETA_BINS, 'photon #eta', False),
    'mu':  ('averageInteractionsPerCrossing',
            [20 + 2 * k for k in range(31)], '#LT#mu#GT', False),
}


def parse_args():
    p = argparse.ArgumentParser()
    p.add_argument('--samples', nargs='+', metavar='LABEL=FILE',
                   default=['mc23a=isocheck-mc23a-phys.root', 'mc23d=isocheck-mc23d-phys.root',
                            'mc23e=isocheck-mc23e-phys.root', 'mc23g=isocheck-mc23g-phys.root'])
    p.add_argument('--ref', default='mc23e', help='label of the reference sample for the ratio')
    p.add_argument('--var', choices=VARS, default='pt')
    p.add_argument('--photons', choices=['both', 'leading'], default='both')
    p.add_argument('--conv', type=int, choices=[0, 1], default=0, help='0 = unconverted, 1 = converted')
    p.add_argument('--eta-max', type=float, default=0.6, help='|eta| cut, only for --var pt')
    p.add_argument('--out', default=None)
    return p.parse_args()


def find_branch(names, *keys):
    match = [n for n in names if all(k in n for k in keys) and n.endswith('NOSYS')]
    if len(match) != 1:
        raise SystemExit(f'expected one branch with {keys}, found {match}')
    return match[0]


def fill(path, args, tag):
    expr, bins, _, eta_cut = VARS[args.var]
    f = ROOT.TFile.Open(path)
    t = f.Get('AnalysisMiniTree')
    names = [b.GetName() for b in t.GetListOfBranches()]
    tight = find_branch(names, 'baselineSelection_Tight_NonIso')
    tight_iso = find_branch(names, 'baselineSelection_Tight_FixedCutLoose')
    conv = find_branch(names, 'isConv')

    events = 'pass_trigger_diphoton_g35_g25_NOSYS && pass_matching_trigger_diphoton_g35_g25_NOSYS && bbyy_TWO_LOOSE_PHOTONS_NOSYS'
    den = ROOT.TH1D(f'den_{tag}', '', len(bins) - 1, array('d', bins))
    num = ROOT.TH1D(f'num_{tag}', '', len(bins) - 1, array('d', bins))

    # vector index k of the ph_ branches corresponds to bbyy_Photon{k+1}
    for k in ([0, 1] if args.photons == 'both' else [0]):
        i = k + 1
        photon = f'{tight}[{k}] && {conv}[{k}] == {args.conv}'
        if eta_cut:
            photon += f' && abs(bbyy_Photon{i}_eta_NOSYS) < {args.eta_max}'
        x = expr.format(i=i)
        t.Draw(f'{x} >>+ den_{tag}', f'{events} && {photon}', 'goff')
        t.Draw(f'{x} >>+ num_{tag}', f'{events} && {photon} && {tight_iso}[{k}]', 'goff')

    den.SetDirectory(0)
    num.SetDirectory(0)
    print(f'{path}: {int(den.Integral())} photons, {int(num.Integral())} isolated')
    f.Close()
    return num, den


def drop_sparse_bins(bins, num, den):
    for b in range(1, len(bins)):
        if den.GetBinContent(b) < MIN_PHOTONS:
            num.SetBinContent(b, 0)
            den.SetBinContent(b, 0)


def ratio(e_num, e_den, bins):
    g = ROOT.TGraphErrors()
    for b in range(1, len(bins)):
        if e_num.GetTotalHistogram().GetBinContent(b) == 0:
            continue
        a, c = e_num.GetEfficiency(b), e_den.GetEfficiency(b)
        if a <= 0 or c <= 0:
            continue
        sa = 0.5 * (e_num.GetEfficiencyErrorLow(b) + e_num.GetEfficiencyErrorUp(b))
        sc = 0.5 * (e_den.GetEfficiencyErrorLow(b) + e_den.GetEfficiencyErrorUp(b))
        r = a / c
        i = g.GetN()
        g.SetPoint(i, 0.5 * (bins[b - 1] + bins[b]), r)
        g.SetPointError(i, 0.5 * (bins[b] - bins[b - 1]), r * ((sa / a)**2 + (sc / c)**2)**0.5)
    return g


def main():
    args = parse_args()
    _, bins, xtitle, eta_cut = VARS[args.var]
    samples = [s.split('=', 1) for s in args.samples]
    labels = [label for label, _ in samples]
    if args.ref not in labels:
        raise SystemExit(f'reference {args.ref} not in {labels}')
    conv_name = 'converted' if args.conv else 'unconverted'
    out = args.out or f'isoEff_campaigns_{args.photons}_{conv_name}_{args.var}'

    pairs = [fill(path, args, f's{n}') for n, (_, path) in enumerate(samples)]
    for num, den in pairs:
        drop_sparse_bins(bins, num, den)
    effs = {label: ROOT.TEfficiency(num, den) for label, (num, den) in zip(labels, pairs)}

    c = ROOT.TCanvas('c', '', 800, 800)
    top = ROOT.TPad('top', '', 0, 0.35, 1, 1)
    bot = ROOT.TPad('bot', '', 0, 0, 1, 0.35)
    for pad in (top, bot):
        pad.SetLeftMargin(0.15)
        pad.SetRightMargin(0.05)
    top.SetBottomMargin(0.02)
    bot.SetTopMargin(0.03)
    bot.SetBottomMargin(0.30)
    top.Draw()
    bot.Draw()

    top.cd()
    frame = top.DrawFrame(bins[0], 0.75, bins[-1], 1.06, ';;Isolation efficiency')
    frame.GetXaxis().SetLabelSize(0)
    frame.GetYaxis().SetTitleOffset(1.4)
    leg = ROOT.TLegend(0.70, 0.06, 0.92, 0.32)
    leg.SetBorderSize(0)
    others = [label for label in labels if label != args.ref]
    style = {args.ref: (ROOT.kBlack, 21)}
    style.update({label: (COLORS[n], MARKERS[n]) for n, label in enumerate(others)})
    for label in [args.ref] + others:
        e = effs[label]
        e.SetLineColor(style[label][0])
        e.SetMarkerColor(style[label][0])
        e.SetMarkerStyle(style[label][1])
        e.Draw('pe same')
        leg.AddEntry(e, label + (' (ref)' if label == args.ref else ''), 'pe')
    leg.Draw()
    which = 'Both photons' if args.photons == 'both' else 'Leading photon'
    eta_text = f', |#eta| < {args.eta_max:g}' if eta_cut else ''
    tex = ROOT.TLatex()
    tex.SetNDC()
    tex.SetTextSize(0.035)
    tex.DrawLatex(0.19, 0.87, 'HH#rightarrowbb#gamma#gamma (#kappa_{#lambda}=1), #sqrt{s} = 13.6 TeV, DAOD_PHYS p7266')
    tex.DrawLatex(0.19, 0.82, f'{which}: {conv_name}, Tight ID{eta_text}')
    tex.DrawLatex(0.19, 0.77, 'Trigger + 2 loose #gamma, FixedCutLoose isolation')

    bot.cd()
    rframe = bot.DrawFrame(bins[0], 0.95, bins[-1], 1.05, f';{xtitle};mc23X / {args.ref}')
    for ax in (rframe.GetXaxis(), rframe.GetYaxis()):
        ax.SetTitleSize(0.08)
        ax.SetLabelSize(0.07)
    rframe.GetYaxis().SetTitleOffset(0.75)
    rframe.GetYaxis().SetNdivisions(504)
    rframe.GetXaxis().SetTitleOffset(1.1)
    graphs = []
    for label in others:
        g = ratio(effs[label], effs[args.ref], bins)
        g.SetLineColor(style[label][0])
        g.SetMarkerColor(style[label][0])
        g.SetMarkerStyle(style[label][1])
        g.Draw('pe same')
        graphs.append(g)
    line = ROOT.TLine(bins[0], 1, bins[-1], 1)
    line.SetLineStyle(2)
    line.Draw()

    c.SaveAs(f'{out}.png')
    c.SaveAs(f'{out}.pdf')


if __name__ == '__main__':
    main()
