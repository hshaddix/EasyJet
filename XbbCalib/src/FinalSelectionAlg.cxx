/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "FinalSelectionAlg.h"
#include <boost/json.hpp>

namespace XBBCALIB {

struct cutvals_t {
  // all the cuts are stored in this structure so you don't have to
  // edit a header file to add one
  std::optional<int64_t> n_lrjets_min;
  std::optional<SG::AuxElement::ConstAccessor<int>> veto_z_is_cc;
};

FinalSelectionAlg::FinalSelectionAlg(const std::string& name, ISvcLocator* sl)
  : AthAlgorithm(name, sl), m_cuts(std::make_unique<cutvals_t>())
{}

FinalSelectionAlg::~FinalSelectionAlg() = default;

StatusCode FinalSelectionAlg::initialize() {
  if (m_passHandle.empty()) {
    ATH_MSG_ERROR("selectionDecoration must be configured");
    return StatusCode::FAILURE;
  }
  // Parse JSON config
  auto obj = boost::json::parse(m_selectionConfigJson.value()).as_object();

  // remove keys as they are read, to make sure there aren't any
  // unused ones
  auto pop_if_contains = [&obj](const std::string& key) {
    if (const auto* v = obj.if_contains(key)) {
      auto ret = std::optional(*v);
      obj.erase(key);
      return ret;
    }
    return std::optional<boost::json::value>(std::nullopt);
  };

  // cuts are filled here
  auto& c = *m_cuts;
  if (auto v = pop_if_contains("n_lrjets_min")) {
    c.n_lrjets_min = v->as_int64();
  }
  if (auto v = pop_if_contains("veto_z_is_cc"); v && v->as_bool()) {
    c.veto_z_is_cc.emplace("z_is_cc");
  }

  // make sure keys aren't unconsumed
  if (!obj.empty()) {
    for (const auto& [key, val]: obj) {
      ATH_MSG_ERROR("unused key: '" << key << "', value: '" << val << "'");
    }
    return StatusCode::FAILURE;
  }

  // event handle must be initialised before decoration handles that reference it
  ATH_CHECK(m_eventHandle.initialize(m_systematicsList));
  ATH_CHECK(m_passHandle.initialize(m_systematicsList, m_eventHandle));

  if (c.n_lrjets_min) {
    m_lrjetsNHandle = std::make_shared<CP::SysReadDecorHandle<int>>(
        "lrjets_n_%SYS%", this);
    ATH_CHECK(m_lrjetsNHandle->initialize(m_systematicsList, m_eventHandle));
  }

  if (!m_blindingVariable.value().empty()) {
    m_blindingHandle = std::make_unique<CP::SysReadDecorHandle<float>>(
        m_blindingVariable.value() + "_%SYS%", this);
    ATH_CHECK(m_blindingHandle->initialize(m_systematicsList, m_eventHandle));
  }

  ATH_CHECK(m_systematicsList.initialize());
  return StatusCode::SUCCESS;
}

StatusCode FinalSelectionAlg::execute(const EventContext& ctx) {
  for (const auto& sys : m_systematicsList.systematicsVector()) {
    const xAOD::EventInfo* event = nullptr;
    ANA_CHECK(m_eventHandle.retrieve(event, sys, ctx));

    bool pass = true;

    // cuts are applied here
    const auto& c = *m_cuts;
    if (auto v = c.n_lrjets_min) {
      pass = pass && (m_lrjetsNHandle->get(*event, sys) >= *v);
    }

    if (c.veto_z_is_cc) {
      pass = pass && ((*c.veto_z_is_cc)(*event) == 0);
    }

    if (m_blindingHandle) {
      float bval = m_blindingHandle->get(*event, sys);
      if (bval > m_blindingMin && bval < m_blindingMax)
        pass = false;
    }
    m_passHandle.set(*event, static_cast<char>(pass), sys);
  }
  return StatusCode::SUCCESS;
}

} // namespace XBBCALIB
