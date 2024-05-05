#pragma once

#include "esp_attr.h"
#include <cstdint>
#include <iterator>
#include <string>
#include <vector>

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
