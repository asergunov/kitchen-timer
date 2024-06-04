#pragma once

#include "esp_attr.h"
#include "esp_netif.h"
#include "esp_pm.h"
#include "esp_sntp.h"
#include "esp_task_wdt.h"

#include <cstdint>
#include <iterator>
#include <queue>
#include <string>
#include <vector>

static const char *const TAG = "kitchen-timer";

namespace kitchen_timer {
namespace sntp {
int64_t RTC_DATA_ATTR correction_us = 0;
int64_t RTC_DATA_ATTR interval_us = 0;
time_t RTC_DATA_ATTR sync_time = 0;
bool force_sync_scheduled = false;
struct timeval RTC_DATA_ATTR prev_sync;
bool RTC_DATA_ATTR has_prev_sync = false;
int call_counter = 0;

bool is_time_to_force_sync() {
  time_t now;
  ::time(&now);
  return sync_time == 0
             ? false
             : now > sync_time + sntp_time->get_update_interval() / 1000 + 10;
}

void force_sync() {
  if (force_sync_scheduled)
    return;

  // if ( !sntp_restart() ) {
  //   ESP_LOGW(TAG, "Cant' schedule restart. Sntp is not running.");
  // } else {
  ESP_LOGD(TAG, "Sntp force sync scheduled.");
  force_sync_scheduled = true;
  // }
}

void sntp_sync_time(struct timeval *tv) {
  call_counter++;
  force_sync_scheduled = false;

  if (has_prev_sync &&
      (prev_sync.tv_sec != tv->tv_sec || prev_sync.tv_usec != tv->tv_usec)) {
    struct timeval now;
    gettimeofday(&now, NULL);
    correction_us =
        (static_cast<int64_t>(tv->tv_sec) - static_cast<int64_t>(now.tv_sec)) *
            1000000 +
        static_cast<int64_t>(tv->tv_usec) - static_cast<int64_t>(now.tv_usec);

    interval_us = (static_cast<int64_t>(tv->tv_sec) -
                   static_cast<int64_t>(prev_sync.tv_sec)) *
                      1000000 +
                  static_cast<int64_t>(tv->tv_usec) -
                  static_cast<int64_t>(prev_sync.tv_usec);
  }

  has_prev_sync = true;
  prev_sync = *tv;
  ::time(&sync_time);
  settimeofday(tv, NULL);
  sntp_set_sync_status(SNTP_SYNC_STATUS_COMPLETED);
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
    std::snprintf(buffer, sizeof(buffer), "%3dh", hours);
  } else if (hours > 9) {
    std::snprintf(buffer, sizeof(buffer), "%2dh%d", hours, minutes);
  } else if (hours > 0) {
    std::snprintf(buffer, sizeof(buffer), "%1dh%02d", hours, minutes);
  } else if (minutes > 0) {
    std::snprintf(buffer, sizeof(buffer), "%02d.%02d", minutes, seconds);
  } else {
    std::snprintf(buffer, sizeof(buffer), "  .%02d", seconds);
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