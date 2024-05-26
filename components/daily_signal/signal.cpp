#include "signal.h"

#include "esp_attr.h"
#include "esphome/components/time/automation.h"
#include "esphome/core/log.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <ctime>

namespace esphome {
namespace daily_signal {

ESPTime next_signal_time(const ESPTime &time, const uint8_t hour,
                         const uint8_t minute) {
  ESPTime signal_time = time;
  if (signal_time.hour > hour ||
      (signal_time.hour == hour && signal_time.minute > minute)) {
    // We are late today. So next day
    signal_time.increment_day();
  }

  // Correct the
  signal_time.timestamp += ((int64_t(hour) - int64_t(signal_time.hour)) * 60 +
                            (int64_t(minute) - int64_t(signal_time.minute))) *
                           60;
  signal_time.hour = hour;
  signal_time.minute = minute;

  return signal_time;
}

static const char *const TAG = "daily_signal";

RTC_DATA_ATTR std::array<CronRTCData, 16> g_rtcData;
size_t g_nextFreeRtcData = 0;

SignalTrigger::SignalTrigger(time::RealTimeClock *time_source)
    : rtc_(time_source), rtcData_(g_rtcData[g_nextFreeRtcData++]) {
  ESP_LOGD(TAG, "Creating cron trigger");

  if (g_nextFreeRtcData >= g_rtcData.size()) {
    ESP_LOGE(TAG, "g_rtcData too small");
    abort();
  }
}

void SignalTrigger::loop() {
  if (!this->is_set_ || !this->rtc_)
    return;

  const auto &time = this->rtc_->now();
  if (!time.is_valid())
    return;

  if (!rtcData_.last_check) {
    // Maybe decrement it a bit?
    rtcData_.last_check = time;
    rtcData_.next_signal = next_signal_time(time, rtcData_.next_signal.hour,
                                            rtcData_.next_signal.minute);
  }

  rtcData_.last_check = time;

  if (rtcData_.next_signal > time)
    return;

  rtcData_.next_signal.increment_day();

  this->trigger();
}

bool SignalTrigger::set_signal_time(uint8_t hour, uint8_t minute) {
  ESP_LOGD(TAG, "Setting signal time to %u:%u", hour, minute);
  if (hour >= 24) {
    ESP_LOGW(TAG, "Hour should be less then 24");
    return false;
  }
  if (minute >= 60) {
    ESP_LOGW(TAG, "Minute should be less then 60");
    return false;
  }

  if (this->is_set_ && rtcData_.next_signal.hour == hour &&
      rtcData_.next_signal.minute == minute) {
    ESP_LOGI(TAG, "Not changed");
    return true;
  }

  const auto &time = this->rtc_->now();
  if (time.is_valid()) {
    rtcData_.last_check = time;
    rtcData_.next_signal = next_signal_time(time, hour, minute);
  } else {
    this->rtcData_.next_signal = {};
    this->rtcData_.last_check = {};
  }

  this->rtcData_.next_signal.hour = hour;
  this->rtcData_.next_signal.minute = minute;
  this->is_set_ = true;
  return true;
}

void SignalTrigger::unset_signal_time() { this->is_set_ = false; }

optional<seconds_type> SignalTrigger::get_seconds_remain() const {
  if (!this->is_set_ || !this->rtc_)
    return {};

  if (!rtcData_.next_signal.is_valid())
    return {};

  const auto &time = this->rtc_->now();
  if (!time.is_valid())
    return {};

  return rtcData_.next_signal.timestamp > time.timestamp
             ? rtcData_.next_signal.timestamp - time.timestamp
             : 0;
}

bool SignalComponent::set_signal_time(const std::string &time_string) {
  if (time_string.empty()) {
    if (trigger_)
      trigger_->unset_signal_time();
    ESP_LOGI(TAG, "Empty line. Cleaning up.");
    return true;
  }

  tm parsed_time;
  if (nullptr == strptime(time_string.c_str(), "%H:%M", &parsed_time)) {
    ESP_LOGW(TAG, "Time expected in format HH:MM, got '%s'",
             time_string.c_str());
    return false;
  }

  ESP_LOGI(TAG, "Time parsed. Setting %d hours and %d minutes",
           parsed_time.tm_hour, parsed_time.tm_min);

  if (trigger_ &&
      !trigger_->set_signal_time(parsed_time.tm_hour, parsed_time.tm_min))
    return false;

  return true;
}

void SignalComponent::set_trigger(SignalTrigger *trigger) {
  ESP_LOGI(TAG, "Setting up the trigger");
  trigger_ = trigger;
  // set_signal_time(time_string_);
}

} // namespace daily_signal
} // namespace esphome