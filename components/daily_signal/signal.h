#pragma once

#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/time.h"

#include "esphome/components/time/real_time_clock.h"

#include <cstddef>
#include <cstdint>
#include <string>


namespace esphome {
namespace daily_signal {

using seconds_type = uint32_t;

struct CronRTCData {
  ESPTime next_signal;
  optional<ESPTime> last_check;
};

class SignalTrigger : public Trigger<>, public Component  {
public:
  SignalTrigger(time::RealTimeClock *rtc);
  bool set_signal_time(uint8_t hour, uint8_t minute);
  void unset_signal_time();
  optional<seconds_type> get_seconds_remain() const;
  void loop() override;
  const ESPTime& get_signal_time() const {
    return rtcData_.next_signal;
  }

protected:
  time::RealTimeClock *rtc_ = nullptr;
  CronRTCData& rtcData_;
  bool is_set_ = false;
};

class SignalComponent : public Component {
public:
  using seconds_type = esphome::daily_signal::seconds_type;
  bool set_signal_time(const std::string &time_string);
  // const std::string &get_signal_time_string() const { return time_string_; }
  void set_trigger(SignalTrigger *trigger);
  optional<seconds_type> get_seconds_remain() const {
    return trigger_ ? trigger_->get_seconds_remain() : optional<seconds_type>{};
  }
  const ESPTime& get_signal_time() const {
    return trigger_->get_signal_time();
  }

protected:
  SignalTrigger *trigger_ = nullptr;
};

} // namespace daily_signal
} // namespace esphome
