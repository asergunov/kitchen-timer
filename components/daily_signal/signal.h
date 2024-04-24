#pragma once

#include "esphome/components/time/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/core/time.h"
#include <cstddef>
#include <sys/_stdint.h>

namespace esphome {
namespace daily_signal {

class SignalComponent;

struct CronRTCData {
  optional<ESPTime> last_check;
};

class CronTrigger : public time::CronTrigger {
public:
  CronTrigger(time::RealTimeClock *time_source);
  void loop() override;

protected:
  CronRTCData &rtcData_;
};

class Timer : public CronTrigger {
public:
  Timer(time::RealTimeClock *rtc) : CronTrigger(rtc) {}
  void start(const esphome::ESPTime& time);
};

class SignalTrigger : public CronTrigger {
public:
  SignalTrigger(time::RealTimeClock *rtc) : CronTrigger(rtc) {}
  bool set_signal_time(uint8_t hour, uint8_t minute);
  void unset_signal_time();
protected:
};

class SignalComponent : public Component {
public:
  bool set_signal_time(const std::string &time_string);
  const std::string &get_signal_time_string() const { return time_string_; }
  void set_trigger(SignalTrigger *trigger) {
    trigger_ = trigger;
    set_signal_time(time_string_);
  }
  void set_time(time::RealTimeClock *clock) { _clock = clock; }

protected:
  std::string time_string_;
  SignalTrigger *trigger_ = nullptr;
  time::RealTimeClock *_clock = nullptr;
};

} // namespace daily_signal
} // namespace esphome
