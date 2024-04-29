#include "timer.h"

#include "esphome/core/log.h"

#include <array>
#include <initializer_list>

namespace esphome {
namespace timer {

static const char *const TAG = "timer";

// RTC_DATA_ATTR std::array<optional<ESPTime>, 16> g_timerRtcData;
// size_t g_nextFreeTimerRtcData = 0;

TimerTrigger::TimerTrigger(time::RealTimeClock *rtc)
    : rtc_(rtc)
//, signal_time_(g_timerRtcData[g_nextFreeTimerRtcData++])
{
  schedule();
}

void TimerTrigger::start(TimerTrigger::seconds_type seconds) {
  ESP_LOGI(TAG, "Starting a trigger for %u seconds", seconds);
  const auto &now = rtc_->now();
  if (seconds <= 0) {
    signal_time_ = {};
  } else {
    signal_time_ = ESPTime::from_epoch_local(now.timestamp + seconds);
    ESP_LOGD(TAG, "Signal time is %s",
             signal_time_->strftime("%H:%M:%S").c_str());
  }
  schedule();
}

optional<TimerTrigger::seconds_type> TimerTrigger::get_seconds_remain() const {
  ESP_LOGD(TAG, "Calculating seconds remain");
  if (!signal_time_) {
    return {};
  }
  const auto &now = rtc_->now();
  if (signal_time_->timestamp > now.timestamp) {
    return signal_time_->timestamp - now.timestamp;
  }
  return 0;
}

void TimerTrigger::schedule() {
  ESP_LOGD(TAG, "Scheduling the beep");
  static const std::string timeName = "timer_timer";
  if (!signal_time_) {
    ESP_LOGD(TAG, "Canceling timeout %s", timeName.c_str());
    cancel_timeout(timeName);
  } else {
    const auto &now = rtc_->now();
    const auto seconds = signal_time_->timestamp > now.timestamp
                             ? signal_time_->timestamp - now.timestamp
                             : 0;
    ESP_LOGD(TAG, "Setting timeout %s to %d seconds", timeName.c_str(),
             seconds);
    set_timeout(timeName, seconds * 1000, [this] {
      ESP_LOGD(TAG, "Triggering");
      this->trigger();
      signal_time_ = {};
    });
  }
}

bool TimerComponent::start(const std::string &time_string) {
  ESP_LOGD(TAG, "Staring by string %s", time_string.c_str());
  if (time_string.empty()) {
    if (trigger_)
      trigger_->start(0);
    ESP_LOGI(TAG, "Empty line. Cleaning up.");
    time_string_ = time_string;
    return true;
  }

  struct tm tm = {};
  bool parsed = false;
  for (const auto &fmt :
       std::initializer_list<const char *>{"%H:%M:%S", "%M:%S", "%S"}) {
    ESP_LOGD(TAG, "Parsing as %s", fmt);
    struct tm tm1 = {};
    if (nullptr != strptime(time_string.c_str(), fmt, &tm1)) {
      ESP_LOGD(TAG, "Time parsed in format %s", fmt);
      tm = tm1;
      parsed = true;
      break;
    }
  }
  if (!parsed) {
    ESP_LOGW(TAG, "Time expected in format [[HH:]MM:]SS, got '%s'",
             time_string.c_str());
    return false;
  }

  ESP_LOGD(TAG, "After parsing tm is %dh %dm %ds", tm.tm_hour, tm.tm_min,
           tm.tm_sec);

  if (trigger_)
    trigger_->start((tm.tm_hour * 60 + tm.tm_min) * 60 + tm.tm_sec);

  time_string_ = time_string;
  return true;
}

void TimerComponent::start(TimerComponent::seconds_type seconds) {
  ESP_LOGD(TAG, "Starting timer for %d seconds", seconds);
  if (trigger_)
    trigger_->start(seconds);
}

void TimerComponent::set_trigger(TimerTrigger *trigger) {
  ESP_LOGD(TAG, "Setting the trigger");
  trigger_ = trigger;
  start(time_string_);
}

void TimerComponent::set_time(time::RealTimeClock *clock) {
  ESP_LOGD(TAG, "Setting the clock");
  _clock = clock;
}

optional<TimerComponent::seconds_type>
TimerComponent::get_seconds_remain() const {
  ESP_LOGD(TAG, "Getting seconds remain");
  if (trigger_)
    return trigger_->get_seconds_remain();
  return {};
}
} // namespace timer
} // namespace esphome