#include "timer.h"

#include "esphome/core/log.h"

#include <array>

namespace esphome {
namespace timer {


static const char *const TAG = "timer";

RTC_DATA_ATTR std::array<optional<ESPTime>, 16> g_timerRtcData;
size_t g_nextFreeTimerRtcData = 0;

TimerTrigger::TimerTrigger(time::RealTimeClock *rtc) 
  : rtc_(rtc)
  , signal_time_(g_timerRtcData[g_nextFreeTimerRtcData++]) {
  schedule();
}

void TimerTrigger::start(TimerTrigger::seconds_type seconds) {
  const auto& now = rtc_->now();
  if(seconds <= 0) {
    signal_time_ = {};
  } else {
    signal_time_ = ESPTime::from_epoch_local(now.timestamp + seconds);
  }
  schedule();
}

TimerTrigger::seconds_type TimerTrigger::get_seconds_remain() const { 
  const auto& now = rtc_->now();
  if(signal_time_ && signal_time_->timestamp > now.timestamp) {
    return signal_time_->timestamp - now.timestamp; 
  }
  return 0;
}

void TimerTrigger::schedule() {
  static const std::string timeName = "timer_timer";
  if(!signal_time_) {
    cancel_timeout(timeName);
  } else {
    const auto& now = rtc_->now();
    const auto seconds  = signal_time_->timestamp > now.timestamp ? signal_time_->timestamp - now.timestamp : 0;
    set_timeout(timeName, seconds*1000, [this]{ 
      this->trigger();
      signal_time_ = {};
    });
  }
}

bool TimerComponent::start(const std::string &time_string) {
  if (time_string.empty()) {
    if (trigger_)
      trigger_->start(0);
    ESP_LOGI(TAG, "Empty line. Cleaning up.");
    time_string_ = time_string;
    return true;
  }

  struct tm tm;
  if (nullptr != strptime(time_string.c_str(), "%H:%M:%S", &tm)) {
    ESP_LOGI(TAG, "Time parsed in format HH:MM:SS");
  } else if (nullptr != strptime(time_string.c_str(), "%M:%S", &tm)) {
    ESP_LOGI(TAG, "Time parsed in format MM:SS");
  } else if (nullptr != strptime(time_string.c_str(), "%S", &tm)) {
    ESP_LOGI(TAG, "Time parsed in format SS");
  } else {
    ESP_LOGW(TAG, "Time expected in format [[HH:]MM:]SS, got '%s'",
             time_string.c_str());
    return false;
  }

  if (trigger_)
    trigger_->start((tm.tm_hour*60 + tm.tm_min)*60 + tm.tm_sec);

  time_string_ = time_string;
  return true;
}

void TimerComponent::start(TimerComponent::seconds_type seconds) {
  ESP_LOGI(TAG, "Starting timer for %d seconds", seconds);
  if(trigger_)
    trigger_->start(seconds);
}

}}