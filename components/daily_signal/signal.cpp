#include "signal.h"
#include "esp_attr.h"
#include "esphome/components/time/automation.h"
#include "esphome/core/log.h"
#include <array>
#include <cstddef>

namespace esphome {
namespace daily_signal {

static const char *const TAG = "daily_signal";

// RTC_DATA_ATTR std::array<CronRTCData, 16> g_rtcData;
// size_t g_nextFreeRtcData = 0;

CronTrigger::CronTrigger(time::RealTimeClock *time_source)
    : time::CronTrigger(time_source)
// , rtcData_(g_rtcData[g_nextFreeRtcData++])
{
  ESP_LOGD(TAG, "Creating cron trigger");
  last_check_ = rtcData_.last_check;

  add_second(0);
  // Set all ones
  days_of_month_.set();
  months_.set();
  days_of_week_.set();
}

void CronTrigger::loop() {
  CronTrigger::loop();
  rtcData_.last_check = last_check_;
}

bool SignalTrigger::set_signal_time(uint8_t hour, uint8_t minute) {
  ESP_LOGD(TAG, "Setting siganl time to %u:%u", hour, minute);
  if (hour >= 24) {
    ESP_LOGW(TAG, "Hour should be less then 24");
    return false;
  }
  if (minute >= 60) {
    ESP_LOGW(TAG, "Minute should be less then 60");
    return false;
  }

  if (hours_[hour] && minutes_[minute]) {
    ESP_LOGI(TAG, "Not changed");
    return true;
  }

  this->hours_.reset();
  this->minutes_.reset();
  this->add_hour(hour);
  this->add_minute(minute);
  last_check_ = {};
  ESP_LOGD(TAG, "Cron s: %s, m: %s, h: %s, dw: %s, dm: %s, m: %s",
           seconds_.to_string().c_str(), minutes_.to_string().c_str(),
           hours_.to_string().c_str(), days_of_week_.to_string().c_str(),
           days_of_month_.to_string().c_str(), months_.to_string().c_str());
  return true;
}

void SignalTrigger::unset_signal_time() {
  this->seconds_.reset();
  this->minutes_.reset();
  last_check_ = {};
}

bool SignalComponent::set_signal_time(const std::string &time_string) {
  if (time_string.empty()) {
    if (trigger_)
      trigger_->unset_signal_time();
    ESP_LOGI(TAG, "Empty line. Cleaning up.");
    time_string_ = time_string;
    return true;
  }

  struct tm tm;
  if (nullptr == strptime(time_string.c_str(), "%H:%M", &tm)) {
    ESP_LOGW(TAG, "Time expected in format HH:MM, got '%s'",
             time_string.c_str());
    return false;
  }

  ESP_LOGI(TAG, "Time parsed. Setting %d hours and %d minutes", hour, minute);

  if (trigger_ && !trigger_->set_signal_time(tm.tm_hour, tm.tm_min))
    return false;

  time_string_ = time_string;
  return true;
}

void SignalComponent::set_trigger(SignalTrigger *trigger) {
  ESP_LOGI(TAG, "Setting up the trigger");
  trigger_ = trigger;
  set_signal_time(time_string_);
}
void SignalComponent::set_time(time::RealTimeClock *clock) { _clock = clock; }
} // namespace daily_signal
} // namespace esphome