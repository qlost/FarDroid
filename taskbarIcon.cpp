#include "stdafx.h"
#include "taskbarIcon.h"
#include <plugin.hpp>

TaskBarIcon::TaskBarIcon() : last_state(S_NO_PROGRESS)
{
}

TaskBarIcon::~TaskBarIcon()
{
  if (last_state != S_NO_PROGRESS)
    SetState(S_NO_PROGRESS);
}

void TaskBarIcon::SetState(State state, double param)
{
  switch (state)
  {
  case S_PROGRESS:
    if (param > 1)
    {
      param = 1;
    }
    if (param < 0)
    {
      param = 0;
    }
    ProgressValue pv;
    pv.Completed = int(param * 100);
    pv.Total = 100;
    fInfo.AdvControl(&MainGuid, ACTL_SETPROGRESSSTATE, TBPS_NORMAL, nullptr);
    fInfo.AdvControl(&MainGuid, ACTL_SETPROGRESSVALUE, 0, &pv);
    break;

  case S_NO_PROGRESS:
    fInfo.AdvControl(&MainGuid, ACTL_SETPROGRESSSTATE, TBPS_NOPROGRESS, nullptr);
    break;
  case S_WORKING:
    fInfo.AdvControl(&MainGuid, ACTL_SETPROGRESSSTATE, TBPS_INDETERMINATE, nullptr);
    break;
  case S_ERROR:
    fInfo.AdvControl(&MainGuid, ACTL_SETPROGRESSSTATE, TBPS_ERROR, nullptr);
    break;
  case S_PAUSED:
    fInfo.AdvControl(&MainGuid, ACTL_SETPROGRESSSTATE, TBPS_PAUSED, nullptr);
    break;
  }
  last_state = state;
}
