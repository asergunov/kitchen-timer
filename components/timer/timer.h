#pragma once

#include "esphome/components/time/real_time_clock.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/time.h"

#include <string>

namespace esphome {
namespace timer {

using seconds_type = uint32_t;

class TimerTrigger : public Trigger<>, public Component {
public:
  using seconds_type = esphome::timer::seconds_type;
  TimerTrigger(time::RealTimeClock *rtc);
  void start(seconds_type seconds);
  optional<seconds_type> get_seconds_remain() const;
  const seconds_type& get_start_seconds() const { return start_seconds_; }

protected:
  void schedule();

protected:
  time::RealTimeClock *rtc_;
  bool& active_;
  time_t& timestamp_;
  seconds_type& start_seconds_;
};

class TimerComponent : public Component {
public:
  using seconds_type = esphome::timer::seconds_type;
  bool start(const std::string &time_string);
  void start(seconds_type seconds);
  void set_trigger(TimerTrigger *trigger);
  void set_time(time::RealTimeClock *clock);
  optional<seconds_type> get_seconds_remain() const;
  const seconds_type &get_start_seconds() const {
    static const seconds_type zero = 0;
    return trigger_ ? trigger_->get_start_seconds() : zero;
  }

protected:
  TimerTrigger *trigger_ = nullptr;
  time::RealTimeClock *_clock = nullptr;
};

} // namespace timer
} // namespace esphome