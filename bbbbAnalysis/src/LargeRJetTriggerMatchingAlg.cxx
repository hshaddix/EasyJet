/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "LargeRJetTriggerMatchingAlg.h"

#include "TrigCompositeUtils/ChainNameParser.h"

#include <algorithm>
#include <set>

namespace HH4B {

  LargeRJetTriggerMatchingAlg::LargeRJetTriggerMatchingAlg(const std::string &name, ISvcLocator *pSvcLocator)
      : AthHistogramAlgorithm(name, pSvcLocator) { }

  std::string LargeRJetTriggerMatchingAlg::sanitize(const std::string& trig)
  {
    std::string modified = trig;
    std::replace(modified.begin(), modified.end(), '-', '_');
    std::replace(modified.begin(), modified.end(), '.', 'p');
    return modified;
  }

  bool LargeRJetTriggerMatchingAlg::hasLargeRJetLeg(const std::string& trig)
  {
    // large-R ("a10", i.e. anti-kt R=1.0) jet legs still report signature "j"
    // in ChainNameParser -- only the leg parts distinguish them from small-R jets
    for (const ChainNameParser::LegInfo& legInfo : ChainNameParser::HLTChainInfo(trig))
    {
      if (legInfo.signature == "j" && legInfo.legName().find("_a10") != std::string::npos)
        return true;
    }
    return false;
  }

  StatusCode LargeRJetTriggerMatchingAlg::initialize()
  {
    ATH_CHECK(m_eventHandle.initialize());

    m_yearKey = "EventInfo.dataTakingYear";
    ATH_CHECK(m_yearKey.initialize());

    ATH_CHECK(m_LRjetsIn.initialize());

    // skim down to chains with at least one large-R jet leg
    for (const auto& [year, chains] : m_triggersByYear.value())
    {
      std::vector<std::string> lrChains;
      for (const auto& trig : chains)
      {
        if (hasLargeRJetLeg(trig)) lrChains.push_back(trig);
        else ATH_MSG_DEBUG("Skipping trigger with no large-R jet leg: " << trig);
      }
      if (!lrChains.empty()) m_LRJetTriggersByYear.emplace(year, std::move(lrChains));
    }

    // one read handle per distinct trigger chain across all configured years
    std::set<std::string> allTriggers;
    for (const auto& [year, chains] : m_LRJetTriggersByYear)
      for (const auto& trig : chains)
        allTriggers.insert(trig);

    for (const auto& trig : allTriggers)
    {
      const std::string modifiedTrigName = sanitize(trig);

      // reads the "trigPassed_<name>" flag written by the ASG trigger-decision config
      SG::ReadDecorHandleKey<xAOD::EventInfo> passKey(m_eventHandle, "trigPassed_"+modifiedTrigName);
      m_triggerPassDecoKeys.emplace(trig, passKey);
      ATH_CHECK(m_triggerPassDecoKeys.at(trig).initialize());

      m_jetIsMatchedKeys.emplace(trig, SG::Decorator<char>("isLRjetMatched_"+modifiedTrigName));
    }

    return StatusCode::SUCCESS;
  }

  StatusCode LargeRJetTriggerMatchingAlg::execute(const EventContext& ctx)
  {
    SG::ReadHandle<xAOD::EventInfo> event(m_eventHandle, ctx);
    ATH_CHECK(event.isValid());

    SG::ReadDecorHandle<xAOD::EventInfo, unsigned int> yearDecor(m_yearKey, ctx);
    const std::string year = std::to_string(yearDecor(*event));

    // triggers configured for this event's data-taking year that actually fired
    std::set<std::string> passedTriggers;
    const auto it = m_LRJetTriggersByYear.find(year);
    if (it != m_LRJetTriggersByYear.end())
    {
      for (const auto& trig : it->second)
      {
        SG::ReadDecorHandle<xAOD::EventInfo, bool> passDecor(m_triggerPassDecoKeys.at(trig), ctx);
        if (passDecor(*event)) passedTriggers.insert(trig);
      }
    }
    else
    {
      ATH_MSG_DEBUG("No boosted LR-jet trigger chains configured for data-taking year " << year);
    }

    SG::ReadHandle<xAOD::JetContainer> LRjetsIn(m_LRjetsIn, ctx);
    ATH_CHECK(LRjetsIn.isValid());

    // Single-object matching: assumes each configured chain requires
    // only one LR jet leg. Revisit if/when asymmetric (multi-leg)
    // LR-jet triggers are added -- use the vector<IParticle*> overload
    // of IMatchingTool::match() for those instead.
    for (const auto& [trig, decorator] : m_jetIsMatchedKeys)
    {
      const bool trigPassed = passedTriggers.contains(trig);//.count(trig) > 0;

      for (const xAOD::Jet *jet : *LRjetsIn)
      {
        decorator(*jet) = trigPassed && m_matchingTool->match(*jet, trig, m_matchThreshold);
      }
    }

    return StatusCode::SUCCESS;
  }

}
