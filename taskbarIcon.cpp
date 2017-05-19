#include "stdafx.h"
#include "taskbarIcon.h"
#include <plugin.hpp>

TaskBarIcon::TaskBarIcon() : lastState(S_NO_PROGRESS)
{
  value.Total = 100;
}

TaskBarIcon::~TaskBarIcon()
{
  if (lastState != S_NO_PROGRESS)
    fInfo.AdvControl(&MainGuid, ACTL_SETPROGRESSSTATE, TBPS_NOPROGRESS, nullptr);
}

void TaskBarIcon::SetState(State state, double param)
{
  switch (state)
  {
  case S_PROGRESS:
    if (param > 1) param = 1;
    value.Completed = int(param * 100);

    fInfo.AdvControl(&MainGuid, ACTL_SETPROGRESSSTATE, TBPS_NORMAL, nullptr);
    fInfo.AdvControl(&MainGuid, ACTL_SETPROGRESSVALUE, 0, &value);
    break;

  case S_NO_PROGRESS:
    fInfo.AdvControl(&MainGuid, ACTL_PROGRESSNOTIFY, 0, nullptr);
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

  lastState = state;
}
