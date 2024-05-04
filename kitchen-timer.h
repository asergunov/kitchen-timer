#pragma once

#include "esp_attr.h"
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
  if (hours > 0) {
    std::snprintf(buffer, sizeof(buffer), "%1dh.%02d", hours, minutes);
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

  const T *operator->() const { return &b_; }

  bool operator==(const FibonacciIterator &rhs) const { return b_ == rhs.b_; }

  bool operator!=(const FibonacciIterator &rhs) const { return b_ != rhs.b_; }

private:
  T a_ = 1;
  T b_ = 1;
};

template <typename T> class FibonacciSpeedIterator {
  void adjustSpeed() {
    ++speed_;
    const auto dt = esphome::millis() - last_step_time_ms;
    last_step_time_ms += dt;

    while (speed_ != min_speed_ && dt > 200) {
      --speed_;
    }
  }

public:
  FibonacciSpeedIterator &operator++() {
    adjustSpeed();
    value_ += *speed_;
    return *this;
  }

  FibonacciSpeedIterator &operator--() {
    adjustSpeed();
    value_ -= std::min(*speed_, value_);
    return *this;
  }

  const T &operator*() const { return value_; }

  const T *operator->() const { return &value_; }

  bool operator==(const FibonacciSpeedIterator &rhs) const {
    return value_ == rhs.value_;
  }

  bool operator!=(const FibonacciSpeedIterator &rhs) const {
    return !this->operator==(rhs);
  }

private:
  T value_ = 1;
  static constexpr FibonacciIterator<T> min_speed_ = {};
  FibonacciIterator<T> speed_;
};

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

  const T *operator->() const { return &a_; }

  bool operator==(const LinearIterator &rhs) const { return a_ == rhs.a_; }

  bool operator!=(const LinearIterator &rhs) const { return a_ != rhs.a_; }

private:
  T a_ = 1;
};

// using TimerValueIterator = LinearIterator<esphome::timer::seconds_type>;
// using TimerValueIterator = FibonacciIterator<esphome::timer::seconds_type>;
using TimerValueIterator = FibonacciSpeedIterator<esphome::timer::seconds_type>;

RTC_DATA_ATTR TimerValueIterator menu_fibonacci_iterator;
