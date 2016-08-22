#pragma once

class TaskBarIcon
{
public:
  TaskBarIcon();
  ~TaskBarIcon();

  enum State { S_PROGRESS, S_NO_PROGRESS, S_WORKING, S_ERROR, S_PAUSED };
  void SetState(State state, double param = 0.0);

protected:
  State last_state;
};

