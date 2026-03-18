/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#ifndef BBBBTTANALYSIS_TTBARREWVARS
#define BBBBTTANALYSIS_TTBARREWVARS

#include <AthenaBaseComps/AthHistogramAlgorithm.h>

#include <AsgDataHandles/ReadHandleKey.h>
#include <AsgDataHandles/WriteDecorHandleKey.h>
#include <AsgDataHandles/WriteDecorHandle.h>

#include <xAODEventInfo/EventInfo.h>
#include <xAODTruth/TruthParticleContainer.h>

namespace HHHBBBBTT
{

class ttbarRewVarsAlg final : public AthHistogramAlgorithm
{

public:
    ttbarRewVarsAlg(const std::string &name, ISvcLocator *pSvcLocator);

    StatusCode initialize() override;
    StatusCode execute() override;

private:

    SG::ReadHandleKey<xAOD::EventInfo>
      m_EventHandleKey{ this, "EventHandleKey", "EventInfo", "EventInfo to read" };

    SG::WriteDecorHandleKey<xAOD::EventInfo>
      m_Pt_top_decor{
        this,"Pt_top","EventInfo.Pt_top","Truth top pT"};

    SG::WriteDecorHandleKey<xAOD::EventInfo>
      m_Pt_antitop_decor{
        this,"Pt_antitop","EventInfo.Pt_antitop","Truth anti-top pT"};

    SG::WriteDecorHandleKey<xAOD::EventInfo>
      m_Pt_ttbar_decor{
        this,"Pt_ttbar","EventInfo.Pt_ttbar","Truth ttbar pT"};

    SG::WriteDecorHandleKey<xAOD::EventInfo>
      m_M_ttbar_decor{
        this,"M_ttbar","EventInfo.M_ttbar","Truth ttbar M"};

    std::set<unsigned int> tt_pythia8_samples = {
      410470,410472,411073,411074,411075,411076,411077,411078,412066,412067,
      412068,412069,412070,412071,407342,407343,407344,407348,407349,407350,
      411335,411336,411337,411079,411080,411081,412072,412073,412074,
      410429,410444,410431,410445,410435,410447,410464,410465,410480,
      410482,601495,601497,601491,601226,601227,410471,603003,603190,
      603192,601229,601230,601237};

    std::set<unsigned int> tt_herwig7_samples = {
      410557,410558,411233,411234,600666,600667,600668,411338,411339,
      412116,412117,411332,411333,411334,411329,411330,411331,
      411088,411089,411090};

};

}

#endif