/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/framework/Monitor.h"
#include "raster/io/event/EventLoop.h"
#include "raster/net/Channel.h"

namespace rdd {

EventLoop::EventLoop(int pollSize, int pollTimeout)
  : poll_(pollSize),
    timeout_(pollTimeout),
    stop_(false),
    loopThread_(),
    handler_(this) {
  poll_.add(waker_.fd(), EPOLLIN, new Event(&waker_));
}

void EventLoop::listen(const std::shared_ptr<Channel>& channel, int backlog) {
  int port = channel->peer().port;
  auto socket = std::make_shared<Socket>(0);
  if (!(*socket) ||
      !(socket->setReuseAddr()) ||
      // !(socket->setLinger(0)) ||
      !(socket->setTCPNoDelay()) ||
      !(socket->setNonBlocking()) ||
      !(socket->bind(port)) ||
      !(socket->listen(backlog))) {
    throw std::runtime_error("socket listen failed");
  }
  Event *event = new Event(channel, socket);
  if (!event) {
    throw std::runtime_error("create listening event failed");
  }
  listenFds_.emplace_back(socket->fd());
  event->setState(Event::kListen);
  dispatchEvent(event);
  RDDLOG(INFO) << *event << " listen on port=" << port;
}

void EventLoop::loopBody(bool once) {
  loopThread_ = pthread_self();

  while (!stop_) {
    uint64_t t0 = timestampNow();

    std::vector<Event*> events;
    std::vector<VoidFunc> callbacks;
    {
      std::lock_guard<std::mutex> guard(eventsLock_);
      events.swap(events_);
    }
    for (auto& e : events) {
      dispatchEvent(e);
    }
    {
      std::lock_guard<std::mutex> guard(callbacksLock_);
      callbacks.swap(callbacks_);
    }
    for (auto& callback : callbacks) {
      callback();
    }

    checkTimeoutEvent();

    if (!once) {
      int n = poll_.poll(timeout_);
      if (n >= 0) {
        for (int i = 0; i < n; ++i) {
          epoll_data_t edata;
          uint32_t etype = poll_.getData(i, edata);
          Event* event = (Event*) edata.ptr;
          if (event) {
            if (event->state() == Event::kWaker) {
              waker_.consume();
            } else {
              handler_.handle(event, etype);
            }
          }
        }
      }
      RDDMON_AVG("loopevent", n);
      RDDMON_MAX("loopevent.max", n);
    }

    uint64_t cost = timePassed(t0) / 1000;
    RDDMON_AVG("loopcost", cost);
    RDDMON_MAX("loopcost.max", cost);

    // Singleton<Actor>::get()->monitoring();

    if (once) {
      break;
    }
  }
  stop_ = false;
  loopThread_ = 0;
}

void EventLoop::stop() {
  poll_.remove(waker_.fd());
  for (auto& fd : listenFds_) {
    poll_.remove(fd);
  }
  stop_ = true;
}

void EventLoop::addEvent(Event* event) {
  {
    std::lock_guard<std::mutex> guard(eventsLock_);
    events_.emplace_back(event);
  }
  waker_.wake();
}

void EventLoop::addCallback(const VoidFunc& callback) {
  {
    std::lock_guard<std::mutex> guard(callbacksLock_);
    callbacks_.emplace_back(callback);
  }
  waker_.wake();
}

void EventLoop::dispatchEvent(Event *event) {
  RDDLOG(V2) << *event
    << " add event with executor(" << (void*)event->executor() << ")";
  switch (event->state()) {
    case Event::kListen:
      addListenEvent(event); break;
    case Event::kNext:
    case Event::kToRead:
      addReadEvent(event); break;
    case Event::kConnect:
    case Event::kToWrite:
      addWriteEvent(event); break;
    default:
      RDDLOG(ERROR) << *event << " cannot add event";
      break;
  }
}

void EventLoop::addListenEvent(Event *event) {
  assert(event->state() == Event::kListen);
  poll_.add(event->fd(), EPOLLIN, event);
}

void EventLoop::addReadEvent(Event *event) {
  assert(event->state() == Event::kNext ||
         event->state() == Event::kToRead);
  RDDLOG(V2) << *event << " add e/rdeadline";
  deadlineHeap_.push(event->state() == Event::kNext ?
                     event->edeadline() : event->rdeadline());
  poll_.add(event->fd(), EPOLLIN, event);
}

void EventLoop::addWriteEvent(Event *event) {
  assert(event->state() == Event::kConnect ||
         event->state() == Event::kToWrite);
  RDDLOG(V2) << *event << " add wdeadline";
  // TODO epoll_wait interval
  //deadlineHeap_.push(event->state() == Event::kConnect ?
  //                    event->cdeadline() : event->wdeadline());
  deadlineHeap_.push(event->wdeadline());
  poll_.add(event->fd(), EPOLLOUT, event);
}

void EventLoop::removeEvent(Event *event) {
  RDDLOG(V2) << *event << " remove deadline";
  deadlineHeap_.erase(event);
  RDDLOG(V2) << *event << " remove event";
  poll_.remove(event->fd());
}

void EventLoop::restartEvent(Event* event) {
  RDDLOG(V2) << *event << " remove deadline";
  deadlineHeap_.erase(event);
  event->restart();  // the next request
  RDDLOG(V2) << *event << " add rdeadline";
  deadlineHeap_.push(event->rdeadline());
}

void EventLoop::checkTimeoutEvent() {
  uint64_t now = timestampNow();

  while (true) {
    auto timeout = deadlineHeap_.pop(now);
    Event* event = timeout.data;
    if (!event) {
      break;
    }
    if (timeout.repeat && Socket::count() < Socket::LCOUNT) {
      timeout.deadline += Socket::LTIMEOUT;
      deadlineHeap_.push(timeout);
    }
    else {
      RDDLOG(V2) << *event << " pop deadline";
      event->setState(Event::kTimeout);
      RDDLOG(WARN) << *event << " remove timeout event: >"
        << timeout.deadline - event->starttime();
      handler_.onTimeout(event);
    }
  }
}

} // namespace rdd
