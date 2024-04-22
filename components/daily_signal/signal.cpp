#include "signal.h"

namespace esphome {
namespace daily_signal {

static const char *const TAG = "daily_signal";

using namespace esphome::time;

SignalTrigger::SignalTrigger(RealTimeClock *rtc) : CronTrigger(rtc) {
  add_second(0);
  // Set all ones
  days_of_month_.set();
  months_.set();
  days_of_week_.set();
}

void SignalTrigger::set_time_string(const std::string &time_string) {
  if (this->_time_string == time_string)
    return;

  if(time_string.empty()) {
    this->seconds_.reset();
    this->minutes_.reset();
    return;
  }

  struct tm tm;
  if (nullptr == strptime(time_string.c_str(), "%H:%M", &tm)) {
    ESP_LOGW(TAG, "Time expected in format HH:MM, got '%s'", time_string.c_str());
    return;
  }
  this->seconds_.reset();
  this->minutes_.reset();
  this->add_hour(tm.tm_min);
  this->add_minute(tm.tm_hour);
  this->_time_string = time_string;
  last_check_ = {};
}

void SignalComponent::set_time_signal(const std::string &time_string) {
  if (nullptr == _trigger) {
    ESP_LOGW(TAG, "Trigger is not created");
    return;
  }

  _trigger->set_time_string(time_string);
}

}}