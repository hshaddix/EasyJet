/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "ZbbHistogramAlg.h"

#include <SystematicsHandles/SysReadDecorHandle.h>
#include "H5Cpp.h"
#include "HDF5Utils/histogram.h"
#include "boost/histogram.hpp"
#include <numeric>
#include <map>

namespace {

namespace bh = boost::histogram;
using WS = bh::accumulators::weighted_sum<float>;
using Hist4D = decltype(bh::make_histogram_with(
    bh::dense_storage<WS>(),
    bh::axis::regular<float>(1, 0.f, 1.f, ""),
    bh::axis::variable<float>(std::vector<float>{0.f, 1.f}, ""),
    bh::axis::category<int>(std::vector<int>{0}, ""),
    bh::axis::category<int>(std::vector<int>{0}, "")));

class ZbbHist final : public IZbbHist {
public:
  ZbbHist(int mass_bins, float mass_lower, float mass_upper,
          const std::vector<float>& pt_edges,
          int n_wps_plus_notag, int n_sys)
  {
    std::vector<int> wp_cats(static_cast<std::size_t>(n_wps_plus_notag));
    std::iota(wp_cats.begin(), wp_cats.end(), 0);
    std::vector<int> sys_cats(static_cast<std::size_t>(n_sys));
    std::iota(sys_cats.begin(), sys_cats.end(), 0);
    m_hist = bh::make_histogram_with(
        bh::dense_storage<WS>(),
        bh::axis::regular<float>(mass_bins, mass_lower, mass_upper, "mass"),
        bh::axis::variable<float>(pt_edges, "pt"),
        bh::axis::category<int>(wp_cats, "wp"),
        bh::axis::category<int>(sys_cats, "sys"));
  }

  void fill(float mass, float pt, int wp_idx, int sys_idx,
            float weight) override {
    m_hist(mass, pt, wp_idx, sys_idx, bh::weight(weight));
  }

  void write(H5::Group& parent,
             const std::string& name,
             const std::vector<std::string>& wp_labels,
             const std::vector<std::string>& sys_labels) const override {
    std::map<std::string, std::vector<std::string>> lmap;
    lmap["wp"]  = wp_labels;
    lmap["sys"] = sys_labels;
    H5Utils::hist::write_hist_to_group(parent, m_hist, name, lmap);
  }

private:
  Hist4D m_hist;
};

} // anonymous namespace


namespace XBBCALIB {

ZbbHistogramAlg::ZbbHistogramAlg(const std::string& name,
                                 ISvcLocator* pSvcLocator)
  : AthAlgorithm(name, pSvcLocator) {}

ZbbHistogramAlg::~ZbbHistogramAlg() = default;

StatusCode ZbbHistogramAlg::initialize() {
  if (m_ptEdges.value().size() < 2) {
    ATH_MSG_ERROR("ptEdges must have at least 2 entries");
    return StatusCode::FAILURE;
  }

  ATH_CHECK(m_eventHandle.initialize(m_systematicsList));

  if (!m_weightDecoName.value().empty()) {
    m_weightHandle = std::make_unique<CP::SysReadDecorHandle<float>>(
        m_weightDecoName.value(), this);
    ATH_CHECK(m_weightHandle->initialize(m_systematicsList, m_eventHandle));
  }

  m_massHandle = std::make_unique<CP::SysReadDecorHandle<float>>(
      m_massVarName.value() + "_%SYS%", this);
  ATH_CHECK(m_massHandle->initialize(m_systematicsList, m_eventHandle));

  m_ptHandle = std::make_unique<CP::SysReadDecorHandle<float>>(
      m_ptVarName.value() + "_%SYS%", this);
  ATH_CHECK(m_ptHandle->initialize(m_systematicsList, m_eventHandle));

  if (!m_selectionDecoName.value().empty()) {
    m_selHandle = std::make_shared<CP::SysReadDecorHandle<char>>(
        m_selectionDecoName.value(), this);
    ATH_CHECK(m_selHandle->initialize(m_systematicsList, m_eventHandle));
  }

  const auto& decos = m_wpDecoNames.value();
  for (int wi = 0; wi < static_cast<int>(decos.size()); ++wi) {
    const auto& deco = decos.at(wi);
    if (deco.empty()) {
      m_wpHandles.emplace_back(wi, nullptr);  // notag: always filled
      m_wpLabels.push_back("notag");
    } else {
      auto h = std::make_unique<CP::SysReadDecorHandle<int>>(
          deco + "_%SYS%", this);
      ATH_CHECK(h->initialize(m_systematicsList, m_eventHandle));
      m_wpHandles.emplace_back(wi, std::move(h));
      m_wpLabels.push_back(deco);
    }
  }

  ATH_CHECK(m_output_svc.retrieve());
  ATH_CHECK(m_systematicsList.initialize());

  const auto& sysvec = m_systematicsList.systematicsVector();
  const int n_sys = static_cast<int>(sysvec.size());
  for (int si = 0; si < n_sys; ++si) {
    m_sysPairs.emplace_back(si, sysvec[si]);
    m_sysLabels.push_back(
        sysvec[si].name().empty() ? "NOSYS" : sysvec[si].name());
  }

  m_hist = std::make_unique<ZbbHist>(
      m_massBins, m_massLower, m_massUpper,
      m_ptEdges.value(),
      static_cast<int>(decos.size()),
      n_sys);

  // Invariants maintained by the loops above -- failures indicate a bug.
  if (m_wpLabels.size() != decos.size()) {
    ATH_MSG_ERROR("WP label count mismatch: "
        << m_wpLabels.size() << " labels vs "
        << decos.size() << " decorations");
    return StatusCode::FAILURE;
  }
  if (m_sysLabels.size() != m_sysPairs.size()) {
    ATH_MSG_ERROR("sys label count mismatch: "
        << m_sysLabels.size() << " labels vs "
        << m_sysPairs.size() << " sys pairs");
    return StatusCode::FAILURE;
  }

  return StatusCode::SUCCESS;
}

StatusCode ZbbHistogramAlg::execute(const EventContext& ctx) {
  for (const auto& [si, sys] : m_sysPairs) {
    const xAOD::EventInfo* event = nullptr;
    ATH_CHECK(m_eventHandle.retrieve(event, sys, ctx));
    if (m_selHandle && !m_selHandle->get(*event, sys)) continue;

    const float mass   = m_massHandle->get(*event, sys);
    const float pt     = m_ptHandle->get(*event, sys);
    const float weight = m_weightHandle
        ? m_weightHandle->get(*event, sys) : 1.f;

    for (const auto& [wi, h] : m_wpHandles)
      if (!h || h->get(*event, sys) != 0)
        m_hist->fill(mass, pt, wi, si, weight);
  }
  return StatusCode::SUCCESS;
}

StatusCode ZbbHistogramAlg::finalize() {
  H5::Group root_grp = m_output_svc->group()->openGroup("/");
  H5::Group sub_grp;
  H5::Group* parent = &root_grp;
  if (!m_groupName.value().empty()) {
    const std::string& gn = m_groupName.value();
    if (!H5Lexists(root_grp.getId(), gn.c_str(), H5P_DEFAULT))
      root_grp.createGroup(gn);
    sub_grp = root_grp.openGroup(gn);
    parent = &sub_grp;
  }
  m_hist->write(*parent, m_histName.value(), m_wpLabels, m_sysLabels);
  return StatusCode::SUCCESS;
}

} // namespace XBBCALIB
