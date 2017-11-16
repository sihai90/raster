/*
 * Copyright (C) 2017, Yeolar
 */

#pragma one

#include <atomic>
#include <map>
#include <memory>
#include <mutex>

#include "raster/util/Algorithm.h"
#include "raster/util/Logging.h"
#include "raster/util/MapUtil.h"
#include "raster/util/SpinLock.h"
#include "raster/util/Time.h"

namespace rdd {

class Degrader {
public:
  virtual bool needDemote() = 0;
};

class CountDegrader : public Degrader {
public:
  CountDegrader() {}

  void setup(bool open, uint32_t limit, uint32_t gap) {
    RDDLOG(INFO) << "Degrader: setup "
      << "open=" << open << ", limit=" << limit << ", gap=" << gap;
    SpinLockGuard guard(lock_);
    open_ = open;
    limit_ = limit;
    gap_ = gap;
  }

  virtual bool needDemote() {
    if (open_) {
      time_t ts = time(nullptr);
      SpinLockGuard guard(lock_);
      ts /= gap_;
      if (ts != ts_) {
        ts_ = ts;
        count_ = 0;
      }
      return ++count_ > limit_;
    }
    return false;
  }

private:
  std::atomic<bool> open_{false};
  SpinLock lock_;
  uint32_t limit_{0};
  uint32_t gap_{0};
  uint32_t count_{0};
  time_t ts_{0};
};

class RateDegrader : public Degrader {
public:
  RateDegrader() {}

  void setup(bool open, uint32_t limit, double rate) {
    RDDLOG(INFO) << "Degrader: setup "
      << "open=" << open << ", limit=" << limit << ", rate=" << rate;
    SpinLockGuard guard(lock_);
    open_ = open;
    rate_ = rate;
    limit_ = limit;
    ticket_ = limit;
  }

  virtual bool needDemote() {
    if (open_) {
      uint64_t ts = timestampNow();
      SpinLockGuard guard(lock_);
      if (ts_ != 0) {
        uint32_t incr = std::max(ts - ts_, 0ul) * rate_;
        ticket_ = std::min(ticket_ + incr, limit_);
      }
      ts_ = ts;
      if (ticket_ == 0) {
        return true;
      }
      ticket_--;
    }
    return false;
  }

private:
  SpinLock lock_;
  std::atomic<bool> open_{false};
  double rate_{0.0};
  uint32_t limit_{0};
  uint32_t ticket_{0};
  time_t ts_{0};
};

class DegraderManager {
public:
  DegraderManager() {}

  template <class Deg, class... Args>
  void setupDegrader(const std::string& name, Args&&... args) {
    std::lock_guard<std::mutex> guard(lock_);
    if (!contain(degraders_, name)) {
      degraders_[name].reset(new Deg());
    }
    Degrader* d = degraders_[name].get();
    DCHECK(typeid(*d) == typeid(Deg));
    ((Deg*)d)->setup(std::forward<Args>(args)...);
  }

  bool hit(const std::string& name) {
    std::lock_guard<std::mutex> guard(lock_);
    auto degrader = get_ptr(degraders_, name);
    return degrader ? degrader->get()->needDemote() : false;
  }

private:
  std::map<std::string, std::unique_ptr<Degrader>> degraders_;
  std::mutex lock_;
};

} // namespace rdd

#define RDDDEG_HIT(name) \
  ::rdd::Singleton< ::rdd::DegraderManager>::get()->hit(name)

