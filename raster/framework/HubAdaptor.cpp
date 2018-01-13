/*
 * Copyright 2017 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "raster/framework/HubAdaptor.h"

#include "raster/framework/Monitor.h"

namespace rdd {

HubAdaptor::HubAdaptor()
  : acceptor_(std::shared_ptr<NetHub>(this)) {
}

void HubAdaptor::configThreads(const std::string& name, size_t threadCount) {
  if (name == "io") {
    auto factory = std::make_shared<ThreadFactory>("IOThreadPool_");
    ioPool_.reset(new IOThreadPoolExecutor(threadCount, factory));
  } else {
    auto factory = std::make_shared<ThreadFactory>("CPUThreadPool" + name + "_");
    cpuPoolMap_.emplace(
        to<int>(name),
        make_unique<CPUThreadPoolExecutor>(threadCount, factory));
  }
}

void HubAdaptor::addService(std::unique_ptr<Service> service) {
  acceptor_.addService(std::move(service));
}

void HubAdaptor::configService(
    const std::string& name, int port, const TimeoutOption& timeoutOpt) {
  acceptor_.configService(name, port, timeoutOpt);
}

void HubAdaptor::startService() {
  acceptor_.start();
}

CPUThreadPoolExecutor*
HubAdaptor::getCPUThreadPoolExecutor(int poolId) {
  auto it = cpuPoolMap_.find(poolId);
  if (it != cpuPoolMap_.end()) {
    return it->second.get();
  }
  RDDLOG(FATAL) << "CPUThreadPool" << poolId << " not found";
  return nullptr;
}

std::shared_ptr<CPUThreadPoolExecutor>
HubAdaptor::getSharedCPUThreadPoolExecutor(int poolId) {
  auto it = cpuPoolMap_.find(poolId);
  if (it != cpuPoolMap_.end()) {
    return it->second;
  }
  RDDLOG(FATAL) << "CPUThreadPool" << poolId << " not found";
  return nullptr;
}

EventLoop* HubAdaptor::getEventLoop() {
  ioPool_->getEventLoop();
}

} // namespace rdd
