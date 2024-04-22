#pragma once

#include "esphome/components/time/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/core/time.h"

namespace esphome {
namespace daily_signal {

class SignalComponent;

class SignalTrigger : public time::CronTrigger {
public:
  SignalTrigger(time::RealTimeClock *time_source);
  void set_time_string(const std::string &time_string);
  const std::string &get_time_signal() const { return _time_string; }

protected:
  std::string _time_string;
};

class SignalComponent : public Component {
public:
  void set_time_signal(const std::string &time_string);
  std::string get_time_signal() const {
    if (!_trigger)
      return {};
    return _trigger->get_time_signal();
  }
  void set_trigger(SignalTrigger *trigger) { _trigger = trigger; }
  void set_time(time::RealTimeClock *clock) { _clock = clock; }

private:
  SignalTrigger *_trigger = nullptr;
  time::RealTimeClock *_clock = nullptr;
};

} // namespace daily_signal
} // namespace esphome
