/*
   Copyright (C) 2002-2024 CERN for the benefit of the ATLAS collaboration

*/


// Always protect against multiple includes!
#ifndef VBSSEMILEPANALYSIS_ENUMS
#define VBSSEMILEPANALYSIS_ENUMS

namespace VBSSEMILEP
{
  enum TriggerChannel
  {
    SLT,SJT, SJTM,
  };

  enum Var_Trigger 
  {
    ele = 0,
    mu = 1,
    leadingele = 2,
    leadingmu = 3,
    subleadingele = 4,
    subleadingmu = 5,
  };

  enum RunBooleans
  {
    is22_75bunches,
    is23_75bunches,
    is23_400bunches,
  };

  enum Booleans{
    PASS_TRIGGER,
    PASS_ONE_LEPTON,
    PASS_TWO_LEPTONS,
    PASS_MET,
    PASS_ONE_LARGE_JET,
    PASS_TWO_SIGNAL_JETS,
    PASS_VBS_BASELINE,
    PASS_RES_BASELINE,
    PASS_B_VETO,
    PASS_B_TAGGED,
    PASS_TWO_SS_CHARGE_LEPTONS,
    PASS_TWO_OS_CHARGE_LEPTONS,
    PASS_MERG_BASELINE,
    PASS_DELTA_R_SS,
    PASS_RES_Z_WINDOW
  };
}

#endif
