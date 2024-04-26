#pragma once

#include "esphome/core/component.h"
#include "esphome/core/time.h"
#include "esphome/core/automation.h"
#include "esphome/components/time/real_time_clock.h"

namespace esphome {
namespace timer {

class TimerTrigger : public Trigger<>, public Component {
public:
  using seconds_type = uint32_t;
  TimerTrigger(time::RealTimeClock *rtc);
  void start(seconds_type seconds);
  seconds_type get_seconds_remain() const;
protected:
  void schedule();
protected:
  time::RealTimeClock *rtc_;
  optional<ESPTime>& signal_time_;
};

class TimerComponent : public Component {
public:
    using seconds_type = TimerTrigger::seconds_type;
  bool start(const std::string &time_string);
  void start(seconds_type secconds);
  void set_trigger(TimerTrigger *trigger) {
    trigger_ = trigger;
    start(time_string_);
  }
  void set_time(time::RealTimeClock *clock) { _clock = clock; }
  seconds_type get_seconds_remain() const {
    return trigger_ ? trigger_->get_seconds_remain() : 0;
  }

protected:
  std::string time_string_;
  TimerTrigger *trigger_ = nullptr;
  time::RealTimeClock *_clock = nullptr;
};

}}