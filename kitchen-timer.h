#pragma once

#include "esp_attr.h"
#include "esp_netif.h"
#include "esp_pm.h"
#include "esp_sntp.h"
#include "esp_task_wdt.h"
// #include "esp_clk_tree.h"

#include <cstdint>
#include <iterator>
#include <queue>
#include <string>
#include <vector>

static const char *const TAG = "kitchen-timer";

namespace kitchen_timer {

RTC_DATA_ATTR float last_battery_percentage = NAN;

namespace sntp {
RTC_DATA_ATTR int64_t correction_us_total = 0;
RTC_DATA_ATTR uint64_t correction_us_abs_total = 0;
RTC_DATA_ATTR bool has_behind_us = false;
RTC_DATA_ATTR int64_t behind_us = 0;
RTC_DATA_ATTR int64_t up_time_us = 0;
RTC_DATA_ATTR time_t sync_try_time = 0;
RTC_DATA_ATTR struct timeval prev_sync;
RTC_DATA_ATTR time_t first_sync_time = 0;
RTC_DATA_ATTR uint32_t sync_count = 0;

bool force_sync_scheduled = false;

void sntp_sync_time(struct timeval *tv) {
  struct timeval now;
  gettimeofday(&now, NULL);
  settimeofday(tv, NULL);

  sync_count++;
  force_sync_scheduled = false;

  if (first_sync_time == 0) {
    first_sync_time = tv->tv_sec;
  } else {
    const auto correction_us_raw =
        (static_cast<int64_t>(tv->tv_sec) - static_cast<int64_t>(now.tv_sec)) *
            1000000 +
        static_cast<int64_t>(tv->tv_usec) - static_cast<int64_t>(now.tv_usec);
    const auto correction_us = correction_us_raw - behind_us;
    behind_us = correction_us_raw;
    if(has_behind_us) {
      correction_us_abs_total += abs(correction_us);
      if(correction_us_abs_total > static_cast<uint64_t>(1) << 63) {
        correction_us_abs_total = abs(correction_us);
        correction_us_total = 0;
      }
      correction_us_total += correction_us;
    }
    has_behind_us = true;
  }

  prev_sync = *tv;
  sntp_set_sync_status(SNTP_SYNC_STATUS_COMPLETED);
  ::time(&sync_try_time);
}

} // namespace sntp
} // namespace kitchen_timer

// Replace weak linked default one
extern "C" void sntp_sync_time(struct timeval *tv) {
  ::kitchen_timer::sntp::sntp_sync_time(tv);
}

namespace alarms {
namespace schedule {

struct Item {
  std::vector<std::string> message;
  uint8_t song_index;
};

using Queue = std::queue<Item>;

Queue queue;

} // namespace schedule

bool is_firing() {
  return !schedule::queue.empty() || play_signal->is_running();
}

void push(uint8_t song_index, std::vector<std::string> message) {
  schedule::queue.push(
      schedule::Item{.message = std::move(message), .song_index = song_index});
  if (!play_signal->is_running()) {
    play_signal->execute();
  }
}

optional<schedule::Item> pop() {
  optional<schedule::Item> result;
  if (!schedule::queue.empty()) {
    result = std::move(schedule::queue.front());
    schedule::queue.pop();
  }
  return result;
}

} // namespace alarms

const std::string &song_by_index(const size_t &index) {
  static const std::string empty;
  return index < songs->value().size() ? songs->value()[index] : empty;
}

const std::string &song_by_index(const optional<size_t> &index) {
  return index ? song_by_index(*index) : song_by_index(static_cast<size_t>(-1));
}

std::vector<std::string>
get_song_name_list(const std::vector<std::string> &_songs) {
  std::vector<std::string> names;
  names.reserve(_songs.size());
  for (const auto &tune : _songs) {
    const auto semicolon_index =
        std::find(std::begin(tune), std::end(tune), ':');
    if(semicolon_index == std::end(tune))
      names.push_back("");
    else
      names.push_back({std::begin(tune), semicolon_index});
  }
  return names;
}

template <typename T, size_t N>
void add_to_recent_timers(std::array<T, N> &recent_timers, const T &seconds) {
  auto i = std::find(recent_timers.begin(), recent_timers.end(), seconds);
  if (i == recent_timers.end()) {
    recent_timers.back() = seconds;
  } else {
    ++i;
  }
  std::rotate(recent_timers.begin(), std::prev(i), i);
}

std::string timer_value_string(uint32_t seconds) {
  char buffer[16];
  const auto hours = seconds / 60 / 60;
  seconds %= (60 * 60);
  const auto minutes = seconds / 60;
  seconds %= 60;
  if (minutes > 99) {
    std::snprintf(buffer, sizeof(buffer), "%3ldh", hours);
  } else if (hours > 9) {
    std::snprintf(buffer, sizeof(buffer), "%2ldh%ld", hours, minutes);
  } else if (hours > 0) {
    std::snprintf(buffer, sizeof(buffer), "%1ldh%02ld", hours, minutes);
  } else if (minutes > 0) {
    std::snprintf(buffer, sizeof(buffer), "%02ld.%02ld", minutes, seconds);
  } else {
    std::snprintf(buffer, sizeof(buffer), "  .%02ld", seconds);
  }
  return {buffer};
}

template <typename T, typename R = decltype((*std::declval<T>().begin())
                                                ->get_seconds_remain())>
R closest_timer(T &&timer_list) {
  R smallest_timer;
  for (const auto &timer : timer_list) {
    const auto &remain = timer->get_seconds_remain();
    if (!remain) {
      continue;
    }
    if (!smallest_timer) {
      smallest_timer = remain;
      continue;
    }

    smallest_timer = std::min(*smallest_timer, *remain);
  }
  return smallest_timer;
}

template <typename T,
          typename R = decltype(std::declval<T>()->get_seconds_remain())>
R closest_timer_il(const std::initializer_list<T> &list) {
  return closest_timer(list);
}

template <typename T> class TimerCustomIterator {
  struct RangeDescr {
    T top;
    T step;
    bool operator<(const RangeDescr &rhs) const { return top > rhs.top; }
    bool operator<(const T &rhs) const { return top > rhs; }
    friend bool operator<(const T &lhs, const RangeDescr &rhs) {
      return lhs > rhs.top;
    }
  };

  static std::array<RangeDescr, 9> ranges_() {
    static std::array<RangeDescr, 9> ranges{
        RangeDescr{60 * 60 * 5, 60 * 60},
        RangeDescr{60 * 60 * 5 / 2, 30 * 60},
        RangeDescr{40 * 60, 10 * 60}, //
        RangeDescr{20 * 60, 5 * 60},  // 4
        RangeDescr{10 * 60, 60},      // 10
        RangeDescr{2 * 60, 30},       // 4
        RangeDescr{60, 15},           // 4
        RangeDescr{20, 10},           // 4
        RangeDescr{0, 5},             // 4
    };
    return ranges;
  }

public:
  TimerCustomIterator &operator++() {
    const auto &ranges = ranges_();
    auto i = std::lower_bound(ranges.begin(), ranges.end(), value_);
    value_ += i->step;
    return *this;
  }

  TimerCustomIterator &operator--() {
    const auto &ranges = ranges_();
    auto i = std::upper_bound(ranges.begin(), ranges.end(), value_);
    if (i == ranges.end())
      --i;
    value_ -= i->step;
    return *this;
  }

  const T &operator*() const { return value_; }

  bool operator==(const TimerCustomIterator &rhs) const {
    return value_ == rhs.value_;
  }

  bool operator!=(const TimerCustomIterator &rhs) const {
    return !this->operator==(rhs);
  }

private:
  T value_ = 0;
};

using TimerValueIterator = TimerCustomIterator<esphome::timer::seconds_type>;

RTC_DATA_ATTR TimerValueIterator menu_new_timer_iterator;

class TimeVal : public timeval {
public:
  TimeVal(long sec, long usec) : timeval{.tv_sec = sec, .tv_usec = usec} {}
  TimeVal() : TimeVal(0, 0) {}

  static TimeVal now() {
    TimeVal tv;
    const auto ec = gettimeofday(&tv, nullptr);
    if (ec != 0) {
      ESP_LOGE(TAG, "Can't gettimeofday, %d", ec);
    }
    return tv;
  }

  template <typename T> static TimeVal from_seconds(const T &s) {
    return {s, 0};
  }

  template <typename T> static TimeVal from_milliseconds(const T &ms) {
    return TimeVal(ms / 1000l, (ms % 1000l) * 1000l);
  }

  template <typename T> static TimeVal from_useconds(const T &us) {
    return TimeVal(us / 1000000l, (us % 1000000l));
  }

  TimeVal operator-(const TimeVal &rhs) const {
    TimeVal res;
    timersub(this, &rhs, &res);
    return res;
  }

  TimeVal operator+(const TimeVal &rhs) const {
    TimeVal res;
    timeradd(this, &rhs, &res);
    return res;
  }

  TimeVal operator+=(const TimeVal &rhs) {
    timeradd(this, &rhs, this);
    return *this;
  }

  TimeVal operator-=(const TimeVal &rhs) {
    timersub(this, &rhs, this);
    return *this;
  }

  bool operator>(const TimeVal &rhs) const {
    return tv_sec > rhs.tv_sec ||
           (tv_sec == rhs.tv_sec && tv_usec > rhs.tv_usec);
  }

  bool operator<(const TimeVal &rhs) const {
    return tv_sec < rhs.tv_sec ||
           (tv_sec == rhs.tv_sec && tv_usec < rhs.tv_usec);
  }

  bool operator==(const TimeVal &rhs) const {
    return tv_sec == rhs.tv_sec && tv_usec == rhs.tv_usec;
  }

  uint64_t to_useconds() const {
    return uint64_t(tv_sec) * 1000000 + uint64_t(tv_usec);
  }

  TimeVal operator%(const TimeVal rhs) const {
    return from_useconds(to_useconds() % rhs.to_useconds());
  }
};

class ClockUpdater {
  uint16_t update_interval_ms_;
  TimeVal next_update_;

public:
  ClockUpdater(uint16_t update_interval_ms)
      : update_interval_ms_(update_interval_ms) {
    reset_next_update();
  }

  void reset_next_update() {
    ESP_LOGD(TAG, "Reset the next update time");
    const auto now = TimeVal::now();
    const auto update_interval =
        TimeVal::from_milliseconds(update_interval_ms_);
    next_update_ = now + update_interval;
    next_update_ -= next_update_ % update_interval;
  }

  bool can_update() const {
    return next_update_ < TimeVal::now() + TimeVal::from_milliseconds(
                                               update_interval_ms_ * 1 / 8);
  }

  uint64_t useconds_to_update() const {
    const auto &now = TimeVal::now();
    return next_update_ < now ? 0 : (next_update_ - now).to_useconds();
  }

  void mark_updated() {
    ESP_LOGD(TAG, "Mark as updated");
    const auto now = TimeVal::now();
    const auto update_interval =
        TimeVal::from_milliseconds(update_interval_ms_);
    next_update_ += update_interval;

    if (next_update_ + update_interval + update_interval < now) {
      reset_next_update();
    }
  }
};

ClockUpdater clock_updater(1000);