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
  recent_timers.back() = seconds;
  std::sort(recent_timers.begin(), recent_timers.end(),
            [](const uint16_t &a, const uint16_t &b) {
              if (a == 0) {
                return false;
              }

              if (b == 0) {
                return true;
              }

              return a < b;
            });
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

auto last_step_time_ms = esphome::millis();

template <typename T> class FibonacciIterator {
public:
  FibonacciIterator &operator++() {
    const auto &next = b_ + a_;
    a_ = b_;
    b_ = next;
    return *this;
  }

  FibonacciIterator &operator--() {
    const auto &prev = b_ - a_;
    b_ = a_;
    a_ = prev;
    return *this;
  }

  const T &operator*() const { return b_; }

  bool operator==(const FibonacciIterator &rhs) const { return b_ == rhs.b_; }

  bool operator!=(const FibonacciIterator &rhs) const { return b_ != rhs.b_; }

private:
  T a_ = 1;
  T b_ = 1;
};

template <typename T, typename S> class SpeedIterator {
  void adjustMinSpeed() {
    min_speed_ = S{};
    constexpr std::array<T, 5> values = {60 * 60, 60 * 10, 60, 10, 1};
    for (auto &&value : values) {
      if (value_ > value) {
        while (*min_speed_ < value)
          ++min_speed_;
        break;
      }
    }
  }
  void adjustSpeed() {
    adjustMinSpeed();
    speed_ = min_speed_;
    return;

    ++speed_;
    while (*min_speed_ > *speed_)
      ++speed_;
    const auto dt = esphome::millis() - last_step_time_ms;
    last_step_time_ms += dt;
    while (*speed_ > *min_speed_ && dt > 200) {
      --speed_;
    }
  }

public:
  SpeedIterator &operator++() {
    adjustSpeed();
    value_ += *speed_;
    return *this;
  }

  SpeedIterator &operator--() {
    adjustSpeed();
    value_ -= std::min(*speed_, value_);
    return *this;
  }

  const T &operator*() const { return value_; }

  bool operator==(const SpeedIterator &rhs) const {
    return value_ == rhs.value_;
  }

  bool operator!=(const SpeedIterator &rhs) const {
    return !this->operator==(rhs);
  }

private:
  T value_ = 0;
  S min_speed_;
  S speed_;
};

template <typename T> class ClockDigitsSpeedIterator {
public:
  ClockDigitsSpeedIterator &operator++() {
    ++speedIndex_;
    while (speedIndex_ >= speeds_().size())
      --speedIndex_;
    return *this;
  }

  ClockDigitsSpeedIterator &operator--() {
    if (speedIndex_ > 0)
      --speedIndex_;
    return *this;
  }

  bool operator==(const ClockDigitsSpeedIterator &rhs) const {
    return speedIndex_ == rhs.speedIndex_;
  }

  bool operator!=(const ClockDigitsSpeedIterator &rhs) const {
    return !this->operator==(rhs);
  }

  const T &operator*() const { return speeds_()[speedIndex_]; }

private:
  static const std::array<T, 15> &speeds_() {
    static const std::array<T, 15> clock_digits_speeds_{
        // Seconds
        1,
        2,
        5,
        1 * 10,
        2 * 10,
        5 * 10,
        // Minutes
        1 * 60,
        2 * 60,
        5 * 60,
        1 * 60 * 10,
        2 * 60 * 10,
        5 * 60 * 10,
        // Hours
        1 * 60 * 60,
        2 * 60 * 60,
        5 * 60 * 60,
    };
    return clock_digits_speeds_;
  }

  uint8_t speedIndex_ = 0;
};

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

template <typename T>
using FibonacciSpeedIterator = SpeedIterator<T, FibonacciIterator<T>>;

template <typename T> class LinearIterator {
public:
  LinearIterator &operator++() {
    ++a_;
    return *this;
  }

  LinearIterator &operator--() {
    --a_;
    return *this;
  }

  const T &operator*() const { return a_; }

  bool operator==(const LinearIterator &rhs) const { return a_ == rhs.a_; }

  bool operator!=(const LinearIterator &rhs) const { return a_ != rhs.a_; }

private:
  T a_ = 1;
};

// using TimerValueIterator = LinearIterator<esphome::timer::seconds_type>;
// using TimerValueIterator = FibonacciIterator<esphome::timer::seconds_type>;
// using TimerValueIterator =
// FibonacciSpeedIterator<esphome::timer::seconds_type>;
// using TimerValueIterator =
//     SpeedIterator<esphome::timer::seconds_type,
//                   ClockDigitsSpeedIterator<esphome::timer::seconds_type>>;
using TimerValueIterator = TimerCustomIterator<esphome::timer::seconds_type>;

RTC_DATA_ATTR TimerValueIterator menu_fibonacci_iterator;
