#pragma once

#include "esp_attr.h"
#include "esp_task_wdt.h"
#include "esp_netif.h"
#include "esp_sntp.h"

#include <cstdint>
#include <iterator>
#include <string>
#include <vector>

bool RTC_DATA_ATTR sntp_synched = false;

static const char *const TAG = "kitchen-timer";

std::vector<std::string>
get_song_name_list(const std::vector<std::string> &_songs) {
  std::vector<std::string> names;
  names.reserve(_songs.size());
  for (const auto &tune : _songs) {
    const auto semicolon_index =
        std::find(std::begin(tune), std::end(tune), ':');
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
  if (hours > 99) {
    std::snprintf(buffer, sizeof(buffer), "%3dh", hours);
  } else if (hours > 9) {
    std::snprintf(buffer, sizeof(buffer), "%2dh%d", hours, minutes);
  } else if (hours > 0) {
    std::snprintf(buffer, sizeof(buffer), "%1dh%02d", hours, minutes);
  } else {
    std::snprintf(buffer, sizeof(buffer), "%02d.%02d", minutes, seconds);
  }
  return {buffer};
}

template <typename T, typename R = decltype((*std::declval<T>().begin())
                                                ->get_seconds_remain())>
R closest_timer(T &&timer_list) {
  R smallest_timer = {};
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
    return uint64_t(tv_sec)*1000000 + uint64_t(tv_usec);
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
    return next_update_ < TimeVal::now() + TimeVal::from_milliseconds(update_interval_ms_ * 1 / 8);
  }

  uint64_t useconds_to_update() const {
    const auto& now = TimeVal::now();
    return next_update_ < now ? 0 : (next_update_ - now).to_useconds();
  }

  void mark_updated() {
    ESP_LOGD(TAG, "Mark as updated");
    const auto now = TimeVal::now();
    const auto update_interval =
        TimeVal::from_milliseconds(update_interval_ms_);
    next_update_ += update_interval;

    if(next_update_ + update_interval + update_interval < now) {
      reset_next_update();
    }
  }
};

ClockUpdater clock_updater(1000);